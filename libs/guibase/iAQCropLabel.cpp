// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQCropLabel.h"

#include <QLabel>

iAQCropLabel::iAQCropLabel()
{
	setup();
}

iAQCropLabel::iAQCropLabel(QString const& text, QString const& qssClass)
{
	setup(text);
	if (!qssClass.isEmpty())
	{
		m_label->setProperty("qssClass", qssClass);
	}
}

void iAQCropLabel::setup(QString const& text)
{
	m_label = new QLabel(text);
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	m_label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	setFrameShape(QFrame::NoFrame); // this to remove borders; background-color: transparent; in qss to make background transparent
	setWidgetResizable(true);
	setContentsMargins(0, 0, 0, 0);
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setWidget(m_label);
	setFixedHeight(m_label->height());
}

void iAQCropLabel::setText(QString const& text)
{
	m_label->setText(text);
	setFixedHeight(m_label->height());
}
