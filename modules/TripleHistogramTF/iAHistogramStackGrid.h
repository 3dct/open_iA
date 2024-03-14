// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QWidget>

#include <QVector>

class iAChartWithFunctionsWidget;
class iASimpleSlicerWidget;

class QLabel;
class QGridLayout;
class QResizeEvent;

class iAHistogramStackGrid : public QWidget
{
public:
	iAHistogramStackGrid(
		QWidget *parent,
		QVector<iAChartWithFunctionsWidget*> const & histograms,
		QVector<iASimpleSlicerWidget*> const & slicers,
		QVector<QLabel*> const & labels,
		Qt::WindowFlags f = Qt::WindowFlags());

	void adjustStretch()
	{
		adjustStretch(size().width());
	}
protected:
	void resizeEvent(QResizeEvent* event) override;
private:
	void adjustStretch(int w);
	QGridLayout *m_gridLayout;
	int m_spacing = 1;
};
