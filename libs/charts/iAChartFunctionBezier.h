// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAChartFunction.h"

#include "iacharts_export.h"

#include <QColor>

#include <vector>

class QPointF;

//! Class representing a bezier curve in an iAChartWithFunctionsWidget.
//! Draws itself, and allows adding, removing and modifying points (and their directions).
class iAcharts_API iAChartFunctionBezier : public iAChartFunction
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
	QString name() const override;
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
