// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// base:
#include "iAITKIO.h" // for image type

struct iAImageComparisonResult
{
	// double dice;
	double equalPixelRate;
};


iAImageComparisonResult CompareImages(iAITKIO::ImagePointer img, iAITKIO::ImagePointer reference);
