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

#pragma once

#include <QLayout>
#include <QRect>
#include <QWidget>

class RightBorderWidget : public QWidget
{
public:
	RightBorderWidget() {}
	virtual ~RightBorderWidget();
	virtual bool hasWidthForHeight();
	virtual int getWidthForHeight(int height);
};

class RightBorderWidgetItem : public QWidgetItem
{

public:
	RightBorderWidgetItem(RightBorderWidget *widget) : QWidgetItem(widget) {}
	//~RightBorderWidgetItem() { delete m_widget; } // TODO: uncomment?
	RightBorderWidget* widget() { return widget(); }

};

class RightBorderLayout : public QLayout
{
public:
	//enum Position { West, North, South, East, Center };

	explicit RightBorderLayout(QWidget *parent, int margin = 0, int spacing = -1);
	RightBorderLayout(int spacing = -1);
	~RightBorderLayout();

	void addItem(QLayoutItem *item) override;
	//void addWidget(QWidget *widget, Position position);
	Qt::Orientations expandingDirections() const override;
	bool hasHeightForWidth() const override;
	int count() const override;
	QLayoutItem *itemAt(int index) const override;
	QSize minimumSize() const override;
	void setGeometry(const QRect &rect) override;
	QSize sizeHint() const override;
	QLayoutItem *takeAt(int index) override;

	void addCenterWidget(QWidget* widget);
	void addRightWidget(RightBorderWidget *widget);

private:
	/*struct ItemWrapper
	{
		ItemWrapper(QLayoutItem *i, Position p) {
			item = i;
			position = p;
		}

		QLayoutItem *item;
		Position position;
	};*/

	enum SizeType { MinimumSize, SizeHint };
	QSize calculateSize(SizeType sizeType) const;
	void incrementSize(QSize &totalSize, QLayoutItem *item, SizeType sizeType) const;

	void addCenter(QLayoutItem* item);
	void addRight(RightBorderWidgetItem* item);

	//QList<ItemWrapper *> list;

	// Widgets that will be placed in the center
	// The ith element has index i-1 in this layout
	QList<QLayoutItem *> centerItems;

	// Widgets that will be placed in the right
	// For N elements in 'centerItems', the ith element in 'rightItems'
	//		has index N+i-1
	QList<RightBorderWidgetItem *> rightItems;
};