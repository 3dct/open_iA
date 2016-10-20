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
#pragma once

#include "open_iA_Core_export.h"
// vtk
#include <QVTKWidget2.h>
#include <vtkSmartPointer.h>

class vtkActor2D;
class vtkCamera;
class vtkInteractorStyle;
class vtkRenderer;

class open_iA_Core_API iAAbstractMagicLensWidget : public QVTKWidget2
{
	Q_OBJECT
public:
	enum ViewMode{
		CENTERED,
		OFFSET
	};
						iAAbstractMagicLensWidget( QWidget * parent = 0 );
	virtual				~iAAbstractMagicLensWidget( );
	void				magicLensOn();
	void				magicLensOff();
	void				setLensSize( int sizeX, int sizeY );
	vtkRenderer*		getLensRenderer( );
	void				setViewMode( ViewMode mode );

	void SetMainRenderWindow(vtkGenericOpenGLRenderWindow* renWin);

signals:
	void MouseMoved();
protected:
	virtual void		mouseMoveEvent( QMouseEvent * event );
	virtual void		updateLens( );
	virtual void		updateGUI( );
	void				getViewportPoints( double points[4] );

	vtkSmartPointer<vtkRenderer>	m_lensRen;
	vtkSmartPointer<vtkRenderer>	m_GUIRen;
	vtkSmartPointer<vtkActor2D>		m_GUIActor;
	int								m_pos[2];
	int								m_size[2];
	double							m_halfSize[2];
	ViewMode						m_viewMode;

private:
	static const double				OFFSET_VAL;
};
