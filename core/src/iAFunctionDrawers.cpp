/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "pch.h"
#include "iAFunctionDrawers.h"

#include "iAAbstractDiagramData.h"

#include <QPainter>
#include <QPolygon>

#include <cmath>


iAAbstractDrawableFunction::iAAbstractDrawableFunction(QColor const & color):
	iAColorable(color)
{}

iASelectedBinDrawer::iASelectedBinDrawer( int position /*= 0*/, QColor const & color /*= Qt::red */ )
: iAAbstractDrawableFunction( color ), m_position( position )
{}

void iASelectedBinDrawer::draw( QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter ) const
{
	int x = (int)(m_position * binWidth);
	int h = painter.device()->height();
	int intBinWidth = static_cast<int>(std::ceil(binWidth));
	painter.fillRect( QRect( x, 0, intBinWidth, h ), getColor() );
}

void iASelectedBinDrawer::setPosition( int position )
{
	m_position = position;
}



iAPolygonBasedFunctionDrawer::iAPolygonBasedFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color):
	iAAbstractDrawableFunction(color),
	m_data(data),
	m_cachedBinWidth(0.0),
	m_cachedCoordConv(0)
{}

void iAPolygonBasedFunctionDrawer::draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const
{
	if (!m_poly || m_cachedBinWidth != binWidth || !m_cachedCoordConv || !m_cachedCoordConv->equals(converter) )
	{
		m_cachedBinWidth = binWidth;
		m_cachedCoordConv = converter->clone();
		if (!computePolygons(binWidth, converter))
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



iALineFunctionDrawer::iALineFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color):
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


bool iALineFunctionDrawer::computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const
{
	iAAbstractDiagramData::DataType const * rawData = m_data->GetData();
	if (!rawData)
		return false;
	int binWidthHalf = binWidth / 2;
	m_poly = QSharedPointer<QPolygon>(new QPolygon);
	m_poly->push_back(QPoint(0, 0));
	for (int j = 0; j < m_data->GetNumBin(); j++)
	{
		int curX = (int)(j * binWidth) + binWidthHalf;
		int curY = converter->Diagram2ScreenY(rawData[j]);
		m_poly->push_back(QPoint(curX, curY));
	}
	m_poly->push_back(QPoint(m_data->GetNumBin() * binWidth, 0 ));
	return true;
}



iAFilledLineFunctionDrawer::iAFilledLineFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color):
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

bool iAFilledLineFunctionDrawer::computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const
{
	iAAbstractDiagramData::DataType const * rawData = m_data->GetData();
	if (!rawData)
		return false;
	m_poly = QSharedPointer<QPolygon>(new QPolygon);
	//m_poly->push_back(QPoint(1, 0));
	int binWidthHalf = binWidth / 2;
	int minBin = static_cast<int>(m_data->GetMinX());
	int maxBin = static_cast<int>(m_data->GetMaxX()-0.01);
	int minX = static_cast<int>(m_data->GetMinX() * binWidth);
	int maxX = static_cast<int>(m_data->GetMaxX() * binWidth);
	for (int j = 0; j < m_data->GetNumBin(); j++)
	{
		if (j < minBin || j > maxBin)
		{
			continue;
		}
		int curX = (int)(j * binWidth) + binWidthHalf;
		if (j == minBin)
		{
			m_poly->push_back(QPoint(minX, 0));
			if (j == maxBin)
			{
				curX = (minX + maxX) / 2;
			}
			else
			{
				curX = (minX + (int)((j + 1) * binWidth)) / 2;
			}
		}
		else if (j == maxBin)
		{
			curX = ((int)(j*binWidth) + maxX) / 2;
		}
		int curY = converter->Diagram2ScreenY(rawData[j]);
		m_poly->push_back(QPoint(curX, curY));
		if (j == maxBin)
		{
			m_poly->push_back(QPoint(maxX, 0));
		}
	}

	//m_poly->push_back(QPoint(m_data->GetNumBin() * binWidth, 0));
	return true;
}



iAStepFunctionDrawer::iAStepFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color) :
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

bool iAStepFunctionDrawer::computePolygons(double binWidth, QSharedPointer<CoordinateConverter> converter) const
{
	iAAbstractDiagramData::DataType const * rawData = m_data->GetData();
	if (!rawData)
		return false;
	m_poly = QSharedPointer<QPolygon>(new QPolygon);
	m_poly->push_back(QPoint(1, 0));
	for (int j = 0; j < m_data->GetNumBin(); j++)
	{
		int curX1 = 1 + (int)(j * binWidth);
		int curX2 = 1 + (int)(j * binWidth) + binWidth;
		int curY = converter->Diagram2ScreenY(rawData[j]);
		m_poly->push_back(QPoint(curX1, curY));
		m_poly->push_back(QPoint(curX2, curY));
	}
	m_poly->push_back(QPoint(m_data->GetNumBin() * binWidth, 0));
	return true;
}



iABarGraphDrawer::iABarGraphDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color, int margin):
	iAAbstractDrawableFunction(color),
	m_data(data),
	m_margin(margin)
{
}

void iABarGraphDrawer::draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const
{
	iAAbstractDiagramData::DataType const * rawData = m_data->GetData();
	int intBinWidth = static_cast<int>(std::ceil(binWidth)) - m_margin;

	if (!rawData)
	{
		return;
	}
	int x, h;
	QColor fillColor = getColor();
	int halfMargin = m_margin / 2;
	for ( int j = 0; j < m_data->GetNumBin(); j++ )
	{
		x = (int)(j * binWidth) + halfMargin;
		h = converter->Diagram2ScreenY(rawData[j]);
		painter.fillRect(QRect(x, 1, intBinWidth, h), fillColor);
	}
}

void iABarGraphDrawer::update()
{
	// nothing to do here, no caching implemented for this drawer
}

iAMultipleFunctionDrawer::iAMultipleFunctionDrawer():
	iAAbstractDrawableFunction(QColor())
{}

void iAMultipleFunctionDrawer::draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const
{
	qreal oldPenWidth = painter.pen().widthF();
	QPen pen = painter.pen();
	pen.setWidthF(3.0f);
	painter.setPen(pen);
	for(auto drawer: m_drawers)
	{
		drawer->draw(painter, binWidth, converter);
	}
	pen.setWidthF(oldPenWidth);
	painter.setPen(pen);
}

void iAMultipleFunctionDrawer::add(QSharedPointer<iAAbstractDrawableFunction> drawer)
{
	m_drawers.push_back(drawer);
}

void iAMultipleFunctionDrawer::update()
{
	// no caching (yet!)
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
