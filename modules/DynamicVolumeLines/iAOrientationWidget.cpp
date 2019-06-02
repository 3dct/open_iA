/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iAOrientationWidget.h"

#include <QPainter>

const double minPixelSize = 2.0;

iAOrientationWidget::iAOrientationWidget(QWidget* parent) : iAQGLWidget(parent)
{
	setMaximumWidth(100);
	setFixedHeight(27);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	iAQGLFormat format;
	format.setSamples(4);
	this->setFormat(format);
	this->setToolTip("The Orientation Widget shows the visible plot area\nin blue (the rest in gray)");
}

QSize iAOrientationWidget::sizeHint() const
{
	return QSize(80, 27);
}

QSize iAOrientationWidget::minimumSizeHint() const
{
	return QSize(40, 10);
}

void iAOrientationWidget::initializeGL()
{
	glClearColor(0.3, 0.3, 0.3, 1.0);
}

void iAOrientationWidget::update(QCustomPlot* plot, double lowerX, double upperX,
	double lowerY, double upperY)
{
	m_plot = plot;
	m_lowerLimitX = lowerX;
	m_upperLimitX = upperX;
	m_lowerLimitY = lowerY;
	m_upperLimitY = upperY;
}

void iAOrientationWidget::paintGL()
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setBrush(QColor(51,153,255));
	painter.setPen(Qt::NoPen);
	double x = m_plot->xAxis->range().lower * width() / (m_upperLimitX - m_lowerLimitX);
	double y = (m_upperLimitY - m_plot->yAxis->range().upper) * height() / (m_upperLimitY - m_lowerLimitY);
	double w = m_plot->xAxis->range().size() * width() / (m_upperLimitX - m_lowerLimitX);
	double h = m_plot->yAxis->range().size() * height() / (m_upperLimitY - m_lowerLimitY);	
	if (w < minPixelSize) w = minPixelSize;
	if (h < minPixelSize) h = minPixelSize;
	painter.drawRect(x, y, w, h);
}