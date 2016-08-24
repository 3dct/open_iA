/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 
#ifndef IAFRACTUREVISMODULE_H
#define IAFRACTUREVISMODULE_H
// iA
#include "iAVisModule.h"
// vtk
#include <vtkSmartPointer.h>
// itk
#include <itkImage.h>
// Qt
#include <QString>
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

				iAFractureVisModule();
	void		enable();
	void		disable();
	void		setData(vtkPoints* points, vtkCellArray* polys, vtkUnsignedCharArray* colors = 0);
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

#endif // IAFRACTUREVISMODULE_H