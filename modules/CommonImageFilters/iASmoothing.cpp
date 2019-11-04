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
#include "iASmoothing.h"

#include <iAConnector.h>
#include <defines.h> // for DIM
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>

#include <itkBilateralImageFilter.h>
#include <itkCurvatureAnisotropicDiffusionImageFilter.h>
#include <itkCurvatureFlowImageFilter.h>
#include <itkDiscreteGaussianImageFilter.h>
#include <itkGradientAnisotropicDiffusionImageFilter.h>
#include <itkMedianImageFilter.h>
#include <itkPatchBasedDenoisingImageFilter.h>
#include <itkRecursiveGaussianImageFilter.h>


typedef float RealType;
typedef itk::Image<RealType, DIM> RealImageType;

template<class T> void medianFilter(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::MedianImageFilter<RealImageType, RealImageType > FilterType;
	auto realImage = castImageTo<RealType>(filter->input()[0]->itkImage());
	auto medianFilter = FilterType::New();
	typename FilterType::InputSizeType indexRadius;
	indexRadius[0] = params["Kernel radius X"].toDouble();
	indexRadius[1] = params["Kernel radius Y"].toDouble();
	indexRadius[2] = params["Kernel radius Z"].toDouble();
	medianFilter->SetRadius(indexRadius);
	medianFilter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->progress()->observe( medianFilter );
	medianFilter->Update();
	if (params["Convert back to input type"].toBool())
		filter->addOutput(castImageTo<T>(medianFilter->GetOutput()));
	else
		filter->addOutput(medianFilter->GetOutput());

}

void iAMedianFilter::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(medianFilter, inputPixelType(), this, parameters);
}

IAFILTER_CREATE(iAMedianFilter)

iAMedianFilter::iAMedianFilter() :
	iAFilter("Median Filter", "Smoothing/Blurring",
		"Applies a median filter to the volume.<br/>"
		"Computes an image where an output voxel is assigned the median value of the voxels "
		"in a neighborhood around the input voxel at that position. The median filter belongs "
		"to the family of nonlinear filters. It is used to smooth an image without being "
		"biased by outliers or shot noise.<br/>"
		"The <em>Kernel radius</em> parameters define the radius of the kernel in x, y and z direction.<br/>"
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MedianImageFilter.html\">"
		"Median Image Filter</a> in the ITK documentation.")
{
	addParameter("Kernel radius X", Discrete, 1, 1);
	addParameter("Kernel radius Y", Discrete, 1, 1);
	addParameter("Kernel radius Z", Discrete, 1, 1);
	addParameter("Convert back to input type", Boolean, false);
}



template<class T>
void recursiveGaussian(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef typename itk::Image<T, DIM> InputImageType;
	typedef itk::RecursiveGaussianImageFilter<InputImageType, RealImageType > RGSFXType;
	typename RGSFXType::Pointer rgsfilterX = RGSFXType::New();
	typedef itk::RecursiveGaussianImageFilter<RealImageType, RealImageType > RGSFYZType;
	typename RGSFYZType::Pointer rgsfilterY = RGSFYZType::New();
	typename RGSFYZType::Pointer rgsfilterZ = RGSFYZType::New();
	rgsfilterX->SetInput(dynamic_cast<InputImageType*>(filter->input()[0]->itkImage()));
	rgsfilterY->SetInput(rgsfilterX->GetOutput());
	rgsfilterZ->SetInput(rgsfilterY->GetOutput());
	rgsfilterX->SetDirection(0); // 0 --> X direction
	rgsfilterY->SetDirection(1); // 1 --> Y direction
	rgsfilterZ->SetDirection(2); // 2 --> Z direction
	rgsfilterX->SetOrder(RGSFXType::ZeroOrder);
	rgsfilterY->SetOrder(RGSFYZType::ZeroOrder);
	rgsfilterZ->SetOrder(RGSFYZType::ZeroOrder);
	rgsfilterX->SetNormalizeAcrossScale(false);
	rgsfilterY->SetNormalizeAcrossScale(false);
	rgsfilterZ->SetNormalizeAcrossScale(false);
	rgsfilterX->SetSigma(params["Sigma"].toDouble());
	rgsfilterY->SetSigma(params["Sigma"].toDouble());
	rgsfilterZ->SetSigma(params["Sigma"].toDouble());
	filter->progress()->observe(rgsfilterZ);
	rgsfilterZ->Update();
	if (params["Convert back to input type"].toBool())
		filter->addOutput(castImageTo<T>(rgsfilterZ->GetOutput()));
	else
		filter->addOutput(rgsfilterZ->GetOutput());
}

IAFILTER_CREATE(iARecursiveGaussian)

void iARecursiveGaussian::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(recursiveGaussian, inputPixelType(), this, parameters);
}

iARecursiveGaussian::iARecursiveGaussian() :
	iAFilter("Recursive Gaussian", "Smoothing/Blurring",
		"Performs a smoothing using a gaussian kernel with the given <em>Sigma</em>.<br/>"
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1RecursiveGaussianImageFilter.html\">"
		"Recursive Gaussian Filter</a> in the ITK documentation.")
{
	addParameter("Sigma", Continuous, 0.1, std::numeric_limits<double>::epsilon());
	addParameter("Convert back to input type", Boolean, false);
}



template<class T> 
void discreteGaussian(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::DiscreteGaussianImageFilter<RealImageType, RealImageType > DGIFType;
	auto realImage = castImageTo<RealType>(dynamic_cast<InputImageType *>(filter->input()[0]->itkImage()));
	typename DGIFType::Pointer dgFilter = DGIFType::New();
	dgFilter->SetVariance(params["Variance"].toDouble());
	dgFilter->SetMaximumError(params["Maximum error"].toDouble());
	dgFilter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->progress()->observe(dgFilter);
	dgFilter->Update();
	if (params["Convert back to input type"].toBool())
		filter->addOutput(castImageTo<T>(dgFilter->GetOutput()));
	else
		filter->addOutput(dgFilter->GetOutput());
}

IAFILTER_CREATE(iADiscreteGaussian)

void iADiscreteGaussian::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(discreteGaussian, inputPixelType(), this, parameters);
}

iADiscreteGaussian::iADiscreteGaussian() :
	iAFilter("Discrete Gaussian", "Smoothing/Blurring",
		"Performs a discrete gaussian blurring using the given <em>Variance</em> and <em>Maximum Error</em>.<br/>"
		"Note that the variance needs to be given in image coordinates (i.e. considering the spacing)."
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1DiscreteGaussianImageFilter.html\">"
		"Discrete Gaussian Filter</a> in the ITK documentation.")
{
	addParameter("Variance", Continuous, 0);
	addParameter("Maximum error", Continuous, 0.01, 0 + std::numeric_limits<double>::epsilon(), 1 - std::numeric_limits<double>::epsilon());
	addParameter("Convert back to input type", Boolean, false);
}

template<class T>
void patchBasedDenoising(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::PatchBasedDenoisingImageFilter<ImageType, ImageType> NonLocalMeansFilter;
	auto nlmFilter(NonLocalMeansFilter::New());
	nlmFilter->SetInput(dynamic_cast<ImageType*>(filter->input()[0]->itkImage()));
	nlmFilter->SetNumberOfIterations(params["Number of iterations"].toDouble());
	nlmFilter->SetKernelBandwidthEstimation(params["Kernel bandwidth estimation"].toBool());
	nlmFilter->SetPatchRadius(params["Patch radius"].toDouble());
	filter->progress()->observe(nlmFilter);
	nlmFilter->Update();
	filter->addOutput(nlmFilter->GetOutput());
}

IAFILTER_CREATE(iANonLocalMeans)

void iANonLocalMeans::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(patchBasedDenoising, inputPixelType(), this, parameters);
}

iANonLocalMeans::iANonLocalMeans() :
	iAFilter("Non-Local Means", "Smoothing/",
		"Performs a non-local means (= patch-based denoising) filtering.<br/>"
		"Implements a denoising filter that uses iterative non-local, "
		"or semi-local, weighted averaging of image patches for image denoising.<br/>"
		"<em>Patch radius</em> specified in physical coordinates (that is, in a unit of pixels,"
		"rather than considering spacing, preferrable it's an even number). "
		"<em>Number of iterations</em> determines how many iterations to perform (default=1). "
		"<em>Kernel bandwidth estimation</em> determines whether kernel-bandwidth should be "
		"estimated automatically from the image data (default=false). <br/>"
		//<em>Noise Sigma</em> specifies the sigma of the noise model, where appropriate (in percent of the image intensity range)."
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1PatchBasedDenoisingImageFilter.html\">"
		"Patch Based Denoising Filter</a> in the ITK documentation.")
{
	// parameters in base class:
	// Patch Weights
	addParameter("Patch radius", Discrete, 2, 0);
	// Noise Model
	// Smoothing Weight
	// Noise Model Fidelity Weight
	addParameter("Kernel bandwidth estimation", Boolean, false);
	// Kernel Bandwidth Update Frequency
	addParameter("Number of iterations", Discrete, 1, 1);
	// Always Treat Components as Euclidean

	// in actual filter class:
	// addParameter("Noise Sigma", Continuous, 5, 0, 100);
	// Smooth Disc Patch Weigts, Boolean
	// Kernel Bandwidth Sigma, Continuous
	// Kernel Bandwitdh Fraction Pixels for Estimation, Continuous
	// Compute Conditional Derivatives, Boolean
	// Use Fast Tensor Computation, Boolean
	// Kernel Bandwith Multiplication Factor
	// Sampler
}


template<class T>
void gradientAnisotropicDiffusion(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::GradientAnisotropicDiffusionImageFilter<RealImageType, RealImageType> GADIFType;
	auto realImage = castImageTo<RealType>(dynamic_cast<InputImageType *>(filter->input()[0]->itkImage()));
	auto gadFilter = GADIFType::New();
	gadFilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	gadFilter->SetTimeStep(params["Time step"].toDouble());
	gadFilter->SetConductanceParameter(params["Conductance"].toDouble());
	gadFilter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->progress()->observe(gadFilter);
	gadFilter->Update();
	if (params["Convert back to input type"].toBool())
		filter->addOutput(castImageTo<T>(gadFilter->GetOutput()));
	else
		filter->addOutput(gadFilter->GetOutput());
}

IAFILTER_CREATE(iAGradientAnisotropicDiffusion)

void iAGradientAnisotropicDiffusion::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(gradientAnisotropicDiffusion, inputPixelType(), this, parameters);
}

iAGradientAnisotropicDiffusion::iAGradientAnisotropicDiffusion() :
	iAFilter("Gradient Anisotropic Diffusion", "Smoothing/Edge preserving smoothing",
		"Performs a gradient anisotropic diffusion.<br/>"
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GradientAnisotropicDiffusionImageFilter.html\">"
		"Gradient Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	addParameter("Number of iterations", Discrete, 100, 1);
	addParameter("Time step", Continuous, 0.0625);
	addParameter("Conductance", Continuous, 1);
	addParameter("Convert back to input type", Boolean, false);
}


template<class T>
void curvatureAnisotropicDiffusion(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::CurvatureAnisotropicDiffusionImageFilter<RealImageType, RealImageType > CADIFType;
	auto realImage = castImageTo<RealType>(dynamic_cast<InputImageType *>(filter->input()[0]->itkImage()));
	auto cadFilter = CADIFType::New();
	cadFilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	cadFilter->SetTimeStep(params["Time step"].toDouble());
	cadFilter->SetConductanceParameter(params["Conductance"].toDouble());
	cadFilter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->progress()->observe(cadFilter);
	cadFilter->Update();
	if (params["Convert back to input type"].toBool())
		filter->addOutput(castImageTo<T>(cadFilter->GetOutput()));
	else
		filter->addOutput(cadFilter->GetOutput());
}

IAFILTER_CREATE(iACurvatureAnisotropicDiffusion)

void iACurvatureAnisotropicDiffusion::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(curvatureAnisotropicDiffusion, inputPixelType(), this, parameters);
}

iACurvatureAnisotropicDiffusion::iACurvatureAnisotropicDiffusion() :
	iAFilter("Curvature Anisotropic Diffusion", "Smoothing/Edge preserving smoothing",
		"Performs an anisotropic diffusion using a modified curvature diffusion equation (MCDE).<br/>"
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1CurvatureAnisotropicDiffusionImageFilter.html\">"
		"Curvature Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	addParameter("Number of iterations", Discrete, 100, 1);
	addParameter("Time step", Continuous, 0.0625);
	addParameter("Conductance", Continuous, 1);
	addParameter("Convert back to input type", Boolean, false);
}



template<class T>
void curvatureFlow(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef typename itk::Image<T, DIM>   InputImageType;
	typedef itk::CurvatureFlowImageFilter<InputImageType, RealImageType> CFFType;
	auto cfFfilter = CFFType::New();
	cfFfilter->SetInput(dynamic_cast<InputImageType*>(filter->input()[0]->itkImage()));
	cfFfilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	cfFfilter->SetTimeStep(params["Time step"].toDouble());
	filter->progress()->observe(cfFfilter);
	cfFfilter->Update();
	if (params["Convert back to input type"].toBool())
		filter->addOutput(castImageTo<T>(cfFfilter->GetOutput()));
	else
		filter->addOutput(cfFfilter->GetOutput());
}

IAFILTER_CREATE(iACurvatureFlow)

void iACurvatureFlow::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(curvatureFlow, inputPixelType(), this, parameters);
}

iACurvatureFlow::iACurvatureFlow() :
	iAFilter("Curvature Flow", "Smoothing/Edge preserving smoothing",
		"Denoise an image using curvature driven flow.<br/>"
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1CurvatureFlowImageFilter.html\">"
		"Curvature Flow Filter</a> in the ITK documentation.")
{
	addParameter("Number of iterations", Discrete, 100, 1);
	addParameter("Time step", Continuous, 0.0625);
	addParameter("Convert back to input type", Boolean, false);
}


template<class T>
void bilateralFilter(iAFilter* filter, QMap<QString, QVariant> const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::BilateralImageFilter< InputImageType, RealImageType > BIFType;
	auto biFilter = BIFType::New();
	biFilter->SetRangeSigma(params["Range sigma"].toDouble());
	biFilter->SetDomainSigma(params["Domain sigma"].toDouble());
	biFilter->SetInput(dynamic_cast< InputImageType * >(filter->input()[0]->itkImage()));
	filter->progress()->observe(biFilter);
	biFilter->Update();
	if (params["Convert back to input type"].toBool())
		filter->addOutput(castImageTo<T>(biFilter->GetOutput()));
	else
		filter->addOutput(biFilter->GetOutput());
}

IAFILTER_CREATE(iABilateral)

void iABilateral::performWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(bilateralFilter, inputPixelType(), this, parameters);
}

iABilateral::iABilateral() :
	iAFilter("Bilateral Image Filter", "Smoothing/Edge preserving smoothing",
		"Bilateral filtering blurs an image using both domain and range neighborhoods.<br/>"
		"Pixels that are close to a pixel in the image domain and similar to a pixel in the image range "
		"are used to calculate the filtered value. Two gaussian kernels (one in the image domain "
		"and one in the image range) are used to smooth the image.<br/>"
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1BilateralImageFilter.html\">"
		"Bilateral Image Filter</a> in the ITK documentation.")
{
	addParameter("Range sigma", Continuous, 50);
	addParameter("Domain sigma", Continuous, 4);
	addParameter("Convert back to input type", Boolean, false);
}
