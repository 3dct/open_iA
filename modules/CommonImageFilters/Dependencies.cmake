set(DEPENDENCIES_LIBRARIES
	iA::base
)
if (OPENCL_FOUND)
	list(APPEND DEPENDENCIES_LIBRARIES OpenCL)    # not ideal - only required for TARGET_VERSION defines
endif()
#set(DEPENDENCIES_ITK_MODULES
#	ITKAnisotropicSmoothing      # for CurvatureAnisotropicDiffusionImageFilter, GradientAnisotropicDiffusionImageFilter
#	ITKConnectedComponents       # for ...ConnectedComponentImageFilter, RelabelComponentImageFilter
#	ITKConvolution               # for ConvolutionImageFilter
#	ITKCurvatureFlow             # for CurvatureFlowImageFilter
#	ITKDenoising                 # for PatchBasedDenoisingImageFilter
#	ITKDistanceMap               # for (Danielsson|SignedMaurer)DistanceMapImageFilter
#	ITKFFT                       # for (Real)HalfHermitianToRealInverseFFTImageFilter, dependency of FFTConvolutionImageFilter
#	ITKFiniteDifference          # for DenseFiniteDifferenceImageFilter (dependency of AnisotropicDiffusionImageFilter), FiniteDifferenceImageFilter (dependency of SparseFieldLevelSetImageFilter)
#	ITKImageAdaptors             # for NthElementImageAdaptor (dependency of HessianRecursiveGaussianImageFilter, PatchBasedDenoisingImageFilter)
#	ITKImageFeature              # for HessianRecursiveGaussianImageFilter
#	ITKImageFusion               # for LabelToRGBImageFilter
#	ITKImageGradient             # for GradientMagnitudeImageFilter
#	ITKImageLabel                # for ScanlineFilterCommon, dependency of ConnectedComponentImageFilter
#	ITKImageNoise                # for AdditiveGaussianNoiseImageFilter, ...
#	ITKImageSources              # for GaussianImageSource, dependency of BilateralImageFilter
#	ITKMathematicalMorphology    # for BinaryBallStructuringElement
#	ITKSmoothing
#	ITKStatistics                # for Histogram, dependency of HistogramMatchingImageFilter
#	ITKThresholding              # for BinaryThresholdImageFilter, dependency of SignedMaurerDistanceMapImageFilter
#	ITKTestKernel                # for PipelineMonitorImageFilter
#	ITKTransform                 # for Transform, dependency of ResampleImageFilter
#)
#if (ITK_VERSION VERSION_LESS "5.0.0" AND OPENCL_FOUND)
#	list(APPEND DEPENDENCIES_ITK_MODULES
#		ITKGPUCommon               # for itkGPUImage.h, required by iASmoothing
#		ITKGPUAnisotropicSmoothing # for itkGPUGradientAnisotropicDiffusionImageFilter
#		ITKGPUFiniteDifference     # for itkGPUDenseFiniteDifferenceImageFilter, required by itkGPUAnisotropicDiffusionImageFilter
#	)
#endif()
#if (HigherOrderAccurateGradient_LOADED)
#	list(APPEND DEPENDENCIES_ITK_MODULES HigherOrderAccurateGradient)
#endif()
set(DEPENDENCIES_IA_TOOLKIT_DIRS
	RemovePeaksOtsu    # for itkFHWRescaleIntensityImageFilter
)
