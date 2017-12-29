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
#include "iASmoothing.h"

#include "iAConnector.h"
#include "defines.h" // for DIM
#include "iAProgress.h"
#include "iAToolsITK.h"
#include "iATypedCallHelper.h"

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

template<class T> void medianFilter(QMap<QString, QVariant> const & params, iAProgress* p, iAConnector* image  )
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::MedianImageFilter<RealImageType, RealImageType > FilterType;
	auto realImage = CastImageTo<RealType>(image->GetITKImage());
	auto filter = FilterType::New();
	FilterType::InputSizeType indexRadius;
	indexRadius[0] = params["Kernel radius X"].toDouble();
	indexRadius[1] = params["Kernel radius Y"].toDouble();
	indexRadius[2] = params["Kernel radius Z"].toDouble();
	filter->SetRadius(indexRadius);
	filter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	p->Observe( filter );
	filter->Update();
	if (params["Convert back to input type"].toBool())
		image->SetImage(CastImageTo<T>(filter->GetOutput()));
	else
		image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

void iAMedianFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(medianFilter, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Kernel radius X", Discrete, 1, 1);
	AddParameter("Kernel radius Y", Discrete, 1, 1);
	AddParameter("Kernel radius Z", Discrete, 1, 1);
	AddParameter("Convert back to input type", Boolean, false);
}



template<class T>
void recursiveGaussian(QMap<QString, QVariant> const & params, iAProgress* p, iAConnector* image)
{
	typedef typename itk::Image<T, DIM> InputImageType;
	typedef itk::RecursiveGaussianImageFilter<InputImageType, RealImageType > RGSFXType;
	typename RGSFXType::Pointer rgsfilterX = RGSFXType::New();
	typedef itk::RecursiveGaussianImageFilter<RealImageType, RealImageType > RGSFYZType;
	typename RGSFYZType::Pointer rgsfilterY = RGSFYZType::New();
	typename RGSFYZType::Pointer rgsfilterZ = RGSFYZType::New();
	rgsfilterX->SetInput(dynamic_cast<InputImageType*>(image->GetITKImage()));
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
	rgsfilterZ->Update();
	p->Observe(rgsfilterZ);
	if (params["Convert back to input type"].toBool())
		image->SetImage(CastImageTo<T>(rgsfilterZ->GetOutput()));
	else
		image->SetImage(rgsfilterZ->GetOutput());
	image->Modified();
	rgsfilterX->ReleaseDataFlagOn();
	rgsfilterY->ReleaseDataFlagOn();
	rgsfilterZ->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iARecursiveGaussian)

void iARecursiveGaussian::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(recursiveGaussian, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Sigma", Continuous, 0.1, std::numeric_limits<double>::epsilon());
	AddParameter("Convert back to input type", Boolean, false);
}



template<class T> 
void discreteGaussian(QMap<QString, QVariant> const & params, iAProgress* p, iAConnector* image)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::DiscreteGaussianImageFilter<RealImageType, RealImageType > DGIFType;
	auto realImage = CastImageTo<RealType>(dynamic_cast<InputImageType *>(image->GetITKImage()));
	DGIFType::Pointer filter = DGIFType::New();
	filter->SetVariance(params["Variance"].toDouble());
	filter->SetMaximumError(params["Maximum error"].toDouble());
	filter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->Update();
	p->Observe(filter);
	if (params["Convert back to input type"].toBool())
		image->SetImage(CastImageTo<T>(filter->GetOutput()));
	else
		image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iADiscreteGaussian)

void iADiscreteGaussian::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(discreteGaussian, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Variance", Continuous, 0);
	AddParameter("Maximum error", Continuous, 0.01, 0 + std::numeric_limits<double>::epsilon(), 1 - std::numeric_limits<double>::epsilon());
	AddParameter("Convert back to input type", Boolean, false);
}

template<class T>
void patchBasedDenoising(QMap<QString, QVariant> const & params, iAProgress* p, iAConnector* con)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::PatchBasedDenoisingImageFilter<ImageType, ImageType> NonLocalMeansFilter;
	auto filter(NonLocalMeansFilter::New());
	filter->SetInput(dynamic_cast<ImageType*>(con->GetITKImage()));
	filter->SetNumberOfIterations(params["Number of iterations"].toDouble());
	filter->SetKernelBandwidthEstimation(params["Kernel bandwidth estimation"].toBool());
	filter->SetPatchRadius(params["Patch radius"].toDouble());
	p->Observe(filter);
	filter->Update();
	con->SetImage(filter->GetOutput());
	con->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iANonLocalMeans)

void iANonLocalMeans::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(patchBasedDenoising, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Patch radius", Discrete, 2, 0);
	// Noise Model
	// Smoothing Weight
	// Noise Model Fidelity Weight
	AddParameter("Kernel bandwidth estimation", Boolean, false);
	// Kernel Bandwidth Update Frequency
	AddParameter("Number of iterations", Discrete, 1, 1);
	// Always Treat Components as Euclidean

	// in actual filter class:
	// AddParameter("Noise Sigma", Continuous, 5, 0, 100);
	// Smooth Disc Patch Weigts, Boolean
	// Kernel Bandwidth Sigma, Continuous
	// Kernel Bandwitdh Fraction Pixels for Estimation, Continuous
	// Compute Conditional Derivatives, Boolean
	// Use Fast Tensor Computation, Boolean
	// Kernel Bandwith Multiplication Factor
	// Sampler
}


template<class T>
void gradientAnisotropicDiffusion(QMap<QString, QVariant> const & params, iAProgress* p, iAConnector* image)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::GradientAnisotropicDiffusionImageFilter<RealImageType, RealImageType> GADIFType;
	auto realImage = CastImageTo<RealType>(dynamic_cast<InputImageType *>(image->GetITKImage()));
	auto filter = GADIFType::New();
	filter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	filter->SetTimeStep(params["Time step"].toDouble());
	filter->SetConductanceParameter(params["Conductance"].toDouble());
	filter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	p->Observe(filter);
	filter->Update();
	if (params["Convert back to input type"].toBool())
		image->SetImage(CastImageTo<T>(filter->GetOutput()));
	else
		image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iAGradientAnisotropicDiffusion)

void iAGradientAnisotropicDiffusion::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(gradientAnisotropicDiffusion, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Number of iterations", Discrete, 100, 1);
	AddParameter("Time step", Continuous, 0.0625);
	AddParameter("Conductance", Continuous, 1);
	AddParameter("Convert back to input type", Boolean, false);
}


template<class T>
void curvatureAnisotropicDiffusion(QMap<QString, QVariant> const & params, iAProgress* p, iAConnector* image)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::CurvatureAnisotropicDiffusionImageFilter<RealImageType, RealImageType > CADIFType;
	auto realImage = CastImageTo<RealType>(dynamic_cast<InputImageType *>(image->GetITKImage()));
	auto filter = CADIFType::New();
	filter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	filter->SetTimeStep(params["Time step"].toDouble());
	filter->SetConductanceParameter(params["Conductance"].toDouble());
	filter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	p->Observe(filter);
	filter->Update();
	if (params["Convert back to input type"].toBool())
		image->SetImage(CastImageTo<T>(filter->GetOutput()));
	else
		image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iACurvatureAnisotropicDiffusion)

void iACurvatureAnisotropicDiffusion::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(curvatureAnisotropicDiffusion, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Number of iterations", Discrete, 100, 1);
	AddParameter("Time step", Continuous, 0.0625);
	AddParameter("Conductance", Continuous, 1);
	AddParameter("Convert back to input type", Boolean, false);
}



template<class T>
void curvatureFlow(QMap<QString, QVariant> const & params, iAProgress* p, iAConnector* image)
{
	typedef typename itk::Image<T, DIM>   InputImageType;
	typedef itk::CurvatureFlowImageFilter<InputImageType, RealImageType> CFFType;
	auto filter = CFFType::New();
	filter->SetInput(dynamic_cast<InputImageType*>(image->GetITKImage()));
	filter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	filter->SetTimeStep(params["Time step"].toDouble());
	p->Observe(filter);
	filter->Update();
	if (params["Convert back to input type"].toBool())
		image->SetImage(CastImageTo<T>(filter->GetOutput()));
	else
		image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iACurvatureFlow)

void iACurvatureFlow::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(curvatureFlow, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Number of iterations", Discrete, 100, 1);
	AddParameter("Time step", Continuous, 0.0625);
	AddParameter("Convert back to input type", Boolean, false);
}


template<class T>
void bilateralFilter(QMap<QString, QVariant> const & params, iAProgress* p, iAConnector* image)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::BilateralImageFilter< InputImageType, RealImageType > BIFType;
	auto filter = BIFType::New();
	filter->SetRangeSigma(params["Range sigma"].toDouble());
	filter->SetDomainSigma(params["Domain sigma"].toDouble());
	filter->SetInput(dynamic_cast< InputImageType * >(image->GetITKImage()));
	p->Observe(filter);
	filter->Update();
	if (params["Convert back to input type"].toBool())
		image->SetImage(CastImageTo<T>(filter->GetOutput()));
	else
		image->SetImage(filter->GetOutput());
	image->Modified();
	filter->ReleaseDataFlagOn();
}

IAFILTER_CREATE(iABilateral)

void iABilateral::PerformWork(QMap<QString, QVariant> const & parameters)
{
	ITK_TYPED_CALL(bilateralFilter, m_con->GetITKScalarPixelType(), parameters, m_progress, m_con);
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
	AddParameter("Range sigma", Continuous, 50);
	AddParameter("Domain sigma", Continuous, 4);
	AddParameter("Convert back to input type", Boolean, false);
}
