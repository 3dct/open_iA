// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARightBorderLayout.h"

iAIBorderItem::~iAIBorderItem()
{}

iARightBorderLayout::iARightBorderLayout(QWidget *parent, Position pos, int margin, int spacing)
	: QLayout(parent), m_pos(pos)
{
	setContentsMargins(margin, margin, margin, margin);
	setSpacing(spacing);
}

iARightBorderLayout::iARightBorderLayout(int spacing)
{
	setSpacing(spacing);
}


iARightBorderLayout::~iARightBorderLayout()
{
	delete m_centerItem;
	delete m_borderItem;
}

void iARightBorderLayout::addItem(QLayoutItem * /*item*/)
{
	// Do nothing
}

void iARightBorderLayout::addWidgetBorder(BorderLayoutItemWrapper *item)
{
	setBorderItem(item);
}

void iARightBorderLayout::addWidgetCenter(QLayoutItem *item)
{
	setCenterItem(item);
}

Qt::Orientations iARightBorderLayout::expandingDirections() const
{
	return Qt::Horizontal | Qt::Vertical;
}

bool iARightBorderLayout::hasHeightForWidth() const
{
	return false;
}

int iARightBorderLayout::count() const
{
	return m_centerItem != 0 ? 1 : 0
		+
		m_borderItem != 0 ? 1 : 0;
}

QLayoutItem *iARightBorderLayout::itemAt(int index) const
{
	switch (index)
	{
	case 0:
		if (m_centerItem)
		{
			return m_centerItem;
		}
		else
		{
			return m_borderItem->layoutItem();
		}
	case 1:
		if (m_centerItem)
		{
			return m_borderItem->layoutItem();
		}
		else
		{
			return 0;
		}
	default:
		return 0;
	}
}

QSize iARightBorderLayout::minimumSize() const
{
	return calculateSize(MinimumSize);
}

void iARightBorderLayout::setGeometry(const QRect &rect)
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
		else
		{
			// TODO: remove?
			//width = m_centerItem->sizeHint().width(); // TODO: sizeHint() or geometry()?
			return;
		}

		m_centerItem->setGeometry(QRect(x, rect.y(), width, rect.height()));

	}
	else
	{// if (m_pos == Top)
		int width = rect.width();//m_centerItem->minimumSize().width();
		int height = m_borderItem->hasHeightForWidth()
			? m_borderItem->getHeightForWidth(width)
			: borderLayoutItem->sizeHint().height();

		borderLayoutItem->setGeometry(QRect(rect.x(), rect.y(), width, height));
		m_centerItem->setGeometry(QRect(rect.x(), rect.y() + height, width, rect.height() - height));
	}
}

QSize iARightBorderLayout::sizeHint() const
{
	return calculateSize(SizeHint);
}

QLayoutItem *iARightBorderLayout::takeAt(int index)
{
	if (index >= 0 && index < count())
	{
		return itemAt(index);
	}
	return 0;
}

void iARightBorderLayout::setCenterItem(QLayoutItem* item)
{
	QLayout::addWidget(item->widget());
	m_centerItem = item;
}

void iARightBorderLayout::setBorderItem(BorderLayoutItemWrapper* item)
{
	QLayout::addWidget(item->layoutItem()->widget());
	m_borderItem = item;
}

void iARightBorderLayout::setCenterWidget(QWidget* widget)
{
	setCenterItem(new QWidgetItem(widget));
}

void iARightBorderLayout::setBorderWidget(iAIBorderWidget *rbw)
{
	setBorderItem(new BorderLayoutItemWrapper(rbw));
}

QSize iARightBorderLayout::calculateSize(SizeType sizeType) const
{
	//return calculateSize(centerItems, sizeType) + calculateSize(rightItems, sizeType);

	QSize totalSize;
	incrementSize(totalSize, m_centerItem, sizeType);
	incrementSize(totalSize, m_borderItem->layoutItem(), sizeType);
	return totalSize;
}

void iARightBorderLayout::incrementSize(QSize &totalSize, QLayoutItem *item, SizeType sizeType) const
{
	QSize itemSize;
	if (sizeType == MinimumSize)
	{
		itemSize = item->minimumSize();
	}
	else  // (sizeType == SizeHint)
	{
		itemSize = item->sizeHint();
	}

	totalSize.rheight() += itemSize.height();
	totalSize.rwidth() += itemSize.width();
}
