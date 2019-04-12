/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iAChartFunction.h"
#include "open_iA_Core_export.h"

#include <QColor>

#include <vector>

class QPointF;

class open_iA_Core_API iAChartFunctionBezier : public iAChartFunction
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
	iAChartFunctionBezier(iADiagramFctWidget *chart, QColor &color, bool reset = true);

	int getType() override { return BEZIER; }
	void draw(QPainter &painter) override;
	void draw(QPainter &painter, QColor color, int lineWidth) override;
	void drawOnTop(QPainter&) override {}
	int selectPoint(QMouseEvent *event, int *x = NULL) override;
	int getSelectedPoint() override { return selectedPoint; }
	int addPoint(int x, int y) override;
	void addColorPoint(int, double, double, double) override {}
	void removePoint(int index) override;
	void moveSelectedPoint(int x, int y) override;
	void changeColor(QMouseEvent *) override{}
	bool isColored() override { return false; }
	bool isEndPoint(int index) override;
	bool isDeletable(int index) override;
	void reset() override;
	void mouseReleaseEvent(QMouseEvent *event) override;

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
