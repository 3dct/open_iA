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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iADiagramWidget.h"

#include "iAMathUtility.h"

#include <QPainter>
#include <QWheelEvent>

#include <cmath>

const double iADiagramWidget::X_ZOOM_STEP = 1.5;
const double iADiagramWidget::Y_ZOOM_STEP = 1.5;
const int    iADiagramWidget::BOTTOM_MARGIN = 40;
const int    iADiagramWidget::LEFT_MARGIN   = 60;
namespace
{
	const int MAX_X_ZOOM = 1000;
}

iADiagramWidget::iADiagramWidget(QWidget* parent):
	QGLWidget(parent),
	yZoom(1.0),
	yZoomStart(1.0),
	xZoom(1.0),
	xZoomStart(1.0),
	translationX(0),
	translationY(0),
	translationStartX(0),
	translationStartY( 0 ),
	leftMargin(LEFT_MARGIN),
	draw(true),
	mode(NO_MODE)
{
	setNewSize();
	image = QImage(width, height, QImage::Format_ARGB32);
	setAutoFillBackground(false);
}

iADiagramWidget::~iADiagramWidget()
{
}

void iADiagramWidget::wheelEvent(QWheelEvent *event)
{
	if ((event->modifiers() & Qt::AltModifier) == Qt::AltModifier ||
		(event->modifiers() & Qt::AltModifier) == Qt::Key_AltGr)
	{
		zoomAlongY(event->delta(), true);
	}
	else
	{
		zoomAlongX(event->delta(), event->x(), true);
	}
	redraw();
}

void iADiagramWidget::resizeEvent(QResizeEvent *event)
{
	if ( (this->geometry().width() != width) || (this->geometry().height() != height) )
	{
		setNewSize();
		//create a QImage newImage with the new window size and load to the QImage image
		QImage newImage(width, height, QImage::Format_ARGB32);
		image = newImage;
		draw = true;
		repaint();
	}
	QWidget::resizeEvent(event);
}

void iADiagramWidget::leaveEvent(QEvent*)
{
	this->clearFocus();
}

void iADiagramWidget::setNewSize()
{
	width = this->geometry().width();
	height = this->geometry().height();
}

void iADiagramWidget::zoomAlongY(double value, bool deltaMode)
{
	if (deltaMode)
	{
		if (value /* = delta */ > 0)
		{
			yZoom *= Y_ZOOM_STEP;
		}
		else
		{
			yZoom /= Y_ZOOM_STEP;
		}
	}
	else
	{
		yZoom = value;
	}
	if (yZoom < 1.0)
		yZoom = 1.0;
}

void iADiagramWidget::zoomAlongX(double value, int x, bool deltaMode)
{
	int xZoomBefore = xZoom;
	int translationXBefore = translationX;
	// maximum zoom level is one bin per axis step, but never less than 1
	const double maxXZoom = getMaxXZoom();
	// don't do anything if we're already at the limit
	if ( (deltaMode &&  (value < 0    && xZoom == 1.0) || (value > 0         && xZoom == maxXZoom)) ||
		 (!deltaMode && (value <= 1.0 && xZoom == 1.0) || (value >= maxXZoom && xZoom == maxXZoom)) )
	{
		return;
	}
	int absoluteX = x-translationX-getLeftMargin();
	double absoluteXRatio = (double)absoluteX/((getActiveWidth()-1)*xZoom);
	if (deltaMode)
	{
		if (value /* = delta */ > 0)
		{
			xZoom *= X_ZOOM_STEP;
		}
		else
		{
			xZoom /= X_ZOOM_STEP;
		}
	}
	else
	{
		xZoom = value;
	}
	if (xZoom < 1.0)
		xZoom = 1.0;
	if (xZoom > maxXZoom)
		xZoom = maxXZoom;

	int absXAfterZoom = (int)(getActiveWidth()*xZoom*absoluteXRatio);

	translationX = clamp(-static_cast<int>(getActiveWidth() * (xZoom-1)), 0,
		-absXAfterZoom +x -getLeftMargin());

	if (xZoomBefore != xZoom || translationXBefore != translationX)
	{
		emit XAxisChanged();
	}
}

double iADiagramWidget::getMaxXZoom() const
{
	return MAX_X_ZOOM;
}

int iADiagramWidget::getActiveWidth() const
{
	return width - getLeftMargin();
}

int iADiagramWidget::getActiveHeight() const
{
	return height - getBottomMargin();
}

QColor iADiagramWidget::getBGGradientColor(int idx)
{
	switch (idx)
	{
		case 0: return QColor(140,140,140,255);
		case 1: return QColor(255,255,255,255);
	}
	return QColor(0, 0, 0);
}

void iADiagramWidget::drawBackground(QPainter &painter)
{
	painter.fillRect( rect(), Qt::white);
}

void iADiagramWidget::resetView()
{
	translationX = 0;
	xZoom = 1.0;
	yZoom = 1.0;
	emit XAxisChanged();
	redraw();
}

void iADiagramWidget::changeMode(int mode, QMouseEvent *event)
{
	switch(mode)
	{
	case MOVE_POINT_MODE:
		break;
	case MOVE_VIEW_MODE:
		dragStartPosX = event->x();
		dragStartPosY = event->y();
		this->mode = MOVE_VIEW_MODE;
		break;
	default:
		this->mode = mode;
		break;
	}
}

void iADiagramWidget::mouseReleaseEvent(QMouseEvent *event)  
{
	switch(event->button())
	{
	case Qt::LeftButton:
		redraw();
		break;
	default:
		break;
	}
	this->mode = NO_MODE;
}

void iADiagramWidget::mousePressEvent(QMouseEvent *event)  
{
	switch(event->button())
	{
	case Qt::LeftButton:
		if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
			((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) &&
			((event->modifiers() & Qt::AltModifier) == Qt::AltModifier))
		{
			zoomY = event->y();
			yZoomStart = yZoom;
			changeMode(Y_ZOOM_MODE, event);
		}
		else if (((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier) &&
			((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier))
		{
			zoomX = event->x();
			zoomY = event->y();
			xZoomStart = xZoom;
			changeMode(X_ZOOM_MODE, event);
		}
		else if ((event->modifiers() & Qt::ShiftModifier) == Qt::ShiftModifier)
		{
			translationStartX = translationX;
			translationStartY = translationY;
			changeMode(MOVE_VIEW_MODE, event);
		}
		break;
	case Qt::RightButton:
		{
			redraw();
		}
		break;
	case Qt::MidButton:
		translationStartX = translationX;
		translationStartY = translationY;
		changeMode(MOVE_VIEW_MODE, event);
		break;
	default:
		break;
	}
}

void iADiagramWidget::mouseMoveEvent(QMouseEvent *event)
{
	switch(this->mode)
	{
	case NO_MODE: /* do nothing */ break;
	case MOVE_POINT_MODE:
		{
			redraw();
		}
		break;
	case MOVE_VIEW_MODE:
		translationX = clamp(-static_cast<int>(getActiveWidth() * (xZoom-1)), 0,
			translationStartX + event->x() - dragStartPosX);
		emit XAxisChanged();
		translationY = translationStartY + event->y() - dragStartPosY;

		if ( translationY >= height * yZoom - height )
			translationY = height * yZoom - height;
		else if ( translationY < -( height * yZoom - height ) )
			translationY = -( height * yZoom - height );

		redraw();
		break;
	case X_ZOOM_MODE:
		zoomAlongX(((zoomY-event->y())/2.0)+xZoomStart, zoomX, false);
		redraw();
		break;
	case Y_ZOOM_MODE:
		{
			int diff = (zoomY-event->y())/2.0;
			if (diff < 0)
				zoomAlongY(-pow(Y_ZOOM_STEP,-diff)+yZoomStart, false);
			else
				zoomAlongY(pow(Y_ZOOM_STEP,diff)+yZoomStart, false);
			redraw();
		}
		break;
	}
	selectBin(event);
}
