/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAPlotTypes.h"

#include  "iALog.h"
#include "iALookupTable.h"
#include "iAMapper.h"
#include "iAPlotData.h"

#include <QPainter>
#include <QPainterPath>
#include <QPolygon>

#include <cmath>

iAPlot::iAPlot(QSharedPointer<iAPlotData> data, QColor const & color):
	iAColorable(color),
	m_data(data),
	m_visible(true)
{}

iAPlot::~iAPlot() {}

QSharedPointer<iAPlotData> iAPlot::data()
{
	return m_data;
}

bool iAPlot::visible() const
{
	return m_visible;
}

void iAPlot::setVisible(bool visible)
{
	m_visible = visible;
}



iASelectedBinPlot::iASelectedBinPlot(QSharedPointer<iAPlotData> proxyData, size_t idx /*= 0*/, QColor const & color /*= Qt::red */ ) :
	iAPlot(proxyData, color), m_idx(idx)
{}

void iASelectedBinPlot::draw(QPainter& painter, size_t startIdx, size_t endIdx, iAMapper const & xMapper, iAMapper const & /*yMapper*/) const
{
	if (m_idx < startIdx || m_idx > endIdx)
	{
		return;
	}
	int x = xMapper.srcToDst(m_data->xValue(m_idx));
	int barWidth = xMapper.srcToDst(m_data->xValue(m_idx + 1)) - x;
	int h = painter.device()->height();
	painter.setPen(getColor());
	painter.drawRect(QRect(x, 0, barWidth, h));
}

void iASelectedBinPlot::setSelectedBin(size_t idx)
{
	m_idx = idx;
}



namespace
{
	void buildLinePolygon(QPolygon& poly, QSharedPointer<iAPlotData> data, size_t startIdx, size_t endIdx,
		iAMapper const& xMapper, iAMapper const& yMapper)
	{
		for (size_t idx = startIdx; idx <= endIdx; ++idx)
		{
			int curX = xMapper.srcToDst(data->xValue(idx));
			int curY = yMapper.srcToDst(data->yValue(idx));
			poly.push_back(QPoint(curX, curY));
		}
	}
}

iALinePlot::iALinePlot(QSharedPointer<iAPlotData> data, QColor const & color) :
	iAPlot(data, color),
	m_lineWidth(1)
{}


void iALinePlot::setLineWidth(int width)
{
	m_lineWidth = width;
}

void iALinePlot::draw(QPainter& painter, size_t startIdx, size_t endIdx, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	if (!m_data)
	{
		return;
	}
	QPolygon poly;
	buildLinePolygon(poly, m_data, startIdx, endIdx, xMapper, yMapper);
	QPen pen(painter.pen());
	pen.setWidth(m_lineWidth);
	pen.setColor(getColor());
	painter.setPen(pen);
	painter.drawPolyline(poly);
}



iAFilledLinePlot::iAFilledLinePlot(QSharedPointer<iAPlotData> data, QColor const & color):
	iAPlot(data, color)
{}

QColor iAFilledLinePlot::getFillColor() const
{
	QColor fillColor = getColor();
	return fillColor;
}

void iAFilledLinePlot::draw(QPainter& painter, size_t startIdx, size_t endIdx, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	if (!m_data)
	{
		return;
	}
	QPolygon poly;
	int pt1x = xMapper.srcToDst(m_data->xValue(startIdx - (startIdx > 0 ? 1 : 0)));
	int pt2x = xMapper.srcToDst(m_data->xValue(endIdx + (endIdx >= m_data->valueCount() ? 1 : 0)));
	poly.push_back(QPoint(pt1x, 0));
	buildLinePolygon(poly, m_data, startIdx, endIdx, xMapper, yMapper);
	poly.push_back(QPoint(pt2x, 0));
	QPainterPath tmpPath;
	tmpPath.addPolygon(poly);
	painter.fillPath(tmpPath, QBrush(getFillColor()));
}



iAStepFunctionPlot::iAStepFunctionPlot(QSharedPointer<iAPlotData> data, QColor const & color) :
	iAPlot(data, color)
{}

QColor iAStepFunctionPlot::getFillColor() const
{
	QColor fillColor = getColor();
	fillColor.setAlpha(getColor().alpha() / 3);
	return fillColor;
}

void iAStepFunctionPlot::draw(QPainter& painter, size_t startIdx, size_t endIdx, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	QPainterPath tmpPath;
	QPolygon poly;
	poly.push_back(QPoint(xMapper.srcToDst(m_data->xValue(startIdx)), 0));
	for (size_t idx = startIdx; idx <= endIdx; ++idx)
	{
		int curX1 = xMapper.srcToDst(m_data->xValue(idx));
		int curX2 = xMapper.srcToDst(m_data->xValue(idx + 1));
		int curY = yMapper.srcToDst(m_data->yValue(idx));
		poly.push_back(QPoint(curX1, curY));
		poly.push_back(QPoint(curX2, curY));
	}
	poly.push_back(QPoint(xMapper.srcToDst(m_data->xValue(endIdx + 1)), 0));
	tmpPath.addPolygon(poly);
	painter.fillPath(tmpPath, QBrush(getFillColor()));
}



iABarGraphPlot::iABarGraphPlot(QSharedPointer<iAPlotData> data, QColor const & color, int margin):
	iAPlot(data, color),
	m_margin(margin)
{}

void iABarGraphPlot::draw(QPainter& painter, size_t startIdx, size_t endIdx, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	QColor fillColor = getColor();
	for (size_t idx = startIdx; idx <= endIdx; ++idx)
	{
		int x = xMapper.srcToDst(m_data->xValue(idx));
		int barWidth = xMapper.srcToDst(m_data->xValue(idx + 1)) - x - m_margin;
		int h = yMapper.srcToDst(m_data->yValue(idx));
		if (m_lut)
		{
			double rgba[4];
			m_lut->getColor(idx, rgba);
			fillColor = QColor(rgba[0]*255, rgba[1]*255, rgba[2]*255, rgba[3]*255);
		}
		painter.fillRect(QRect(x, 1, barWidth, h), fillColor);
	}
}

void iABarGraphPlot::setLookupTable(QSharedPointer<iALookupTable> lut)
{
	m_lut = lut;
}



iAPlotCollection::iAPlotCollection():
	iAPlot(QSharedPointer<iAPlotData>(), QColor())
{}

void iAPlotCollection::draw(QPainter& painter, size_t startIdx, size_t endIdx, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	qreal oldPenWidth = painter.pen().widthF();
	QPen pen = painter.pen();
	pen.setWidthF(3.0f);
	painter.setPen(pen);
	for(auto drawer: m_drawers)
	{
		drawer->draw(painter, startIdx, endIdx, xMapper, yMapper);
	}
	pen.setWidthF(oldPenWidth);
	painter.setPen(pen);
}

void iAPlotCollection::add(QSharedPointer<iAPlot> drawer)
{
	if (m_drawers.size() > 0)
	{
		if (m_drawers[0]->data()->xBounds()[0] != drawer->data()->xBounds()[0] ||
			m_drawers[0]->data()->xBounds()[1] != drawer->data()->xBounds()[1] ||
			m_drawers[0]->data()->valueCount() != drawer->data()->valueCount() ||
			m_drawers[0]->data()->valueType() != drawer->data()->valueType())
		{
			LOG(lvlError, "iAPlotCollection::add - ERROR - Incompatible drawer added!");
		}
	}
	m_drawers.push_back(drawer);
}

void iAPlotCollection::clear()
{
	m_drawers.clear();
}

QSharedPointer<iAPlotData> iAPlotCollection::data()
{
	if (m_drawers.size() > 0)
	{
		return m_drawers[0]->data();
	}
	else
	{
		LOG(lvlWarn, "iAPlotCollection::data() called before any plots were added!");
		return QSharedPointer<iAPlotData>();
	}
}

void iAPlotCollection::setColor(QColor const & color)
{
	iAColorable::setColor(color);
	for (auto drawer : m_drawers)
	{
		drawer->setColor(color);
	}
}
