/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iAChartFunction.h"

#include "charts_export.h"

#include <QColor>

#include <vector>

class QPointF;

//! Class representing a bezier curve in an iAChartWithFunctionsWidget.
//! Draws itself, and allows adding, removing and modifying points (and their directions).
class charts_API iAChartFunctionBezier : public iAChartFunction
{
public:
	iAChartFunctionBezier(iAChartWithFunctionsWidget *chart, QColor &color, bool reset = true);

	void draw(QPainter &painter) override;
	void draw(QPainter &painter, QColor penColor, int lineWidth) override;
	void drawOnTop(QPainter&) override {}
	int selectPoint(int mouseX, int mouseY) override;
	int getSelectedPoint() const override { return m_selectedPoint; }
	int addPoint(int mouseX, int mouseY) override;
	void addColorPoint(int, double, double, double) override {}
	void removePoint(int index) override;
	void moveSelectedPoint(int mouseX, int mouseY) override;
	bool isEndPoint(int index) const override;
	bool isDeletable(int index) const override;
	void reset() override;
	virtual QString name() const override;
	size_t numPoints() const override;
	void mouseReleaseEvent(QMouseEvent *event) override;

	void push_back(double x, double y);
	std::vector<QPointF> &getPoints() { return m_realPoints; }
private:
	bool isFunctionPoint(int point);
	bool isControlPoint(int point);

	void insert(unsigned int index, unsigned int x, unsigned int y);

	void setViewPoint(int selPntIdx);
	void setOppositeViewPoint(int selPntIdx);

	int getFunctionPointIndex(int index);
	double getLength(QPointF start, QPointF end);

	QColor m_color;
	int m_selectedPoint;
	double m_controlDist;
	std::vector<QPointF> m_viewPoints;
	std::vector<QPointF> m_realPoints;
};
