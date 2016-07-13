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
#include <vtkType.h>
#include <vtkSmartPointer.h>

class vtkPolyLine;
class vtkCellArray;
class vtkPolyData;
class vtkPoints;
class vtkActor;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkImageData;



struct iASlicerProfile 
{
	iASlicerProfile();

	void	SetVisibility ( bool isVisible );
	void	GetPoint ( vtkIdType id, double pos_out[3] );
	void	initialize (vtkRenderer * ren);
	int		setup ( double posY, vtkImageData * imgData );
	
public:
	static const int Z_COORD = 0;

protected:
	vtkRenderer			* m_ren;

	iALineSegment		m_profileLine;
	iALinePointers		m_zeroLine;

	//plot
	vtkSmartPointer<vtkPolyLine>		m_plotPolyLine;
	vtkSmartPointer<vtkCellArray>		m_plotCells;
	vtkSmartPointer<vtkPolyData>		m_plotPolyData;
	vtkSmartPointer<vtkPoints>			m_plotPoints;
	vtkSmartPointer<vtkActor>			m_plotActor;
	vtkSmartPointer<vtkActor>			m_plotActorHalo;  
	vtkSmartPointer<vtkPolyDataMapper>	m_plotMapper;
	float								m_plotScaleFactor;
};
