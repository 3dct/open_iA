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

#include "RightBorderLayout.h"

RightBorderLayout::RightBorderLayout(QWidget *parent, int margin, int spacing)
	: QLayout(parent)
{
	setMargin(margin);
	setSpacing(spacing);
}

RightBorderLayout::RightBorderLayout(int spacing)
{
	setSpacing(spacing);
}


RightBorderLayout::~RightBorderLayout()
{
	delete m_centerItem;
	delete m_rightItem;
}

void RightBorderLayout::addItem(QLayoutItem *item)
{
	// Do nothing
}

void RightBorderLayout::addWidgetRight(RightBorderLayoutItemWrapper *item)
{
	setRight(item);
}

void RightBorderLayout::addWidgetCenter(QLayoutItem *item)
{
	setCenter(item);
}

Qt::Orientations RightBorderLayout::expandingDirections() const
{
	return Qt::Horizontal | Qt::Vertical;
}

bool RightBorderLayout::hasHeightForWidth() const
{
	return false;
}

int RightBorderLayout::count() const
{
	return m_centerItem != 0 ? 1 : 0
		+
		m_rightItem != 0 ? 1 : 0;
}

QLayoutItem *RightBorderLayout::itemAt(int index) const
{
	switch (index)
	{
	case 0:
		if (m_centerItem)
		{
			return m_centerItem;
		}
		else {
			return m_rightItem->layoutItem();
		}
	case 1:
		if (m_centerItem)
		{
			return m_rightItem->layoutItem();
		}
		else {
			return 0;
		}
	default:
		return 0;
	}
}

QSize RightBorderLayout::minimumSize() const
{
	return calculateSize(MinimumSize);
}

void RightBorderLayout::setGeometry(const QRect &rect)
{
	/*QLayout::setGeometry(rect);

	QWidget *a = m_centerItem->widget();
	const QRect b = m_centerItem->geometry();
	const QRect c = a->geometry();

	m_centerItem->setGeometry(rect);
	//m_rightItem->layoutItem()->setGeometry(rect);*/

	QLayoutItem *rightLayoutItem = m_rightItem->layoutItem();

	int width = m_rightItem->hasWidthForHeight()
		? m_rightItem->getWidthForHeight(rect.height())
		: rightLayoutItem->sizeHint().width(); // TODO: sizeHint() or geometry()?

	int x = rect.x() + rect.width() - width;

	rightLayoutItem->setGeometry(QRect(
		x,
		rect.y(),
		width,
		rect.height()
	));

	if (x > rect.x())
	{
		width = x - rect.x(); // remaining width
		x = rect.x();
	}
	else {
		// TODO: remove?
		//width = m_centerItem->sizeHint().width(); // TODO: sizeHint() or geometry()?
		return;
	}

	m_centerItem->setGeometry(QRect(
		x,
		rect.y(),
		width,
		rect.height()
	));
}

QSize RightBorderLayout::sizeHint() const
{
	return calculateSize(SizeHint);
}

QLayoutItem *RightBorderLayout::takeAt(int index)
{
	if (index >= 0 && index < count()) {
		return itemAt(index);
	}
	return 0;
}

void RightBorderLayout::setCenter(QLayoutItem* item)
{
	QLayout::addWidget(item->widget());
	m_centerItem = item;
}

void RightBorderLayout::setRight(RightBorderLayoutItemWrapper* item)
{
	QLayout::addWidget(item->layoutItem()->widget());
	m_rightItem = item;
}

void RightBorderLayout::setCenterWidget(QWidget* widget)
{
	setCenter(new QWidgetItem(widget));
}

void RightBorderLayout::setRightWidget(IRightBorderWidget *rbw)
{
	setRight(new RightBorderLayoutItemWrapper(rbw));
}

QSize RightBorderLayout::calculateSize(SizeType sizeType) const
{
	//return calculateSize(centerItems, sizeType) + calculateSize(rightItems, sizeType);

	QSize totalSize;
	incrementSize(totalSize, m_centerItem, sizeType);
	incrementSize(totalSize, m_rightItem->layoutItem(), sizeType);
	return totalSize;
}

void RightBorderLayout::incrementSize(QSize &totalSize, QLayoutItem *item, SizeType sizeType) const
{
	QSize itemSize;
	if (sizeType == MinimumSize)
		itemSize = item->minimumSize();
	else // (sizeType == SizeHint)
		itemSize = item->sizeHint();

	totalSize.rheight() += itemSize.height();
	totalSize.rwidth() += itemSize.width();
}