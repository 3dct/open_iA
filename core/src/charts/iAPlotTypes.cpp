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
#include "iAPlotTypes.h"

#include  "iAConsole.h"
#include "iALookupTable.h"
#include "iAMapper.h"
#include "iAPlotData.h"

#include <QPainter>
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



iASelectedBinPlot::iASelectedBinPlot(QSharedPointer<iAPlotData> proxyData, int position /*= 0*/, QColor const & color /*= Qt::red */ )
: iAPlot(proxyData, color ), m_position( position )
{}

void iASelectedBinPlot::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	int x = xMapper.srcToDst(m_position) - ((m_data->GetRangeType() == Discrete) ? binWidth/2 : 0);
	int h = painter.device()->height();
	painter.setPen(getColor());
	painter.drawRect( QRect( x, 0, binWidth, h ) );
}

void iASelectedBinPlot::setPosition( int position )
{
	m_position = position;
}



namespace
{
	bool buildLinePolygon(QPolygon & poly, iAPlotData::DataType const * rawData, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper)
	{
		if (!rawData)
			return false;
		poly.push_back(QPoint(xMapper.srcToDst(startBin - (startBin > 0 ? 1 : 0)), 0));
		for (size_t curBin = startBin; curBin <= endBin; ++curBin)
		{
			int curX = xMapper.srcToDst(curBin);
			int curY = yMapper.srcToDst(rawData[curBin]);
			poly.push_back(QPoint(curX, curY));
		}
		poly.push_back(QPoint(xMapper.srcToDst(endBin), 0));
		return true;
	}
}

iALinePlot::iALinePlot(QSharedPointer<iAPlotData> data, QColor const & color):
	iAPlot(data, color)
{}

void iALinePlot::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	QPolygon poly;
	if (!buildLinePolygon(poly, m_data->GetRawData(), startBin, endBin, xMapper, yMapper))
		return;
	QPen pen(painter.pen());
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

void iAFilledLinePlot::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	QPolygon poly;
	if (!buildLinePolygon(poly, m_data->GetRawData(), startBin, endBin, xMapper, yMapper))
		return;
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

void iAStepFunctionPlot::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	QPainterPath tmpPath;
	iAPlotData::DataType const * rawData = m_data->GetRawData();
	if (!rawData)
		return;
	QPolygon poly;
	poly.push_back(QPoint(xMapper.srcToDst(startBin), 0));
	for (size_t curBin = startBin; curBin <= endBin; ++curBin)
	{
		int curX1 = xMapper.srcToDst(curBin - ((m_data->GetRangeType() == Discrete) ? 0.5 : 0));
		int curX2 = xMapper.srcToDst(curBin + ((m_data->GetRangeType() == Discrete) ? 0.5 : 1));
		int curY = yMapper.srcToDst(rawData[curBin]);
		poly.push_back(QPoint(curX1, curY));
		poly.push_back(QPoint(curX2, curY));
	}
	poly.push_back(QPoint(xMapper.srcToDst(endBin), 0));
	tmpPath.addPolygon(poly);
	painter.fillPath(tmpPath, QBrush(getFillColor()));
}



iABarGraphPlot::iABarGraphPlot(QSharedPointer<iAPlotData> data, QColor const & color, int margin):
	iAPlot(data, color),
	m_margin(margin)
{}

void iABarGraphPlot::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	iAPlotData::DataType const * rawData = m_data->GetRawData();
	if (!rawData)
		return;
	int barWidth = static_cast<int>(std::ceil(binWidth)) - m_margin;
	QColor fillColor = getColor();
	for (size_t curBin = startBin; curBin <= endBin; ++curBin)
	{
		int x = xMapper.srcToDst(curBin) - ((m_data->GetRangeType() == Discrete) ? barWidth/2 : 0);
		int h = yMapper.srcToDst(rawData[curBin]);
		if (m_lut)
		{
			double rgba[4];
			m_lut->getColor(curBin, rgba);
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

void iAPlotCollection::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, iAMapper const & xMapper, iAMapper const & yMapper) const
{
	qreal oldPenWidth = painter.pen().widthF();
	QPen pen = painter.pen();
	pen.setWidthF(3.0f);
	painter.setPen(pen);
	for(auto drawer: m_drawers)
	{
		drawer->draw(painter, binWidth, startBin, endBin, xMapper, yMapper);
	}
	pen.setWidthF(oldPenWidth);
	painter.setPen(pen);
}

void iAPlotCollection::add(QSharedPointer<iAPlot> drawer)
{
	if (m_drawers.size() > 0)
	{
		if (m_drawers[0]->data()->XBounds()[0] != drawer->data()->XBounds()[0] ||
			m_drawers[0]->data()->XBounds()[1] != drawer->data()->XBounds()[1] ||
			m_drawers[0]->data()->GetNumBin() != drawer->data()->GetNumBin() ||
			m_drawers[0]->data()->GetRangeType() != drawer->data()->GetRangeType())
		{
			DEBUG_LOG("iAPlotCollection::add - ERROR - Incompatible drawer added!");
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
		return m_drawers[0]->data();
	else
	{
		DEBUG_LOG("iAPlotCollection::data() called before any plots were added!");
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
