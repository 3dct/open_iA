/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenb�ck, Artem & Alexander Amirkhanov, B. Fr�hler   *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#pragma once

#include <QLayout>
#include <QRect>
#include <QWidget>

#include <QOpenGLWidget>

class IRightBorderItem
{
public:
	//IRightBorderWidget() {}
	//virtual ~IRightBorderWidget(); // TODO: uncomment?
	virtual bool hasWidthForHeight() = 0;
	virtual int getWidthForHeight(int height) = 0;
};

class IRightBorderLayoutItem : public IRightBorderItem
{
public:
	virtual QLayoutItem* layoutItem() = 0;
};

class IRightBorderWidget : public IRightBorderItem
{
public:
	virtual QWidget* widget() = 0;
};

class RightBorderLayoutItemWrapper : IRightBorderLayoutItem
{
public:
	RightBorderLayoutItemWrapper(IRightBorderItem* rbi, QLayoutItem *layoutItem) : m_rbi(rbi), m_layoutItem(layoutItem) {}
	RightBorderLayoutItemWrapper(IRightBorderWidget* rbw) : m_rbi(rbw), m_layoutItem(new QWidgetItem(rbw->widget())) {}
	RightBorderLayoutItemWrapper(IRightBorderLayoutItem* rbli) : m_rbi(rbli), m_layoutItem(rbli->layoutItem()) {}
	bool RightBorderLayoutItemWrapper::hasWidthForHeight() { return m_rbi->hasWidthForHeight(); }
	int RightBorderLayoutItemWrapper::getWidthForHeight(int height) { return m_rbi->getWidthForHeight(height); }
	QLayoutItem* RightBorderLayoutItemWrapper::layoutItem() { return m_layoutItem; }

private:
	IRightBorderItem *m_rbi;
	QLayoutItem *m_layoutItem;

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

	void addWidgetRight(RightBorderLayoutItemWrapper *item);
	void addWidgetCenter(QLayoutItem *item);

	void setCenterWidget(QWidget* widget);
	void setRightWidget(IRightBorderWidget *rbw);

	/*void setCenter(QLayoutItem* item);
	void setRight(RightBorderLayoutItemWrapper* item);*/

private:
	enum SizeType { MinimumSize, SizeHint };
	QSize calculateSize(SizeType sizeType) const;
	void incrementSize(QSize &totalSize, QLayoutItem *item, SizeType sizeType) const;

	void setCenter(QLayoutItem *item);
	void setRight(RightBorderLayoutItemWrapper *item);

	QLayoutItem *m_centerItem;
	RightBorderLayoutItemWrapper *m_rightItem;
};