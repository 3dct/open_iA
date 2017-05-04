/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenb�ck, Artem & Alexander Amirkhanov, B. Fr�hler   *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <QObject>

class QColor;
class QMouseEvent;
class QPainter;
class iADiagramFctWidget;

class dlg_function: public QObject
{
	Q_OBJECT
public:
	static const int TRANSFER = 0;
	static const int GAUSSIAN = 1;
	static const int BEZIER   = 2;

	dlg_function(iADiagramFctWidget* fctDiagram) : fctDiagram(fctDiagram) { }

	virtual int getType() = 0;

	virtual void draw(QPainter &painter) = 0;
	virtual void draw(QPainter &painter, QColor color, int lineWidth) = 0;
	virtual void drawOnTop(QPainter &painter) = 0;

	virtual int selectPoint(QMouseEvent *event, int *x = 0) = 0;
	virtual int getSelectedPoint() = 0;
	virtual int addPoint(int x, int y) = 0;
	virtual void addColorPoint(int x, double red = -1.0, double green = -1.0, double blue = -1.0) = 0;
	virtual void removePoint(int index) = 0;
	virtual void moveSelectedPoint(int x, int y) = 0;
	virtual void changeColor(QMouseEvent *event) = 0;
	
	virtual bool isColored() = 0;
	virtual bool isEndPoint(int index) = 0;
	virtual bool isDeletable(int index) = 0;

	virtual void reset() = 0;
	
	virtual void mousePressEvent(QMouseEvent *event) = 0;
	virtual void mouseMoveEvent(QMouseEvent *event) = 0;
	virtual void mouseReleaseEvent(QMouseEvent *event) = 0;
	virtual void mouseReleaseEventAfterNewPoint(QMouseEvent *event) = 0;
	
	iADiagramFctWidget *fctDiagram;
};
