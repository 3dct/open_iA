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
static const char *WEIGHT_FORMAT = "%.10f";
static const QString DISABLED_TEXT_COLOR = "rgb(0,0,0)"; // black
static const QString DISABLED_BACKGROUND_COLOR = "rgba(0,0,0,127)"; // transparent black
static const QString DEFAULT_MODALITY_LABELS[3] = { "A", "B", "C" };

iAHistogramStack::iAHistogramStack(QWidget * parent, MdiChild *mdiChild, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f)
{
	m_stackedLayout = new QStackedLayout(this);
	m_stackedLayout->setStackingMode(QStackedLayout::StackAll);

	m_disabledLabel = new QLabel();
	m_disabledLabel->setAlignment(Qt::AlignCenter);
	m_disabledLabel->setStyleSheet("background-color: " + DISABLED_BACKGROUND_COLOR + "; color: " + DISABLED_TEXT_COLOR);

	QWidget *gridWidget = new QWidget(this);
	m_gridLayout = new QGridLayout(gridWidget);

	m_stackedLayout->addWidget(gridWidget);
	m_stackedLayout->addWidget(m_disabledLabel);
	m_stackedLayout->setCurrentIndex(1);

	int i;

	//iATransferFunction *tfs[3];
	for (i = 0; i < 3; ++i) {
		//m_modalitiesActive[i] = mdiChild->GetModality(i);
		//vtkImageData *imageData = modality->GetImage().GetPointer();

		// Slicer, label and weight {
		QWidget *rightWidget = new QWidget(gridWidget);
		QVBoxLayout *rightWidgetLayout = new QVBoxLayout(rightWidget);

		m_slicerWidgets[i] = new iASimpleSlicerWidget(rightWidget);
		m_weightLabels[i] = new QLabel(rightWidget);

		m_modalityLabels[i] = new QLabel(DEFAULT_MODALITY_LABELS[i]);
		m_modalityLabels[i]->setStyleSheet("font-weight: bold");

		rightWidgetLayout->addWidget(m_slicerWidgets[i]);
		rightWidgetLayout->addWidget(m_modalityLabels[i]);
		rightWidgetLayout->addWidget(m_weightLabels[i]);
		
		m_slicerWidgets[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
		m_modalityLabels[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		m_weightLabels[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
		// }

		// Add to (grid) layout {
		//m_gridLayout->addWidget(m_histograms[i], i, 0);
		m_gridLayout->addWidget(rightWidget, i, 1);
		// }
	}
	//m_transferFunction = new iAWeightedTransfer(tfs[0], tfs[1], tfs[2]);

	for (i = 0; i < mdiChild->GetModalities()->size(); ++i) {
		addModality(mdiChild->GetModality(i));
	}
	updateModalities(mdiChild);
}

iAHistogramStack::~iAHistogramStack()
{
	if (m_slicerWidgets[0]) delete m_slicerWidgets[0];
	if (m_slicerWidgets[1]) delete m_slicerWidgets[1];
	if (m_slicerWidgets[2]) delete m_slicerWidgets[2];
	if (m_histograms[0]) delete m_histograms[0];
	if (m_histograms[1]) delete m_histograms[1];
	if (m_histograms[2]) delete m_histograms[2];
}

void iAHistogramStack::setWeight(BCoord bCoord)
{
	QString text;
	m_weightLabels[0]->setText(text.sprintf(WEIGHT_FORMAT, bCoord.getAlpha()));
	m_weightLabels[1]->setText(text.sprintf(WEIGHT_FORMAT, bCoord.getBeta()));
	m_weightLabels[2]->setText(text.sprintf(WEIGHT_FORMAT, bCoord.getGamma()));

	if (isReady()) {
		double opArr[3] = { bCoord.getAlpha(), bCoord.getBeta(), bCoord.getGamma() };
		for (int i = 0; i < 3; ++i)
		{

			vtkPiecewiseFunction *opFunc = m_modalitiesActive[i]->GetTransfer()->GetOpacityFunction();
			double pntVal[4];
			for (int j = 0; j < opFunc->GetSize(); ++j)
			{
				opFunc->GetNodeValue(j, pntVal);
				pntVal[1] = opArr[i];
				opFunc->SetNodeValue(j, pntVal);
			}
			m_histograms[i]->redraw();
		}
		emit transferFunctionChanged();
	}
}

void iAHistogramStack::setSlicerMode(iASlicerMode slicerMode, int dimensionLength)
{
	if (isReady()) {
		m_slicerWidgets[0]->changeMode(slicerMode, dimensionLength);
		m_slicerWidgets[1]->changeMode(slicerMode, dimensionLength);
		m_slicerWidgets[2]->changeMode(slicerMode, dimensionLength);
	}
}

void iAHistogramStack::setSliceNumber(int sliceNumber)
{
	if (isReady()) {
		m_slicerWidgets[0]->setSliceNumber(sliceNumber);
		m_slicerWidgets[1]->setSliceNumber(sliceNumber);
		m_slicerWidgets[2]->setSliceNumber(sliceNumber);
	}
}

void iAHistogramStack::resizeEvent(QResizeEvent* event)
{
	adjustStretch(event->size().width());
}

void iAHistogramStack::adjustStretch(int totalWidth)
{
	int slicerHeight = m_slicerWidgets[0]->size().height();
	m_gridLayout->setColumnStretch(0, totalWidth - slicerHeight);
	m_gridLayout->setColumnStretch(1, slicerHeight);
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
	return m_modalitiesActive[index];
}

// Public
void iAHistogramStack::addModality(QSharedPointer<iAModality> modality, MdiChild* mdiChild)
{
	addModality(modality);
	updateModalities(mdiChild);
}
// Private
void iAHistogramStack::addModality(QSharedPointer<iAModality> modality)
{
	m_modalitiesAvailable.append(modality);
	emit modalityAdded(modality, m_modalitiesAvailable.length() - 1);
}

// Public
void iAHistogramStack::removeModality(QSharedPointer<iAModality> modality, MdiChild* mdiChild)
{
	removeModality(modality);
	updateModalities(mdiChild);
}
// Private
void iAHistogramStack::removeModality(QSharedPointer<iAModality> modality)
{
	m_modalitiesAvailable.removeOne(modality);
}

// Temporary // TODO: remove
void iAHistogramStack::updateModalities(MdiChild* mdiChild)
{
	m_modalitiesAvailable.clear();
	for (int i = 0; i < mdiChild->GetModalities()->size(); ++i) {
		addModality(mdiChild->GetModality(i));
	}
	updateModalities2(mdiChild);
}

void iAHistogramStack::updateModalities2(MdiChild* mdiChild)
{
	if (m_modalitiesAvailable.size() >= 3) {
		if (m_modalitiesAvailable.contains(m_modalitiesActive[0]) &&
			m_modalitiesAvailable.contains(m_modalitiesActive[1]) &&
			m_modalitiesAvailable.contains(m_modalitiesActive[2])) {

			return;
		}
	} else {
		if (!isReady()) {
			disable();
		}
		return;
	}

	for (int i = 0; i < m_modalitiesAvailable.size(); ++i) {
		if (!m_modalitiesActive[i].isNull() && !m_modalitiesAvailable.contains(m_modalitiesActive[i])) {
			//delete m_modalitiesActive[i];
			delete m_histograms[i];
			//delete m_slicerWidgets[i];
		}
		m_modalitiesActive[i] = m_modalitiesAvailable[i];
	}

	iATransferFunction *tfs[3];
	for (int i = 0; i < 3; ++i) {
		// Slicer {
		m_slicerWidgets[i]->changeModality(m_modalitiesAvailable[i]);
		// }

		// Histogram {
		if (!m_modalitiesActive[i]->GetHistogramData() || m_modalitiesActive[i]->GetHistogramData()->GetNumBin() != mdiChild->GetPreferences().HistogramBins)
		{
			m_modalitiesActive[i]->ComputeImageStatistics();
			m_modalitiesActive[i]->ComputeHistogramData(mdiChild->GetPreferences().HistogramBins);
		}

		m_histograms[i] = new iADiagramFctWidget(this, mdiChild);
		QSharedPointer<iAPlot> histogramPlot = QSharedPointer<iAPlot>(
			new	iABarGraphDrawer(m_modalitiesActive[i]->GetHistogramData(), QColor(70, 70, 70, 255)));
		m_histograms[i]->AddPlot(histogramPlot);
		m_histograms[i]->SetTransferFunctions(m_modalitiesActive[i]->GetTransfer()->GetColorFunction(),
			m_modalitiesActive[i]->GetTransfer()->GetOpacityFunction());
		m_histograms[i]->updateTrf();

		m_gridLayout->addWidget(m_histograms[i], i, 0);

		connect((dlg_transfer*)(m_histograms[i]->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction(i)));
		// }

		// Transfer function {
		if (m_histograms[i]->getSelectedFunction()->getType() != dlg_function::TRANSFER) {
			tfs[i] = 0; // TODO
		} else {
			dlg_transfer* transfer = (dlg_transfer*)m_histograms[i]->getSelectedFunction();
			tfs[i] = new iASimpleTransferFunction(transfer->GetColorFunction(), transfer->GetOpacityFunction());
		}
		// }

		m_modalitiesActive[i] = m_modalitiesAvailable[i];
	}
	m_transferFunction = new iAWeightedTransfer(tfs[0], tfs[1], tfs[2]);

	adjustStretch(size().width());
	enable();
}

void iAHistogramStack::enable()
{
	m_disabledLabel->hide();
}

void iAHistogramStack::disable()
{
	int modalitiesCount = m_modalitiesAvailable.size();
	QString modalit_y_ies_is_are = modalitiesCount == 2 ? "modality is" : "modalities are";
	QString nameA = modalitiesCount >= 1 ? m_modalitiesAvailable[0]->GetName() : "missing";
	QString nameB = modalitiesCount >= 2 ? m_modalitiesAvailable[1]->GetName() : "missing";
	//QString nameC = modalitiesCount >= 3 ? m_modalitiesAvailable[2]->GetName() : "missing";
	m_disabledLabel->setText(
		"Unable to set up this widget.\n" +
		QString::number(3 - modalitiesCount) + " " + modalit_y_ies_is_are + " missing.\n" +
		"\n" +
		"Modality " + DEFAULT_MODALITY_LABELS[0] + ": " + nameA + "\n" +
		"Modality " + DEFAULT_MODALITY_LABELS[1] + ": " + nameB + "\n" +
		"Modality " + DEFAULT_MODALITY_LABELS[2] + ": missing"
	);
	m_disabledLabel->show();
}

bool iAHistogramStack::isReady()
{
	return m_modalitiesActive[2];
}