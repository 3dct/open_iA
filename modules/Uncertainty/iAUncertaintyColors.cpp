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
#include "iAUncertaintyColors.h"

// "qualitative" 7-class dark-2 from colorbrewer2.org:
QColor iAUncertaintyColors::ScatterPlotDots(27,158,119);	// 1
QColor iAUncertaintyColors::SelectedPixel(217,95,2);		// 2
QColor iAUncertaintyColors::MemberBar(166,118,29);			// 7
QColor iAUncertaintyColors::UncertaintyDistribution(117,112,179);	// 3
QColor iAUncertaintyColors::SelectedMember(230, 171, 2);  // 6
// (217, 95,2);  // 2
// (231,41,138); // 4
// except for this, this is "sequential" 9-class Blues color 9:
QColor iAUncertaintyColors::LabelDistributionBase(8,48,107);
