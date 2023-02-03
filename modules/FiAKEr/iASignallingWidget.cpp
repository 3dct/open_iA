// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASignallingWidget.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

void iASignallingWidget::mouseDoubleClickEvent(QMouseEvent*)
{
	emit dblClicked();
}

void iASignallingWidget::mouseReleaseEvent(QMouseEvent* ev)
{
	emit clicked(ev->button(), ev->modifiers());
}

void iASignallingWidget::paintEvent(QPaintEvent* /*ev*/)
{
	QPainter painter(this);
	painter.fillRect(rect(), QApplication::palette().color(QWidget::backgroundRole()));
}
