// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

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
