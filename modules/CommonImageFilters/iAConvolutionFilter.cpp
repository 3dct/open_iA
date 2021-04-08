/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAConvolutionFilter.h"

#include <defines.h>    // for DIM
#include <iAConnector.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#include <itkConvolutionImageFilter.h>
#include <itkFFTConvolutionImageFilter.h>
#include <itkFFTNormalizedCorrelationImageFilter.h>
#include <itkNormalizedCorrelationImageFilter.h>
#include <itkPipelineMonitorImageFilter.h>
#include <itkStreamingImageFilter.h>
#pragma GCC diagnostic pop

namespace
{
	using FloatImageType = itk::Image<float, DIM>;
}

template<class T> void convolution(iAFilter* filter)
{
	using ImageType = itk::Image<T, DIM>;
	using ConvFilterType = itk::ConvolutionImageFilter<ImageType, FloatImageType, FloatImageType>;

	if (itkScalarPixelType(filter->input()[1]->itkImage()) != itk::ImageIOBase::FLOAT)
	{
		LOG(lvlWarn, "Kernel Image should be of float type; it will be typecast!");
	}
	auto kernelImg = castImageTo<float>(filter->input()[1]->itkImage());
	auto convFilter = ConvFilterType::New();
	convFilter->SetInput(dynamic_cast<ImageType*>(filter->input()[0]->itkImage()));
	convFilter->SetKernelImage(dynamic_cast<FloatImageType*>(kernelImg.GetPointer()));
	filter->progress()->observe(convFilter);
	convFilter->Update();
	filter->addOutput(convFilter->GetOutput());
}

void iAConvolution::performWork(QMap<QString, QVariant> const & /*parameters*/)
{
	ITK_TYPED_CALL(convolution, inputPixelType(), this);
}

IAFILTER_CREATE(iAConvolution)

iAConvolution::iAConvolution() :
	iAFilter("Convolution", "Convolution",
		"Convolve a given image with an arbitrary (float) image kernel.<br/>"
		"This filter operates by centering the flipped kernel image "
		"at each pixel "
		"in the image and computing the inner product between pixel values "
		"in the image and pixel values in the kernel.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConvolutionImageFilter.html\">"
		"Convolution Filter</a> in the ITK documentation.", 2)
{
	setInputName(1u, "Kernel image");
}

// FFT-based convolution instead of spatial domain convolution
template<class T> void fft_convolution(iAFilter* filter)
{
	using ImageType = itk::Image<T, DIM>;
	using ConvFilterType = itk::FFTConvolutionImageFilter<ImageType, FloatImageType, FloatImageType>;
	if (itkScalarPixelType(filter->input()[1]->itkImage()) != itk::ImageIOBase::FLOAT)
	{
		LOG(lvlWarn, "Kernel Image should be of float type; it will be typecast!");
	}
	auto kernelImg = castImageTo<float>(filter->input()[1]->itkImage());
	auto fftConvFilter = ConvFilterType::New();
	fftConvFilter->SetInput(dynamic_cast<ImageType *>(filter->input()[0]->itkImage()));
	fftConvFilter->SetKernelImage(dynamic_cast<FloatImageType*>(kernelImg.GetPointer()));
	fftConvFilter->SetNormalize(true);
	filter->progress()->observe(fftConvFilter);
	fftConvFilter->Update();
	filter->addOutput(fftConvFilter->GetOutput());
}

void iAFFTConvolution::performWork(QMap<QString, QVariant> const & /*parameters*/)
{
	ITK_TYPED_CALL(fft_convolution, inputPixelType(), this);
}

IAFILTER_CREATE(iAFFTConvolution)

iAFFTConvolution::iAFFTConvolution() :
	iAFilter("FFT Convolution", "Convolution",
		"Convolve a given image with a kernel using multiplication in the Fourier domain.<br/>"
		"The first input (=active mdi child) is convolved with the selected kernel image."
		"This filter produces output equivalent to the output of the Convolution Filter. "
		"However, it takes advantage of the convolution theorem to accelerate the convolution "
		"computation when the kernel is large.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FFTConvolutionImageFilter.html\">"
		"FFT Convolution Filter</a> in the ITK documentation.", 2)
{
	setInputName(1u, "Kernel image");
}

template<class T> void correlation(iAFilter* filter)
{
	using ImageType = itk::Image<T, DIM>;
	using CorrelationFilterType = itk::NormalizedCorrelationImageFilter<ImageType, FloatImageType, FloatImageType>;

	if (itkScalarPixelType(filter->input()[1]->itkImage()) != itk::ImageIOBase::FLOAT)
	{
		LOG(lvlWarn, "Kernel Image should be of float type; it will be typecast!");
	}
	auto kernelImg = castImageTo<float>(filter->input()[1]->itkImage());
	// The radius of the kernel must be the radius of the patch, NOT the size of the patch
	itk::Size<3> radius = kernelImg->GetLargestPossibleRegion().GetSize();
	radius[0] = (radius[0] - 1) / 2;
	radius[1] = (radius[1] - 1) / 2;
	radius[2] = (radius[2] - 1) / 2;

	itk::ImageKernelOperator<float, 3> kernelOperator;
	kernelOperator.SetImageKernel(dynamic_cast<FloatImageType*>(kernelImg.GetPointer()));
	kernelOperator.CreateToRadius(radius);

	auto img = dynamic_cast<ImageType *>(filter->input()[0]->itkImage());
	auto corrFilter = CorrelationFilterType::New();
	corrFilter->SetInput(img);
	corrFilter->SetTemplate(kernelOperator);
	filter->progress()->observe(corrFilter);
	corrFilter->Update();
	filter->addOutput(corrFilter->GetOutput());
}

void iACorrelation::performWork(QMap<QString, QVariant> const & /*parameters*/)
{
	ITK_TYPED_CALL(correlation, inputPixelType(), this);
}

IAFILTER_CREATE(iACorrelation)

iACorrelation::iACorrelation() :
	iAFilter("Correlation", "Convolution",
		"Computes the normalized correlation of an image and a template.<br/>"
		"This filter calculates the normalized correlation between an image "
		"(the first input / the active mdi child) and the template. "
		"Normalized correlation is frequently use in feature "
		"detection because it is invariant to local changes in contrast.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1NormalizedCorrelationImageFilter.html\">"
		"Normalized Correlation Filter</a> in the ITK documentation.", 2)
{
	setInputName(1u, "Template image");
}

//NCC calculation using fft
template<class T> void fft_correlation(iAFilter* filter)
{
	using CorrelationFilterType = itk::FFTNormalizedCorrelationImageFilter<FloatImageType, FloatImageType>;

	auto inImg = castImageTo<float>(filter->input()[0]->itkImage());
	if (itkScalarPixelType(filter->input()[1]->itkImage()) != itk::ImageIOBase::FLOAT)
	{
		LOG(lvlWarn, "Template Image should be of float type; it will be typecast!");
	}
	auto templateImg = castImageTo<float>(filter->input()[1]->itkImage());

	auto corrFilter = CorrelationFilterType::New();
	corrFilter->SetInput(dynamic_cast<FloatImageType*>(inImg.GetPointer()));
	//filter->SetFixedImage(img);
	corrFilter->SetMovingImage(dynamic_cast<FloatImageType*>(templateImg.GetPointer()));
	corrFilter->Modified();
	filter->progress()->observe(corrFilter);
	corrFilter->Update();
	filter->addOutput(corrFilter->GetOutput());
}

void iAFFTCorrelation::performWork(QMap<QString, QVariant> const & /*parameters*/)
{
	ITK_TYPED_CALL(fft_correlation, inputPixelType(), this);
}

IAFILTER_CREATE(iAFFTCorrelation)

iAFFTCorrelation::iAFFTCorrelation() :
	iAFilter("FFT Correlation", "Convolution",
		"Calculate normalized cross correlation using FFTs.<br/>"
		"This filter calculates the normalized cross correlation (NCC) "
		"of two images using FFTs instead of spatial correlation. It is much "
		"faster than spatial correlation for reasonably large structuring "
		"elements.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FFTNormalizedCorrelationImageFilter.html\">"
		"FFT Normalized Correlation Filter</a> in the ITK documentation.")
{
	setInputName(1u, "Template image");
}

template<class T> void streamed_fft_correlation(iAFilter* filter)
{
	using MonitorFilterType = itk::PipelineMonitorImageFilter<FloatImageType>;
	using StreamingFilterType = itk::StreamingImageFilter<FloatImageType, FloatImageType>;
	using CorrelationFilterType = itk::FFTNormalizedCorrelationImageFilter<FloatImageType, FloatImageType>;
	//using CorrelationFilterType = itk::NormalizedCorrelationImageFilter<FloatImageType, FloatImageType, FloatImageType>;

	if (itkScalarPixelType(filter->input()[1]->itkImage()) != itk::ImageIOBase::FLOAT)
	{
		LOG(lvlWarn, "Template Image should be of float type; it will be typecast!");
	}
	auto templateImg = castImageTo<float>(filter->input()[1]->itkImage());

	auto inImg = castImageTo<float>(filter->input()[0]->itkImage());
	auto monitorFilter = MonitorFilterType::New();
	monitorFilter->SetInput(dynamic_cast<FloatImageType*>(inImg.GetPointer()));
	//monitorFilter->DebugOn();

	int numStreamDivisions = 100;

	auto streamer = StreamingFilterType::New();
	streamer->SetInput(monitorFilter->GetOutput());
	streamer->SetNumberOfStreamDivisions(numStreamDivisions);

	auto corrFilter = CorrelationFilterType::New();
	corrFilter->SetInput(streamer->GetOutput());
	//filter->SetFixedImage(img);
	corrFilter->SetMovingImage(dynamic_cast<FloatImageType*>(templateImg.GetPointer()));
	filter->progress()->observe(corrFilter);
	corrFilter->Update();
	filter->addOutput(corrFilter->GetOutput());
}

void iAStreamedFFTCorrelation::performWork(QMap<QString, QVariant> const & /*parameters*/)
{
	ITK_TYPED_CALL(streamed_fft_correlation, inputPixelType(), this);
}

IAFILTER_CREATE(iAStreamedFFTCorrelation)

iAStreamedFFTCorrelation::iAStreamedFFTCorrelation() :
	iAFilter("Streaming FFT Correlation", "Convolution",
		"Calculate normalized cross correlation using FFTs (streamed).<br/>"
		"This filter calculates the normalized cross correlation (NCC) "
		"of two images using FFTs instead of spatial correlation. It is much "
		"faster than spatial correlation for reasonably large structuring "
		"elements. It is using a streaming filter, meaning it is suitable for "
		"processing huge datasets.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FFTNormalizedCorrelationImageFilter.html\">"
		"FFT Normalized Correlation Filter</a> and"
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1StreamingImageFilter.html\">"
		"Streaming Filters</a> in the ITK documentation.", 2)
{
	setInputName(1u, "Template image");
}
