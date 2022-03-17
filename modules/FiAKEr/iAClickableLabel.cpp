/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iAClickableLabel.h"

#include <QApplication>    // for qApp->palette()
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
		painter.setPen(qApp->palette().color(QPalette::Text));
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
		painter.translate( painter.fontMetrics().height(), (geometry().height() + painter.fontMetrics().horizontalAdvance(text())) / 2 );
#else
		painter.translate( painter.fontMetrics().height(), (geometry().height() + painter.fontMetrics().width(text())) / 2 );
#endif
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
