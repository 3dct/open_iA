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
#include "iAThresholding.h"

#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include "itkMaximumDistance.h"

#include <vtkImageData.h>

#include <itkAdaptiveOtsuThresholdImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkGradientMagnitudeImageFilter.h>
#include <itkOtsuThresholdImageFilter.h>
#include <itkOtsuMultipleThresholdsImageFilter.h>
#include <itkRobustAutomaticThresholdImageFilter.h>
#include <itkRemovePeaksOtsuThresholdImageFilter.h>

#include <QLocale>


// Binary Threshold

template<class T>
void binary_threshold_template(double l, double u, double o, double i, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< T, 3 >   OutputImageType;
	typedef itk::BinaryThresholdImageFilter<InputImageType, OutputImageType> BTIFType;
	typename BTIFType::Pointer filter = BTIFType::New();
	filter->SetLowerThreshold(T(l));
	filter->SetUpperThreshold(T(u));
	filter->SetOutsideValue(T(o));
	filter->SetInsideValue(T(i));
	filter->SetInput(dynamic_cast<InputImageType *>(image->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iABinaryThreshold)

void iABinaryThreshold::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(binary_threshold_template, itkType,
		parameters["Lower Threshold"].toDouble(),
		parameters["Upper Threshold"].toDouble(),
		parameters["Outside Value"].toDouble(),
		parameters["Inside Value"].toDouble(),
		m_progress, m_con);
}

iABinaryThreshold::iABinaryThreshold() :
	iAFilter("Binary Thresholding", "Segmentation/Global Thresholding",
		"Computes a segmented image based on whether the intensity is inside a given interval.<br/>"
		"Two thresholds (lower, upper) can be specified; "
		"if a voxel value is between these two (including the threshold values themselves), "
		"then the output is set to the <em>inside</em> value at this voxel, "
		"otherwise the <em>outside</em> value.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BinaryThresholdImageFilter.html\">"
		"Binary Threshold Filter</a> in the ITK documentation.")
{
	AddParameter("Lower Threshold", Continuous, 0);
	AddParameter("Upper Threshold", Continuous, 32768);
	AddParameter("Outside Value", Continuous, 0);
	AddParameter("Inside Value", Continuous, 1);
}


// Robust Automatic Threshold (RAT)

template<class T> 
void rats_threshold_template(double & rthresh_ptr, double pow, double o, double i, iAProgress* p, iAConnector* image  )
{
	typedef typename itk::Image< T, 3 >   InputImageType;
	typedef typename itk::Image< T, 3 >   OutputImageType;
	typedef typename itk::Image< float, 3 >   GradientImageType;
	typedef itk::GradientMagnitudeImageFilter< InputImageType, GradientImageType > GMFType;
	typename GMFType::Pointer gmfilter = GMFType::New();
	gmfilter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	p->Observe( gmfilter );
	gmfilter->Update(); 
	typedef typename itk::RobustAutomaticThresholdImageFilter < InputImageType, GradientImageType, OutputImageType > RATIFType;
	typename RATIFType::Pointer filter = RATIFType::New();
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	filter->SetGradientImage(gmfilter->GetOutput());
	filter->SetOutsideValue( T(o) );
	filter->SetInsideValue( T(i) );
	filter->SetPow( pow );
	p->Observe( filter );
	filter->Update();
	rthresh_ptr = (double)filter->GetThreshold();
	image->SetImage(filter->GetOutput());
	image->Modified();
	gmfilter->ReleaseDataFlagOn();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iARatsThreshold)

void iARatsThreshold::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	double threshold;
	ITK_TYPED_CALL(rats_threshold_template, itkType,
		threshold,
		parameters["Power"].toDouble(),
		parameters["Outside Value"].toDouble(),
		parameters["Inside Value"].toDouble(),
		m_progress, m_con);
	AddMsg(QString("%1  Rats Threshold = %2").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(QString::number(threshold)));
}

iARatsThreshold::iARatsThreshold() :
	iAFilter("RATS", "Segmentation/Global Thresholding",
		"Robust automatic threshold selection. <br/>"
		"This filter determines a global threshold by weighting the pixel "
		"values by their gradient (thus setting the threshold to the value "
		"where the intensities change most).<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RobustAutomaticThresholdImageFilter.html\">"
		"RATS Filter</a> in the ITK documentation.")
{
	AddParameter("Power", Continuous, 1);
	AddParameter("Outside Value", Continuous, 0);
	AddParameter("Inside Value", Continuous, 1);
}


// Otsu's Threshold

template<class T> 
void otsu_threshold_template(double & othresh, double b, double o, double i, bool removepeaks, iAProgress* p, iAConnector* image  )
{
	typedef typename itk::Image< T, 3 >   InputImageType;
	typedef typename itk::Image< T, 3 >   OutputImageType;
	typedef typename itk::OtsuThresholdImageFilter < InputImageType, OutputImageType > OTIFType;
	typedef typename itk::RemovePeaksOtsuThresholdImageFilter < InputImageType, OutputImageType > RPOTIFType;

	if (removepeaks)
	{
		typename RPOTIFType::Pointer filter = RPOTIFType::New();
		filter->SetNumberOfHistogramBins( T (b) );
		filter->SetOutsideValue( T(o) );
		filter->SetInsideValue( T(i) );
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		othresh = (double)filter->GetThreshold();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}
	else
	{
		typename OTIFType::Pointer filter = OTIFType::New();
		filter->SetNumberOfHistogramBins( T (b) );
		filter->SetOutsideValue( T(o) );
		filter->SetInsideValue( T(i) );
		filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
		p->Observe( filter );
		filter->Update();
		othresh = (double)filter->GetThreshold();
		image->SetImage(filter->GetOutput());
		image->Modified();
		filter->ReleaseDataFlagOn();
	}
}

IAFILTER_CREATE(iAOtsuThreshold)

void iAOtsuThreshold::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	double threshold;
	ITK_TYPED_CALL(otsu_threshold_template, itkType,
		threshold,
		parameters["Number of Histogram Bins"].toDouble(),
		parameters["Outside Value"].toDouble(),
		parameters["Inside Value"].toDouble(),
		parameters["Remove Peaks"].toBool(),
		m_progress, m_con);
	AddMsg(QString("%1  Otsu Threshold = %2").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(QString::number(threshold)));
}

iAOtsuThreshold::iAOtsuThreshold() :
	iAFilter("Otsu Threshold", "Segmentation/Global Thresholding",
		"Creates an segmented image using Otsu's method.<br/>"
		"The result is segmented into foreground (inside value) and background (outside value), "
		"the method tries to maximize the between-class variance using a histogram.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1OtsuThresholdImageFilter.html\">"
		"Otsu Threshold Filter</a> in the ITK documentation.")
{
	AddParameter("Number of Histogram Bins", Discrete, 128, 2);
	AddParameter("Outside Value", Continuous, 0);
	AddParameter("Inside Value", Continuous, 1);
	AddParameter("Remove Peaks", Boolean, false);
}


// Adaptive Otsu

template<class T>
void adaptive_otsu_threshold(double r, unsigned int s, unsigned int l, unsigned int c, double b, double o, double i,
	unsigned int splineOrder,
	iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< T, 3 >   OutputImageType;
	typename InputImageType::SizeType radius;
	radius.Fill(T(r));
	typedef itk::AdaptiveOtsuThresholdImageFilter< InputImageType, OutputImageType > AOTIFType;
	typename AOTIFType::Pointer filter = AOTIFType::New();
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	filter->SetOutsideValue(T(o));
	filter->SetInsideValue(T(i));
	filter->SetNumberOfHistogramBins(b);
	filter->SetNumberOfControlPoints(c);
	filter->SetNumberOfLevels(l);
	filter->SetNumberOfSamples(s);
	filter->SetRadius(radius);
	filter->SetSplineOrder(splineOrder);
	filter->Update();
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iAAdaptiveOtsuThreshold)

void iAAdaptiveOtsuThreshold::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(adaptive_otsu_threshold, itkType,
		parameters["Radius"].toDouble(),
		parameters["Samples"].toUInt(),
		parameters["Levels"].toUInt(),
		parameters["Control Points"].toUInt(),
		parameters["Number of Histogram Bins"].toUInt(),
		parameters["Outside Value"].toDouble(),
		parameters["Inside Value"].toDouble(),
		parameters["Spline Order"].toUInt(),
		m_progress, m_con);
}

iAAdaptiveOtsuThreshold::iAAdaptiveOtsuThreshold() :
	iAFilter("Adaptive Otsu", "Segmentation/Local Thresholding",
		"A local thresholding filter based on Otsu's method.<br/>"
		"For more information, see the "
		"<a href=\"https://github.com/ITKTools/ITKTools/blob/master/src/thresholdimage/itkAdaptiveOtsuThresholdImageFilter.h\">"
		"Adaptive Otsu Threshold source code</a> in the ITKTools.")
{
	AddParameter("Number of Histogram Bins", Discrete, 256, 2);
	AddParameter("Outside Value", Continuous, 0);
	AddParameter("Inside Value", Continuous, 1);
	AddParameter("Radius", Continuous, 8);
	AddParameter("Samples", Discrete, 5000);
	AddParameter("Levels", Discrete, 3);
	AddParameter("Control Points", Discrete, 50, 1);
	AddParameter("Spline Order", Discrete, 3, 2);
}


// Otsu Multiple Threshold

template<class T>
void otsu_multiple_threshold_template( std::vector<double>& omthresh, double b, double n, bool valleyemphasis, iAProgress* p, iAConnector* image )
{
	typedef typename itk::Image< T, 3 >   InputImageType;
	typedef typename itk::Image< T, 3 >   OutputImageType;
	typedef typename itk::OtsuMultipleThresholdsImageFilter < InputImageType, OutputImageType > OMTIFType;
	typename OMTIFType::Pointer filter = OMTIFType::New();
	filter->SetNumberOfHistogramBins( T (b) );
	filter->SetNumberOfThresholds( T (n) );
	filter->SetInput( dynamic_cast< InputImageType * >( image->GetITKImage() ) );
	filter->SetValleyEmphasis( valleyemphasis );
	p->Observe( filter );
	filter->Update();
	omthresh = filter->GetThresholds();
	image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iAOtsuMultipleThreshold)

void iAOtsuMultipleThreshold::Run(QMap<QString, QVariant> const & parameters)
{
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	std::vector<double> omthresh;
	ITK_TYPED_CALL(otsu_multiple_threshold_template, itkType,
		omthresh,
		parameters["Number of Histogram Bins"].toDouble(),
		parameters["Number of Thresholds"].toDouble(),
		parameters["Valley Emphasis"].toBool(),
		m_progress, m_con);
	AddMsg(QString("%1  Otsu Multiple Thresholds = %2").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(omthresh.size()));
	for (int i = 0; i< omthresh.size(); i++) AddMsg(QString("    Threshold number %1 = %2").arg(i).arg(omthresh[i]));
}

iAOtsuMultipleThreshold::iAOtsuMultipleThreshold() :
	iAFilter("Otsu Multiple Thresholds", "Segmentation/Global Thresholding",
		"Performs Otsu's method with multiple thresholds.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1OtsuMultipleThresholdsImageFilter.html\">"
		"Otsu Multiple Threshold Filter</a> in the ITK documentation.")
{
	AddParameter("Number of Histogram Bins", Discrete, 256, 2);
	AddParameter("Number of Thresholds", Discrete, 2, 1);
	AddParameter("Valley Emphasis", Boolean, 1);
}


// Maximum Distance Threshold

template<class T>
void maximum_distance_template(int* mdfHighInt_ptr, int* mdfLowInt_ptr, int* mdfThresh_ptr,
	double li, double b, bool u, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, 3 >   InputImageType;
	typedef itk::Image< T, 3 >   OutputImageType;
	typedef itk::MaximumDistance< InputImageType > MaximumDistanceType;
	auto filter = MaximumDistanceType::New();
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	filter->SetBins(b);
	if (u)
		filter->SetCentre(li);
	else
		filter->SetCentre(32767);
	p->Observe(filter);
	filter->Update();
	image->SetImage(filter->GetOutput());
	filter->GetThreshold(mdfThresh_ptr);
	filter->GetLowIntensity(mdfLowInt_ptr);
	filter->GetHighIntensity(mdfHighInt_ptr);
	image->Modified();
}

void iAMaximumDistance::Run(QMap<QString, QVariant> const & parameters)
{
	int mdfHighInt, mdfLowInt, mdfThresh;
	iAConnector::ITKScalarPixelType itkType = m_con->GetITKScalarPixelType();
	ITK_TYPED_CALL(maximum_distance_template, itkType,
		&mdfHighInt, &mdfLowInt, &mdfThresh,
		parameters["Low Intensity"].toDouble(),
		parameters["Number of Histogram Bins"].toDouble(),
		parameters["Use Low Intensity"].toBool(),
		m_progress, m_con);
	AddMsg(QString("Maximum Distance Threshold = %1").arg(QString::number(mdfThresh, 10)));
	AddMsg(QString("Maximum Distance Low Peak = %1").arg(QString::number(mdfLowInt, 10)));
	AddMsg(QString("Maximum Distance High Peak = %1").arg(QString::number(mdfHighInt, 10)));
}

IAFILTER_CREATE(iAMaximumDistance)

iAMaximumDistance::iAMaximumDistance() :
	iAFilter("Maximum Distance", "Segmentation/Global Thresholding",
		"A global threshold based on the maximum distance of peaks in the histogram, for voids segmentation.")
{
	AddParameter("Number of Histogram Bins", Discrete, 256, 2);
	AddParameter("Low Intensity", Continuous, 0);
	AddParameter("Use Low Intensity", Boolean, false);
}
