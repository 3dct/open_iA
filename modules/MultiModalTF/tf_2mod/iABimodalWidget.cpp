// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iABimodalWidget.h"

#include <iAMdiChild.h>

#include "iAInterpolationSliderWidget.h"
#include "iAHistogramStackGrid.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QSplitter>

iABimodalWidget::iABimodalWidget(iAMdiChild *mdiChild):
	iAMultimodalWidget(mdiChild, TWO)
{
	connect(this, &iABimodalWidget::dataSetsLoaded_beforeUpdate, this, &iABimodalWidget::dataSetsLoaded_beforeUpdateSlot);
	if (isReady())
	{
		initialize();
	}
}

void iABimodalWidget::dataSetsLoaded_beforeUpdateSlot()
{
	initialize();
}

void iABimodalWidget::initialize()
{
	QVector<iAChartWithFunctionsWidget*> histograms;
	QVector<iASimpleSlicerWidget*> slicers;
	m_labels.clear();

	for (int i = 0; i < 2; i++)
	{
		QLabel *l = new QLabel(dataSetName(i));
		l->setStyleSheet("font-weight: bold");
		l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

		m_labels.push_back(l);
		histograms.push_back(w_histogram(i));
		slicers.push_back(w_slicer(i));
	}

	QWidget *optionsContainer = new QWidget();
	optionsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QHBoxLayout *optionsContainerLayout = new QHBoxLayout(optionsContainer);
	optionsContainerLayout->addStretch();
	optionsContainerLayout->addWidget(w_slicerModeLabel());
	optionsContainerLayout->addWidget(w_sliceNumberLabel());

	//QWidget *wmain = new QWidget();
	//QHBoxLayout *wmainl = new QHBoxLayout(wmain);

	//m_slider = new iAInterpolationSlider(Qt::Vertical, wmain);
	m_slider = new iAInterpolationSliderWidget();
	m_slider->setContentsMargins(20, 20, 20, 20);

	QWidget *wleft = new QWidget();
	QVBoxLayout *wleftl = new QVBoxLayout(wleft);
	wleftl->setSpacing(1);
	wleftl->setContentsMargins(0, 0, 0, 0);

	auto grid = new iAHistogramStackGrid(wleft, histograms, slicers, m_labels);

	wleftl->addWidget(optionsContainer, 0);
	wleftl->addWidget(grid, 1);

	//wmainl->addWidget(wleft, 1);
	//wmainl->addWidget(m_slider, 0);

	QSplitter *splitter = new QSplitter(Qt::Horizontal);
	splitter->addWidget(wleft);
	splitter->addWidget(m_slider);
	splitter->setStretchFactor(1, 0);

	m_innerLayout->addWidget(splitter);

	grid->adjustStretch();

	connect(m_slider, &iAInterpolationSliderWidget::tChanged, this, &iABimodalWidget::tChanged);
	tChanged(m_slider->getT());

	m_slider->changeModalities(dataSetImage(0), dataSetImage(1));
}

void iABimodalWidget::tChanged(double t)
{
	setWeightsProtected(t);
}

void iABimodalWidget::dataSetChanged(size_t dataSetIdx)
{
	Q_UNUSED(dataSetIdx);
	for (int i = 0; i < 2; i++)
	{
		m_labels[i]->setText(dataSetName(i));
	}
}
