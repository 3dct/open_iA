// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h>          // for DIM
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAMathUtility.h>
#include <iAProgress.h>
#include <iAToolsITK.h>    // for setIndexOffsetToZero
#include <iAToolsVTK.h>    // for adjustIndexAndSizeToImage
#include <iATypedCallHelper.h>
#include <iAValueTypeVectorHelpers.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkBSplineInterpolateImageFunction.h>
#include <itkConstantPadImageFilter.h>
#include <itkNearestNeighborInterpolateImageFunction.h>
#include <itkResampleImageFilter.h>
#include <itkExtractImageFilter.h>
#include <itkWindowedSincInterpolateImageFunction.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkImageData.h>
#include <vtkImageExtractComponents.h>

#include <QtConcurrent/qtconcurrentfilter.h>

IAFILTER_DEFAULT_CLASS(iACopy)

class iAExtractComponent : public iAFilter, private iAAutoRegistration<iAFilter, iAExtractComponent, iAFilterRegistry>
{
public:
	iAExtractComponent();
	void adaptParametersToInput(QVariantMap& params, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets) override;
private:
	void performWork(QVariantMap const& parameters) override;
};

class iASimpleResampleFilter : public iAFilter, private iAAutoRegistration<iAFilter, iASimpleResampleFilter, iAFilterRegistry>
{
public:
	iASimpleResampleFilter();
	void adaptParametersToInput(QVariantMap& params, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets) override;
private:
	void performWork(QVariantMap const& parameters) override;
};

class iAResampleFilter : public iAFilter, private iAAutoRegistration<iAFilter, iAResampleFilter, iAFilterRegistry>
{
public:
	iAResampleFilter();
	void adaptParametersToInput(QVariantMap& params, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets) override;
private:
	void performWork(QVariantMap const& parameters) override;
};

class iAExtractImageFilter : public iAFilter, private iAAutoRegistration<iAFilter, iAExtractImageFilter, iAFilterRegistry>
{
public:
	iAExtractImageFilter();
	void adaptParametersToInput(QVariantMap& params, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets) override;
private:
	void performWork(QVariantMap const& parameters) override;
};

IAFILTER_DEFAULT_CLASS(iAPadImageFilter);

void iACopy::performWork(QVariantMap const& /*parameters*/)
{
	vtkNew<vtkImageData> copiedImg;
	copiedImg->DeepCopy(imageInput(0)->vtkImage());
	addOutput(std::make_shared<iAImageData>(copiedImg));
}

iACopy::iACopy() :
	iAFilter("Copy", "",
		"Copy the input image to output.<br/>"
		"That is, this filter simply directly returns a copy of the input, without any modifications.")
{
}


void iAExtractComponent::performWork(QVariantMap const& parameters)
{
	int const componentNr = parameters["Component to extract"].toInt();
	auto img = imageInput(0)->vtkImage();
	if (componentNr > img->GetNumberOfScalarComponents())
	{
		LOG(lvlWarn,
			QString("Invalid value for 'Component to extract': %1 exceeds valid range 1..%2")
				.arg(componentNr)
				.arg(img->GetNumberOfScalarComponents()));
		return;
	}
	auto extractFilter = vtkSmartPointer<vtkImageExtractComponents>::New();
	extractFilter->SetInputData(img);
	extractFilter->SetComponents(componentNr);
	progress()->observe(extractFilter);
	extractFilter->Update();
	addOutput(std::make_shared<iAImageData>(extractFilter->GetOutput()));
}

iAExtractComponent::iAExtractComponent() :
	iAFilter("Extract Component", "Geometric Transformations",
		"Extract single component from multi-component image.<br/>"
		"For more information, see the "
		"<a href=\"https://vtk.org/doc/nightly/html/classvtkImageExtractComponents.html\">"
		"Extract Components</a> filter in the VTK documentation.")
{
	addParameter("Component to extract", iAValueType::Discrete, 1, 1, 1);
}

void iAExtractComponent::adaptParametersToInput(QVariantMap& /* params */, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets)
{
	auto img = dynamic_cast<iAImageData*>(dataSets.begin()->second.get());
	paramsWritable()[0]->adjustMinMax(img->vtkImage()->GetNumberOfScalarComponents());
}


namespace
{
	const QString InterpLinear("Linear");
	const QString InterpNearestNeighbour("Nearest Neighbour");
	const QString InterpBSpline("BSpline");
	const QString InterpWindowedSinc("Windowed Sinc");

	QStringList interpolators()
	{
		QStringList result;
		result << InterpLinear << InterpNearestNeighbour << InterpBSpline << InterpWindowedSinc;
		return result;
	}

	template <typename InputImageType>
	void setInterpolator(typename itk::ResampleImageFilter<InputImageType, InputImageType>::Pointer resampler,
		QString const & interpolatorName)
	{
		if (interpolatorName == InterpLinear)
		{
			resampler->SetInterpolator(itk::LinearInterpolateImageFunction<InputImageType, double>::New());
		}
		else if (interpolatorName == InterpNearestNeighbour)
		{
			resampler->SetInterpolator(itk::NearestNeighborInterpolateImageFunction<InputImageType, double>::New());
		}
		else if (interpolatorName == InterpBSpline)
		{
			resampler->SetInterpolator(itk::BSplineInterpolateImageFunction<InputImageType, double>::New());
		}
		else if (interpolatorName == InterpWindowedSinc)
		{
			typedef itk::Function::HammingWindowFunction<3> WindowFunctionType;
			typedef itk::ZeroFluxNeumannBoundaryCondition<InputImageType> ConditionType;
			typedef itk::WindowedSincInterpolateImageFunction<
				InputImageType, 3,
				WindowFunctionType,
				ConditionType,
				double> InterpolatorType;
			auto interpolator = InterpolatorType::New();
			resampler->SetInterpolator(interpolator);
		}
	}
}

template<typename T> void simpleResampler(iAFilter* filter, QVariantMap const & parameters)
{
	auto inImg = filter->imageInput(0)->itkImage();
	auto inSize = filter->imageInput(0)->vtkImage()->GetDimensions();

	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::ResampleImageFilter<InputImageType, InputImageType> ResampleFilterType;
	auto resampler = ResampleFilterType::New();
	typename ResampleFilterType::SizeType outSize;
	setFromVectorVariant<int>(outSize, parameters["Size"]);
	auto inSpc = inImg->GetSpacing();
	QString interpolatorName = parameters["Interpolator"].toString();
	setInterpolator<InputImageType>(resampler, interpolatorName);
	resampler->SetInput(dynamic_cast<InputImageType*>(inImg));
	auto inOri = inImg->GetOrigin();
	typename ResampleFilterType::SpacingType outSpc;
	itk::Point<double, 3> outOri;
	bool adjust = parameters["Adjust origin"].toBool();
	for (int i = 0; i < 3; ++i)
	{
		outSpc[i] = inSpc[i] * (static_cast<double>(inSize[i]) / outSize[i]);
		outOri[i] = inOri[i] + (adjust ? (outSpc[i] / 2 - inSpc[i] / 2) : 0);
	}
	resampler->SetOutputOrigin(outOri);
	resampler->SetOutputSpacing(outSpc);
	resampler->SetSize(outSize);
	resampler->SetDefaultPixelValue(0);
	filter->progress()->observe( resampler );
	resampler->Update( );
	filter->addOutput(std::make_shared<iAImageData>(resampler->GetOutput()) );
}


void iASimpleResampleFilter::performWork(QVariantMap const& parameters)
{
	ITK_TYPED_CALL(simpleResampler, inputScalarType(), this, parameters);
}

iASimpleResampleFilter::iASimpleResampleFilter() :
	iAFilter("Simple Resample", "Geometric Transformations",
		"Resample the image to a new size.<br/>"
		"For more information, see the "
		"<a href=\"https ://itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html\">"
		"Resample Filter</a> in the ITK documentation.")
{
	addParameter("Size", iAValueType::Vector3i, variantVector<int>({1, 1, 1}));
	addParameter("Interpolator", iAValueType::Categorical, interpolators());
	addParameter("Adjust origin", iAValueType::Boolean, true);
}

void iASimpleResampleFilter::adaptParametersToInput(QVariantMap& params, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets)
{
	auto dim = dynamic_cast<iAImageData*>(dataSets.begin()->second.get())->vtkImage()->GetDimensions();
	params["Size"] = variantVector<int>({dim[0], dim[1], dim[2]});
}



template <typename T>
void resampler(iAFilter* filter, QVariantMap const& parameters)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::ResampleImageFilter<InputImageType, InputImageType> ResampleFilterType;
	auto resampler = ResampleFilterType::New();

	typename ResampleFilterType::OriginPointType origin;
	setFromVectorVariant<double>(origin, parameters["Origin"]);
	typename ResampleFilterType::SpacingType spacing;
	setFromVectorVariant<double>(spacing, parameters["Spacing"]);
	typename ResampleFilterType::SizeType size;
	setFromVectorVariant<int>(size, parameters["Size"]);
	QString interpolatorName = parameters["Interpolator"].toString();
	setInterpolator<InputImageType>(resampler, interpolatorName);
	resampler->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	resampler->SetOutputOrigin(origin);
	resampler->SetOutputSpacing(spacing);
	resampler->SetSize(size);
	resampler->SetDefaultPixelValue(0);
	filter->progress()->observe(resampler);
	resampler->Update();
	filter->addOutput(std::make_shared<iAImageData>(resampler->GetOutput()));
}

void iAResampleFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(resampler, inputScalarType(), this, parameters);
}

iAResampleFilter::iAResampleFilter() :
	iAFilter("Resample", "Geometric Transformations",
		"Resample the image to a new size.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ResampleImageFilter.html\">"
		"Resample Filter</a> in the ITK documentation.")
{
	addParameter("Origin", iAValueType::Vector3i, variantVector<int>({0, 0, 0}));
	addParameter("Spacing", iAValueType::Vector3, variantVector<double>({1.0, 1.0, 1.0}));
	addParameter("Size", iAValueType::Vector3i, variantVector<int>({1, 1, 1}));
	addParameter("Interpolator", iAValueType::Categorical, interpolators());
}

void iAResampleFilter::adaptParametersToInput(QVariantMap& params, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets)
{
	auto img = dynamic_cast<iAImageData*>(dataSets.begin()->second.get())->vtkImage();
	auto spc = img->GetSpacing();
	params["Spacing"] = variantVector<double>({spc[0], spc[1], spc[2]});
	auto dim     = img->GetDimensions();
	params["Size"]    = variantVector<int>({dim[0], dim[1], dim[2]});
}


template<typename T>
void extractImage(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::Image< T, DIM > OutputImageType;
	typedef itk::ExtractImageFilter< InputImageType, OutputImageType > EIFType;

	typename EIFType::InputImageRegionType::SizeType size;
	setFromVectorVariant<int>(size, parameters["Size"]);

	typename EIFType::InputImageRegionType::IndexType index;
	setFromVectorVariant<int>(index, parameters["Index"]);

	typename EIFType::InputImageRegionType region; region.SetIndex(index); region.SetSize(size);

	auto extractFilter = EIFType::New();
	extractFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	extractFilter->SetExtractionRegion(region);
	filter->progress()->observe(extractFilter);
	extractFilter->Update();

	filter->addOutput(std::make_shared<iAImageData>(setIndexOffsetToZero<T>(extractFilter->GetOutput())));
}

void iAExtractImageFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(extractImage, inputScalarType(), this, parameters);
}

iAExtractImageFilter::iAExtractImageFilter() :
	iAFilter("Extract Image", "Geometric Transformations",
		"Extract a part of the image.<br/>"
		"Both <em>Index</em> and <em>Size</em> values are in pixel units.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ExtractImageFilter.html\">"
		"Extract Image Filter</a> in the ITK documentation.")
{
	addParameter("Index", iAValueType::Vector3i, variantVector<int>({0, 0, 0}));
	addParameter("Size", iAValueType::Vector3i, variantVector<int>({1, 1, 1}));
}

void iAExtractImageFilter::adaptParametersToInput(QVariantMap& params, std::map<size_t, std::shared_ptr<iADataSet>> const& dataSets)
{
	auto img = dynamic_cast<iAImageData*>(dataSets.begin()->second.get());
	adjustIndexAndSizeToImage(params, img->vtkImage());
}





template<typename T> void padImage(iAFilter* filter, QVariantMap const & parameters)
{
	typedef itk::Image< T, DIM > InputImageType;
	typedef itk::ConstantPadImageFilter<InputImageType, InputImageType> PadType;

	typename PadType::InputImageRegionType::SizeType lowerPadSize;
	lowerPadSize[0] = parameters["Lower X padding"].toUInt();
	lowerPadSize[1] = parameters["Lower Y padding"].toUInt();
	lowerPadSize[2] = parameters["Lower Z padding"].toUInt();
	typename PadType::InputImageRegionType::SizeType upperPadSize;
	upperPadSize[0] = parameters["Upper X padding"].toUInt();
	upperPadSize[1] = parameters["Upper Y padding"].toUInt();
	upperPadSize[2] = parameters["Upper Z padding"].toUInt();

	auto padFilter = PadType::New();
	padFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	padFilter->SetPadLowerBound(lowerPadSize);
	padFilter->SetPadUpperBound(upperPadSize);
	padFilter->SetConstant(parameters["Value"].toDouble());
	filter->progress()->observe(padFilter);
	padFilter->Update();

	filter->addOutput(std::make_shared<iAImageData>(setIndexOffsetToZero<T>(padFilter->GetOutput())));
}

void iAPadImageFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(padImage, inputScalarType(), this, parameters);
}

iAPadImageFilter::iAPadImageFilter() :
	iAFilter("Pad Image", "Geometric Transformations",
		"Pad image on one or more side with given number of zero pixels.<br/>"
		"<em>Lower (x, y, z) padding</em> specifies the amount of pixels to be appended before the current first x/y/z pixel. "
		"<em>Upper (x, y, z) padding</em> specifies the amount of pixels to be appended after the current last x/y/z pixel. "
		"Pixels are added with the specified <em>Value</em><br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConstantPadImageFilter.html\">"
		"Extract Image Filter</a> in the ITK documentation.")
{
	addParameter("Lower X padding", iAValueType::Discrete, 1, 0);
	addParameter("Lower Y padding", iAValueType::Discrete, 1, 0);
	addParameter("Lower Z padding", iAValueType::Discrete, 1, 0);
	addParameter("Upper X padding", iAValueType::Discrete, 1, 0);
	addParameter("Upper Y padding", iAValueType::Discrete, 1, 0);
	addParameter("Upper Z padding", iAValueType::Discrete, 1, 0);
	addParameter("Value", iAValueType::Continuous, 0.0);
}
