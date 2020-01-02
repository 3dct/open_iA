/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iASimilarity.h"

#include <defines.h>          // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>
#include <mdichild.h>

#include <itkCastImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkImageRegionConstIterator.h>
#include <itkImageToHistogramFilter.h>
#include <itkJoinImageFilter.h>
#include <itkNormalizedCorrelationImageToImageMetric.h>
#include <itkMeanSquaresImageToImageMetric.h>
#include <itkStatisticsImageFilter.h>
#include <itkTranslationTransform.h>

#include <vtkImageData.h>

//! Custom image similarity metric - "equal pixel rate", i.e.
//!     number of equal pixels / number of total pixels
template<class ImageType>
double computeEqualPixelRate(typename ImageType::Pointer img, typename ImageType::Pointer ref)
{
	typename ImageType::RegionType reg = ref->GetLargestPossibleRegion();
	int size = reg.GetSize()[0] * reg.GetSize()[1] * reg.GetSize()[2];
	double sumEqual = 0.0f;
#pragma omp parallel for reduction(+:sumEqual)
	for (int i = 0; i < size; ++i)
	{
		if (img->GetBufferPointer()[i] != 0 &&
			img->GetBufferPointer()[i] == ref->GetBufferPointer()[i])
			++sumEqual;
	}
	return sumEqual / size;
}

template<class T>
void similarity_metrics(iAFilter* filter, QMap<QString, QVariant> const & parameters)
{
	typedef itk::Image< T, DIM > ImageType;
	size_t size[3], index[3];
	size[0] = parameters["Size X"].toUInt(); size[1] = parameters["Size Y"].toUInt(); size[2] = parameters["Size Z"].toUInt();
	index[0] = parameters["Index X"].toUInt(); index[1] = parameters["Index Y"].toUInt(); index[2] = parameters["Index Z"].toUInt();
	auto activeExtract = extractImage(filter->input()[0]->itkImage(), index, size);
	auto nonActiveExtract = extractImage(filter->input()[1]->itkImage(), index, size);
	ImageType* img = dynamic_cast<ImageType*>(activeExtract.GetPointer());
	ImageType* ref = dynamic_cast<ImageType*>(nonActiveExtract.GetPointer());
	typedef itk::TranslationTransform < double, DIM > TransformType;
	typedef itk::LinearInterpolateImageFunction<ImageType, double >	InterpolatorType;
	auto transform = TransformType::New();
	transform->SetIdentity();
	auto interpolator = InterpolatorType::New();
	interpolator->SetInputImage(img);
	typename TransformType::ParametersType params(transform->GetNumberOfParameters());
	double range = 0, imgMean = 0, imgVar = 0, refMean = 0, refVar = 0, mse = 0;
	if (parameters["Peak Signal-to-Noise Ratio"].toBool() ||
		parameters["Structural Similarity Index"].toBool() ||
		parameters["Normalized RMSE"].toBool())
	{
		double imgMin, imgMax, refMin, refMax;
		getStatistics(img, &imgMin, &imgMax, &imgMean, nullptr, &imgVar);
		getStatistics(ref, &refMin, &refMax, &refMean, nullptr, &refVar);
		range = std::max(refMax, imgMax) - std::min(refMin, imgMin);
	}
	if (parameters["Mean Squared Error"].toBool() ||
		parameters["RMSE"].toBool() ||
		parameters["Normalized RMSE"].toBool() ||
		parameters["Peak Signal-to-Noise Ratio"].toBool())
	{
		typedef itk::MeanSquaresImageToImageMetric<	ImageType, ImageType > MSMetricType;
		auto msmetric = MSMetricType::New();
		msmetric->SetFixedImage(img);
		msmetric->SetFixedImageRegion(img->GetLargestPossibleRegion());
		msmetric->SetMovingImage(ref);
		msmetric->SetTransform(transform);
		msmetric->SetInterpolator(interpolator);
		params.Fill(0.0);
		msmetric->Initialize();
		mse = msmetric->GetValue(params);
		if (parameters["Mean Squared Error"].toBool())
			filter->addOutputValue("Mean Squared Error", mse);
		if (parameters["RMSE"].toBool())
			filter->addOutputValue("RMSE", std::sqrt(mse));
		if (parameters["Normalized RMSE"].toBool())
			filter->addOutputValue("Normalized RMSE", std::sqrt(mse) / range);

	}
	if (parameters["Peak Signal-to-Noise Ratio"].toBool())
	{
		double psnr = 20 * std::log10(range) - 10 * log10(mse);
		filter->addOutputValue("Peak Signal-to-Noise Ratio", psnr);
	}
	if (parameters["Mean Absolute Error"].toBool())
	{
		itk::ImageRegionConstIterator<ImageType> imgIt(img, img->GetLargestPossibleRegion());
		itk::ImageRegionConstIterator<ImageType> refIt(ref, ref->GetLargestPossibleRegion());
		imgIt.GoToBegin(); refIt.GoToBegin();
		double diffSum = 0;	size_t count = 0;
		while (!imgIt.IsAtEnd() && !refIt.IsAtEnd())
		{
			diffSum += std::abs(static_cast<double>(refIt.Get() - imgIt.Get()));
			++imgIt; ++refIt; ++count;
		}
		filter->addOutputValue("Mean Absolute Error", diffSum / count);
	}
	if (parameters["Normalized Correlation"].toBool())
	{
		typedef itk::NormalizedCorrelationImageToImageMetric< ImageType, ImageType > NCMetricType;
		auto ncmetric = NCMetricType::New();
		ncmetric->SetFixedImage(img);
		ncmetric->SetFixedImageRegion(img->GetLargestPossibleRegion());
		ncmetric->SetMovingImage(ref);
		ncmetric->SetTransform(transform);
		ncmetric->SetInterpolator(interpolator);
		params.Fill(0.0);
		ncmetric->Initialize();
		double ncVal = ncmetric->GetValue(params);
		filter->addOutputValue("Normalized Correlation Metric", ncVal);
	}
	if (parameters["Mutual Information"].toBool())
	{
		//ITK-Example: https://itk.org/Doxygen/html/Examples_2Statistics_2ImageMutualInformation1_8cxx-example.html
		typedef itk::JoinImageFilter< ImageType, ImageType > JoinFilterType;
		auto joinFilter = JoinFilterType::New();
		joinFilter->SetInput1(img);
		joinFilter->SetInput2(ref);
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
		filter->addOutputValue("Image 1 Entropy", entr1);
		filter->addOutputValue("Image 2 Entropy", entr2);
		filter->addOutputValue("Joint Entropy", jointEntr);
		filter->addOutputValue("Mutual Information", mutInf);
		filter->addOutputValue("Normalized Mutual Information 1", norMutInf1);
		filter->addOutputValue("Normalized Mutual Information 2", norMutInf2);
	}
	if (parameters["Structural Similarity Index"].toBool())
	{
		itk::ImageRegionConstIterator<ImageType> imgIt(img, img->GetLargestPossibleRegion());
		itk::ImageRegionConstIterator<ImageType> refIt(ref, ref->GetLargestPossibleRegion());
		imgIt.GoToBegin(); refIt.GoToBegin();
		double covSum = 0;	size_t count = 0;
		while (!imgIt.IsAtEnd() && !refIt.IsAtEnd())
		{
			covSum += (imgIt.Get() - imgMean) * (refIt.Get() - refMean);
			++imgIt; ++refIt; ++count;
		}
		double covariance = covSum / count;
		double c1 = std::pow(parameters["Structural Similarity k1"].toDouble() * range, 2);
		double c2 = std::pow(parameters["Structural Similarity k2"].toDouble() * range, 2);
		double ssim = ((2 * imgMean * refMean + c1) * (2 * covariance + c2)) /
			((imgMean * imgMean + refMean * refMean + c1) * (imgVar + refVar + c2));
		filter->addOutputValue("Structural Similarity Index", ssim);
	}
	if (parameters["Equal pixel rate"].toBool())
	{
		filter->addOutputValue("Equal pixel rate", computeEqualPixelRate<ImageType>(img, ref));
	}
}

iASimilarity::iASimilarity() : iAFilter("Similarity", "Metrics",
	"Calculates the similarity between two images according to different metrics.<br/>"
	"<strong>NOTE</strong>: Normalize the images (by setting its mean to zero and variance to one -> "
	"see the the Normalize Image Filter under Intensity) before calculating the similarity metrics!<br/>"
	"<a href=\"https://itk.org/Doxygen/html/ImageSimilarityMetricsPage.html\">General information on ITK similarity metrics</a>.<br/>"
	"<em><a href=\"https://itk.org/Doxygen/html/classitk_1_1MeanSquaresImageToImageMetric.html\">"
	"Mean Squared Error (MSE) Metric</a></em>: The optimal value of the metric is zero, which means that the two input images are equal. "
	"Poor matches between images A and B result in large values of the metric. This metric relies on the assumption that intensity "
	"representing the same homologous point must be the same in both images.<br/>"
	"<em>RMSE</em> (Root Mean Square Error) yields the square root of the MSE, which is the mean absolute difference in intensity, "
	"which is a more intuitive measure for difference as it is in the same unit as the intensity values of the image. "
	"The <em>Normalized RMSE</em> yields a value between 0 and 1, where 0 signifies that the images are equal, "
	"and 1 that the images are as different as possible (that is, that they have the maximum possible difference at each point). "
	"It is calculated by dividing the RMSE by the maximum possible difference.<br/>"
	"The <em>Peak Signal-to-Noise Ratio</em> is computed as 10 * log10(max_intensity² / MSE), where MSE is the Mean Squared Error, "
	"and max_intensity is the maximum possible intensity difference between the two specified images.<br/>"
	"<a href=\"https://itk.org/Doxygen/html/classitk_1_1NormalizedCorrelationImageToImageMetric.html\">"
	"Normalized Correlation Metric</a>: Note the −1 factor in the metric computation. This factor is used to make the "
	"metric be optimal when its minimum is reached.The optimal value of the metric is then minus one. Misalignment "
	"between the images results in small measure values.<br/>"
	"More Information on Mutual Information is given in the "
	"<a href=\"https://itk.org/ItkSoftwareGuide.pdf\">ITK Software Guide</a> in the sections '3.10.4 Mutual "
	"Information Metric' (pp. 262-264) and '5.3.2 Information Theory' (pp. 462-471)."
	"The <em>Structural Similarity Index</em> Metric (SSIM) is a metric calculated from mean, variance and covariance "
	"of the two compared images. For more details see e.g. the "
	"<a href=\"https://en.wikipedia.org/wiki/Structural_similarity\">Structural Similarity index article in wikipedia</a>, "
	"the two parameters k1 and k2 are used exactly as defined there. "
	"<em>Equal pixel rate</em> computes the ratio between voxels with same value and the total voxel count.",
	2, 0)
{
	addParameter("Index X", Discrete, 0);
	addParameter("Index Y", Discrete, 0);
	addParameter("Index Z", Discrete, 0);
	addParameter("Size X", Discrete, 1);
	addParameter("Size Y", Discrete, 1);
	addParameter("Size Z", Discrete, 1);
	addParameter("Mean Squared Error", Boolean, false);
	addParameter("RMSE", Boolean, true);
	addParameter("Normalized RMSE", Boolean, false);
	addParameter("Peak Signal-to-Noise Ratio", Boolean, true);
	addParameter("Mean Absolute Error", Boolean, true);
	addParameter("Normalized Correlation", Boolean, false);
	addParameter("Mutual Information", Boolean, false);
	addParameter("Histogram Bins", Discrete, 256, 2);
	addParameter("Structural Similarity Index", Boolean, true);
	addParameter("Structural Similarity k1", Continuous, 0.01);
	addParameter("Structural Similarity k2", Continuous, 0.03);
	addParameter("Equal pixel rate", Boolean, false);
}

IAFILTER_CREATE(iASimilarity)

void iASimilarity::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(similarity_metrics, inputPixelType(), this, parameters);
}

QSharedPointer<iAFilterRunnerGUI> iASimilarityFilterRunner::create()
{
	return QSharedPointer<iAFilterRunnerGUI>(new iASimilarityFilterRunner());
}

QMap<QString, QVariant> iASimilarityFilterRunner::loadParameters(QSharedPointer<iAFilter> filter, MdiChild* sourceMdi)
{
	auto params = iAFilterRunnerGUI::loadParameters(filter, sourceMdi);
	int const * dim = sourceMdi->imagePointer()->GetDimensions();
	if (params["Index X"].toUInt() >= dim[0])
		params["Index X"] = 0;
	if (params["Index Y"].toUInt() >= dim[1])
		params["Index Y"] = 0;
	if (params["Index Z"].toUInt() >= dim[2])
		params["Index Z"] = 0;
	params["Size X"] = std::min(params["Size X"].toUInt(), dim[0] - params["Index X"].toUInt());
	params["Size Y"] = std::min(params["Size Y"].toUInt(), dim[1] - params["Index Y"].toUInt());
	params["Size Z"] = std::min(params["Size Z"].toUInt(), dim[2] - params["Index Z"].toUInt());
	return params;
}
