// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALinePointers.h"

#include <vtkPoints.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

iALinePointers::iALinePointers():
	points(vtkSmartPointer<vtkPoints>::New())
{
	points->Allocate(2);
	for (vtkIdType i = 0; i < 2; i++)
	{
		m_cones[i].source->SetResolution(4);
		m_cones[i].actor->GetProperty()->SetAmbientColor(1.0, 1.0, 1.0);
		m_cones[i].actor->GetProperty()->SetAmbient(1.0);
		//m_cones[i].actor->GetProperty()->SetLineWidth(1);
	}
	m_cones[1].source->SetDirection(-1, 0, 0);
}

void iALinePointers::setVisible(bool visible)
{
	for (vtkIdType i = 0; i < 2; i++)
	{
		m_cones[i].actor->SetVisibility(visible);
	}
}

void iALinePointers::addToRenderer(vtkRenderer * ren)
{
	for (vtkIdType i = 0; i < 2; i++)
	{
		ren->AddActor(m_cones[i].actor);
	}
}

void iALinePointers::updatePosition(double posY, double zeroLevelPosY, double startX, double endX, double const * spacing)
{
	points->SetPoint(0, startX, zeroLevelPosY, ZCoord);
	points->SetPoint(1, endX, zeroLevelPosY, ZCoord);
	double scaling = spacing[0] > spacing[1] ? spacing[0] : spacing[1];
	double height = ConeHeight * scaling;
	for (int i = 0; i < 2; ++i)
	{
		m_cones[i].source->SetHeight(height);
		m_cones[i].source->SetRadius(height / 4.0);
		m_cones[i].source->SetCenter(points->GetPoint(i));
	}
	m_cones[0].actor->SetPosition(-height / 2, posY, 0);
	m_cones[1].actor->SetPosition( height / 2, posY, 0);
}
