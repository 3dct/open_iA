// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPlotTypes.h"

#include  "iALog.h"
#include "iALookupTable.h"
#include "iAMapper.h"
#include "iAPlotData.h"

#include <QPainter>
#include <QPainterPath>
#include <QPolygon>

// iAPlot

iAPlot::iAPlot(std::shared_ptr<iAPlotData> data, QColor const & color):
	m_data(data),
	m_visible(true),
	m_color(color)
{}

iAPlot::~iAPlot() {}

void iAPlot::drawLegendItem(QPainter& painter, QRect const& rect)
{
	painter.setPen(visible()? m_color: QColor(m_color.red(), m_color.green(), m_color.blue(), 64));
	int y = rect.center().y();
	painter.drawLine(rect.left(), y, rect.right(), y);
}

std::shared_ptr<iAPlotData> iAPlot::data()
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

void iAPlot::setColor(QColor const& color)
{
	m_color = color;
}

QColor const iAPlot::color() const
{
	return m_color;
}

// iASelectedBinPlot

iASelectedBinPlot::iASelectedBinPlot(std::shared_ptr<iAPlotData> proxyData, size_t idx /*= 0*/, QColor const & color /*= Qt::red */ ) :
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
	painter.setPen(color());
	painter.drawRect(QRect(x, 0, barWidth, h));
}

void iASelectedBinPlot::setSelectedBin(size_t idx)
{
	m_idx = idx;
}



namespace
{
	void buildLinePolygon(QPolygon& poly, std::shared_ptr<iAPlotData> data, size_t startIdx, size_t endIdx,
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

// iALinePlot

iALinePlot::iALinePlot(std::shared_ptr<iAPlotData> data, QColor const& color) :
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
	pen.setColor(color());
	painter.setPen(pen);
	painter.drawPolyline(poly);
}

// iAFilledLinePlot

iAFilledLinePlot::iAFilledLinePlot(std::shared_ptr<iAPlotData> data, QColor const& color) :
	iAPlot(data, color)
{}

QColor iAFilledLinePlot::getFillColor() const
{
	return color();
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

namespace
{
	void drawBoxLegendItem(QPainter& painter, QRect const& rect, QColor const & col, bool visible)
	{
		if (visible)
		{
			painter.fillRect(rect, QBrush(col));
		}
		else
		{
			painter.save();
			painter.setPen(QPen(col, 2));
			painter.drawRect(rect);
			painter.restore();
		}
	}
}

void iAFilledLinePlot::drawLegendItem(QPainter& painter, QRect const& rect)
{
	drawBoxLegendItem(painter, rect, getFillColor(), visible());
}

// iAStepFunctionPlot

iAStepFunctionPlot::iAStepFunctionPlot(std::shared_ptr<iAPlotData> data, QColor const& color) :
	iAPlot(data, color)
{}

QColor iAStepFunctionPlot::getFillColor() const
{
	QColor fillColor = color();
	fillColor.setAlpha(color().alpha() / 3);
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

void iAStepFunctionPlot::drawLegendItem(QPainter& painter, QRect const& rect)
{
	drawBoxLegendItem(painter, rect, getFillColor(), visible());
}

// iABarGraphPlot

iABarGraphPlot::iABarGraphPlot(std::shared_ptr<iAPlotData> data, QColor const& color, int margin) :
	iAPlot(data, color),
	m_margin(margin)
{}

void iABarGraphPlot::draw(QPainter& painter, size_t startIdx, size_t endIdx, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	QColor fillColor = color();
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

void iABarGraphPlot::drawLegendItem(QPainter& painter, QRect const& rect)
{	// TODO: figure out what todo do if m_lut is set...
	drawBoxLegendItem(painter, rect, color(), visible());
}

void iABarGraphPlot::setLookupTable(std::shared_ptr<iALookupTable> lut)
{
	m_lut = lut;
}

// iAPlotCollection

iAPlotCollection::iAPlotCollection() :
	iAPlot(std::shared_ptr<iAPlotData>(), QColor())
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

void iAPlotCollection::add(std::shared_ptr<iAPlot> drawer)
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

std::shared_ptr<iAPlotData> iAPlotCollection::data()
{
	if (m_drawers.size() > 0)
	{
		return m_drawers[0]->data();
	}
	else
	{
		LOG(lvlWarn, "iAPlotCollection::data() called before any plots were added!");
		return std::shared_ptr<iAPlotData>();
	}
}

void iAPlotCollection::setColor(QColor const & color)
{
	iAPlot::setColor(color);
	for (auto drawer : m_drawers)
	{
		drawer->setColor(color);
	}
}
