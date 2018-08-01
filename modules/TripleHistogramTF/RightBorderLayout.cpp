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
	QLayoutItem *l;
	while ((l = takeAt(0)))
		delete l;
}

void RightBorderLayout::addItem(QLayoutItem *item)
{
	addCenter(item);
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
	return centerItems.size() + rightItems.size();
}

QLayoutItem *RightBorderLayout::itemAt(int index) const
{
	if (index < centerItems.size())
	{
		return centerItems.value(index);
	}
	else {
		return rightItems.value(index - centerItems.size());
	}
}

QSize RightBorderLayout::minimumSize() const
{
	return calculateSize(MinimumSize);
}

void RightBorderLayout::setGeometry(const QRect &rect)
{
	/*ItemWrapper *center = 0;
	int eastWidth = 0;
	int westWidth = 0;
	int northHeight = 0;
	int southHeight = 0;
	int centerHeight = 0;
	int i;*/

	QLayout::setGeometry(rect);



	int x = rect.x() + rect.width();
	int width;
	int i;
	
	{
		RightBorderWidgetItem *item;
		for (i = rightItems.size() - 1; i >=0; --i)
		{
			item = rightItems.at(i);
			width = item->widget()->getWidthForHeight(rect.height());

			x -= width;

			item->setGeometry(QRect(
				x,
				rect.y(),
				width,
				rect.height()
			));
		}
	}

	{
		QLayoutItem *item;
		for (i = rightItems.size() - 1; i >= 0; --i)
		{
			item = rightItems.at(i);
			width = item->sizeHint().width();

			x -= width;

			item->setGeometry(QRect(
				x,
				rect.y(),
				width,
				rect.height()
			));
		}
	}



	/*for (i = 0; i < list.size(); ++i) {
		ItemWrapper *wrapper = list.at(i);
		QLayoutItem *item = wrapper->item;
		Position position = wrapper->position;

		if (position == North) {
			item->setGeometry(QRect(rect.x(), northHeight, rect.width(),
				item->sizeHint().height()));

			northHeight += item->geometry().height() + spacing();
		}
		else if (position == South) {
			item->setGeometry(QRect(item->geometry().x(),
				item->geometry().y(), rect.width(),
				item->sizeHint().height()));

			southHeight += item->geometry().height() + spacing();

			item->setGeometry(QRect(rect.x(),
				rect.y() + rect.height() - southHeight + spacing(),
				item->geometry().width(),
				item->geometry().height()));
		}
		else if (position == Center) {
			center = wrapper;
		}
	}

	centerHeight = rect.height() - northHeight - southHeight;

	for (i = 0; i < list.size(); ++i) {
		ItemWrapper *wrapper = list.at(i);
		QLayoutItem *item = wrapper->item;
		Position position = wrapper->position;

		if (position == West) {
			item->setGeometry(QRect(rect.x() + westWidth, northHeight,
				item->sizeHint().width(), centerHeight));

			westWidth += item->geometry().width() + spacing();
		}
		else if (position == East) {
			item->setGeometry(QRect(item->geometry().x(), item->geometry().y(),
				item->sizeHint().width(), centerHeight));

			eastWidth += item->geometry().width() + spacing();

			item->setGeometry(QRect(
				rect.x() + rect.width() - eastWidth + spacing(),
				northHeight, item->geometry().width(),
				item->geometry().height()));
		}
	}

	if (center)
		center->item->setGeometry(QRect(westWidth, northHeight,
			rect.width() - eastWidth - westWidth,
			centerHeight));*/
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

void RightBorderLayout::addCenter(QLayoutItem* item)
{
	centerItems.append(item);
}

void RightBorderLayout::addRight(RightBorderWidgetItem* item)
{
	rightItems.append(item);
}

void RightBorderLayout::addCenterWidget(QWidget* widget)
{
	centerItems.append(new QWidgetItem(widget));
}

void RightBorderLayout::addRightWidget(RightBorderWidget *widget)
{
	rightItems.append(new RightBorderWidgetItem(widget));
}

QSize RightBorderLayout::calculateSize(SizeType sizeType) const
{
	//return calculateSize(centerItems, sizeType) + calculateSize(rightItems, sizeType);

	QSize totalSize;
	for (QLayoutItem* item : centerItems)
	{
		incrementSize(totalSize, item, sizeType);
	}
	for (QLayoutItem* item : rightItems)
	{
		incrementSize(totalSize, item, sizeType);
	}
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