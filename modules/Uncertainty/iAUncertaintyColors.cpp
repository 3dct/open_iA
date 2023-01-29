// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAUncertaintyColors.h"

// "qualitative" 8-class dark-2 from colorbrewer2.org:
QColor iAUncertaintyColors::ScatterPlotDots(27,158,119);	// 1
QColor iAUncertaintyColors::SelectedPixel(230, 171, 2);		// 6
QColor iAUncertaintyColors::MemberBar(217,95,2);			// 2
QColor iAUncertaintyColors::UncertaintyDistribution(117,112,179);	// 3
QColor iAUncertaintyColors::SelectedMember(102,102,102);    // 8
// (231,41,138); // 4
// except for this, this is "sequential" "single hue" "9-class Blues" color 9:
//QColor iAUncertaintyColors::LabelDistributionBase(8,48,107);

// "sequential" "single hue" "9-class Greens" color 9:
// leads to "shrill" green hue...
//QColor iAUncertaintyColors::LabelDistributionBase(0, 68, 27);

// "sequential" "single hue" "9-class Purples" color 9:
QColor iAUncertaintyColors::LabelDistributionBase(63, 0, 125);
