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
#include "iAConvolutionFilter.h"

#include "defines.h"    // for DIM
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"

#include <itkCastImageFilter.h>
#include <itkConvolutionImageFilter.h>
#include <itkFFTConvolutionImageFilter.h>
#include <itkFFTNormalizedCorrelationImageFilter.h>
#include <itkNormalizedCorrelationImageFilter.h>
#include <itkPipelineMonitorImageFilter.h>
#include <itkStreamingImageFilter.h>

#include <vtkContourFilter.h>
#include <vtkCutter.h>
#include <vtkKochanekSpline.h>
#include <vtkPlane.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSplineFilter.h>
#include <vtkStripper.h>
#include <vtkTubeFilter.h>


template<class T> void convolution_template(QVector<iAConnector*> & image, iAProgress* p)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::Image<float, DIM> KernelImageType;
	typedef itk::ConvolutionImageFilter<ImageType, KernelImageType, KernelImageType> ConvFilterType;

	auto filter = ConvFilterType::New();
	auto img = dynamic_cast<ImageType *>(image[0]->GetITKImage());
	auto kernelImg = dynamic_cast<KernelImageType*>(image[1]->GetITKImage());
	if (!kernelImg)
	{
		throw std::invalid_argument("Kernel Image must be of float type!");
	}
	filter->SetInput(img);
	filter->SetKernelImage(kernelImg);
	p->Observe(filter);
	filter->Update();
	image[0]->SetImage(filter->GetOutput());
	image[0]->Modified();
}

void iAConvolution::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(convolution_template, m_con->GetITKScalarPixelType(), m_cons, m_progress);
}

IAFILTER_CREATE(iAConvolution)

iAConvolution::iAConvolution() :
	iAFilter("Convolution", "Convolution",
		"Convolve a given image with an arbitrary (float) image kernel.<br/>"
		"This filter operates by centering the flipped kernel image "
		"(the additional input) at each pixel "
		"in the image and computing the inner product between pixel values "
		"in the image and pixel values in the kernel.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1ConvolutionImageFilter.html\">"
		"Convolution Filter</a> in the ITK documentation.", 2)
{}

// FFT-based convolution instead of spatial domain convolution
template<class T> void fft_convolution_template(QVector<iAConnector*> & image, iAProgress* p)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::Image<float, DIM> KernelImageType;
	typedef itk::FFTConvolutionImageFilter<ImageType, KernelImageType, KernelImageType> ConvFilterType;

	auto kernelImg = dynamic_cast<KernelImageType *>(image[1]->GetITKImage());
	if (!kernelImg)
	{
		throw std::invalid_argument("Kernel Image must be of float type!");
	}
	auto filter = ConvFilterType::New();
	filter->SetInput(dynamic_cast<ImageType *>(image[0]->GetITKImage()));
	filter->SetKernelImage(kernelImg);
	filter->SetNormalize(true);
	p->Observe(filter);
	filter->Update();
	image[0]->SetImage(filter->GetOutput());
	image[0]->Modified();
}

void iAFFTConvolution::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(fft_convolution_template, m_con->GetITKScalarPixelType(), m_cons, m_progress);
}

IAFILTER_CREATE(iAFFTConvolution)

iAFFTConvolution::iAFFTConvolution() :
	iAFilter("FFT Convolution", "Convolution",
		"Convolve a given image with a kernel using multiplication in the Fourier domain.<br/>"
		"The first input (=active mdi child) is convolved with the selected "
		"kernel image (the additional input)."
		"This filter produces output equivalent to the output of the Convolution Filter. "
		"However, it takes advantage of the convolution theorem to accelerate the convolution "
		"computation when the kernel is large.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1FFTConvolutionImageFilter.html\">"
		"FFT Convolution Filter</a> in the ITK documentation.", 2)
{}

template<class T> void correlation_template(QVector<iAConnector*> & image, iAProgress* p)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::Image<float, DIM> KernelImageType;

	auto kernelImg = dynamic_cast<KernelImageType*>(image[1]->GetITKImage());
	if (!kernelImg)
	{
		throw std::invalid_argument("Kernel Image must be of float type!");
	}
	// The radius of the kernel must be the radius of the patch, NOT the size of the patch
	itk::Size<3> radius = kernelImg->GetLargestPossibleRegion().GetSize();
	radius[0] = (radius[0] - 1) / 2;
	radius[1] = (radius[1] - 1) / 2;
	radius[2] = (radius[2] - 1) / 2;

	itk::ImageKernelOperator<float, 3> kernelOperator;
	kernelOperator.SetImageKernel(kernelImg);
	kernelOperator.CreateToRadius(radius);

	auto img = dynamic_cast<ImageType *>(image[0]->GetITKImage());
	typedef itk::NormalizedCorrelationImageFilter<ImageType, KernelImageType, KernelImageType> CorrelationFilterType;
	auto filter = CorrelationFilterType::New();
	filter->SetInput(img);
	filter->SetTemplate(kernelOperator);
	p->Observe(filter);
	filter->Update();
	image[0]->SetImage(filter->GetOutput());
	image[0]->Modified();
}

void iACorrelation::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(correlation_template, m_con->GetITKScalarPixelType(), m_cons, m_progress);
}

IAFILTER_CREATE(iACorrelation)

iACorrelation::iACorrelation() :
	iAFilter("Correlation", "Convolution",
		"Computes the normalized correlation of an image and a template.<br/>"
		"This filter calculates the normalized correlation between an image "
		"(the first input / the active mdi child) and the template (the "
		"additional input. Normalized correlation is frequently use in feature "
		"detection because it is invariant to local changes in contrast.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1NormalizedCorrelationImageFilter.html\">"
		"Normalized Correlation Filter</a> in the ITK documentation.", 2)
{}

//NCC calculation using fft
template<class T> void fft_correlation_template(QVector<iAConnector*> & image, iAProgress* p)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::Image<float, DIM> KernelImageType;
	auto img = dynamic_cast<ImageType *>(image[0]->GetITKImage());
	auto templateImg = dynamic_cast<KernelImageType*>(image[1]->GetITKImage());
	if (!templateImg)
	{
		throw std::invalid_argument("Template Image must be of float type!");
	}

	//cast input image to float
	typedef itk::CastImageFilter<ImageType, KernelImageType> CasterType;
	auto caster = CasterType::New();
	caster->SetInput(img);

	typedef itk::FFTNormalizedCorrelationImageFilter<KernelImageType, KernelImageType> CorrelationFilterType;
	auto filter = CorrelationFilterType::New();
	filter->SetInput(caster->GetOutput());
	//filter->SetFixedImage(img);
	filter->SetMovingImage(templateImg);
	filter->Modified();
	p->Observe(filter);
	filter->Update();
	image[0]->SetImage(filter->GetOutput());
	image[0]->Modified();
}

void iAFFTCorrelation::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(fft_correlation_template, m_con->GetITKScalarPixelType(), m_cons, m_progress);
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
		"FFT Normalized Correlation Filter</a> in the ITK documentation.", 2)
{}

template<class T> void streamed_fft_correlation_template(QVector<iAConnector*> & image, iAProgress* p)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::Image<float, DIM> KernelImageType;
	typedef itk::CastImageFilter<ImageType, KernelImageType> CasterType;
	typedef itk::PipelineMonitorImageFilter<KernelImageType> MonitorFilterType;
	typedef itk::StreamingImageFilter<KernelImageType, KernelImageType> StreamingFilterType;
	typedef itk::FFTNormalizedCorrelationImageFilter<KernelImageType, KernelImageType> CorrelationFilterType;
	//typedef itk::NormalizedCorrelationImageFilter<KernelImageType, KernelImageType, KernelImageType> CorrelationFilterType;

	auto templateImg = dynamic_cast<KernelImageType*>(image[1]->GetITKImage());
	if (!templateImg)
	{
		throw std::invalid_argument("Template Image must be of float type!");
	}
	//cast input image to float
	auto caster = CasterType::New();
	caster->SetInput(dynamic_cast<ImageType *>(image[0]->GetITKImage()));
	auto monitorFilter = MonitorFilterType::New();
	monitorFilter->SetInput(caster->GetOutput());
	//monitorFilter->DebugOn();

	int numStreamDivisions = 100;

	auto streamer = StreamingFilterType::New();
	streamer->SetInput(monitorFilter->GetOutput());
	streamer->SetNumberOfStreamDivisions(numStreamDivisions);

	auto filter = CorrelationFilterType::New();
	filter->SetInput(streamer->GetOutput());
	//filter->SetFixedImage(img);
	filter->SetMovingImage(templateImg);
	p->Observe(filter);
	filter->Update();
	image[0]->SetImage(filter->GetOutput());
	image[0]->Modified();
}

void iAStreamedFFTCorrelation::Run(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(streamed_fft_correlation_template, m_con->GetITKScalarPixelType(), m_cons, m_progress);
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
{}
