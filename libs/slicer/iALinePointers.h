// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAvtkSourcePoly.h>

#include <vtkSmartPointer.h>

#include <array>

class vtkPoints;
class vtkRenderer;

//! A horizontal line that can be added to a vtkRenderer, with two cones marking start and end of the line
class iALinePointers
{
public:
	iALinePointers();
	void updatePosition(double posY, double zeroLevelPosY, double startX, double endX, double const * spacing);
	void setVisible(bool visible);
	void addToRenderer(vtkRenderer * renderer);

private:
	vtkSmartPointer<vtkPoints> points;
	std::array<iAConeSource, 2> m_cones;
	static const int ConeHeight = 10;
	static const int ZCoord = 0;
};
