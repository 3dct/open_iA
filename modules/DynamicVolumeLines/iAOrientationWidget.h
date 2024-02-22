// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <qcustomplot.h>

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

class iAOrientationWidget : public QOpenGLWidget, public QOpenGLFunctions
{
	Q_OBJECT

public:
	iAOrientationWidget(QWidget* parent = 0);

	void update(QCustomPlot* plot, double lowerX, double upperX, double lowerY, double upperY);

	QSize minimumSizeHint() const override;
	QSize sizeHint() const override;

protected:
	void initializeGL() override;
	void paintGL() override;

private:
	QCustomPlot *m_plot;
	double m_lowerLimitX, m_upperLimitX, m_lowerLimitY, m_upperLimitY;
};
