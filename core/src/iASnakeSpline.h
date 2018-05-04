/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iASpline.h"
#include <vtkDiskSource.h>

typedef std::vector<iADiskData*> disc_vector;

class iASnakeSpline
{
public:
	iASnakeSpline()
	  : m_selectedPntInd(-1), m_ren(0), m_radius(RADIUS)
	{}

	void initialize(vtkRenderer * ren, double imageSpacing)
	{
		m_ren = ren;
		m_ren->AddActor(m_spline.GetActor());
		m_radius = RADIUS * imageSpacing;
	}

	~iASnakeSpline()
	{
		for(disc_vector::size_type i = 0; i != m_snakeDisks.size(); i++)
			delete m_snakeDisks[i];
	}

	void addPoint(double x, double y)
	{
		// create spline point source, mapper and actor
		iADiskData * snakeDisk = new iADiskData;

		// create point source
		snakeDisk->source->SetOuterRadius(m_radius);
		snakeDisk->source->Update();

		// initialize mapper and actor
		snakeDisk->actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
		snakeDisk->actor->GetProperty()->SetOpacity(0.8);
		snakeDisk->actor->SetVisibility(true);
		snakeDisk->actor->SetPosition(x, y, Z);

		// add actor to actor list to be able to find the point for selection
		m_snakeDisks.push_back(snakeDisk);

		// add actor to the slice view renderer
		m_ren->AddActor(snakeDisk->actor);

		// add point to the spline points
		double pos[3] = {x, y, Z};
		m_spline.addPoint(pos);

		// the new created point is now the selected one
		m_selectedPntInd = m_snakeDisks.size() - 1;
	}

	void movePoint(disc_vector::size_type selectedPntInd, double x, double y)
	{
		if (static_cast<int>(selectedPntInd) < m_spline.GetNumberOfPoints())
		{
			// get current position of point and only move in two directions depending on slice view
			double * currentPos = m_snakeDisks[selectedPntInd]->actor->GetPosition();
			m_snakeDisks[selectedPntInd]->actor->SetPosition(x, y, Z);
			m_spline.SetPoint(selectedPntInd, x, y, Z);
		}

		// update spline curve
		m_spline.Modified();
	}

	disc_vector::size_type CalculateSelectedPoint(double x, double y)
	{
		m_selectedPntInd = -1;
		for(disc_vector::size_type i = 0; i != m_snakeDisks.size(); i++)
		{
			double *handlePos = m_snakeDisks[i]->actor->GetPosition();

			if ( x >= handlePos[0] - m_radius &&  x <= handlePos[0] + m_radius &&
				 y >= handlePos[1] - m_radius &&  y <= handlePos[1] + m_radius )
				m_selectedPntInd = i;

			if (m_selectedPntInd != -1)
				break;
		}
		return m_selectedPntInd;
	}

	void deleteAllPoints()
	{
		// remove point snakeDisks
		for(disc_vector::size_type i = 0; i != m_snakeDisks.size(); i++)
		{
			m_ren->RemoveActor(m_snakeDisks[i]->actor);
			delete m_snakeDisks[i];
		}

		// clear snakeDisks list
		m_snakeDisks.clear();

		// reset point lists
		m_spline.Reset();

		// hide spline curve
		m_spline.SetVisibility(false);
	}

	void SetVisibility(bool isVisible)
	{
		for(disc_vector::size_type i = 0; i != m_snakeDisks.size(); i++)
			m_snakeDisks[i]->actor->SetVisibility(isVisible);
		m_spline.SetVisibility(isVisible);
	}

	disc_vector::size_type selectedPointIndex()
	{
		return m_selectedPntInd;
	}

	void deselectPoint()
	{
		m_selectedPntInd = -1;
	}

public:
	static const int RADIUS = 5;

protected:
	iASpline  m_spline;							//vtk classes for representing a spline
	disc_vector	m_snakeDisks;					// spline point snakeDisks
	disc_vector::size_type m_selectedPntInd;	// currently selected spline point
	vtkRenderer * m_ren;
	double m_radius;
	static const double Z;
};

const double iASnakeSpline::Z = 0.2;
