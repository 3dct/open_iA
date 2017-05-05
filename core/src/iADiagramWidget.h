/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#define WIN32_LEAN_AND_MEAN		// apparently QGLWidget might include windows.h...
#include <QGLWidget>
#include <QImage>

class open_iA_Core_API iADiagramWidget : public QGLWidget
{
	Q_OBJECT
public:
	static const int BOTTOM_MARGIN;
	enum Mode { NO_MODE, MOVE_POINT_MODE, MOVE_VIEW_MODE, X_ZOOM_MODE, Y_ZOOM_MODE };
	iADiagramWidget(QWidget* parent);
	virtual ~iADiagramWidget();

	double getZoom()         const { return xZoom;         }
	double getYZoom()        const { return yZoom;         }
	int    getTranslationX()  const { return translationX; }
	int    getTranslationY()  const { return translationY; }

	virtual int    getBottomMargin() const { return BOTTOM_MARGIN; }
	virtual int    getLeftMargin()   const { return leftMargin;    }

	int    getActiveWidth()  const;
	int    getActiveHeight() const;
	int getHeight() const;

	virtual void redraw() =0;

public slots:
	void resetView();
signals:
	void XAxisChanged();
protected:
	// TODO: Make private!
	static const double X_ZOOM_STEP;
	static const double Y_ZOOM_STEP;
	static const int LEFT_MARGIN;
	virtual void resizeEvent (QResizeEvent *event);

	int zoomX;
	int zoomY;
	double yZoom;
	double yZoomStart;
	double xZoom;
	double xZoomStart;

	int translationX;
	int translationY;
	int translationStartX;
	int translationStartY;
	int dragStartPosX;
	int dragStartPosY;

	int leftMargin;
	QImage		image;
	
	bool draw; //< TODO: check what exactly this is supposed to solve. seems to cause only trouble
	int width, height;
	int mode;

	void zoomAlongY(double value, bool deltaMode);
	void zoomAlongX(double value, int x, bool deltaMode);

	virtual double getMaxXZoom() const;
	virtual void drawBackground(QPainter &painter);
	virtual QColor getBGGradientColor(int idx);

	virtual void setNewSize();

	virtual void mousePressEvent(QMouseEvent *event);
	virtual void changeMode(int mode, QMouseEvent *event);
	virtual void selectBin(QMouseEvent *event) =0;
	virtual void mouseMoveEvent(QMouseEvent *event);
private:
	virtual void wheelEvent(QWheelEvent *event);
	virtual void leaveEvent(QEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
};
