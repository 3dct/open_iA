// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFixedAspectWidget.h"

#include "iALog.h"

#include <QApplication>
#include <QPainter>
#include <QVBoxLayout>

//! Widget with a fixed colored background color.
class iAColoredWidget: public QWidget
{
public:
	iAColoredWidget()
	{}
	void paintEvent(QPaintEvent* /*ev*/) override
	{
		QPainter p(this);
		p.fillRect(rect(), QApplication::palette().color(QWidget::backgroundRole()));
	}
};

//! The internal iAQVTKWidget of an iAFixedAspectWidget which actually keeps its aspect ratio.
class iAFixedAspectWidgetInternal: public iAQVTKWidget
{
public:
	iAFixedAspectWidgetInternal(double aspect):
		m_aspect(aspect)
	{}
	bool hasHeightForWidth() const override
	{
		return true;
	}
	int heightForWidth(int w) const override
	{
		int newHeight = static_cast<int>(w*m_aspect);
		return newHeight;
	}
private:
	double m_aspect;
};

iAFixedAspectWidget::iAFixedAspectWidget(double aspect, Qt::Alignment verticalAlign):
	m_fill1(nullptr),
	m_fill2(nullptr)
{
	setLayout(new QVBoxLayout());
	layout()->setSpacing(0);
	layout()->setContentsMargins(0, 0, 0, 0);
	if (verticalAlign != Qt::AlignTop)
	{
		m_fill1 = new iAColoredWidget();
		m_fill1->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		layout()->addWidget(m_fill1);
	}
	m_widget = new iAFixedAspectWidgetInternal(aspect);
	layout()->addWidget(m_widget);
	if (verticalAlign != Qt::AlignBottom)
	{
		m_fill2 = new iAColoredWidget();
		m_fill2->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		layout()->addWidget(m_fill2);
	}
}

iAQVTKWidget* iAFixedAspectWidget::vtkWidget()
{
	return m_widget;
}

void iAFixedAspectWidget::setBGRole(QPalette::ColorRole role)
{
	if (m_fill1)
	{
		m_fill1->setBackgroundRole(role);
	}
	if (m_fill2)
	{
		m_fill2->setBackgroundRole(role);
	}
}
