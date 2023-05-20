// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iacharts_export.h"

#include <QObject>

class QColor;
class QMouseEvent;
class QPainter;
class iAChartWithFunctionsWidget;

//! Abstract base class for representing some kind of function in an an iAChartWithFunctionsWidget.
class iAcharts_API iAChartFunction: public QObject
{
	Q_OBJECT
public:
	static const int PointRadius = 6;
	static const int PointRadiusSelected = 10;
	static const int LineWidthUnselected = 1;
	static const int LineWidthSelected   = 2;
	static const QColor DefaultColor;
	//! size of a point in pixels
	static int pointRadius(bool selected);

	iAChartFunction(iAChartWithFunctionsWidget* chart);

	virtual void draw(QPainter &painter) = 0;
	virtual void draw(QPainter &painter, QColor color, int lineWidth) = 0;
	virtual void drawOnTop(QPainter &painter) = 0;

	virtual int selectPoint(int mouseX, int mouseY) = 0;
	virtual int getSelectedPoint() const = 0;
	virtual int addPoint(int mouseX, int mouseY) = 0;
	virtual void addColorPoint(int x, double red = -1.0, double green = -1.0, double blue = -1.0) = 0;
	virtual void removePoint(int index) = 0;
	virtual void moveSelectedPoint(int mouseX, int mouseY) = 0;
	virtual void changeColor();
	virtual bool isColored() const;
	virtual bool isEndPoint(int index) const = 0;
	virtual bool isDeletable(int index) const = 0;
	virtual size_t numPoints() const = 0;

	virtual void reset() = 0;
	virtual void mouseReleaseEvent(QMouseEvent*);
	virtual void mouseReleaseEventAfterNewPoint(QMouseEvent*);

	virtual QString name() const = 0;

	iAChartWithFunctionsWidget* m_chart;
};

void drawPoint(QPainter& painter, int x, int y, bool selected, QColor const & color = iAChartFunction::DefaultColor, double radiusFactor=1);
