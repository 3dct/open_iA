/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <QApplication>
#include <QPainter>
#include "iALinearColorGradientBar.h"

iALinearColorGradientBar::iALinearColorGradientBar(QWidget *parent, QMap<double, QColor> colormap)
	: QWidget(parent)
{
	m_colormap = colormap;
}

QSize iALinearColorGradientBar::sizeHint() const
{
	return QSize(100, 20);
}

QSize iALinearColorGradientBar::minimumSizeHint() const
{
	return QSize(100, 20);
}

void iALinearColorGradientBar::paintEvent(QPaintEvent *e)
{
	Q_UNUSED(e);
	QPainter painter(this);
	QLinearGradient grad(0.0, 0.0, width(), height());
	QMap<double, QColor>::iterator it;
	for (it = m_colormap.begin(); it != m_colormap.end(); ++it)
		grad.setColorAt(it.key(), it.value());
	painter.fillRect(0, 0, width(), height(), grad);
}