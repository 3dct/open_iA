/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iABimodalWidget.h"

#include <iAModality.h>
#include <mdichild.h>

#include "iAInterpolationSliderWidget.h"
#include "iAHistogramStackGrid.h"

#include <QLabel>
#include <QHBoxLayout>
#include <QSplitter>

iABimodalWidget::iABimodalWidget(MdiChild *mdiChild):
	iAMultimodalWidget(mdiChild, TWO)
{
	connect(this, &iABimodalWidget::modalitiesLoaded_beforeUpdate, this, &iABimodalWidget::modalitiesLoaded_beforeUpdateSlot);
	if (isReady())
	{
		initialize();
	}
}

void iABimodalWidget::modalitiesLoaded_beforeUpdateSlot()
{
	initialize();
}

void iABimodalWidget::initialize()
{
	QVector<iAChartWithFunctionsWidget*> histograms;
	QVector<iASimpleSlicerWidget*> slicers;
	m_labels.clear();

	for (int i = 0; i < 2; i++) {
		QLabel *l = new QLabel(m_mdiChild->modality(i)->name());
		l->setStyleSheet("font-weight: bold");
		l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

		m_labels.push_back(l);
		histograms.push_back(w_histogram(i).data());
		slicers.push_back(w_slicer(i).data());
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
	m_slider->setContentsMargins(QMargins(20, 20, 20, 20));

	QWidget *wleft = new QWidget();
	QVBoxLayout *wleftl = new QVBoxLayout(wleft);
	wleftl->setSpacing(1);
	wleftl->setMargin(0);

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

	m_slider->changeModalities(getModalityImage(0), getModalityImage(1));
}

void iABimodalWidget::tChanged(double t)
{
	setWeightsProtected(t);
}

void iABimodalWidget::modalitiesChanged()
{
	for (int i = 0; i < 2; i++)
		m_labels[i]->setText(m_mdiChild->modality(i)->name());
}
