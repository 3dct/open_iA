// Copyright (c) open_iA contributors
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
	void	show( ) override;
	void	hide( ) override;

private:
	vtkSmartPointer<vtkOBJReader>		m_reader;
	vtkSmartPointer<vtkPolyDataMapper>	m_mapper;
	vtkSmartPointer<vtkActor>			m_actor;
};
