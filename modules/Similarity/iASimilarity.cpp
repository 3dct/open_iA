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

#include <vtkImageData.h>

#include <QLocale>

template<class T>
int similarity_metrics_template( iAProgress* p, iAConnector* image2, iAConnector* image, bool ms, bool nc, bool mi, int miHistoBins,
	double &msVal, double &ncVal, double &entr1, double &entr2, double &jointEntr, double &mutInf, double &norMutInf1, double &norMutInf2)
{
	typedef itk::Image< T, DIM > ImageType;
	typedef itk::TranslationTransform < double, DIM > TransformType;
	typename TransformType::Pointer transform = TransformType::New();
	transform->SetIdentity();
	typedef itk::LinearInterpolateImageFunction<ImageType, double >	InterpolatorType;
	typename InterpolatorType::Pointer interpolator = InterpolatorType::New();
	interpolator->SetInputImage(dynamic_cast<ImageType *>(image->GetITKImage()));
	TransformType::ParametersType params(transform->GetNumberOfParameters());

	if (ms)
	{
		typedef itk::MeanSquaresImageToImageMetric<	ImageType, ImageType > MSMetricType;
		typename MSMetricType::Pointer msmetric = MSMetricType::New();
		msmetric->SetFixedImage(dynamic_cast<ImageType *>(image->GetITKImage()));
		msmetric->SetFixedImageRegion(dynamic_cast<ImageType *>(image->GetITKImage())->GetLargestPossibleRegion());
		msmetric->SetMovingImage(dynamic_cast<ImageType *>(image2->GetITKImage()));
		msmetric->SetTransform(transform);
		msmetric->SetInterpolator(interpolator);
		params.Fill(0.0);
		msmetric->Initialize();
		msVal = msmetric->GetValue(params);
	}
	if (nc)
	{
		typedef itk::NormalizedCorrelationImageToImageMetric< ImageType, ImageType > NCMetricType;
		typename NCMetricType::Pointer ncmetric = NCMetricType::New();
		ncmetric->SetFixedImage(dynamic_cast<ImageType *>(image->GetITKImage()));
		ncmetric->SetFixedImageRegion(dynamic_cast<ImageType *>(image->GetITKImage())->GetLargestPossibleRegion());
		ncmetric->SetMovingImage(dynamic_cast<ImageType *>(image2->GetITKImage()));
		ncmetric->SetTransform(transform);
		ncmetric->SetInterpolator(interpolator);
		params.Fill(0.0);
		ncmetric->Initialize();
		ncVal = ncmetric->GetValue(params);
	}
	if (mi)
	{
		//ITK-Example: https://itk.org/Doxygen/html/Examples_2Statistics_2ImageMutualInformation1_8cxx-example.html
		typedef itk::JoinImageFilter< ImageType, ImageType > JoinFilterType;
		typename JoinFilterType::Pointer joinFilter = JoinFilterType::New();
		joinFilter->SetInput1(dynamic_cast<ImageType *>(image->GetITKImage()));
		joinFilter->SetInput2(dynamic_cast<ImageType *>(image2->GetITKImage()));
		joinFilter->Update();

		typedef typename JoinFilterType::OutputImageType  VectorImageType;
		typedef itk::Statistics::ImageToHistogramFilter<VectorImageType >  HistogramFilterType;
		typename HistogramFilterType::Pointer histogramFilter = HistogramFilterType::New();
		histogramFilter->SetInput(joinFilter->GetOutput());
		histogramFilter->SetMarginalScale(10.0);
		typedef typename HistogramFilterType::HistogramSizeType   HistogramSizeType;
		HistogramSizeType size(2);
		size[0] = miHistoBins;  // number of bins for the first  channel
		size[1] = miHistoBins;  // number of bins for the second channel
		histogramFilter->SetHistogramSize(size);
		typedef typename HistogramFilterType::HistogramMeasurementVectorType HistogramMeasurementVectorType;
		HistogramMeasurementVectorType binMinimum(3);
		HistogramMeasurementVectorType binMaximum(3);
		binMinimum[0] = -0.5;
		binMinimum[1] = -0.5;
		binMinimum[2] = -0.5;
		binMaximum[0] = miHistoBins + 0.5;
		binMaximum[1] = miHistoBins + 0.5;
		binMaximum[2] = miHistoBins + 0.5;
		histogramFilter->SetHistogramBinMinimum(binMinimum);
		histogramFilter->SetHistogramBinMaximum(binMaximum);
		histogramFilter->Update();
		typedef typename HistogramFilterType::HistogramType  HistogramType;
		const HistogramType * histogram = histogramFilter->GetOutput();
		typename HistogramType::ConstIterator itr = histogram->Begin();
		typename HistogramType::ConstIterator end = histogram->End();
		const double Sum = histogram->GetTotalFrequency();

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

		size[0] = miHistoBins;  // number of bins for the first  channel
		size[1] = 1;  // number of bins for the second channel
		histogramFilter->SetHistogramSize(size);
		histogramFilter->Update();
		itr = histogram->Begin();
		end = histogram->End();
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
		size[1] = miHistoBins;  // number of bins for the second channel
		histogramFilter->SetHistogramSize(size);
		histogramFilter->Update();
		itr = histogram->Begin();
		end = histogram->End();
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

		mutInf = entr1 + entr2 - jointEntr;
		norMutInf1 = 2.0 * mutInf / (entr1 + entr2);
		norMutInf2 = (entr1 + entr2) / jointEntr;
	}
	return EXIT_SUCCESS;
}


iASimilarity::iASimilarity(QString fn, iASimilarityFilterType fid, vtkImageData* i, vtkPolyData* p, iALogger* logger, QObject* parent)
	: iAAlgorithm(fn, i, p, logger, parent), m_type(fid)
{}

void iASimilarity::run()
{
	switch (m_type)
	{
	case SIMILARITY_METRICS:
		calcSimilarityMetrics(); break;
	default:
		addMsg(tr("unknown filter type"));
	}
}

void iASimilarity::calcSimilarityMetrics()
{
	addMsg(tr("%1  %2 started.").arg(QLocale().toString(Start(), QLocale::ShortFormat))
		.arg(getFilterName()));

	getConnector()->SetImage(getVtkImageData()); getConnector()->Modified();
	getFixedConnector()->SetImage(image2); getFixedConnector()->Modified();
	double msVal = 0.0, ncVal = -1.0, entr1Val = 0.0, entr2Val = 0.0, jointEntrVal = 0.0, mutInfVal = 0.0, norMutInf1Val = 0.0, norMutInf2Val = 0.0;
	
	try
	{
		iAConnector::ITKScalarPixelType itkType = getConnector()->GetITKScalarPixelType();

		ITK_TYPED_CALL(similarity_metrics_template, itkType, getItkProgress(), getFixedConnector(), getConnector(),
			meanSqaures, normalizedCorrelation, mutualInformation, miHistoBins, msVal, ncVal, entr1Val, entr2Val, jointEntrVal, mutInfVal, norMutInf1Val, norMutInf2Val);
	}
	catch (itk::ExceptionObject &excep)
	{
		addMsg(tr("%1  %2 terminated unexpectedly. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
			.arg(getFilterName())
			.arg(Stop()));
		addMsg(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
	
	addMsg("");
	if (meanSqaures)
	{
		addMsg(QString("    Mean Squares Metric = %1\n").arg(QString::number(msVal)));
	}
	if (normalizedCorrelation)
	{
		addMsg(QString("    Normalized Correlation Metric = %1\n").arg(QString::number(ncVal)));
	}
	if (mutualInformation)
	{
		addMsg(QString("    Image 1 Entrop = %1").arg(QString::number(entr1Val)));
		addMsg(QString("    Image 2 Entrop = %1").arg(QString::number(entr2Val)));
		addMsg(QString("    Joint Entropy = %1").arg(QString::number(jointEntrVal)));
		addMsg(QString("    Mutual Information = %1").arg(QString::number(mutInfVal)));
		addMsg(QString("    Normalized Mutual Information 1 = %1").arg(QString::number(norMutInf1Val)));
		addMsg(QString("    Normalized Mutual Information 2 = %1\n").arg(QString::number(norMutInf2Val)));
	}
	
	addMsg(tr("%1  %2 finished. Elapsed time: %3 ms").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(getFilterName())
		.arg(Stop()));
}
