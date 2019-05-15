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

#include "iABimodalWidget.h"

#include "iAInterpolationSliderWidget.h"
#include "iAHistogramStackGrid.h"

#include <QLabel>
#include <QHBoxLayout>

iABimodalWidget::iABimodalWidget(QWidget *parent, MdiChild *mdiChild)
	:
	iAMultimodalWidget(parent, mdiChild, TWO)
{
	connect(this, SIGNAL(modalitiesLoaded_beforeUpdate()), this, SLOT(modalitiesLoaded_beforeUpdateSlot()));
	if (isReady()) {
		initialize();
	}
}

void iABimodalWidget::modalitiesLoaded_beforeUpdateSlot()
{
	initialize();
}

void iABimodalWidget::initialize()
{
	QString strings[2] = { "A", "B" };
	QVector<QLabel*> labels;
	QVector<iADiagramFctWidget*> histograms;
	QVector<iASimpleSlicerWidget*> slicers;

	for (int i = 0; i < 2; i++) {
		QLabel *l = new QLabel(strings[i]);
		l->setStyleSheet("font-weight: bold");
		l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

		labels.push_back(l);
		histograms.push_back(w_histogram(i).data());
		slicers.push_back(w_slicer(i).data());
	}

	QWidget *optionsContainer = new QWidget();
	optionsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QHBoxLayout *optionsContainerLayout = new QHBoxLayout(optionsContainer);
	optionsContainerLayout->addWidget(w_slicerModeComboBox());
	optionsContainerLayout->addWidget(w_sliceNumberSlider());

	QWidget *wmain = new QWidget();
	QHBoxLayout *wmainl = new QHBoxLayout(wmain);

	//m_slider = new iAInterpolationSlider(Qt::Vertical, wmain);
	m_slider = new iAInterpolationSlider(wmain);
	m_slider->setContentsMargins(QMargins(20, 20, 20, 20));

	QWidget *wleft = new QWidget(wmain);
	QVBoxLayout *wleftl = new QVBoxLayout(wleft);
	wleftl->setSpacing(1);
	wleftl->setMargin(0);

	auto grid = new iAHistogramStackGrid(wleft, histograms, slicers, labels);

	wleftl->addWidget(optionsContainer, 0);
	wleftl->addWidget(grid, 1);

	wmainl->addWidget(wleft, 1);
	wmainl->addWidget(m_slider, 0);

	m_innerLayout->addWidget(wmain);

	grid->adjustStretch();
	m_slider->setT(0.5);

	connect(m_slider, SIGNAL(tChanged(double)), this, SLOT(tChanged(double)));
	tChanged(m_slider->getT());
}

void iABimodalWidget::tChanged(double t)
{
	setWeightsProtected(t);
}