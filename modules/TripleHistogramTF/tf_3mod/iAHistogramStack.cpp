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

#include "iAHistogramStack.h"

#include "iATripleModalityWidget.h"
#include "iABarycentricTriangleWidget.h"
#include "iASimpleSlicerWidget.h"
#include "iAHistogramStackGrid.h"

#include <iATransferFunction.h>
#include <charts/iAChartWithFunctionsWidget.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSplitter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QVector>
#include <QCheckBox>

iAHistogramStack::iAHistogramStack(QWidget* parent, iATripleModalityWidget *tripleModalityWidget, MdiChild *mdiChild, Qt::WindowFlags f)
	: m_tmw(tripleModalityWidget)
{
}

void iAHistogramStack::initialize(QString const names[3])
{
	for (int i = 0; i < 3; i++) {
		auto l = new QLabel(names[i]);
		l->setStyleSheet("font-weight: bold; font-size: 10pt;");
		l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_labels.push_back(l);
	}

	QVector<iAChartWithFunctionsWidget*> histograms;
	histograms.push_back(m_tmw->w_histogram(0).data());
	histograms.push_back(m_tmw->w_histogram(1).data());
	histograms.push_back(m_tmw->w_histogram(2).data());

	QVector<iASimpleSlicerWidget*> slicers;
	slicers.push_back(m_tmw->w_slicer(0).data());
	slicers.push_back(m_tmw->w_slicer(1).data());
	slicers.push_back(m_tmw->w_slicer(2).data());

	m_tmw->w_triangle()->recalculatePositions();

	QWidget *optionsContainer = new QWidget();
	//optionsContainer->setStyleSheet("background-color:blue"); // test spacing/padding/margin
	optionsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QHBoxLayout *optionsContainerLayout = new QHBoxLayout(optionsContainer);
	optionsContainerLayout->addWidget(m_tmw->w_layoutComboBox());
	optionsContainerLayout->addWidget(m_tmw->w_checkBox_weightByOpacity());
	optionsContainerLayout->addWidget(m_tmw->w_checkBox_syncedCamera());
	optionsContainerLayout->addStretch();
	optionsContainerLayout->addWidget(m_tmw->w_slicerModeLabel());
	optionsContainerLayout->addWidget(m_tmw->w_sliceNumberLabel());
	
	m_grid = new iAHistogramStackGrid(this, histograms, slicers, m_labels);

	QWidget *leftWidget = new QWidget();
	QVBoxLayout *leftWidgetLayout = new QVBoxLayout(leftWidget);
	leftWidgetLayout->setSpacing(1);
	leftWidgetLayout->setMargin(0);
	leftWidgetLayout->addWidget(optionsContainer);
	leftWidgetLayout->addWidget(m_grid);

	m_splitter = new QSplitter(Qt::Horizontal);
	m_splitter->addWidget(leftWidget);
	m_splitter->addWidget(m_tmw->w_triangle());
	m_splitter->setStretchFactor(1, 0);

	QLayout *parentLayout = new QHBoxLayout(this); // TODO we don't need any layout here because QSplitter is the only widget... what to do?
	parentLayout->addWidget(m_splitter);

	m_grid->adjustStretch();
}

void iAHistogramStack::updateModalityNames(QString const names[3])
{
	for (int i = 0; i < m_labels.size(); i++)
		m_labels[i]->setText(names[i]);
}