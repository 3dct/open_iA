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
 
#include "pch.h"
#include "iAFast3DMagicLensWidget.h"
#include "iAConsole.h"

// std
#define _USE_MATH_DEFINES
#include <cmath>
// vtk
#include <QVTKInteractor.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkPoints.h>
#include <vtkPolyLine.h>
#include <vtkCellArray.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkActor2D.h>
#include <vtkActor2DCollection.h>
#include <vtkProperty2D.h>

#include <QMouseEvent>

iAFast3DMagicLensWidget::iAFast3DMagicLensWidget( QWidget * parent /*= 0 */ )
	: iAAbstractMagicLensWidget( parent )
	, m_viewAngle{ 15. }
{
	setFocusPolicy(Qt::StrongFocus);	// to receive the KeyPress Event!
}

iAFast3DMagicLensWidget::~iAFast3DMagicLensWidget()
{
}

void iAFast3DMagicLensWidget::updateLens()
{
	iAAbstractMagicLensWidget::updateLens();
	// preparations

	if (GetRenderWindow()->GetRenderers()->GetNumberOfItems() <= 0)
		return;
	
	vtkCamera * mainCam = GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	vtkCamera * magicLensCam = m_lensRen->GetActiveCamera();

	if( mainCam->GetUseOffAxisProjection() == 0 )
	{
		mainCam->UseOffAxisProjectionOn();
	}
	if( magicLensCam->GetUseOffAxisProjection() == 0 )
	{
		magicLensCam->UseOffAxisProjectionOn();
	}

	int * pos = GetInteractor()->GetEventPosition();

	// copy camera position and rotation
	magicLensCam->SetPosition( mainCam->GetPosition() );
	magicLensCam->SetRoll( mainCam->GetRoll() );
	magicLensCam->SetFocalPoint( mainCam->GetFocalPoint() );

	// setup parameters for frustum calculations
	double w =  (double)m_size[0] / height();
	double h = (double)m_size[1] / height();
	double p[2] = { ( (double)pos[0] * 2 / width() - 1 ) * ( (double)width() / height() ), (double)pos[1] * 2 / height() - 1 };
	double z = calculateZ( m_viewAngle );
	magicLensCam->SetScreenBottomLeft(  p[0] - w, p[1] - h, z );
	magicLensCam->SetScreenBottomRight( p[0] + w, p[1] - h, z );
	magicLensCam->SetScreenTopRight(    p[0] + w, p[1] + h, z );
}

void iAFast3DMagicLensWidget::resizeEvent( QResizeEvent * event )
{
	QVTKWidget2::resizeEvent( event );

	if (GetRenderWindow()->GetRenderers()->GetNumberOfItems() <= 0)
		return;
						// TODO: VOLUME: find better way to get "main" renderer here!
	vtkCamera * mainCam = GetRenderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	double w = (double)width() / height();	// calculate width aspect ratio
	double z = calculateZ( m_viewAngle );
	mainCam->SetScreenBottomLeft( -w, -1, z );
	mainCam->SetScreenBottomRight( w, -1, z );
	mainCam->SetScreenTopRight(    w,  1, z );
}

inline double iAFast3DMagicLensWidget::calculateZ( double viewAngle )
{
	return -1. / std::tan( viewAngle * M_PI / 180. );
}

void iAFast3DMagicLensWidget::mouseReleaseEvent( QMouseEvent * event )
{
	if (Qt::RightButton == event->button())
		emit rightButtonReleasedSignal();
	else if (Qt::LeftButton == event->button())
		emit leftButtonReleasedSignal();
	QVTKWidget2::mouseReleaseEvent(event);
}
