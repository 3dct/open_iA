// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <defines.h> // for DIM
#include <iAFilterDefault.h>
#include <iAImageData.h>
#include <iAItkVersion.h>
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
#ifndef ITKNOGPU
// now defined via CMake option:
//#define CL_TARGET_OPENCL_VERSION 110
#include <itkGPUImage.h>
#include <itkGPUGradientAnisotropicDiffusionImageFilter.h>
#if ITK_VERSION_NUMBER >= ITK_VERSION_CHECK(5, 1, 0)
// split into a separate file starting with ITK 5.1 (previously included in itkGPUGradientAnisotropicDiffusionImageFilter.h)
#include <itkGPUGradientAnisotropicDiffusionImageFilterFactory.h>
#endif
#endif

#include <iAItkVersion.h>

#if (!defined(ITKNOGPU) && (ITK_VERSION_NUMBER >= ITK_VERSION_CHECK(5,1,0) && ITK_VERSION_NUMBER < ITK_VERSION_CHECK(5,2,0)))
#ifndef _MSC_VER
#warning("With ITK 5.1.x, GPU-accelerated filters don't work in open_iA, see https://github.com/InsightSoftwareConsortium/ITK/issues/1381. Disabling GPU support")
#else
#pragma message("With ITK 5.1.x, GPU-accelerated filters don't work in open_iA, see https://github.com/InsightSoftwareConsortium/ITK/issues/1381. Disabling GPU support")
#endif
#define ITKNOGPU
#endif

namespace itk
{
	class ProcessObject;
}


// Blurring
IAFILTER_DEFAULT_CLASS(iADiscreteGaussian);
IAFILTER_DEFAULT_CLASS(iARecursiveGaussian);
IAFILTER_DEFAULT_CLASS(iAMedianFilter);

class iANonLocalMeans : public iAFilter, private iAAutoRegistration<iAFilter, iANonLocalMeans, iAFilterRegistry>
{
public:
	iANonLocalMeans();
	void abort() override;
private:
	void performWork(QVariantMap const& parameters) override;
	itk::ProcessObject* m_itkProcess;
};

// Edge-Preserving
IAFILTER_DEFAULT_CLASS(iAGradientAnisotropicDiffusion);
IAFILTER_DEFAULT_CLASS(iACurvatureAnisotropicDiffusion);
IAFILTER_DEFAULT_CLASS(iACurvatureFlow);
IAFILTER_DEFAULT_CLASS(iABilateral);
#ifndef ITKNOGPU
IAFILTER_DEFAULT_CLASS(iAGPUEdgePreservingSmoothing)
#endif


template<class T> void medianFilter(iAFilter* filter, QVariantMap const & params)
{
	typedef typename itk::Image<T, DIM> InputImageType;
	typedef itk::MedianImageFilter<InputImageType, InputImageType> FilterType;
	auto medianFilter = FilterType::New();
	typename FilterType::InputSizeType indexRadius;
	indexRadius[0] = params["Kernel radius X"].toDouble();
	indexRadius[1] = params["Kernel radius Y"].toDouble();
	indexRadius[2] = params["Kernel radius Z"].toDouble();
	medianFilter->SetRadius(indexRadius);
	medianFilter->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	filter->progress()->observe( medianFilter );
	medianFilter->Update();
	filter->addOutput(medianFilter->GetOutput());
}

void iAMedianFilter::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(medianFilter, inputScalarType(), this, parameters);
}

iAMedianFilter::iAMedianFilter() :
	iAFilter("Median Filter", "Smoothing/Blurring",
		"Applies a median filter to the volume.<br/>"
		"Computes an image where an output voxel is assigned the median value of the voxels "
		"in a neighborhood around the input voxel at that position. The median filter belongs "
		"to the family of nonlinear filters. It is used to smooth an image without being "
		"biased by outliers or shot noise.<br/>"
		"The <em>Kernel radius</em> parameters define the radius of the kernel in x, y and z direction. "
		"This radius defines the size of the neighborhood in pixels/voxels in each direction; that is, a "
		"radius of 1 leads to a kernel size of 3 (1 in both directions plus the center pixel), a radius of 2 to "
		"a kernel size of 5, and so on.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1MedianImageFilter.html\">"
		"Median Image Filter</a> in the ITK documentation.")
{
	addParameter("Kernel radius X", iAValueType::Discrete, 1, 1);
	addParameter("Kernel radius Y", iAValueType::Discrete, 1, 1);
	addParameter("Kernel radius Z", iAValueType::Discrete, 1, 1);
}


typedef float RealType;
typedef itk::Image<RealType, DIM> RealImageType;


template<class T>
void recursiveGaussian(iAFilter* filter, QVariantMap const & params)
{
	typedef typename itk::Image<T, DIM> InputImageType;
	typedef itk::RecursiveGaussianImageFilter<InputImageType, RealImageType > RGSFXType;
	typename RGSFXType::Pointer rgsfilterX = RGSFXType::New();
	typedef itk::RecursiveGaussianImageFilter<RealImageType, RealImageType > RGSFYZType;
	typename RGSFYZType::Pointer rgsfilterY = RGSFYZType::New();
	typename RGSFYZType::Pointer rgsfilterZ = RGSFYZType::New();
	rgsfilterX->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
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

void iARecursiveGaussian::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(recursiveGaussian, inputScalarType(), this, parameters);
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
	addParameter("Sigma", iAValueType::Continuous, 0.1, std::numeric_limits<double>::epsilon());
	addParameter("Convert back to input type", iAValueType::Boolean, false);
}



template<class T>
void discreteGaussian(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::DiscreteGaussianImageFilter<RealImageType, RealImageType > DGIFType;
	auto realImage = castImageTo<RealType>(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	typename DGIFType::Pointer dgFilter = DGIFType::New();
	dgFilter->SetVariance(params["Variance"].toDouble());
	dgFilter->SetMaximumError(params["Maximum error"].toDouble());
	dgFilter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->progress()->observe(dgFilter);
	dgFilter->Update();
	if (params["Convert back to input type"].toBool())
	{
		filter->addOutput(castImageTo<T>(dgFilter->GetOutput()));
	}
	else
	{
		filter->addOutput(dgFilter->GetOutput());
	}
}

void iADiscreteGaussian::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(discreteGaussian, inputScalarType(), this, parameters);
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
	addParameter("Variance", iAValueType::Continuous, 0);
	addParameter("Maximum error", iAValueType::Continuous, 0.01, 0 + std::numeric_limits<double>::epsilon(), 1 - std::numeric_limits<double>::epsilon());
	addParameter("Convert back to input type", iAValueType::Boolean, false);
}


template<class T>
void patchBasedDenoising(iAFilter* filter, QVariantMap const & params, itk::ProcessObject* & itkProcess)
{
	typedef itk::Image<T, DIM> ImageType;
	typedef itk::PatchBasedDenoisingImageFilter<ImageType, ImageType> NonLocalMeansFilter;
	auto nlmFilter(NonLocalMeansFilter::New());
	itkProcess = nlmFilter.GetPointer();
	nlmFilter->SetInput(dynamic_cast<ImageType*>(filter->imageInput(0)->itkImage()));
	nlmFilter->SetNumberOfIterations(params["Number of iterations"].toDouble());
	nlmFilter->SetKernelBandwidthEstimation(params["Kernel bandwidth estimation"].toBool());
	nlmFilter->SetPatchRadius(params["Patch radius"].toDouble());
	filter->progress()->observe(nlmFilter);
	nlmFilter->Update();
	filter->addOutput(nlmFilter->GetOutput());
}

void iANonLocalMeans::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(patchBasedDenoising, inputScalarType(), this, parameters, m_itkProcess);
}

void iANonLocalMeans::abort()
{
	m_itkProcess->SetAbortGenerateData(true);
}

iANonLocalMeans::iANonLocalMeans() :
	iAFilter("Non-Local Means", "Smoothing",
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
		"Patch Based Denoising Filter</a> in the ITK documentation.", 1u, 1u, true),
	m_itkProcess(nullptr)
{
	// parameters in base class:
	// Patch Weights
	addParameter("Patch radius", iAValueType::Discrete, 2, 0);
	// Noise Model
	// Smoothing Weight
	// Noise Model Fidelity Weight
	addParameter("Kernel bandwidth estimation", iAValueType::Boolean, false);
	// Kernel Bandwidth Update Frequency
	addParameter("Number of iterations", iAValueType::Discrete, 1, 1);
	// Always Treat Components as Euclidean

	// in actual filter class:
	// addParameter("Noise Sigma", iAValueType::Continuous, 5, 0, 100);
	// Smooth Disc Patch Weigts, iAValueType::Boolean
	// Kernel Bandwidth Sigma, iAValueType::Continuous
	// Kernel Bandwitdh Fraction Pixels for Estimation, iAValueType::Continuous
	// Compute Conditional Derivatives, iAValueType::Boolean
	// Use Fast Tensor Computation, iAValueType::Boolean
	// Kernel Bandwith Multiplication Factor
	// Sampler
}


template<class T>
void gradientAnisotropicDiffusion(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::GradientAnisotropicDiffusionImageFilter<RealImageType, RealImageType> GADIFType;
	auto realImage = castImageTo<RealType>(dynamic_cast<InputImageType *>(filter->imageInput(0)->itkImage()));
	auto gadFilter = GADIFType::New();
	gadFilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	gadFilter->SetTimeStep(params["Time step"].toDouble());
	gadFilter->SetConductanceParameter(params["Conductance"].toDouble());
	gadFilter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->progress()->observe(gadFilter);
	gadFilter->Update();
	if (params["Convert back to input type"].toBool())
	{
		filter->addOutput(castImageTo<T>(gadFilter->GetOutput()));
	}
	else
	{
		filter->addOutput(gadFilter->GetOutput());
	}
}

void iAGradientAnisotropicDiffusion::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(gradientAnisotropicDiffusion, inputScalarType(), this, parameters);
}

iAGradientAnisotropicDiffusion::iAGradientAnisotropicDiffusion() :
	iAFilter("Gradient Anisotropic Diffusion", "Smoothing/Edge preserving smoothing",
		"Performs a gradient anisotropic diffusion.<br/>"
		"<em>Number of iterations</em> determines how many steps of diffusion are performed; "
		"the appropriate number depends on the application and the image; as a general rule, the "
		"more iterations are performed, the more diffused the image will be. <br/>"
		"<em>Time step</em> sets the time step to be used for each iteration (update). The time "
		"step is constrained at run-time to keep the solution stable. In general, the time step "
		"should be at or below PixelSpacing / 2^(N+1), where N is the dimensionality of the image.<br/>"
		"<em>Conductance</em> governs the sensitivity of the conductance equation."
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GradientAnisotropicDiffusionImageFilter.html\">"
		"Gradient Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	addParameter("Number of iterations", iAValueType::Discrete, 100, 1);
	addParameter("Time step", iAValueType::Continuous, 0.0625);
	addParameter("Conductance", iAValueType::Continuous, 1);
	addParameter("Convert back to input type", iAValueType::Boolean, false);
}


#ifndef ITKNOGPU

template<class T>
void GPU_gradient_anisotropic_diffusion(iAFilter* filter, QVariantMap const & params)
{
	// register object factory for GPU image and filter
	itk::ObjectFactoryBase::RegisterFactory(itk::GPUImageFactory::New());


	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::Image< float, DIM >   RealImageType;

	typedef itk::GPUGradientAnisotropicDiffusionImageFilter<RealImageType, RealImageType> GADIFType;
	auto realImage = castImageTo<RealType>(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	auto gadFilter = GADIFType::New();
	gadFilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	gadFilter->SetTimeStep(params["Time step"].toDouble());
	gadFilter->SetConductanceParameter(params["Conductance"].toDouble());
	gadFilter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->progress()->observe(gadFilter);
	gadFilter->Update();
	if (params["Convert back to input type"].toBool())
	{
		filter->addOutput(castImageTo<T>(gadFilter->GetOutput()));
	}
	else
	{
		filter->addOutput(gadFilter->GetOutput());
	}
}

void iAGPUEdgePreservingSmoothing::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(GPU_gradient_anisotropic_diffusion, inputScalarType(), this, parameters);
}

iAGPUEdgePreservingSmoothing::iAGPUEdgePreservingSmoothing() :
	iAFilter("Gradient Anisotropic Diffusion (GPU)", "Smoothing/Edge preserving smoothing",
		"Performs GPU-accelerated gradient anisotropic diffusion.<br/>"
		"<em>Number of iterations</em> determines how many steps of diffusion are performed; "
		"the appropriate number depends on the application and the image; as a general rule, the "
		"more iterations are performed, the more diffused the image will be. <br/>"
		"<em>Time step</em> sets the time step to be used for each iteration (update). The time "
		"step is constrained at run-time to keep the solution stable. In general, the time step "
		"should be at or below PixelSpacing / 2^(N+1), where N is the dimensionality of the image.<br/>"
		"<em>Conductance</em> governs the sensitivity of the conductance equation."
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1GPUGradientAnisotropicDiffusionImageFilter.html\">"
		"GPU Gradient Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	addParameter("Number of iterations", iAValueType::Discrete, 100, 1);
	addParameter("Time step", iAValueType::Continuous, 0.0625);
	addParameter("Conductance", iAValueType::Continuous, 1);
	addParameter("Convert back to input type", iAValueType::Boolean, false);
}

#endif


template<class T>
void curvatureAnisotropicDiffusion(iAFilter* filter, QVariantMap const& params)
{
	typedef itk::Image<T, DIM> InputImageType;
	typedef itk::CurvatureAnisotropicDiffusionImageFilter<RealImageType, RealImageType > CADIFType;
	auto realImage = castImageTo<RealType>(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	auto cadFilter = CADIFType::New();
	cadFilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	cadFilter->SetTimeStep(params["Time step"].toDouble());
	cadFilter->SetConductanceParameter(params["Conductance"].toDouble());
	cadFilter->SetInput(dynamic_cast<RealImageType*>(realImage.GetPointer()));
	filter->progress()->observe(cadFilter);
	cadFilter->Update();
	if (params["Convert back to input type"].toBool())
	{
		filter->addOutput(castImageTo<T>(cadFilter->GetOutput()));
	}
	else
	{
		filter->addOutput(cadFilter->GetOutput());
	}
}

void iACurvatureAnisotropicDiffusion::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(curvatureAnisotropicDiffusion, inputScalarType(), this, parameters);
}

iACurvatureAnisotropicDiffusion::iACurvatureAnisotropicDiffusion() :
	iAFilter("Curvature Anisotropic Diffusion", "Smoothing/Edge preserving smoothing",
		"Performs an anisotropic diffusion using a modified curvature diffusion equation (MCDE).<br/>"
		"<em>Number of iterations</em> determines how many steps of diffusion are performed; "
		"the appropriate number depends on the application and the image; as a general rule, the "
		"more iterations are performed, the more diffused the image will be. <br/>"
		"<em>Time step</em> sets the time step to be used for each iteration (update). The time "
		"step is constrained at run-time to keep the solution stable. In general, the time step "
		"should be at or below PixelSpacing / 2^(N+1), where N is the dimensionality of the image.<br/>"
		"<em>Conductance</em> governs the sensitivity of the conductance equation."
		"If <em>convert back to input type</em> is enabled, the resulting image "
		"will have the same type as the input image; if it is not enabled, the result "
		"will be a single precision floating point image.<br/>"
		"For more information, see the "
		"<a href=\"https://itk.org/Doxygen/html/classitk_1_1CurvatureAnisotropicDiffusionImageFilter.html\">"
		"Curvature Anisotropic Diffusion Filter</a> in the ITK documentation.")
{
	addParameter("Number of iterations", iAValueType::Discrete, 100, 1);
	addParameter("Time step", iAValueType::Continuous, 0.0625);
	addParameter("Conductance", iAValueType::Continuous, 1);
	addParameter("Convert back to input type", iAValueType::Boolean, false);
}


template<class T>
void curvatureFlow(iAFilter* filter, QVariantMap const & params)
{
	typedef typename itk::Image<T, DIM>   InputImageType;
	typedef itk::CurvatureFlowImageFilter<InputImageType, RealImageType> CFFType;
	auto cfFfilter = CFFType::New();
	cfFfilter->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
	cfFfilter->SetNumberOfIterations(params["Number of iterations"].toUInt());
	cfFfilter->SetTimeStep(params["Time step"].toDouble());
	filter->progress()->observe(cfFfilter);
	cfFfilter->Update();
	if (params["Convert back to input type"].toBool())
	{
		filter->addOutput(castImageTo<T>(cfFfilter->GetOutput()));
	}
	else
	{
		filter->addOutput(cfFfilter->GetOutput());
	}
}

void iACurvatureFlow::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(curvatureFlow, inputScalarType(), this, parameters);
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
	addParameter("Number of iterations", iAValueType::Discrete, 100, 1);
	addParameter("Time step", iAValueType::Continuous, 0.0625);
	addParameter("Convert back to input type", iAValueType::Boolean, false);
}


template<class T>
void bilateralFilter(iAFilter* filter, QVariantMap const & params)
{
	typedef itk::Image< T, DIM >   InputImageType;
	typedef itk::BilateralImageFilter< InputImageType, RealImageType > BIFType;
	auto biFilter = BIFType::New();
	biFilter->SetRangeSigma(params["Range sigma"].toDouble());
	biFilter->SetDomainSigma(params["Domain sigma"].toDouble());
	biFilter->SetInput(dynamic_cast< InputImageType * >(filter->imageInput(0)->itkImage()));
	filter->progress()->observe(biFilter);
	biFilter->Update();
	if (params["Convert back to input type"].toBool())
	{
		filter->addOutput(castImageTo<T>(biFilter->GetOutput()));
	}
	else
	{
		filter->addOutput(biFilter->GetOutput());
	}
}

void iABilateral::performWork(QVariantMap const & parameters)
{
	ITK_TYPED_CALL(bilateralFilter, inputScalarType(), this, parameters);
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
	addParameter("Range sigma", iAValueType::Continuous, 50);
	addParameter("Domain sigma", iAValueType::Continuous, 4);
	addParameter("Convert back to input type", iAValueType::Boolean, false);
}
