// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAClickableLabel.h"

#include <QApplication>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>

iAClickableLabel::iAClickableLabel(QString const& text, bool vertical):
	QLabel(text), m_vertical(vertical)
{}

void iAClickableLabel::mouseDoubleClickEvent(QMouseEvent* ev)
{
	   Q_UNUSED(ev);
	   emit dblClicked();
}

void iAClickableLabel::mouseReleaseEvent(QMouseEvent* ev)
{
	   Q_UNUSED(ev);
	   emit clicked();
}

void iAClickableLabel::paintEvent(QPaintEvent* ev)
{
	if (m_vertical)
	{
		QPainter painter(this);
		painter.setPen(QApplication::palette().color(QPalette::Text));
		painter.translate( painter.fontMetrics().height(), (geometry().height() + painter.fontMetrics().horizontalAdvance(text())) / 2 );
		painter.rotate(270);
		painter.drawText(0, 0, text());
	}
	else
	{
		QLabel::paintEvent(ev);
	}
}

QSize iAClickableLabel::minimumSizeHint() const
{
	if (m_vertical)
	{
		QSize s = QLabel::minimumSizeHint();
		return QSize(s.height(), s.height());
	}
	else
	{
		return QLabel::minimumSizeHint();
	}
}

QSize iAClickableLabel::sizeHint() const
{
	if (m_vertical)
	{
		QSize s = QLabel::sizeHint();
		return QSize(s.height(), s.height());
	}
	else
	{
		return QLabel::sizeHint();
	}
}
