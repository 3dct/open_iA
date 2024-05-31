// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// iA
#include "iAVisModule.h"
// vtk
#include <vtkSmartPointer.h>
// Qt
#include <QColor>

class vtkActor;
class vtkCubeSource;
class vtkPolyDataMapper;

struct iABoundingBoxSettings
{
	QColor Color;
};

class iABoundingBoxVisModule : public iAVisModule
{
public:
				iABoundingBoxVisModule( );
	void		show( ) override;
	void		hide( ) override;
	void		setSize( double * size );
	void		setColor( double r, double g, double b );
	void		setPosition( double x, double y, double z );
	void		setLineWidth( float w );

	iABoundingBoxSettings	settings;

protected:
	vtkSmartPointer<vtkCubeSource>			m_cubeSource;
	vtkSmartPointer<vtkPolyDataMapper>		m_mapper;
	vtkSmartPointer<vtkActor>				m_actor;
};
