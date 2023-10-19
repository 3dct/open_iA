// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAOrientationWidget.h"

#include <QPainter>

const double minPixelSize = 2.0;

iAOrientationWidget::iAOrientationWidget(QWidget* parent) : QOpenGLWidget(parent)
{
	setMaximumWidth(100);
	setFixedHeight(27);
	setBackgroundRole(QPalette::Base);
	setAutoFillBackground(true);
	QSurfaceFormat format;
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
	initializeOpenGLFunctions();
	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
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
	if (w < minPixelSize)
	{
		w = minPixelSize;
	}
	if (h < minPixelSize)
	{
		h = minPixelSize;
	}
	painter.drawRect(x, y, w, h);
}
