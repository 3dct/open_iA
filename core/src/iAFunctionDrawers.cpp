/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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

iASelectedBinDrawer::iASelectedBinDrawer( int position /*= 0*/, QColor const & color /*= Qt::red */ )
: iAColorable( color ), m_position( position )
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

iAPolygonBasedFunctionDrawer::iAPolygonBasedFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data):
	m_data(data),
	m_cachedBinWidth(0.0)
{}

iAPolygonBasedFunctionDrawer::iAPolygonBasedFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color):
	iAColorable(color),
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


iALineFunctionDrawer::iALineFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data):
	iAPolygonBasedFunctionDrawer(data)
{
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
	int curY = converter->Diagram2ScreenY(rawData[0]);
	m_poly->push_back(QPoint(0, curY));
	for (int j = 0; j < m_data->GetNumBin(); j++)
	{
		int curX = (int)(j * binWidth) + binWidthHalf;
		curY = converter->Diagram2ScreenY(rawData[j]);
		m_poly->push_back(QPoint(curX, curY));
	}
	m_poly->push_back(QPoint((m_data->GetNumBin() + 1) * binWidth, curY));
	return true;
}


iAFilledLineFunctionDrawer::iAFilledLineFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data):
	iAPolygonBasedFunctionDrawer(data, Qt::blue)
{
}

iAFilledLineFunctionDrawer::iAFilledLineFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color):
	iAPolygonBasedFunctionDrawer(data, color)
{
}

QColor iAFilledLineFunctionDrawer::getFillColor() const
{
	QColor fillColor = getColor();
	fillColor.setAlpha( getColor().alpha() / 3);
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
	m_poly->push_back(QPoint(1, 0));
	int binWidthHalf = binWidth / 2;
	for (int j = 0; j < m_data->GetNumBin(); j++)
	{
		int curX = (int)(j * binWidth) + binWidthHalf;
		int curY = converter->Diagram2ScreenY(rawData[j]);
		m_poly->push_back(QPoint(curX, curY));
	}
	m_poly->push_back(QPoint(m_data->GetNumBin() * binWidth, 0));
	return true;
}



iAStepFunctionDrawer::iAStepFunctionDrawer(QSharedPointer<iAAbstractDiagramData> data) :
	iAPolygonBasedFunctionDrawer(data, Qt::blue)
{
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




iABarGraphDrawer::iABarGraphDrawer(QSharedPointer<iAAbstractDiagramData> data):
	iAColorable(QColor(70,70,70,255)),
	m_data(data),
	m_margin(0)
{
}

iABarGraphDrawer::iABarGraphDrawer(QSharedPointer<iAAbstractDiagramData> data, QColor const & color, int margin):
	iAColorable(color),
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
	for ( int j = 0; j < m_data->GetNumBin(); j++ )
	{
		x = (int)(j * binWidth) + m_margin/2;
		h = converter->Diagram2ScreenY(rawData[j]);
		QColor fillColor = getColor();
		fillColor.setAlpha( getColor().alpha() / 3);
		painter.fillRect(QRect(x, 1, intBinWidth, h), fillColor);
	}
}

void iABarGraphDrawer::update()
{
	// nothing to do here, no caching implemented for this drawer
}



void iAMultipleFunctionDrawer::draw(QPainter& painter, double binWidth, QSharedPointer<CoordinateConverter> converter) const
{
	qreal oldPenWidth = painter.pen().widthF();
	QPen pen = painter.pen();
	pen.setWidthF(3.0f);
	painter.setPen(pen);
	QVector<QSharedPointer<iAAbstractDrawableFunction> >::const_iterator it = lines.constBegin();
	while (it != lines.constEnd())
	{
		(*it)->draw(painter, binWidth, converter);
		++it;
	}
	pen.setWidthF(oldPenWidth);
	painter.setPen(pen);
}

void iAMultipleFunctionDrawer::add(QSharedPointer<iAAbstractDrawableFunction> line)
{
	lines.push_back(line);
}

void iAMultipleFunctionDrawer::update()
{
	// no caching (yet!)
}
void iAMultipleFunctionDrawer::clear()
{
	lines.clear();
}