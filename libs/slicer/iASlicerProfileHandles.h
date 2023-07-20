// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iALinePointers.h"

#include <vtkDiskSource.h>
#include <vtkLineSource.h>

#include <array>

class vtkActor;
class vtkImageData;
class vtkRenderer;

//! Shows handles for start and end of a profile line on the given (slicer) renderer.
class iASlicerProfileHandles
{
public:
	iASlicerProfileHandles();

	void addToRenderer(vtkRenderer * ren);
	void setPointScaling(double scaling);
	void setVisibility(bool isVisible);
	void findSelectedPointIdx(double x, double y);
	void setup(int pointInd, double const * pos3d, double const * pos2d, vtkImageData *imgData);
	int pointIdx() const;
	double const * position(int pointIdx);

	static const int ZCoord = 0;
	static const int PointRadius = 5;

protected:
	double              m_radius;               //!< radius, taking into account image spacing
	int                 m_profPntInd;           //!< currently selected point of profile
	std::array<iAvtkSourcePoly<vtkLineSource>, 2> m_hLine, m_vLine; //!< horizontal and vertical lines
	iAvtkSourcePoly<vtkLineSource> m_profLine;     //!< profile line
	iALinePointers      m_zeroLine;             //!< zero line
	std::array<iAvtkSourcePoly<vtkDiskSource>, 2> m_points;    //!< data for the disk visualizations of start and end point
	double              m_positions[2][3];
};
