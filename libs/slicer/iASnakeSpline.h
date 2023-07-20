// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iASpline.h"

#include <iAvtkSourcePoly.h>

typedef std::vector<iAvtkSourcePoly<vtkDiskSource>*> disc_vector;

//! Visualize a snake spline (for snake slicer - currently not fully working!) across the slicer.
class iASnakeSpline
{
public:
	static const disc_vector::size_type NoPointSelected = std::numeric_limits<disc_vector::size_type>::max();

	iASnakeSpline()
	  : m_selectedPntInd(NoPointSelected), m_ren(0), m_radius(RADIUS)
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
		auto snakeDisk = new iAvtkSourcePoly<vtkDiskSource>();

		// create point source
		snakeDisk->source->SetInnerRadius(0.0);
		snakeDisk->source->SetCircumferentialResolution(20);
		snakeDisk->source->SetOuterRadius(m_radius);
		snakeDisk->source->Update();

		// initialize mapper and actor
		snakeDisk->actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
		snakeDisk->actor->GetProperty()->SetOpacity(0.8);
		snakeDisk->actor->SetVisibility(true);
		snakeDisk->actor->SetPosition(x, y, ZCoord);

		// add actor to actor list to be able to find the point for selection
		m_snakeDisks.push_back(snakeDisk);

		// add actor to the slice view renderer
		m_ren->AddActor(snakeDisk->actor);

		// add point to the spline points
		double pos[3] = {x, y, ZCoord};
		m_spline.addPoint(pos);

		// the new created point is now the selected one
		m_selectedPntInd = m_snakeDisks.size() - 1;
	}

	void movePoint(disc_vector::size_type selectedPntInd, double x, double y)
	{
		if (static_cast<int>(selectedPntInd) < m_spline.GetNumberOfPoints())
		{
			// get current position of point and only move in two directions depending on slice view
			//double * currentPos = m_snakeDisks[selectedPntInd]->actor->GetPosition();
			m_snakeDisks[selectedPntInd]->actor->SetPosition(x, y, ZCoord);
			m_spline.SetPoint(selectedPntInd, x, y, ZCoord);
		}

		// update spline curve
		m_spline.Modified();
	}

	disc_vector::size_type CalculateSelectedPoint(double x, double y)
	{
		m_selectedPntInd = NoPointSelected;
		for (disc_vector::size_type i = 0; i != m_snakeDisks.size(); i++)
		{
			double *handlePos = m_snakeDisks[i]->actor->GetPosition();

			if (x >= handlePos[0] - m_radius && x <= handlePos[0] + m_radius &&
				y >= handlePos[1] - m_radius && y <= handlePos[1] + m_radius)
			{
				m_selectedPntInd = i;
			}

			if (m_selectedPntInd != NoPointSelected)
			{
				break;
			}
		}
		return m_selectedPntInd;
	}

	void deleteAllPoints()
	{
		// remove point snakeDisks
		for (disc_vector::size_type i = 0; i != m_snakeDisks.size(); i++)
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
		for (disc_vector::size_type i = 0; i != m_snakeDisks.size(); i++)
		{
			m_snakeDisks[i]->actor->SetVisibility(isVisible);
		}
		m_spline.SetVisibility(isVisible);
	}

	disc_vector::size_type selectedPointIndex()
	{
		return m_selectedPntInd;
	}

	void deselectPoint()
	{
		m_selectedPntInd = NoPointSelected;
	}

	static const int RADIUS = 5;

protected:
	iASpline  m_spline;							//vtk classes for representing a spline
	disc_vector	m_snakeDisks;					// spline point snakeDisks
	disc_vector::size_type m_selectedPntInd;	// currently selected spline point
	vtkRenderer * m_ren;
	double m_radius;
	static const double ZCoord;
};

const double iASnakeSpline::ZCoord = 0;
