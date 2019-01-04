/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAIntensityModuleInterface.h"

#include "iAIntensity.h"

#include <iAFilterRegistry.h>

void iAIntensityModuleInterface::Initialize()
{
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
}
