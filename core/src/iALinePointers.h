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

#include <vtkSmartPointer.h>

class vtkActor;
class vtkConeSource;
class vtkPoints;
class vtkPolyDataMapper;
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
	// TODO: check whether these are deleted! use std::array instead?:
	vtkSmartPointer<vtkActor> actors[2];
	vtkSmartPointer<vtkPolyDataMapper> mappers[2];
	vtkSmartPointer<vtkConeSource> pointers[2];
	static const int ConeHeight = 10;
	static const int ZCoord = 0;
};
