/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
#pragma once

#include "iALineSegment.h"
#include "iALinePointers.h"
#include "iADiskData.h"

class vtkRenderer;
class vtkImageData;
class vtkActor;

struct iAArbitraryProfileOnSlicer 
{
	iAArbitraryProfileOnSlicer();
	~iAArbitraryProfileOnSlicer();

	void initialize(vtkRenderer * ren);
	void setPointScaling(double scaling);
	void SetVisibility(bool isVisible);
	void FindSelectedPntInd(double x, double y);
	int setup(int pointInd, double * pos3d, double * pos2d, vtkImageData *imgData);
	int GetPntInd() const;
	double * GetPosition(int index);

	static const int Z_COORD = 1;
	static const int ARB_RADIUS = 5;

protected:
	vtkRenderer * m_ren;	//vtk renderer
	double m_radius;		//radius, taking into account image spacing
	int m_arbProfPntInd;	// currently selected point of arbitrary profile

	iALineSegment		m_hLine[2], m_vLine[2]; //horizontal and vertical lines
	iALineSegment		m_profLine;				//profile line
	iALinePointers		m_zeroLine;				//zero line
	iADiskData			m_points[2];
	double				m_positions[2][3];
};
