// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h>    // for DIM
#include <iAAttributeDescriptor.h>
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAProgress.h>
#include <iAStringHelper.h>
#include <iATypedCallHelper.h>

// from Toolkit
#include <iAAdaptiveOtsuThresholdImageFilter.h>
#include <iAMaximumDistanceFilter.h>
#include <iARemovePeaksOtsuThresholdImageFilter.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#if __clang_major__ > 10
#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#else
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
#endif
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <itkBinaryThresholdImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkGradientMagnitudeImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#pragma clang diagnostic push
#if __clang_major__ > 10
#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#else
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
#endif
#include <itkHuangThresholdImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#include <itkIntermodesThresholdImageFilter.h>
#include <itkIsoDataThresholdImageFilter.h>
#include <itkKittlerIllingworthThresholdImageFilter.h>
#include <itkLiThresholdImageFilter.h>
#include <itkMaximumEntropyThresholdImageFilter.h>
#include <itkMomentsThresholdImageFilter.h>
#include <itkOtsuThresholdImageFilter.h>
#include <itkOtsuMultipleThresholdsImageFilter.h>
#include <itkRobustAutomaticThresholdImageFilter.h>
#include <itkRenyiEntropyThresholdImageFilter.h>
#include <itkShanbhagThresholdImageFilter.h>
#include <itkThresholdLabelerImageFilter.h>
#include <itkTriangleThresholdImageFilter.h>
#include <itkYenThresholdImageFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

IAFILTER_DEFAULT_CLASS(iABinaryThreshold)
IAFILTER_DEFAULT_CLASS(iAMultiThreshold)
IAFILTER_DEFAULT_CLASS(iARatsThreshold)
IAFILTER_DEFAULT_CLASS(iAOtsuThreshold)
IAFILTER_DEFAULT_CLASS(iAAdaptiveOtsuThreshold)
IAFILTER_DEFAULT_CLASS(iAOtsuMultipleThreshold)
IAFILTER_DEFAULT_CLASS(iAMaximumDistance)
IAFILTER_DEFAULT_CLASS(iAParameterlessThresholding)

// Binary Threshold

template<class T>
void binary_threshold(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using OutputImageType = itk::Image<T, DIM>;
	auto binThreshFilter = itk::BinaryThresholdImageFilter<InputImageType, OutputImageType>::New();
	binThreshFilter->SetLowerThreshold(T(parameters["Lower threshold"].toDouble()));
	binThreshFilter->SetUpperThreshold(T(parameters["Upper threshold"].toDouble()));
	binThreshFilter->SetOutsideValue(T(parameters["Outside value"].toDouble()));
	binThreshFilter->SetInsideValue(T(parameters["Inside value"].toDouble()));
	binThreshFilter->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	filter->progress()->observe(binThreshFilter);
	binThreshFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(binThreshFilter->GetOutput()));
}

void iABinaryThreshold::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(binary_threshold, inputScalarType(), this, parameters);
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
	addParameter("Lower threshold", iAValueType::Continuous, 0);
	addParameter("Upper threshold", iAValueType::Continuous, 32768);
	addParameter("Outside value", iAValueType::Continuous, 0);
	addParameter("Inside value", iAValueType::Continuous, 1);
}


// Multi Threshold

template <class T>
void multi_threshold(iAFilter* filter, QVariantMap const& parameters)
{

	std::vector<T> thresholds;

	QString numString = parameters["Thresholds"].toString().replace(" ", "");
	;
	auto numberStringArray = numString.split(";");


	for (QString numberString : numberStringArray)
	{
		bool ok;
		thresholds.push_back(iAConverter<T>::toT(numberString, &ok));
	}

	using InputImageType = itk::Image<T, DIM>;
	using OutputImageType = itk::Image<T, DIM>;
	auto multiThreshFilter = itk::ThresholdLabelerImageFilter<InputImageType, OutputImageType>::New();
	multiThreshFilter->SetThresholds(thresholds);
	multiThreshFilter->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	filter->progress()->observe(multiThreshFilter);
	multiThreshFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(multiThreshFilter->GetOutput()));
}

void iAMultiThreshold::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(multi_threshold, inputScalarType(), this, parameters);
}

iAMultiThreshold::iAMultiThreshold() :
	iAFilter("Multi Thresholding", "Segmentation/Global Thresholding",

		"Label an input image according to a set of thresholds<br/>"
		"This filter produces an output image whose pixels are labeled progressively according to the classes identified by a set of thresholds.Values equal to a threshold is considered to be in the lower class."
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ThresholdLabelerImageFilter.html\">"
		"Multi Threshold Filter</a> in the ITK documentation.<br/>"
		"The thresholds are seperated with semicolon \";\"")
{
	addParameter("Thresholds", iAValueType::String);
}



// Robust Automatic Threshold (RAT)

template<class T>
void rats_threshold(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using OutputImageType = itk::Image<T, DIM>;
	using GradientImageType = itk::Image< float, DIM>;
	auto gmfilter = itk::GradientMagnitudeImageFilter<InputImageType, GradientImageType>::New();
	gmfilter->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	filter->progress()->observe( gmfilter );
	gmfilter->Update();
	auto ratsFilter = itk::RobustAutomaticThresholdImageFilter<InputImageType, GradientImageType, OutputImageType>::New();
	ratsFilter->SetInput(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	ratsFilter->SetGradientImage(gmfilter->GetOutput());
	ratsFilter->SetOutsideValue(static_cast<T>(parameters["Outside value"].toDouble()));
	ratsFilter->SetInsideValue(static_cast<T>(parameters["Inside value"].toDouble()));
	ratsFilter->SetPow( parameters["Power"].toDouble() );
	filter->progress()->observe( ratsFilter );
	ratsFilter->Update();
	filter->addOutputValue("Threshold", static_cast<double>(ratsFilter->GetThreshold()));
	filter->addOutput(std::make_shared<iAImageData>(ratsFilter->GetOutput()));
}

void iARatsThreshold::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(rats_threshold, inputScalarType(), this, parameters);
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
	addParameter("Power", iAValueType::Continuous, 1);
	addParameter("Outside value", iAValueType::Continuous, 0);
	addParameter("Inside value", iAValueType::Continuous, 1);
	addOutputValue("Threshold");
}


// Otsu's Threshold
namespace
{
	constexpr int OtsuThresholdDefaultNumBins = 128;
}

template<class T>
void otsu_threshold(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using OutputImageType = itk::Image<T, DIM>; // output is always binary, so using (unsigned) char might make sense;
	// especially for real-valued types same type as input doesn't make sense!

	double numBins = parameters["Number of histogram bins"].toDouble();
	double maxBins = std::numeric_limits<unsigned int>::max();
	if (numBins < maxBins)
	{	// max bins must be smaller than unsigned int max, but also smaller than number of values in current datatype!
		maxBins = static_cast<double>(std::numeric_limits<T>::max()) + 1.0;
	}
	if (numBins < 2 || numBins > maxBins)
	{
		LOG(lvlWarn,
			QString("Number of histogram bins outside of valid range 2..%1, resetting it to default value %2")
				.arg(maxBins)
				.arg(OtsuThresholdDefaultNumBins));
		numBins = OtsuThresholdDefaultNumBins;
	}
	if (parameters["Remove peaks"].toBool())
	{
		auto otsuFilter = iARemovePeaksOtsuThresholdImageFilter<InputImageType, OutputImageType>::New();
		otsuFilter->SetNumberOfHistogramBins(static_cast<unsigned int>(numBins));
		otsuFilter->SetOutsideValue(static_cast<T>(parameters["Outside value"].toDouble()));
		otsuFilter->SetInsideValue(static_cast<T>(parameters["Inside value"].toDouble()));
		otsuFilter->SetInput(dynamic_cast<InputImageType*>( filter->imageInput(0)->itkImage() ) );
		filter->progress()->observe( otsuFilter );
		otsuFilter->Update();
		filter->addOutputValue("Threshold", (double)otsuFilter->GetThreshold());
		filter->addOutput(std::make_shared<iAImageData>(otsuFilter->GetOutput()));
	}
	else
	{
		auto otsuFilter = itk::OtsuThresholdImageFilter<InputImageType, OutputImageType>::New();
		otsuFilter->SetNumberOfHistogramBins(static_cast<unsigned int>(numBins));
		otsuFilter->SetOutsideValue( static_cast<T>(parameters["Outside value"].toDouble()) );
		otsuFilter->SetInsideValue(static_cast<T>(parameters["Inside value"].toDouble()));
		otsuFilter->SetInput(dynamic_cast< InputImageType * >( filter->imageInput(0)->itkImage() ) );
		filter->progress()->observe( otsuFilter );
		otsuFilter->Update();
		filter->addOutputValue("Threshold", (double)otsuFilter->GetThreshold());
		filter->addOutput(std::make_shared<iAImageData>(otsuFilter->GetOutput()));
	}
}

void iAOtsuThreshold::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(otsu_threshold, inputScalarType(), this, parameters);
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
	addParameter("Number of histogram bins", iAValueType::Discrete, 128, 2);
	addParameter("Outside value", iAValueType::Continuous, 0);
	addParameter("Inside value", iAValueType::Continuous, 1);
	addParameter("Remove peaks", iAValueType::Boolean, false);
	addOutputValue("Threshold");
}


// Adaptive Otsu

template<class T>
void adaptive_otsu_threshold(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using OutputImageType = itk::Image<T, DIM>;
	typename InputImageType::SizeType radius;
	radius.Fill(T(parameters["Radius"].toDouble()));
	auto adotFilter = iAAdaptiveOtsuThresholdImageFilter<InputImageType, OutputImageType>::New();
	adotFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	adotFilter->SetOutsideValue(T(parameters["Outside value"].toDouble()));
	adotFilter->SetInsideValue(T(parameters["Inside value"].toDouble()));
	adotFilter->SetNumberOfHistogramBins(parameters["Number of histogram bins"].toUInt());
	adotFilter->SetNumberOfControlPoints(parameters["Control points"].toUInt());
	adotFilter->SetNumberOfLevels(parameters["Levels"].toUInt());
	adotFilter->SetNumberOfSamples(parameters["Samples"].toUInt());
	adotFilter->SetRadius(radius);
	adotFilter->SetSplineOrder(parameters["Spline order"].toUInt());
	adotFilter->Update();
	filter->progress()->observe(adotFilter);
	adotFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(adotFilter->GetOutput()));
}

void iAAdaptiveOtsuThreshold::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(adaptive_otsu_threshold, inputScalarType(), this, parameters);
}

iAAdaptiveOtsuThreshold::iAAdaptiveOtsuThreshold() :
	iAFilter("Adaptive Otsu", "Segmentation/Local Thresholding",
		"A local thresholding filter based on Otsu's method.<br/>"
		"For more information, see the "
		"<a href=\"https://github.com/ITKTools/ITKTools/blob/master/src/thresholdimage/itkAdaptiveOtsuThresholdImageFilter.h\">"
		"Adaptive Otsu Threshold source code</a> in the ITKTools.")
{
	addParameter("Number of histogram bins", iAValueType::Discrete, 256, 2);
	addParameter("Outside value", iAValueType::Continuous, 0);
	addParameter("Inside value", iAValueType::Continuous, 1);
	addParameter("Radius", iAValueType::Continuous, 8);
	addParameter("Samples", iAValueType::Discrete, 5000);
	addParameter("Levels", iAValueType::Discrete, 3);
	addParameter("Control points", iAValueType::Discrete, 50, 1);
	addParameter("Spline order", iAValueType::Discrete, 3, 2);
}


// Otsu Multiple Threshold

template<class T>
void otsu_multiple_threshold(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	using OutputImageType = itk::Image<T, DIM>;
	auto otsumultiFilter = itk::OtsuMultipleThresholdsImageFilter<InputImageType, OutputImageType>::New();
	otsumultiFilter->SetNumberOfHistogramBins( T (parameters["Number of histogram bins"].toDouble()) );
	otsumultiFilter->SetNumberOfThresholds( T (parameters["Number of thresholds"].toDouble()) );
	otsumultiFilter->SetInput( dynamic_cast< InputImageType * >( filter->imageInput(0)->itkImage() ) );
	otsumultiFilter->SetValleyEmphasis( parameters["Valley emphasis"].toBool() );
	filter->progress()->observe( otsumultiFilter );
	otsumultiFilter->Update();
	for (size_t i = 0; i < otsumultiFilter->GetThresholds().size(); i++)
	{
		filter->addOutputValue(QString("Otsu multiple threshold %1").arg(i), otsumultiFilter->GetThresholds()[i]);
	}
	filter->addOutput(std::make_shared<iAImageData>(otsumultiFilter->GetOutput()));
}

void iAOtsuMultipleThreshold::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(otsu_multiple_threshold, inputScalarType(), this, parameters);
}

iAOtsuMultipleThreshold::iAOtsuMultipleThreshold() :
	iAFilter("Otsu Multiple Thresholds", "Segmentation/Global Thresholding",
		"Performs Otsu's method with multiple thresholds.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1OtsuMultipleThresholdsImageFilter.html\">"
		"Otsu Multiple Threshold Filter</a> in the ITK documentation.")
{
	addParameter("Number of histogram bins", iAValueType::Discrete, 256, 2);
	addParameter("Number of thresholds", iAValueType::Discrete, 2, 1);
	addParameter("Valley emphasis", iAValueType::Boolean, 1);
}


// Maximum Distance Threshold

template<class T>
void maximum_distance(iAFilter* filter, QVariantMap const & parameters)
{
	using InputImageType = itk::Image<T, DIM>;
	auto maxFilter = iAMaximumDistanceFilter<InputImageType>::New();
	maxFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	maxFilter->SetBinWidth(parameters["Width of histogram bin"].toDouble());
	if (parameters["Use low intensity"].toBool())
	{
		maxFilter->SetCentre(parameters["Low intensity"].toDouble());
	} // if not set, centre / low intensity will be automatically determined as maximum possible pixelType value / 2
	filter->progress()->observe(maxFilter);
	maxFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(maxFilter->GetOutput()));
	filter->addOutputValue("Maximum distance threshold", static_cast<double>(maxFilter->GetThreshold()));
	filter->addOutputValue("Maximum distance low peak",  maxFilter->GetLowIntensity());
	filter->addOutputValue("Maximum distance high peak", maxFilter->GetHighIntensity());
}

void iAMaximumDistance::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(maximum_distance, inputScalarType(), this, parameters);
}

iAMaximumDistance::iAMaximumDistance() :
	iAFilter("Maximum Distance", "Segmentation/Global Thresholding",
		"A global threshold based on the maximum distance of peaks in the histogram, for voids segmentation.<br/>"
		"Note: This filter only works with images with a positive integer pixel data type (unsigned char, unsigned short, unsigned int).")
{
	addParameter("Width of histogram bin", iAValueType::Discrete, 256, 1);
	addParameter("Low intensity", iAValueType::Continuous, 0);
	addParameter("Use low intensity", iAValueType::Boolean, false);
	addOutputValue("Maximum distance threshold");
	addOutputValue("Maximum distance low peak");
	addOutputValue("Maximum distance high peak");
}


// Parameterless Thresholding Methods

namespace
{
	enum ParameterlessThresholdingIDs
	{
		P_OTSU_THRESHOLD,
		P_ISODATA_THRESHOLD,
		P_MAXENTROPY_THRESHOLD,
		P_MOMENTS_THRESHOLD,
		P_YEN_THRESHOLD,
		P_RENYI_THRESHOLD,
		P_SHANBHAG_THRESHOLD,
		P_INTERMODES_THRESHOLD,
		P_HUANG_THRESHOLD,
		P_LI_THRESHOLD,
		P_KITTLERILLINGWORTH_THRESHOLD,
		P_TRIANGLE_THRESHOLD,
		P_MINIMUM_THRESHOLD
	};

	QStringList const & GetParameterlessThresholdingNames()
	{
		static QStringList ParameterlessThresholdingNames;
		if (ParameterlessThresholdingNames.empty())
		{
			ParameterlessThresholdingNames.push_back("Otsu Threshold");
			ParameterlessThresholdingNames.push_back("Isodata Threshold");
			ParameterlessThresholdingNames.push_back("Maximum Entropy Threshold");
			ParameterlessThresholdingNames.push_back("Moments Threshold");
			ParameterlessThresholdingNames.push_back("Yen Threshold");
			ParameterlessThresholdingNames.push_back("Renyi Entropy Threshold");
			ParameterlessThresholdingNames.push_back("Shanbhag Threshold");
			ParameterlessThresholdingNames.push_back("Intermodes Threshold");
			ParameterlessThresholdingNames.push_back("Huang Threshold");
			ParameterlessThresholdingNames.push_back("Li Threshold");
			ParameterlessThresholdingNames.push_back("Kittler Illingworth Threshold");
			ParameterlessThresholdingNames.push_back("Triangle Threshold");
			ParameterlessThresholdingNames.push_back("Minimum Threshold");
		}
		return ParameterlessThresholdingNames;
	}

	int MapMethodNameToID(QString const & name)
	{
		return static_cast<int>(GetParameterlessThresholdingNames().indexOf(name));
	}
}

iAParameterlessThresholding::iAParameterlessThresholding() :
	iAFilter("Parameterless Thresholding", "Segmentation/Global Thresholding",
		"Performs a parameterless global thresholding (threshold determined automatically based on histogram).<br/>"
		"Several different <em>Method</em>s for determining the threshold are available, "
		"you can also set the <em>Number of histogram bins</em> employed in the histogram used for determining the threshold. "
		"The <em>Inside value</em> is assigned to values below the computed threshold value, the <em>Outside value</em> is assigned to values above the threshold.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1OtsuThresholdImageFilter.html\">"
		"Otsu Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1IsoDataThresholdImageFilter.html\">"
		"IsoData Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MaximumEntropyThresholdImageFilter.html\">"
		"Maximum Entropy Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MomentsThresholdImageFilter.html\">"
		"Moments Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_YenThresholdImageFilter.html\">"
		"Yen Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RenyiEntropyThresholdImageFilter.html\">"
		"Renyi Entropy Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ShanbhagThresholdImageFilter.html\">"
		"Shanbhag Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1IntermodesThresholdImageFilter.html\">"
		"Intermodes Threshold Filter</a> (the 'Minimum Threshold' also uses this filter, with 'Use Intermodes' set to false), the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1HuangThresholdImageFilter.html\">"
		"Huang Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1LiThresholdImageFilter.html\">"
		"Li Threshold Filter</a>, the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1KittlerIllingworthThresholdImageFilter.html\">"
		"Kittler Illingworth Threshold Filter</a>, and the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1TriangleThresholdImageFilter.html\">"
		"Triangle Threshold Filter</a> "
		" in the ITK documentation."
	)
{
	addParameter("Method", iAValueType::Categorical, GetParameterlessThresholdingNames());
	addParameter("Number of histogram bins", iAValueType::Discrete, 128, 2);
	addParameter("Inside value", iAValueType::Continuous, 0);
	addParameter("Outside value", iAValueType::Continuous, 1);
	addOutputValue("Threshold");
}

template <typename T>
void parameterless(iAFilter* filter, QVariantMap const & params)
{
	using InputImageType = itk::Image<T, iAITKIO::Dim>;
	using MaskImageType = itk::Image<T, iAITKIO::Dim>;
	typename itk::HistogramThresholdImageFilter<InputImageType, MaskImageType>::Pointer plFilter;
	switch (MapMethodNameToID(params["Method"].toString()))
	{
	case P_OTSU_THRESHOLD:
		plFilter = itk::OtsuThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_ISODATA_THRESHOLD:
		plFilter = itk::IsoDataThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_MAXENTROPY_THRESHOLD:
		plFilter = itk::MaximumEntropyThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_MOMENTS_THRESHOLD:
		plFilter = itk::MomentsThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_YEN_THRESHOLD:
		plFilter = itk::YenThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_RENYI_THRESHOLD:
		plFilter = itk::RenyiEntropyThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_SHANBHAG_THRESHOLD:
		plFilter = itk::ShanbhagThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_INTERMODES_THRESHOLD:
		plFilter = itk::IntermodesThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_HUANG_THRESHOLD:
		plFilter = itk::HuangThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_LI_THRESHOLD:
		plFilter = itk::LiThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_KITTLERILLINGWORTH_THRESHOLD:
		plFilter = itk::KittlerIllingworthThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_TRIANGLE_THRESHOLD:
		plFilter = itk::TriangleThresholdImageFilter <InputImageType, MaskImageType>::New();
		break;
	case P_MINIMUM_THRESHOLD:
	{
		auto minimumFilter = itk::IntermodesThresholdImageFilter <InputImageType, MaskImageType>::New();
		minimumFilter->SetUseInterMode(false);
		plFilter = minimumFilter;
		break;
	}
	}
	plFilter->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	plFilter->SetNumberOfHistogramBins(params["Number of histogram bins"].toUInt());
	plFilter->SetOutsideValue(params["Outside value"].toDouble());
	plFilter->SetInsideValue(params["Inside value"].toDouble());
	filter->progress()->observe(plFilter);
	plFilter->Update();
	filter->addOutput(std::make_shared<iAImageData>(plFilter->GetOutput()));
	filter->addOutputValue("Threshold", static_cast<double>(plFilter->GetThreshold()));
}

void iAParameterlessThresholding::performWork(QVariantMap const & params)
{
	ITK_TYPED_CALL(parameterless, inputScalarType(), this, params);
}
