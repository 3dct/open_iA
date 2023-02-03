// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// iA
#include "iAVisModule.h"
// vtk
#include <vtkSmartPointer.h>
// Qt
#include <QString>

class vtkPolyDataMapper;
class vtkActor;
class vtkOBJReader;

class iADefectVisModule : public iAVisModule
{
public:
			iADefectVisModule( );
	void	setInputFile( QString path );
	void	setColor( double r, double g, double b );
	void	setOpacity( double opacity );
	void	show( );
	void	hide( );

private:
	vtkSmartPointer<vtkOBJReader>		m_reader;
	vtkSmartPointer<vtkPolyDataMapper>	m_mapper;
	vtkSmartPointer<vtkActor>			m_actor;
};
