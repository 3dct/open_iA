/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "pch.h"
#include "iASimilarity.h"

#include "defines.h"          // for DIM
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkCastImageFilter.h>
#include <itkImageToHistogramFilter.h>
#include <itkJoinImageFilter.h>
#include <itkNormalizedCorrelationImageToImageMetric.h>
#include <itkMeanSquaresImageToImageMetric.h>
#include <itkTranslationTransform.h>

template<class T>
void similarity_metrics_template( iAProgress* p, QVector<iAConnector*> images,
	QMap<QString, QVariant> const & parameters, iAFilter* filter)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::TranslationTransform < double, DIM > TransformType;
	typedef itk::LinearInterpolateImageFunction<ImageType, double >	InterpolatorType;
	auto transform = TransformType::New();
	transform->SetIdentity();
	auto interpolator = InterpolatorType::New();
	interpolator->SetInputImage(dynamic_cast<ImageType *>(images[0]->GetITKImage()));
	TransformType::ParametersType params(transform->GetNumberOfParameters());

	if (parameters["Mean Squares"].toBool())
	{
		typedef itk::MeanSquaresImageToImageMetric<	ImageType, ImageType > MSMetricType;
		auto msmetric = MSMetricType::New();
		msmetric->SetFixedImage(dynamic_cast<ImageType *>(images[0]->GetITKImage()));
		msmetric->SetFixedImageRegion(dynamic_cast<ImageType *>(images[0]->GetITKImage())->GetLargestPossibleRegion());
		msmetric->SetMovingImage(dynamic_cast<ImageType *>(images[1]->GetITKImage()));
		msmetric->SetTransform(transform);
		msmetric->SetInterpolator(interpolator);
		params.Fill(0.0);
		msmetric->Initialize();
		double msVal = msmetric->GetValue(params);
		filter->AddOutputValue("Mean Squares Metric", msVal);
	}
	if (parameters["Normalized Correlation"].toBool())
	{
		typedef itk::NormalizedCorrelationImageToImageMetric< ImageType, ImageType > NCMetricType;
		auto ncmetric = NCMetricType::New();
		ncmetric->SetFixedImage(dynamic_cast<ImageType *>(images[0]->GetITKImage()));
		ncmetric->SetFixedImageRegion(dynamic_cast<ImageType *>(images[0]->GetITKImage())->GetLargestPossibleRegion());
		ncmetric->SetMovingImage(dynamic_cast<ImageType *>(images[1]->GetITKImage()));
		ncmetric->SetTransform(transform);
		ncmetric->SetInterpolator(interpolator);
		params.Fill(0.0);
		ncmetric->Initialize();
		double ncVal = ncmetric->GetValue(params);
		filter->AddOutputValue("Normalized Correlation Metric", ncVal);
	}
	if (parameters["Mutual Information"].toBool())
	{
		//ITK-Example: https://itk.org/Doxygen/html/Examples_2Statistics_2ImageMutualInformation1_8cxx-example.html
		typedef itk::JoinImageFilter< ImageType, ImageType > JoinFilterType;
		auto joinFilter = JoinFilterType::New();
		joinFilter->SetInput1(dynamic_cast<ImageType *>(images[0]->GetITKImage()));
		joinFilter->SetInput2(dynamic_cast<ImageType *>(images[1]->GetITKImage()));
		joinFilter->Update();

		typedef typename JoinFilterType::OutputImageType  VectorImageType;
		typedef itk::Statistics::ImageToHistogramFilter<VectorImageType >  HistogramFilterType;
		auto histogramFilter = HistogramFilterType::New();
		histogramFilter->SetInput(joinFilter->GetOutput());
		histogramFilter->SetMarginalScale(10.0);
		typedef typename HistogramFilterType::HistogramSizeType   HistogramSizeType;
		HistogramSizeType size(2);
		size[0] = parameters["Histogram Bins"].toDouble();  // number of bins for the first  channel
		size[1] = parameters["Histogram Bins"].toDouble();  // number of bins for the second channel
		histogramFilter->SetHistogramSize(size);
		/*
		typedef typename HistogramFilterType::HistogramMeasurementVectorType HistogramMeasurementVectorType;
		HistogramMeasurementVectorType binMinimum(3);
		HistogramMeasurementVectorType binMaximum(3);
		// shouldn't this be the min and max DATA values?
		binMinimum[0] = -0.5;
		binMinimum[1] = -0.5;
		binMinimum[2] = -0.5;
		binMaximum[0] = miHistoBins + 0.5;
		binMaximum[1] = miHistoBins + 0.5;
		binMaximum[2] = miHistoBins + 0.5;
		histogramFilter->SetHistogramBinMinimum(binMinimum);
		histogramFilter->SetHistogramBinMaximum(binMaximum);
		*/
		histogramFilter->SetAutoMinimumMaximum(true);
		histogramFilter->Update();
		typedef typename HistogramFilterType::HistogramType  HistogramType;
		const HistogramType * histogram = histogramFilter->GetOutput();
		auto itr = histogram->Begin();
		auto end = histogram->End();
		const double Sum = histogram->GetTotalFrequency();
		double jointEntr = 0;
		while (itr != end)
		{
			const double count = itr.GetFrequency();
			if (count > 0.0)
			{
				const double probability = count / Sum;
				jointEntr += -probability * std::log(probability) / std::log(2.0);
			}
			++itr;
		}

		size[0] = parameters["Histogram Bins"].toDouble();  // number of bins for the first  channel
		size[1] = 1;  // number of bins for the second channel
		histogramFilter->SetHistogramSize(size);
		histogramFilter->Update();
		itr = histogram->Begin();
		end = histogram->End();
		double entr1 = 0;
		while (itr != end)
		{
			const double count = itr.GetFrequency();
			if (count > 0.0)
			{
				const double probability = count / Sum;
				entr1 += -probability * std::log(probability) / std::log(2.0);
			}
			++itr;
		}

		size[0] = 1;  // number of bins for the first channel
		size[1] = parameters["Histogram Bins"].toDouble();  // number of bins for the second channel
		histogramFilter->SetHistogramSize(size);
		histogramFilter->Update();
		itr = histogram->Begin();
		end = histogram->End();
		double entr2 = 0;
		while (itr != end)
		{
			const double count = itr.GetFrequency();
			if (count > 0.0)
			{
				const double probability = count / Sum;
				entr2 += -probability * std::log(probability) / std::log(2.0);
			}
			++itr;
		}
		double mutInf = entr1 + entr2 - jointEntr;
		double norMutInf1 = 2.0 * mutInf / (entr1 + entr2);
		double norMutInf2 = (entr1 + entr2) / jointEntr;
		filter->AddOutputValue("Image 1 Entropy", entr1);
		filter->AddOutputValue("Image 2 Entropy", entr2);
		filter->AddOutputValue("Joint Entropy", jointEntr);
		filter->AddOutputValue("Mutual Information", mutInf);
		filter->AddOutputValue("Normalized Mutual Information 1", norMutInf1);
		filter->AddOutputValue("Normalized Mutual Information 2", norMutInf2);
	}

}

iASimilarity::iASimilarity() : iAFilter("Similarity", "Metrics",
	"Calculates the similarity between two images according to different metrics.<br/>"
	"<strong>NOTE</strong>: Normalize the images before calculating the similarity metrics!<br/>"
	"<a href=\"https://itk.org/Doxygen/html/ImageSimilarityMetricsPage.html\">General information on ITK similarity metrics</a>.<br/>"
	"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MeanSquaresImageToImageMetric.html\">"
	"Mean Squares Metric</a>: The optimal value of the metric is zero. Poor matches between images A and B result in large "
	"values of the metric. This metric relies on the assumption that intensity representing the same homologous point "
	"must be the same in both images.<br/>"
	"<a href=\"https://itk.org/Doxygen/html/classitk_1_1NormalizedCorrelationImageToImageMetric.html\">"
	"Normalized Correlation Metric</a>: Note the −1 factor in the metric computation. This factor is used to make the "
	"metric be optimal when its minimum is reached.The optimal value of the metric is then minus one. Misalignment "
	"between the images results in small measure values.<br/>"
	"More Information on Mutual Information is given in the "
	"<a href=\"https://itk.org/ItkSoftwareGuide.pdf\">ITK Software Guide</a> in the sections '3.10.4 Mutual "
	"Information Metric' (pp. 262-264) and '5.3.2 Information Theory' (pp. 462-471).", 2, 0)
{
	AddParameter("Mean Squares", Boolean, true);
	AddParameter("Normalized Correlation", Boolean, false);
	AddParameter("Mutual Information", Boolean, false);
	AddParameter("Histogram Bins", Discrete, 256, 2);
}

IAFILTER_CREATE(iASimilarity)

void iASimilarity::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(similarity_metrics_template, m_con->GetITKScalarPixelType(), m_progress, m_cons, parameters, this);
}
