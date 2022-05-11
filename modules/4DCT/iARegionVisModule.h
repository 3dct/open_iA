/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
