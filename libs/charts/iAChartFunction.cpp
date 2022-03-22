/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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