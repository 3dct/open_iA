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
 
#ifndef IA_PIE_CHART_GLYPH
#define IA_PIE_CHART_GLYPH

#include "iAPieSource.h"

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>

class iAPieChartGlyph
{
public:
	iAPieChartGlyph(double portion) 
		:
		sectorSrc(vtkSmartPointer<iAPieSource>::New()),
		mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
		actor(vtkSmartPointer<vtkActor>::New())
	{
		sectorSrc->SetStartAngle(0.0);
		sectorSrc->SetEndAngle(360.0*portion);
		Init();
	}

	iAPieChartGlyph(double startAngle, double endAngle) 
		:
		sectorSrc(vtkSmartPointer<iAPieSource>::New()),
		mapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
		actor(vtkSmartPointer<vtkActor>::New())
	{
		sectorSrc->SetStartAngle(startAngle);
		sectorSrc->SetEndAngle(endAngle);
		Init();
	}

	iAPieChartGlyph( const iAPieChartGlyph& other ) :
		sectorSrc( other.sectorSrc ), 
		mapper( other.mapper ), 
		actor( other.actor )
	{}

	vtkSmartPointer<iAPieSource>		sectorSrc;
	vtkSmartPointer<vtkPolyDataMapper>	mapper;
	vtkSmartPointer<vtkActor>			actor;  
private:
	void Init()
	{
		mapper->SetInputConnection(sectorSrc->GetOutputPort());
		actor->SetMapper(mapper);
	}
};

#endif	//IA_PIE_CHART_GLYPH
