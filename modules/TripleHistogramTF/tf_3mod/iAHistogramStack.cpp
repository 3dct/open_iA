// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAHistogramStack.h"

#include "iATripleModalityWidget.h"
#include "iABarycentricTriangleWidget.h"
#include "iASimpleSlicerWidget.h"
#include "iAHistogramStackGrid.h"

#include <iATransferFunction.h>
#include <iAChartWithFunctionsWidget.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QSplitter>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QVector>
#include <QCheckBox>

iAHistogramStack::iAHistogramStack(iATripleModalityWidget* tripleModalityWidget):
	m_splitter(nullptr),
	m_grid(nullptr),
	m_tmw(tripleModalityWidget)
{
}

void iAHistogramStack::initialize(std::array<QString, 3> const names)
{
	for (int i = 0; i < 3; i++)
	{
		auto l = new QLabel(names[i]);
		l->setStyleSheet("font-weight: bold; font-size: 10pt;");
		l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_labels.push_back(l);
	}

	QVector<iAChartWithFunctionsWidget*> histograms;
	QVector<iASimpleSlicerWidget*> slicers;
	for (int i = 0; i < 3; ++i)
	{
		histograms.push_back(m_tmw->w_histogram(i));
		slicers.push_back(m_tmw->w_slicer(i));
	}

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
	leftWidgetLayout->setContentsMargins(0, 0, 0, 0);
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

void iAHistogramStack::updateDataSetNames(std::array<QString, 3> names)
{
	for (int i = 0; i < m_labels.size(); i++)
	{
		m_labels[i]->setText(names[i]);
	}
}
