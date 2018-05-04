/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "dlg_function.h"
#include "open_iA_Core_export.h"

#include <QColor>

#include <vector>

class QPointF;

class open_iA_Core_API dlg_bezier : public dlg_function
{
	QColor color;

	unsigned int selectedPoint;
	bool   active;
	double controlDist;
	double length;
	double oppositeLength;
	std::vector<QPointF> viewPoints;
	std::vector<QPointF> realPoints;

public:
	dlg_bezier(iADiagramFctWidget *chart, QColor &color, bool reset = true);

	int getType() { return BEZIER; }

	// abstract functions
	void draw(QPainter &painter);
	void draw(QPainter &painter, QColor color, int lineWidth);
	void drawOnTop(QPainter&) {}

	int selectPoint(QMouseEvent *event, int *x = NULL);
	int getSelectedPoint() { return selectedPoint; }
	int addPoint(int x, int y);
	void addColorPoint(int, double, double, double) {}
	void removePoint(int index);
	void moveSelectedPoint(int x, int y);
	void changeColor(QMouseEvent *) {}

	bool isColored() { return false; }
	bool isEndPoint(int index);
	bool isDeletable(int index);

	void reset();

	void mousePressEvent(QMouseEvent*) {}
	void mouseMoveEvent(QMouseEvent*)  {}
	void mouseReleaseEvent(QMouseEvent *event);
	void mouseReleaseEventAfterNewPoint(QMouseEvent*) {}

	// additional public functions
	void push_back(double x, double y);
	std::vector<QPointF> &getPoints() { return realPoints; }

private:
	bool isFunctionPoint(int point);
	bool isControlPoint(int point);

	void insert(unsigned int index, unsigned int x, unsigned int y);

	void setViewPoint(int selectedPoint);
	void setOppositeViewPoint(int selectedPoint);

	int getFunctionPointIndex(int index);
	double getLength(QPointF start, QPointF end);

	// convert view to data
	double v2dX(int x);
	double v2dY(int y);

	// convert data to view
	int d2vX(double x);
	int d2vY(double y);

	// convert data to image
	int d2iX(double x);
	int d2iY(double y);
};
