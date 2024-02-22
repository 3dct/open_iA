// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// iA
#include "iAVisModule.h"
// vtk
#include <vtkSmartPointer.h>
// itk
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImage.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif
// Qt
#include <QColor>

class vtkActor;
class vtkCellArray;
class vtkPoints;
class vtkPolyDataMapper;
class vtkUnsignedCharArray;

class iAFractureVisModule : public iAVisModule
{
public:
	typedef itk::Image<double, 2>		MapType;
	enum MapName { Heightmap, Colormap };

				iAFractureVisModule( );
	void		show( );
	void		hide( );
	void		setData( vtkPoints* points, vtkCellArray* polys, vtkUnsignedCharArray* colors = 0 );
	void		load( QString fileName );
	void		save( QString fileName );
	void		extract( QString fileName );
	void		setSize( double * size );
	void		setColorMap( QString fileName );
	void		setLowIntensityColor( QColor color );
	void		setHighIntensityColor( QColor color );
	void		setColor( QColor color );
	void		setAmbient( double coeff );
	void		setOpacity( double val );
	void		getBounds( double * bounds );

private:
	void				calculateMap( MapType * map, QString fileName, MapName mapName );
	void				calculatePoints( vtkPoints * points, MapType * heightmap, double * size );
	void				calculatePolys( vtkCellArray * polys, MapType * heightmap );
	void				calculateColors( vtkUnsignedCharArray * colors, MapType * colormap, MapType * heightmap );

	vtkSmartPointer<vtkPolyDataMapper>		m_surfMapper;
	vtkSmartPointer<vtkActor>				m_surfActor;
	const int								Step;
	itk::SmartPointer<MapType>				m_heightmap;
	itk::SmartPointer<MapType>				m_colormap;
	vtkSmartPointer<vtkPoints>				m_points;
	vtkSmartPointer<vtkCellArray>			m_polys;
	vtkSmartPointer<vtkUnsignedCharArray>	m_colors;
	double									m_size[3];
	QColor									m_lowIntensity;
	QColor									m_highIntensity;
};
