// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAChartFunction.h"

#include "iAChartWithFunctionsWidget.h"

#include <QPainter>

iAChartFunction::iAChartFunction(iAChartWithFunctionsWidget* chart) : m_chart(chart) { }

void iAChartFunction::changeColor()
{}

bool iAChartFunction::isColored() const
{
	return false;
}

void iAChartFunction::mouseReleaseEvent(QMouseEvent*)
{}

void iAChartFunction::mouseReleaseEventAfterNewPoint(QMouseEvent*)
{}

int iAChartFunction::pointRadius(bool selected)
{
	return selected ? PointRadiusSelected : PointRadius;
}

const QColor iAChartFunction::DefaultColor(255, 128, 0, 255);

void drawPoint(QPainter& painter, int x, int y, bool selected, QColor const& color, double sizeFactor)
{
	QPen pointPen = painter.pen();
	pointPen.setColor(iAChartFunction::DefaultColor); pointPen.setWidth(iAChartFunction::LineWidthUnselected);
	QPen pointPenSel = painter.pen();
	pointPenSel.setColor(Qt::red); pointPenSel.setWidth(iAChartFunction::LineWidthSelected);
	painter.setPen(selected ? pointPenSel : pointPen);
	painter.setBrush(QBrush(color));
	int radius = iAChartFunction::pointRadius(selected) * sizeFactor;
	painter.drawEllipse(x - radius, y - radius, 2*radius, 2*radius);
}
