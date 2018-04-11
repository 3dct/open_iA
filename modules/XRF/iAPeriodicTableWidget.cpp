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
#include "iAPeriodicTableWidget.h"

#include "iAElementConstants.h"
#include "iAElementSelectionListener.h"

#include <QEvent>
#include <QMouseEvent>
#include <QPainter>

#include <sstream>


iAPeriodicTableWidget::iAPeriodicTableWidget(QWidget *parent):
	QWidget(parent),
	m_currentElemIdx(-1)
{
	setMouseTracking(true);
}

namespace
{
	const int Margin = 5;
}

void iAPeriodicTableWidget::drawElement(QPainter& painter, QPoint const & upperLeft, int elementWidth, int elementHeight, int elemIdx)
{
	m_elementPositions.push_back(std::pair<QPoint, int>(upperLeft, elemIdx));
	QRect elementRect(upperLeft, QSize(elementWidth, elementHeight));
	painter.drawRect(elementRect);

	QMap<QString, std::pair<double, QColor> >::const_iterator concIt =  m_concentration.find(PeriodicTable::elements[elemIdx].shortname.c_str());
	if (concIt != m_concentration.end())
	{
		QRect concRect(elementRect);
		concRect.translate(0, 1+elementHeight * (1 - concIt.value().first ));
		concRect.setHeight(elementHeight * concIt.value().first);
		concRect.setLeft(concRect.left()+1);
		painter.setPen(Qt::NoPen);
		painter.setBrush(concIt.value().second);
		painter.drawRect(concRect);
		painter.setPen(getPenColor(painter));
		painter.setBrush(Qt::NoBrush);
	}

	QPoint middlePoint (upperLeft.x() + elementWidth/2, upperLeft.y() + elementHeight/2);

	QFont myFont;
	QFontMetrics fm(myFont);
	int textWidth  = fm.width(PeriodicTable::elements[elemIdx].shortname.c_str());
	int textHeight = fm.height();
			
	QRect textRect(middlePoint.x()-textWidth/2, middlePoint.y()-textHeight/2, textWidth, textHeight);
	QTextOption o;
	o.setAlignment(Qt::AlignCenter);
	painter.drawText(textRect, PeriodicTable::elements[elemIdx].shortname.c_str(), o);
}

QColor iAPeriodicTableWidget::getPenColor(QPainter const & painter)
{
	return (painter.background().color() == Qt::black) ? Qt::white : Qt::black;
}

void iAPeriodicTableWidget::paintEvent(QPaintEvent * e)
{
	m_elementPositions.clear();
	QWidget::paintEvent(e);
	QPainter painter(this);
	painter.setPen(getPenColor(painter));

	const int width = geometry().width();
	m_elementWidth = (width-(2*Margin)) / PeriodicTable::MaxElementsPerLine;
	const int height = geometry().height();
	m_elementHeight = (height-(2*Margin)) / (PeriodicTable::LineCount + PeriodicTable::ExtraTableLines + 1);

	int widthDivRemainder = width - (m_elementWidth * PeriodicTable::MaxElementsPerLine);
	int leftMargin = widthDivRemainder / 2 + ((widthDivRemainder % 2 != 0) ? 1 : 0);
	int rightMargin = widthDivRemainder / 2;
	int lineStartElemIdx = 0;
	int heightDivRemainder = height - (m_elementHeight * (PeriodicTable::LineCount + PeriodicTable::ExtraTableLines + 1));
	int upperMargin = heightDivRemainder / 2;
	for (int line=0; line<PeriodicTable::LineCount; ++line)
	{
		int colCount = std::min(PeriodicTable::ElementsPerLine[line], PeriodicTable::MaxElementsPerLine);
		
		for (int elem=0; elem < colCount; ++elem)
		{
			QPoint upperLeft(0, upperMargin+(line*m_elementHeight));
			if (elem < PeriodicTable::ElementsOnLeftSide[line])
			{
				upperLeft.setX(leftMargin + (elem*m_elementWidth));
			}
			else
			{
				int offsetFromRight = std::min(PeriodicTable::ElementsPerLine[line], PeriodicTable::MaxElementsPerLine) -  elem;
				
				upperLeft.setX(width - rightMargin - (offsetFromRight * m_elementWidth));
			}
			
			int elemIdx = lineStartElemIdx + 
				(elem < PeriodicTable::ElementsOnLeftSide[line]
					? elem
					: PeriodicTable::ElementsPerLine[line] - 
						(std::min(PeriodicTable::ElementsPerLine[line], PeriodicTable::MaxElementsPerLine) - elem));

			drawElement(painter, upperLeft, m_elementWidth, m_elementHeight, elemIdx);
		}

		if (PeriodicTable::ElementsPerLine[line] > PeriodicTable::MaxElementsPerLine)
		{
			for (int i=0;
				i < PeriodicTable::ElementsPerLine[line] - PeriodicTable::MaxElementsPerLine;
				++i)
			{
				int elemIdx = lineStartElemIdx + 
					PeriodicTable::ElementsOnLeftSide[line] + i;

				QPoint upperLeft(leftMargin + (2+i)*m_elementWidth,
					upperMargin + (line + PeriodicTable::ExtraTableLines + 1)*m_elementHeight);

				drawElement(painter, upperLeft, m_elementWidth, m_elementHeight, elemIdx);
			}
		}

		lineStartElemIdx += PeriodicTable::ElementsPerLine[line];
	}
}

int iAPeriodicTableWidget::getElementFromMousePos(QPoint pos)
{
	for (QVector<std::pair<QPoint, int> >::const_iterator it = m_elementPositions.begin();
		it != m_elementPositions.end();
		++it)
	{
		QRect elemRect(it->first.x(), it->first.y(), m_elementWidth, m_elementHeight);
		if (elemRect.contains(pos))
		{
			return it->second;
		}
	}
	return -1;
}

void iAPeriodicTableWidget::mouseMoveEvent(QMouseEvent * e)
{
	if (!m_listener)
	{
		return;
	}
	int elementIdx = getElementFromMousePos(e->pos());
	if (m_currentElemIdx != elementIdx)
	{
		if (m_currentElemIdx != -1)
		{
			m_listener->ElementLeave(m_currentElemIdx);
		}
		m_currentElemIdx = elementIdx;
		if (elementIdx != -1)
		{
			m_listener->ElementEnter(elementIdx);
		}
	}
}


void iAPeriodicTableWidget::setConcentration(QString const & elementName, double percentage, QColor const & color)
{
	m_concentration.insert(elementName, std::pair<double, QColor>(percentage, color));
}

void iAPeriodicTableWidget::setListener(QSharedPointer<iAElementSelectionListener> listener)
{
	m_listener = listener;
}

int iAPeriodicTableWidget::GetCurrentElement() const
{
	return m_currentElemIdx;
}
