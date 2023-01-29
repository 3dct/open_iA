// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFast3DMagicLensWidget.h"

#include <QVTKInteractor.h>
#include <vtkActor2DCollection.h>
#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>

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
	if (renderWindow()->GetRenderers()->GetNumberOfItems() <= 0)
	{
		return;
	}
	vtkCamera * mainCam = renderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	vtkCamera * magicLensCam = m_lensRen->GetActiveCamera();

	if( mainCam->GetUseOffAxisProjection() == 0 )
	{
		mainCam->UseOffAxisProjectionOn();
	}
	if( magicLensCam->GetUseOffAxisProjection() == 0 )
	{
		magicLensCam->UseOffAxisProjectionOn();
	}

	int * pos = interactor()->GetEventPosition();

	// copy camera position and rotation
	magicLensCam->SetPosition( mainCam->GetPosition() );
	magicLensCam->SetRoll( mainCam->GetRoll() );
	magicLensCam->SetFocalPoint( mainCam->GetFocalPoint() );

	// setup parameters for frustum calculations
	double pixelHeight = height() * devicePixelRatio();
	double pixelWidth = width() * devicePixelRatio();
	double w = m_size[0] / pixelHeight;
	double h = m_size[1] / pixelHeight;
	double p[2] = {
		(pos[0] * 2.0 / pixelWidth - 1) * (pixelWidth / pixelHeight),
		pos[1] * 2.0 / pixelHeight - 1
	};
	double z = calculateZ( m_viewAngle );
	magicLensCam->SetScreenBottomLeft(  p[0] - w, p[1] - h, z );
	magicLensCam->SetScreenBottomRight( p[0] + w, p[1] - h, z );
	magicLensCam->SetScreenTopRight(    p[0] + w, p[1] + h, z );
}

void iAFast3DMagicLensWidget::resizeEvent( QResizeEvent * event )
{
	iAVtkWidget::resizeEvent( event );

	if (renderWindow()->GetRenderers()->GetNumberOfItems() <= 0)
	{
		return;
	}
						// TODO: VOLUME: find better way to get "main" renderer here!
	vtkCamera * mainCam = renderWindow()->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
	double w = (double)width() / height();	// calculate width aspect ratio
	double z = calculateZ( m_viewAngle );
	mainCam->SetScreenBottomLeft( -w, -1, z );
	mainCam->SetScreenBottomRight( w, -1, z );
	mainCam->SetScreenTopRight(    w,  1, z );
}

inline double iAFast3DMagicLensWidget::calculateZ( double viewAngle )
{
	return -1. / std::tan( viewAngle * vtkMath::Pi() / 180. );
}

void iAFast3DMagicLensWidget::mouseReleaseEvent( QMouseEvent * event )
{
	if (Qt::RightButton == event->button())
	{
		emit rightButtonReleasedSignal();
	}
	else if (Qt::LeftButton == event->button())
	{
		emit leftButtonReleasedSignal();
	}
	iAVtkWidget::mouseReleaseEvent( event );
}
