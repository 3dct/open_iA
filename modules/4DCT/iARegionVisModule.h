// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

// iA
#include "iAVisModule.h"
// vtk
#include <vtkSmartPointer.h>
// Qt
#include <QColor>
// itk
#include <itkImage.h>

class vtkActor;
class vtkPolyDataMapper;
class vtkPolyDataSilhouette;
class vtkImageData;

struct iARegionVisSettings
{
	double Position[3];
	double SilhoetteOpacity;
	double SurfaceOpacity;
	QColor SilhoetteColor;
	QColor SurfaceColor;
	double SilhoetteWidth;
};

class iARegionVisModule : public iAVisModule
{
public:
	typedef itk::Image<double, 3>		DensityMapType;

				iARegionVisModule( );
	void		show( );
	void		hide( );
	void		setData( vtkImageData * image );
	void		setPosition( double * position );
	void		setSilhoetteOpacity( double opacity );
	void		setSurfaceOpacity( double opacity );
	void		setSilhoetteColor( double r, double g, double b );
	void		setSurfaceColor( double r, double g, double b );
	void		setImage( QString fileName );
	void		setSilhoetteLineWidth( double width );
	void		setDefectDensity( double densityVal );
	void		setDensityMapDimension( int dimX, int dimY, int dimZ );
	void		setDensityMapDimension( int * dim );

	iARegionVisSettings		settings;

private:
	void		calculateDensityMap( QString fileName, iARegionVisModule* visModule );

	vtkSmartPointer<vtkPolyDataMapper>		m_regionMapper;
	vtkSmartPointer<vtkActor>				m_regionActor;
	vtkSmartPointer<vtkPolyDataSilhouette>	m_silhouette;
	vtkSmartPointer<vtkPolyDataMapper>		m_silhouetteMapper;
	vtkSmartPointer<vtkActor>				m_silhouetteActor;

	int			m_densityMapSize[3];
	double		m_densityVal;
};
