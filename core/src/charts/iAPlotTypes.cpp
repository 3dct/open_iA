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

#include "iALookupTable.h"
#include "iAMapper.h"
#include "iAPlotData.h"

#include <QPainter>
#include <QPolygon>

#include <cmath>

class iADummyPlotData : public iAPlotData
{
public:
	DataType const * GetRawData() const override
	{
		return nullptr;
	}
	size_t GetNumBin() const override
	{
		return 0;
	}
	double GetSpacing() const override
	{
		return 1;
	}
	double const * XBounds() const override
	{
		static double xbounds[2];
		xbounds[0] = 0; xbounds[1] = 0;
		return xbounds;
	}
	DataType const * YBounds() const override
	{
		static double ybounds[2]; ybounds[0] = 0; ybounds[1] = 0;
		return ybounds;
	}
};

iAPlot::iAPlot(QColor const & color):
	iAColorable(color),
	m_visible(true)
{}

iAPlot::~iAPlot() {}

QSharedPointer<iAPlotData> iAPlot::data()
{
	return QSharedPointer<iAPlotData>(new iADummyPlotData);
}

bool iAPlot::visible() const
{
	return m_visible;
}

void iAPlot::setVisible(bool visible)
{
	m_visible = visible;
}

void iAPlot::update() {}


iASelectedBinDrawer::iASelectedBinDrawer( int position /*= 0*/, QColor const & color /*= Qt::red */ )
: iAPlot( color ), m_position( position )
{}

void iASelectedBinDrawer::draw( QPainter& painter, double binWidth, size_t startBin, size_t endBin, QSharedPointer<iAMapper> yMapper) const
{
	int x = (int)(m_position * binWidth);
	int h = painter.device()->height();
	int intBinWidth = static_cast<int>(std::ceil(binWidth));
	painter.setPen(getColor());
	painter.drawRect( QRect( x, 0, intBinWidth, h ) );
}

void iASelectedBinDrawer::setPosition( int position )
{
	m_position = position;
}



iAPolygonBasedFunctionDrawer::iAPolygonBasedFunctionDrawer(QSharedPointer<iAPlotData> data, QColor const & color):
	iAPlot(color),
	m_data(data),
	m_cachedBinWidth(0.0),
	m_cachedYMapper(0),
	m_cachedEndBin(0),
	m_cachedStartBin(0)
{}

void iAPolygonBasedFunctionDrawer::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, QSharedPointer<iAMapper> yMapper) const
{
	if (!m_poly || m_cachedBinWidth != binWidth || !m_cachedYMapper || !(*m_cachedYMapper.data() == *yMapper.data())
		|| startBin != m_cachedStartBin || endBin != m_cachedEndBin )
	{
		m_cachedBinWidth = binWidth;
		m_cachedYMapper = yMapper->clone();
		m_cachedStartBin = startBin;
		m_cachedEndBin = endBin;
		if (!computePolygons(binWidth, startBin, endBin, yMapper))
		{
			return;
		}
	}
	drawPoly(painter, m_poly);
}

void iAPolygonBasedFunctionDrawer::update()
{
	// reset the polygon; next time we draw, it will be recreated!
	m_poly.clear();
}

QSharedPointer<iAPlotData> iAPolygonBasedFunctionDrawer::data()
{
	return m_data;
}



iALineFunctionDrawer::iALineFunctionDrawer(QSharedPointer<iAPlotData> data, QColor const & color):
	iAPolygonBasedFunctionDrawer(data, color)
{
}

void iALineFunctionDrawer::drawPoly(QPainter& painter, QSharedPointer<QPolygon> poly) const
{
	QPen pen(painter.pen());
	pen.setColor(getColor());
	painter.setPen(pen);
	painter.drawPolyline(*poly.data());
}

bool iALineFunctionDrawer::computePolygons(double binWidth, size_t startBin, size_t endBin, QSharedPointer<iAMapper> yMapper) const
{
	iAPlotData::DataType const * rawData = m_data->GetRawData();
	if (!rawData)
		return false;
	int binWidthHalf = binWidth / 2;
	m_poly = QSharedPointer<QPolygon>(new QPolygon);
	// one extra bin in each direction to see lines leading in direction of the previous/next data values
	startBin = (startBin > 0 ? startBin - 1 : startBin);
	endBin = endBin < m_data->GetNumBin() ? endBin + 1 : endBin;
	m_poly->push_back(QPoint((startBin - 1)*binWidth, 0));
	for (int j = startBin; j < endBin; j++)
	{
		int curX = (int)(j * binWidth)+ binWidthHalf;
		int curY = yMapper->srcToDst(rawData[j]);
		m_poly->push_back(QPoint(curX, curY));
	}
	m_poly->push_back(QPoint(endBin * binWidth, 0 ));
	return true;
}


iAFilledLineFunctionDrawer::iAFilledLineFunctionDrawer(QSharedPointer<iAPlotData> data, QColor const & color):
	iAPolygonBasedFunctionDrawer(data, color)
{
}

QColor iAFilledLineFunctionDrawer::getFillColor() const
{
	QColor fillColor = getColor();
	return fillColor;
}

void iAFilledLineFunctionDrawer::drawPoly(QPainter& painter, QSharedPointer<QPolygon> poly) const
{
	QPainterPath tmpPath;
	tmpPath.addPolygon(*poly.data());
	painter.fillPath(tmpPath, QBrush(getFillColor()));
}

bool iAFilledLineFunctionDrawer::computePolygons(double binWidth, size_t startBin, size_t endBin, QSharedPointer<iAMapper> yMapper) const
{
	iAPlotData::DataType const * rawData = m_data->GetRawData();
	if (!rawData)
		return false;
	m_poly = QSharedPointer<QPolygon>(new QPolygon);
	int binWidthHalf = binWidth / 2;
	// one extra bin in each direction to see lines leading in direction of the previous/next data values
	startBin = (startBin > 0 ? startBin - 1 : startBin);
	endBin = endBin < m_data->GetNumBin() ? endBin + 1 : endBin;
	m_poly->push_back(QPoint(startBin-1 * binWidth, 0));
	for (int j = startBin; j < endBin; j++)
	{
		int curX = (int)(j * binWidth) + binWidthHalf;
		int curY = yMapper->srcToDst(rawData[j]);
		m_poly->push_back(QPoint(curX, curY));
	}
	m_poly->push_back(QPoint(endBin * binWidth, 0));
	return true;
}


iAStepFunctionDrawer::iAStepFunctionDrawer(QSharedPointer<iAPlotData> data, QColor const & color) :
	iAPolygonBasedFunctionDrawer(data, color)
{
}

QColor iAStepFunctionDrawer::getFillColor() const
{
	QColor fillColor = getColor();
	fillColor.setAlpha(getColor().alpha() / 3);
	return fillColor;
}

void iAStepFunctionDrawer::drawPoly(QPainter& painter, QSharedPointer<QPolygon> poly) const
{
	QPainterPath tmpPath;
	tmpPath.addPolygon(*poly.data());
	painter.fillPath(tmpPath, QBrush(getFillColor()));
}

bool iAStepFunctionDrawer::computePolygons(double binWidth, size_t startBin, size_t endBin, QSharedPointer<iAMapper> yMapper) const
{
	iAPlotData::DataType const * rawData = m_data->GetRawData();
	if (!rawData)
		return false;
	m_poly = QSharedPointer<QPolygon>(new QPolygon);
	m_poly->push_back(QPoint(startBin * binWidth, 0));
	for (int j = startBin; j < endBin; j++)
	{
		int curX1 = (int)(j * binWidth);
		int curX2 = (int)((j+1) * binWidth);
		int curY = yMapper->srcToDst(rawData[j]);
		m_poly->push_back(QPoint(curX1, curY));
		m_poly->push_back(QPoint(curX2, curY));
	}
	m_poly->push_back(QPoint(endBin * binWidth, 0));
	return true;
}


QSharedPointer<iAPlotData> iABarGraphDrawer::data()
{
	return m_data;
}

iABarGraphDrawer::iABarGraphDrawer(QSharedPointer<iAPlotData> data, QColor const & color, int margin):
	iAPlot(color),
	m_data(data),
	m_margin(margin)
{
}

void iABarGraphDrawer::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, QSharedPointer<iAMapper> yMapper) const
{
	iAPlotData::DataType const * rawData = m_data->GetRawData();
	int intBinWidth = static_cast<int>(std::ceil(binWidth)) - m_margin;

	if (!rawData)
	{
		return;
	}
	int x, h;
	QColor fillColor = getColor();
	int halfMargin = m_margin / 2;
	for ( int j = startBin; j < endBin; j++ )
	{
		x = (int)(j * binWidth) + halfMargin;
		h = yMapper->srcToDst(rawData[j]);
		if (m_lut)
		{
			double rgba[4];
			m_lut->getColor(j, rgba);
			fillColor = QColor(rgba[0]*255, rgba[1]*255, rgba[2]*255, rgba[3]*255);
		}
		painter.fillRect(QRect(x, 1, intBinWidth, h), fillColor);
	}
}


void iABarGraphDrawer::setLookupTable(QSharedPointer<iALookupTable> lut)
{
	m_lut = lut;
}


iAMultipleFunctionDrawer::iAMultipleFunctionDrawer():
	iAPlot(QColor())
{}

void iAMultipleFunctionDrawer::draw(QPainter& painter, double binWidth, size_t startBin, size_t endBin, QSharedPointer<iAMapper> yMapper) const
{
	qreal oldPenWidth = painter.pen().widthF();
	QPen pen = painter.pen();
	pen.setWidthF(3.0f);
	painter.setPen(pen);
	for(auto drawer: m_drawers)
	{
		drawer->draw(painter, binWidth, startBin, endBin, yMapper);
	}
	pen.setWidthF(oldPenWidth);
	painter.setPen(pen);
}

void iAMultipleFunctionDrawer::add(QSharedPointer<iAPlot> drawer)
{
	m_drawers.push_back(drawer);
}

void iAMultipleFunctionDrawer::clear()
{
	m_drawers.clear();
}

void iAMultipleFunctionDrawer::setColor(QColor const & color)
{
	iAColorable::setColor(color);
	for (auto drawer : m_drawers)
	{
		drawer->setColor(color);
	}
}
