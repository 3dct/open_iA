// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	const int PeriodicTableMargin = 5;
}

void iAPeriodicTableWidget::drawElement(QPainter& painter, QPoint const & upperLeft, int elementWidth, int elementHeight, int elemIdx)
{
	m_elementPositions.push_back(std::pair<QPoint, int>(upperLeft, elemIdx));
	QRect elementRect(upperLeft, QSize(elementWidth, elementHeight));
	painter.drawRect(elementRect);

	auto concIt = m_concentration.find(PeriodicTable::elements[elemIdx].shortname.c_str());
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
	int textWidth  = fm.horizontalAdvance(PeriodicTable::elements[elemIdx].shortname.c_str());
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
	m_elementWidth = (width - (2*PeriodicTableMargin)) / PeriodicTable::MaxElementsPerLine;
	const int height = geometry().height();
	m_elementHeight = (height-(2*PeriodicTableMargin)) / (PeriodicTable::LineCount + PeriodicTable::ExtraTableLines + 1);

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
	for (auto it = m_elementPositions.cbegin(); it != m_elementPositions.cend(); ++it)
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

void iAPeriodicTableWidget::setListener(std::shared_ptr<iAElementSelectionListener> listener)
{
	m_listener = listener;
}

int iAPeriodicTableWidget::GetCurrentElement() const
{
	return m_currentElemIdx;
}
