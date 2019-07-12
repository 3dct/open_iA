/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "iALineSegment.h"
#include "iALinePointers.h"
#include "iADiskData.h"

class vtkRenderer;
class vtkImageData;
class vtkActor;

//! Adds handles for start and end of a profile line to the given renderer
class iAArbitraryProfileOnSlicer
{
public:
	iAArbitraryProfileOnSlicer();

	void addToRenderer(vtkRenderer * ren);
	void setPointScaling(double scaling);
	void setVisibility(bool isVisible);
	void findSelectedPointIdx(double x, double y);
	int setup(int pointInd, double const * pos3d, double const * pos2d, vtkImageData *imgData);
	int pointIdx() const;
	double const * position(int pointIdx);

	static const int ZCoord = 0;
	static const int PointRadius = 5;

protected:
	double              m_radius;               //!< radius, taking into account image spacing
	int                 m_arbProfPntInd;        //!< currently selected point of arbitrary profile
	iALineSegment       m_hLine[2], m_vLine[2]; //!< horizontal and vertical lines
	iALineSegment       m_profLine;             //!< profile line
	iALinePointers      m_zeroLine;             //!< zero line
	iADiskData          m_points[2];            //!< data for the disk visualizations of start and end point
	double              m_positions[2][3];
};
