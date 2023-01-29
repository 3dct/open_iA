// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQtCaptionWidget.h"

#include "iAGEMSeConstants.h"

#include <QPainter>
#include <QVBoxLayout>

// TODO: replace with QLabel!

iAQtCaptionWidget::iAQtCaptionWidget(QWidget* parent, QString const & name):
	QWidget(parent),
	m_name(name),
	m_height(CaptionHeight)
{
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	setFixedHeight(m_height);
}
void iAQtCaptionWidget::paintEvent(QPaintEvent* /*ev*/)
{
	QPainter painter(this);
	QRect r(geometry());
	painter.fillRect(0, 0, r.width(), r.height(), DefaultColors::CaptionBrush);
	painter.setPen(DefaultColors::CaptionFontColor);
	QFont f(painter.font());
	f.setPointSize(FontSize);
	painter.setFont(f);
	QFontMetrics fm(painter.font());
	painter.drawText(10, m_height-5, m_name);
}


void SetCaptionedContent(QWidget* parent, QString const & caption, QWidget* w)
{
	QVBoxLayout	* mainLayout = new QVBoxLayout();
	mainLayout->addWidget(new iAQtCaptionWidget(parent, caption));
	mainLayout->addWidget(w);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->setSpacing(0);
	mainLayout->setAlignment(Qt::AlignTop);
	parent->setLayout(mainLayout);
}
