// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QLayout>

class QRect;
class QWidget;

class iAIBorderItem
{
public:
	virtual ~iAIBorderItem();
	virtual bool hasWidthForHeight() = 0;
	virtual int getWidthForHeight(int height) = 0;
	virtual bool hasHeightForWidth() = 0;
	virtual int getHeightForWidth(int width) = 0;
};

class iAIBorderLayoutItem : public iAIBorderItem
{
public:
	virtual QLayoutItem* layoutItem() = 0;
};

class iAIBorderWidget : public iAIBorderItem
{
public:
	virtual QWidget* widget() = 0;
};

class iASquareBorderWidget : public iAIBorderWidget
{
public:
	iASquareBorderWidget(QWidget* widget) : m_widget(widget) {}
	bool hasWidthForHeight() override { return true; }
	int getWidthForHeight(int height) override { return height; }
	bool hasHeightForWidth() override { return true; }
	int getHeightForWidth(int width) override { return width; }
	QWidget* widget() override { return m_widget; }

private:
	QWidget *m_widget;

};

class BorderLayoutItemWrapper : iAIBorderLayoutItem
{
public:
	BorderLayoutItemWrapper(iAIBorderItem* rbi, QLayoutItem *layoutItem) : m_rbi(rbi), m_layoutItem(layoutItem)
	{}
	BorderLayoutItemWrapper(iAIBorderWidget* rbw) : m_rbi(rbw), m_layoutItem(new QWidgetItem(rbw->widget()))
	{}
	BorderLayoutItemWrapper(iAIBorderLayoutItem* rbli) : m_rbi(rbli), m_layoutItem(rbli->layoutItem())
	{}
	bool hasWidthForHeight()
	{
		return m_rbi->hasWidthForHeight();
	}
	int getWidthForHeight(int height)
	{
		return m_rbi->getWidthForHeight(height);
	}
	bool hasHeightForWidth()
	{
		return m_rbi->hasHeightForWidth();
	}
	int getHeightForWidth(int width)
	{
		return m_rbi->getHeightForWidth(width);
	}
	QLayoutItem* layoutItem()
	{
		return m_layoutItem;
	}

private:
	iAIBorderItem *m_rbi;
	QLayoutItem *m_layoutItem;

};

class iARightBorderLayout : public QLayout
{
public:
	enum Position { Right, Top };

	explicit iARightBorderLayout(QWidget *parent, Position pos = Right, int margin = 0, int spacing = -1);
	iARightBorderLayout(int spacing = -1);
	~iARightBorderLayout();

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

	void setPosition(Position pos);

	void addWidgetBorder(BorderLayoutItemWrapper *item);
	void addWidgetCenter(QLayoutItem *item);

	void setCenterWidget(QWidget* widget);
	void setBorderWidget(iAIBorderWidget *rbw);

private:
	enum SizeType { MinimumSize, SizeHint };
	QSize calculateSize(SizeType sizeType) const;
	void incrementSize(QSize &totalSize, QLayoutItem *item, SizeType sizeType) const;

	void setCenterItem(QLayoutItem *item);
	void setBorderItem(BorderLayoutItemWrapper *item); // TODO: should really be a pointer?

	QLayoutItem *m_centerItem;
	BorderLayoutItemWrapper *m_borderItem;
	Position m_pos;
};
