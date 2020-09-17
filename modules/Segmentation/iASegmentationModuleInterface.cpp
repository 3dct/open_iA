/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASegmentationModuleInterface.h"

#include "iAFuzzyCMeans.h"
#include "iALevelSet.h"
#include "iAPCA.h"
#include "iARandomWalker.h"
#include "iARegionGrowing.h"
#include "iASVMImageFilter.h"
#include "iAThresholding.h"
#include "iAWatershedSegmentation.h"

#include <iAFilterRegistry.h>

void iASegmentationModuleInterface::Initialize()
{
	REGISTER_FILTER(iACopy);

	REGISTER_FILTER(iABinaryThreshold);
	REGISTER_FILTER(iAOtsuThreshold);
	REGISTER_FILTER(iAOtsuMultipleThreshold);
	REGISTER_FILTER(iAMaximumDistance);
	REGISTER_FILTER(iARatsThreshold);
	REGISTER_FILTER(iAAdaptiveOtsuThreshold);
	REGISTER_FILTER(iAParameterlessThresholding);

	REGISTER_FILTER(iAConfidenceConnectedRegionGrow);
	REGISTER_FILTER(iAConnectedThresholdRegionGrow);
	REGISTER_FILTER(iANeighborhoodConnectedRegionGrow);

	REGISTER_FILTER(iAWatershed);
	REGISTER_FILTER(iAMorphologicalWatershed);

	REGISTER_FILTER(iAFCMFilter);
	REGISTER_FILTER(iAKFCMFilter);
#if ITK_VERSION_MAJOR < 5
// MSKFCM filter is implemented with the help of itk Barriers, which got removed with ITK 5
	REGISTER_FILTER(iAMSKFCMFilter);
#endif

	REGISTER_FILTER(iACannySegmentationLevelSet);
	REGISTER_FILTER(iALaplacianSegmentationLevelSet);
	REGISTER_FILTER(iAZeroCrossing);

	REGISTER_FILTER(iASVMImageFilter);
	REGISTER_FILTER(iAKMeans);

	REGISTER_FILTER(iAPCA);
	REGISTER_FILTER(iARandomWalker);
	REGISTER_FILTER(iAExtendedRandomWalker);
	REGISTER_FILTER(iAMaximumDecisionRule);
	REGISTER_FILTER(iALabelImageToSeeds);
}
