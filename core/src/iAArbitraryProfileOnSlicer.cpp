/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAArbitraryProfileOnSlicer.h"

#include <vtkActor.h>
#include <vtkDiskSource.h>
#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>



iAArbitraryProfileOnSlicer::iAArbitraryProfileOnSlicer():
	m_radius(PointRadius),
	m_arbProfPntInd(-1)
{
	for (int i = 0; i < 2; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			m_positions[i][j] = 0;
		}
	}

	m_profLine.actor->GetProperty()->SetColor(0.59, 0.73, 0.94);//ffa800//150, 186, 240
	m_profLine.actor->GetProperty()->SetLineWidth(2.0);
	m_profLine.actor->GetProperty()->SetLineStipplePattern(0x00ff);//0xf0f0
	m_profLine.actor->GetProperty()->SetLineStippleRepeatFactor(1);
	m_profLine.actor->GetProperty()->SetPointSize(2);

	for (int i = 0; i < 2; i++)
	{
		m_points[i].source->SetOuterRadius(m_radius);
		m_points[i].actor->GetProperty()->SetOpacity(0.3);

		m_hLine[i].actor->GetProperty()->SetOpacity(0.3);
		m_hLine[i].actor->GetProperty()->SetLineWidth(2.0);
		m_vLine[i].actor->GetProperty()->SetOpacity(0.3);
		m_vLine[i].actor->GetProperty()->SetLineWidth(2.0);
	}
	m_hLine[0].actor->GetProperty()->SetColor(1.0, 0.65, 0.0);
	m_vLine[0].actor->GetProperty()->SetColor(1.0, 0.65, 0.0);
	m_points[0].actor->GetProperty()->SetColor(1.0, 0.65, 0.0);
	m_hLine[1].actor->GetProperty()->SetColor(0.0, 0.65, 1.0);
	m_vLine[1].actor->GetProperty()->SetColor(0.0, 0.65, 1.0);
	m_points[1].actor->GetProperty()->SetColor(0.0, 0.65, 1.0);
}

void iAArbitraryProfileOnSlicer::setVisibility( bool isVisible )
{
	m_profLine.actor->SetVisibility(isVisible);

	for (int i=0; i<2; i++)
	{
		m_points[i].actor->SetVisibility(isVisible);
		m_hLine[i].actor->SetVisibility(isVisible);
		m_vLine[i].actor->SetVisibility(isVisible);
	}
}

void iAArbitraryProfileOnSlicer::setPointScaling( double scaling )
{
	m_radius = PointRadius * scaling;
	for (vtkIdType i = 0; i < 2; i++)
	{
		m_points[i].source->SetOuterRadius(m_radius);
	}
}

void iAArbitraryProfileOnSlicer::findSelectedPointIdx( double x, double y )
{
	m_arbProfPntInd = -1;
	for (int i = 0; i < 2; i++)
	{
		double *handlePos = m_points[i].actor->GetPosition();
		if (x >= handlePos[0] - m_radius && x <= handlePos[0] + m_radius
			&& y >= handlePos[1] - m_radius && y <= handlePos[1] + m_radius)
		{
			m_arbProfPntInd = i;
			break;
		}
	}
}

void iAArbitraryProfileOnSlicer::addToRenderer( vtkRenderer * ren )
{
	ren->AddActor(m_profLine.actor);
	for (vtkIdType i=0; i<2; i++)
	{
		ren->AddActor(m_hLine[i].actor);
		ren->AddActor(m_vLine[i].actor);
		ren->AddActor(m_points[i].actor);
	}
}

int iAArbitraryProfileOnSlicer::setup( int pointIdx, double const * pos3d, double const * pos2d, vtkImageData *imgData )
{
	if (pointIdx < 0 || pointIdx>1)
	{
		return 0;
	}
	for (int i = 0; i < 3; i++)
	{
		m_positions[pointIdx][i] = pos3d[i];
	}

	// get spacing for point creation whose size depends on
	double * spacing	= imgData->GetSpacing();
	double * origin		= imgData->GetOrigin();
	int * dimensions	= imgData->GetDimensions();

	m_profLine.points->SetPoint(pointIdx, pos2d[0], pos2d[1], ZCoord);

	m_hLine[pointIdx].points->SetPoint(0, origin[0], pos2d[1], ZCoord);
	m_hLine[pointIdx].points->SetPoint(1, origin[0] + dimensions[0]*spacing[0], pos2d[1], ZCoord);
	m_hLine[pointIdx].lineSource->SetPoint1(m_hLine[pointIdx].points->GetPoint(0));
	m_hLine[pointIdx].lineSource->SetPoint2(m_hLine[pointIdx].points->GetPoint(1));

	m_vLine[pointIdx].points->SetPoint(0, pos2d[0], origin[1], ZCoord);
	m_vLine[pointIdx].points->SetPoint(1, pos2d[0], origin[1] + dimensions[1]*spacing[1], ZCoord);
	m_vLine[pointIdx].lineSource->SetPoint1(m_vLine[pointIdx].points->GetPoint(0));
	m_vLine[pointIdx].lineSource->SetPoint2(m_vLine[pointIdx].points->GetPoint(1));

	if (pointIdx == 0)
	{
		m_profLine.lineSource->SetPoint1(m_profLine.points->GetPoint(0));
	}
	else
	{
		m_profLine.lineSource->SetPoint2(m_profLine.points->GetPoint(1));
	}

	double currentPos[3]; m_points[pointIdx].actor->GetPosition(currentPos);
	m_points[pointIdx].actor->SetPosition(pos2d[0], pos2d[1], ZCoord);

	setPointScaling(spacing[0] > spacing[1] ? spacing[0] : spacing[1]);
	return 1;
}

int iAArbitraryProfileOnSlicer::pointIdx() const
{
	return m_arbProfPntInd;
}

double const * iAArbitraryProfileOnSlicer::position( int pointIdx)
{
	return m_positions[pointIdx];
}
