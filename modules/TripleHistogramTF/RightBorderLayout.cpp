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

RightBorderLayout::RightBorderLayout(QWidget *parent, Position pos, int margin, int spacing)
	: QLayout(parent), m_pos(pos)
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
	delete m_borderItem;
}

void RightBorderLayout::addItem(QLayoutItem *item)
{
	// Do nothing
}

void RightBorderLayout::addWidgetBorder(BorderLayoutItemWrapper *item)
{
	setBorderItem(item);
}

void RightBorderLayout::addWidgetCenter(QLayoutItem *item)
{
	setCenterItem(item);
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
		m_borderItem != 0 ? 1 : 0;
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
			return m_borderItem->layoutItem();
		}
	case 1:
		if (m_centerItem)
		{
			return m_borderItem->layoutItem();
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
	QLayoutItem *borderLayoutItem = m_borderItem->layoutItem();

	if (m_pos == Right)
	{
		int width = m_borderItem->hasWidthForHeight()
			? m_borderItem->getWidthForHeight(rect.height())
			: borderLayoutItem->sizeHint().width(); // TODO: sizeHint() or geometry()?

		int x = rect.x() + rect.width() - width;

		borderLayoutItem->setGeometry(QRect(x, rect.y(), width, rect.height()
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

		m_centerItem->setGeometry(QRect(x, rect.y(), width, rect.height()
		));

	} else {// if (m_pos == Top)
		int width = rect.width();//m_centerItem->minimumSize().width();
		int height = m_borderItem->hasHeightForWidth()
			? m_borderItem->getHeightForWidth(width)
			: borderLayoutItem->sizeHint().height();

		borderLayoutItem->setGeometry(QRect(rect.x(), rect.y(), width, height));
		m_centerItem->setGeometry(QRect(rect.x(), rect.y() + height, width, rect.height() - height));
	}
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

void RightBorderLayout::setCenterItem(QLayoutItem* item)
{
	QLayout::addWidget(item->widget());
	m_centerItem = item;
}

void RightBorderLayout::setBorderItem(BorderLayoutItemWrapper* item)
{
	QLayout::addWidget(item->layoutItem()->widget());
	m_borderItem = item;
}

void RightBorderLayout::setCenterWidget(QWidget* widget)
{
	setCenterItem(new QWidgetItem(widget));
}

void RightBorderLayout::setBorderWidget(IBorderWidget *rbw)
{
	setBorderItem(new BorderLayoutItemWrapper(rbw));
}

QSize RightBorderLayout::calculateSize(SizeType sizeType) const
{
	//return calculateSize(centerItems, sizeType) + calculateSize(rightItems, sizeType);

	QSize totalSize;
	incrementSize(totalSize, m_centerItem, sizeType);
	incrementSize(totalSize, m_borderItem->layoutItem(), sizeType);
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