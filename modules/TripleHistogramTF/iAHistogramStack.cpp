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
#include "RightBorderLayout.h"

#include "vtkImageData.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QString>

// Debug
#include <QDebug>

// Required for the histogram
#include "iAModalityList.h"
#include "iAModality.h"
#include "iAModalityTransfer.h"
#include "charts/iAHistogramData.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAProfileWidget.h"
#include "iAPreferences.h"
#include "dlg_transfer.h"

// Required to create slicer
#include "vtkColorTransferFunction.h"
#include "vtkCamera.h"
#include "iASlicerData.h"

// TODO: does this belong here?
static const char *m_weightFormat = "%.10f";

iAHistogramStack::iAHistogramStack(QWidget * parent, MdiChild *mdiChild, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f)
{
	m_mainLayout = new QGridLayout(this);

	//iATransferFunction *tfs[3];
	for (int i = 0; i < 3; ++i) {
		//m_modalities[i] = mdiChild->GetModality(i);
		//vtkImageData *imageData = modality->GetImage().GetPointer();

		// Slicer, label and weight {
		QWidget *rightWidget = new QWidget(this);
		QVBoxLayout *rightWidgetLayout = new QVBoxLayout(rightWidget);

		m_slicerWidgets[i] = new iASimpleSlicerWidget(rightWidget);
		m_weightLabels[i] = new QLabel(rightWidget);

		m_modalityLabels[i] = new QLabel(m_labels[i]);
		m_modalityLabels[i]->setStyleSheet("font-weight: bold");

		rightWidgetLayout->addWidget(m_slicerWidgets[i]);
		rightWidgetLayout->addWidget(m_modalityLabels[i]);
		rightWidgetLayout->addWidget(m_weightLabels[i]);
		
		m_slicerWidgets[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
		m_modalityLabels[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_weightLabels[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		// }

		// Add to (grid) layout {
		//m_mainLayout->addWidget(m_histograms[i], i, 0);
		m_mainLayout->addWidget(rightWidget, i, 1);
		// }
	}
	//m_transferFunction = new iAWeightedTransfer(tfs[0], tfs[1], tfs[2]);
}

iAHistogramStack::~iAHistogramStack()
{
	delete m_slicerWidgets[0];
	delete m_slicerWidgets[1];
	delete m_slicerWidgets[2];
	delete m_histograms[0];
	delete m_histograms[1];
	delete m_histograms[2];
}

void iAHistogramStack::setWeight(BCoord bCoord)
{
	QString text;
	m_weightLabels[0]->setText(text.sprintf(m_weightFormat, bCoord.getAlpha()));
	m_weightLabels[1]->setText(text.sprintf(m_weightFormat, bCoord.getBeta()));
	m_weightLabels[2]->setText(text.sprintf(m_weightFormat, bCoord.getGamma()));
}

void iAHistogramStack::setSlicerMode(iASlicerMode slicerMode, int dimensionLength)
{
	m_slicerWidgets[0]->changeMode(slicerMode, dimensionLength);
	m_slicerWidgets[1]->changeMode(slicerMode, dimensionLength);
	m_slicerWidgets[2]->changeMode(slicerMode, dimensionLength);
}

void iAHistogramStack::setSliceNumber(int sliceNumber)
{
	m_slicerWidgets[0]->setSliceNumber(sliceNumber);
	m_slicerWidgets[1]->setSliceNumber(sliceNumber);
	m_slicerWidgets[2]->setSliceNumber(sliceNumber);
}

void iAHistogramStack::resizeEvent(QResizeEvent* event)
{
	int slicerHeight = m_slicerWidgets[0]->size().height();
	m_mainLayout->setColumnStretch(0, event->size().width() - slicerHeight);
	m_mainLayout->setColumnStretch(1,					      slicerHeight);
}

iAWeightedTransfer* iAHistogramStack::getTransferFunction()
{
	return m_transferFunction;
}

void iAHistogramStack::updateTransferFunction(int index)
{
	m_slicerWidgets[index]->update();
	emit transferFunctionChanged();
}

void iAHistogramStack::setModalityLabel(QString label, int index)
{
	m_modalityLabels[index]->setText(label);
}

QSharedPointer<iAModality> iAHistogramStack::getModality(int index)
{
	return m_modalities[index];
}

void iAHistogramStack::removeModality(QSharedPointer<iAModality> modality, MdiChild* mdiChild)
{
	m_modalities2.removeOne(modality);
	updateModalities(mdiChild);
}

void iAHistogramStack::addModality(QSharedPointer<iAModality> modality, MdiChild* mdiChild)
{
	m_modalities2.append(modality);
	updateModalities(mdiChild);
}

void iAHistogramStack::updateModalities(MdiChild* mdiChild)
{
	if (m_modalities2.size() == 3) {
		if (m_modalities2.contains(m_modalities[0]) &&
			m_modalities2.contains(m_modalities[1]) &&
			m_modalities2.contains(m_modalities[2])) {

			return;
		}
	} else {
		return;
	}

	for (int i = 0; i < 3; ++i) {
		if (!m_modalities2.contains(m_modalities[i])) {
			//delete m_modalities[i];
			delete m_histograms[i];
		}
	}

	iATransferFunction *tfs[3];
	for (int i = 0; i < 3; ++i) {
		// Slicer {
		m_slicerWidgets[i]->changeModality(m_modalities2[i]);
		// }

		// Histogram {
		if (!m_modalities[i]->GetHistogramData() || m_modalities[i]->GetHistogramData()->GetNumBin() != mdiChild->GetPreferences().HistogramBins)
		{
			m_modalities[i]->ComputeImageStatistics();
			m_modalities[i]->ComputeHistogramData(mdiChild->GetPreferences().HistogramBins);
		}

		m_histograms[i] = new iADiagramFctWidget(this, mdiChild);
		QSharedPointer<iAPlot> histogramPlot = QSharedPointer<iAPlot>(
			new	iABarGraphDrawer(m_modalities[i]->GetHistogramData(), QColor(70, 70, 70, 255)));
		m_histograms[i]->AddPlot(histogramPlot);
		m_histograms[i]->SetTransferFunctions(m_modalities[i]->GetTransfer()->GetColorFunction(),
			m_modalities[i]->GetTransfer()->GetOpacityFunction());
		m_histograms[i]->updateTrf();

		m_mainLayout->addWidget(m_histograms[i], i, 0);

		connect((dlg_transfer*)(m_histograms[i]->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction(i)));
		// }

		// Transfer function {
		if (m_histograms[i]->getSelectedFunction()->getType() != dlg_function::TRANSFER) {
			tfs[i] = 0;
		}
		else {
			dlg_transfer* transfer = (dlg_transfer*)m_histograms[i]->getSelectedFunction();
			tfs[i] = new iASimpleTransferFunction(transfer->GetColorFunction(), transfer->GetOpacityFunction());
		}
		// }

		m_modalities[i] = m_modalities2[i];
	}
	m_transferFunction = new iAWeightedTransfer(tfs[0], tfs[1], tfs[2]);
}