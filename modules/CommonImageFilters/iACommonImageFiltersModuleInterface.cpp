/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iACommonImageFiltersModuleInterface.h"

#include "iACastImageFilter.h"
#include "iAConnectedComponentFilters.h"
#include "iAConvolutionFilter.h"
#include "iADistanceMap.h"
#include "iAEdgeDetectionFilters.h"
#include "iAGeometricTransformations.h"
#include "iAGradients.h"
#include "iAHessianEigenanalysis.h"
#include "iAIntensity.h"
#include "iAMorphologyFilters.h"
#include "iANoiseFilters.h"
#include "iASmoothing.h"
#include "iATransformations.h"

#include <iAFilterRegistry.h>

void iACommonImageFiltersModuleInterface::Initialize()
{
	// Edge detection:
	REGISTER_FILTER(iACannyEdgeDetection);

	// Casting / Datatype conversion:
	REGISTER_FILTER(iACastImageFilter);
	REGISTER_FILTER(iAConvertToRGBAFilter);

	// Connected component / relabeling:
	REGISTER_FILTER(iAConnectedComponents);
	REGISTER_FILTER(iAScalarConnectedComponents);
	REGISTER_FILTER(iARelabelComponents);

	// Distance Maps
	REGISTER_FILTER(iASignedMaurerDistanceMap);
	REGISTER_FILTER(iADanielssonDistanceMap);

	// (FFT) Convolution / Normalized cross-correlation
	REGISTER_FILTER(iAConvolution);
	REGISTER_FILTER(iAFFTConvolution);
	REGISTER_FILTER(iACorrelation);
	REGISTER_FILTER(iAFFTCorrelation);
	REGISTER_FILTER(iAStreamedFFTCorrelation);

	// Geometric transformations
	REGISTER_FILTER_WITH_RUNNER(iAResampleFilter, iAResampleFilterRunner);
	REGISTER_FILTER_WITH_RUNNER(iAExtractImageFilter, iAExtractImageFilterRunner);
	REGISTER_FILTER(iAPadImageFilter);

	// Gradient filters:
	REGISTER_FILTER(iADerivative);
	REGISTER_FILTER(iAGradientMagnitude);
	REGISTER_FILTER(iAGradientMagnitudeRecursiveGaussian);
	REGISTER_FILTER(iAHigherOrderAccurateDerivative);

	// Hessian eigen-analysis / Laplacian
	REGISTER_FILTER(iAHessianEigenanalysis);
	REGISTER_FILTER(iALaplacian);

	// Intensity transformations
	// Filters requiring 1 input image:
	REGISTER_FILTER(iAAdaptiveHistogramEqualization);
	REGISTER_FILTER(iAGeneralThreshold);
	REGISTER_FILTER(iAIntensityWindowingFilter);
	REGISTER_FILTER(iAInvertIntensityFilter);
	REGISTER_FILTER(iAMaskIntensityFilter);
	REGISTER_FILTER(iANormalizeIntensityFilter);
	REGISTER_FILTER(iARescaleIntensityFilter);
	REGISTER_FILTER(iAShiftScaleIntensityFilter);
	// Filters requiring 2 input images:
	REGISTER_FILTER(iAAddFilter);
	REGISTER_FILTER(iADifferenceFilter);
	REGISTER_FILTER(iAHistogramMatchingFilter);
	REGISTER_FILTER(iASubtractFilter);

	// Morphological filters
	REGISTER_FILTER(iADilation);
	REGISTER_FILTER(iAErosion);
	REGISTER_FILTER(iAVesselEnhancement);
	REGISTER_FILTER(iAMorphOpening);
	REGISTER_FILTER(iAMorphClosing);
	REGISTER_FILTER(iAFillHole);
	REGISTER_FILTER(iAOpeningByReconstruction);
	REGISTER_FILTER(iAClosingByReconstruction);

	// Filters adding noise
	REGISTER_FILTER(iAAdditiveGaussianNoise);
	REGISTER_FILTER(iASaltAndPepperNoise);
	REGISTER_FILTER(iAShotNoise);
	REGISTER_FILTER(iASpeckleNoise);

	// Smoothing / Noise reduction
	REGISTER_FILTER(iADiscreteGaussian);
	REGISTER_FILTER(iARecursiveGaussian);
	REGISTER_FILTER(iAMedianFilter);
	REGISTER_FILTER(iANonLocalMeans);
	REGISTER_FILTER(iAGradientAnisotropicDiffusion);
	REGISTER_FILTER(iACurvatureAnisotropicDiffusion);
	REGISTER_FILTER(iACurvatureFlow);
	REGISTER_FILTER(iABilateral);
#ifndef ITKNOGPU
	REGISTER_FILTER(iAGPUEdgePreservingSmoothing);
#endif

	// Transformations
	REGISTER_FILTER(iARotate);
	REGISTER_FILTER(iAPermuteAxes);
	REGISTER_FILTER(iAFlipAxis);
	REGISTER_FILTER(iATranslate);
}
