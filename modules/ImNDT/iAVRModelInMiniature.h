// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAVRCubicVis.h"

//! Class for the abstract 3D Model in Miniature visualization
class iAVRModelInMiniature : public iAVRCubicVis
{
public:
	iAVRModelInMiniature(vtkRenderer* ren);
	void createCubeModel() override;
	void setScale(double x, double y, double z);
	void setPos(double x, double y, double z);
	void addPos(double x, double y, double z);
	void setOrientation(double x, double y, double z);
};
