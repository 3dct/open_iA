/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
 
#include "iAHistogramStack.h"
#include "iATransferFunction.h"
#include "charts/iADiagramFctWidget.h"

iAHistogramStack::iAHistogramStack(QWidget* parent, MdiChild* mdiChild, Qt::WindowFlags f)
	: iATripleModalityHistograms(parent, mdiChild, f)
{
	m_gridLayout = new QGridLayout(this);

	for (int i = 0; i < 3; i++) {
		m_modalityLabels[i] = new QLabel(DEFAULT_MODALITY_LABELS[i]);
		m_modalityLabels[i]->setStyleSheet("font-weight: bold");
		m_modalityLabels[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	}
}

void iAHistogramStack::initialize()
{
	for (int i = 0; i < 3; i++) {
		m_gridLayout->addWidget(m_histograms[i], i, 0);
		m_gridLayout->addWidget(m_slicerWidgets[i], i, 1);
		m_gridLayout->addWidget(m_modalityLabels[i], i, 2);
	}
	m_gridLayout->setSpacing(1);
	m_gridLayout->setMargin(0);

	adjustStretch(size().width());
}

void iAHistogramStack::setModalityLabel(QString label, int index)
{
	m_modalityLabels[index]->setText(label);
	iATripleModalityHistograms::setModalityLabel(label, index);
}

void iAHistogramStack::resized(int w, int h)
{
	adjustStretch(w);
}

void iAHistogramStack::adjustStretch(int totalWidth)
{
	int histogramHeight = m_histograms[0]->size().height();
	m_gridLayout->setColumnStretch(0, totalWidth - histogramHeight);
	m_gridLayout->setColumnStretch(1, histogramHeight);
}