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

#include "iATripleModalityHistograms.h"
#include "RightBorderLayout.h"
#include "dlg_modalities.h"
#include "iAChannelVisualizationData.h"
#include "mdiChild.h"
#include "iAModalityTransfer.h"
#include "iAModalityList.h"

#include "vtkImageData.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSmartPointer.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QString>

// Debug
#include <QDebug>

// Required for the histogram
#include "iAModality.h"
#include "charts/iAHistogramData.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAProfileWidget.h"
#include "iAPreferences.h"
#include "dlg_transfer.h"

// Required to create slicer
#include "vtkColorTransferFunction.h"
#include "vtkCamera.h"
#include "vtkImageActor.h"
#include "iASlicerData.h"

//static const char *WEIGHT_FORMAT = "%.10f";
static const QString DISABLED_TEXT_COLOR = "rgb(0,0,0)"; // black
static const QString DISABLED_BACKGROUND_COLOR = "rgba(255,255,255)"; // white

iATripleModalityHistograms::iATripleModalityHistograms(QWidget * parent, MdiChild *mdiChild, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f)
{
	m_mdiChild = mdiChild;

	m_disabledLabel = new QLabel();
	m_disabledLabel->setAlignment(Qt::AlignCenter);
	m_disabledLabel->setStyleSheet("background-color: " + DISABLED_BACKGROUND_COLOR + "; color: " + DISABLED_TEXT_COLOR);

	mdiChild->getSlicerDataXY()->GetImageActor()->SetOpacity(0.0);
	mdiChild->getSlicerDataXZ()->GetImageActor()->SetOpacity(0.0);
	mdiChild->getSlicerDataYZ()->GetImageActor()->SetOpacity(0.0);

	//setStyleSheet("background-color:red"); // test spacing/padding/margin
}

iATripleModalityHistograms::~iATripleModalityHistograms()
{
	if (m_copyTFs[0]) delete m_copyTFs[0];
	if (m_copyTFs[1]) delete m_copyTFs[1];
	if (m_copyTFs[2]) delete m_copyTFs[2];
}

void iATripleModalityHistograms::setWeight(BCoord bCoord)
{
	if (bCoord == m_weightCur) {
		return;
	}

	//QString text;
	//m_weightLabels[0]->setText(text.sprintf(WEIGHT_FORMAT, bCoord.getAlpha()));
	//m_weightLabels[1]->setText(text.sprintf(WEIGHT_FORMAT, bCoord.getBeta()));
	//m_weightLabels[2]->setText(text.sprintf(WEIGHT_FORMAT, bCoord.getGamma()));

	m_weightCur = bCoord;
	applyWeights();
	emit transferFunctionChanged();
}

void iATripleModalityHistograms::setSlicerMode(iASlicerMode slicerMode, int dimensionLength)
{
	m_slicerMode = slicerMode;
	if (isReady()) {
		m_slicerWidgets[0]->setSlicerMode(slicerMode, dimensionLength);
		m_slicerWidgets[1]->setSlicerMode(slicerMode, dimensionLength);
		m_slicerWidgets[2]->setSlicerMode(slicerMode, dimensionLength);
	}
}

void iATripleModalityHistograms::setSlicerMode(iASlicerMode slicerMode)
{
	m_slicerMode = slicerMode;
	if (isReady()) {
		m_slicerWidgets[0]->setSlicerMode(slicerMode);
		m_slicerWidgets[1]->setSlicerMode(slicerMode);
		m_slicerWidgets[2]->setSlicerMode(slicerMode);
	}
}

void iATripleModalityHistograms::setSliceNumber(int sliceNumber)
{
	m_sliceNumber = sliceNumber;
	if (isReady()) {
		m_slicerWidgets[0]->setSliceNumber(sliceNumber);
		m_slicerWidgets[1]->setSliceNumber(sliceNumber);
		m_slicerWidgets[2]->setSliceNumber(sliceNumber);
	}
}

void iATripleModalityHistograms::resizeEvent(QResizeEvent* event)
{
	if (isReady()) {
		resized(event->size().width(), event->size().height());
	}
}

void iATripleModalityHistograms::updateTransferFunction(int index)
{
	updateOriginalTransferFunction(index);
	m_slicerWidgets[index]->update();
	m_histograms[index]->redraw();
	emit transferFunctionChanged();
}

void iATripleModalityHistograms::setModalityLabel(QString label, int index)
{
	// TODO change triangle's labels
}


QSharedPointer<iAModality> iATripleModalityHistograms::getModality(int index)
{
	return m_modalitiesActive[index];
}

double iATripleModalityHistograms::getWeight(int index)
{
	return m_weightCur[index];
}

// When new modalities are added/removed
void iATripleModalityHistograms::updateModalities()
{
	if (m_mdiChild->GetModalities()->size() >= 3) {
		if (containsModality(m_mdiChild->GetModality(0)) &&
			containsModality(m_mdiChild->GetModality(1)) &&
			containsModality(m_mdiChild->GetModality(2))) {

			return;
		}
	} else {
		int i = 0;
		for (; i < 3 && i < m_mdiChild->GetModalities()->size(); ++i) {
			m_modalitiesActive[i] = m_mdiChild->GetModality(i);
		}
		for (; i < 3; i++) {
			m_modalitiesActive[i] = nullptr;
			//if (m_weightLabels[i]) delete m_weightLabels[i];
			//if (m_modalityLabels[i]) delete m_modalityLabels[i];
			//if (m_slicerWidgets[i]) delete m_slicerWidgets[i];
			//if (m_histograms[i]) delete m_histograms[i];
		}
		return;
	}

	// Initialize modalities being added
	for (int i = 0; i < 3; ++i) {
		m_modalitiesActive[i] = m_mdiChild->GetModality(i);

		// Histogram {
		if (!m_modalitiesActive[i]->GetHistogramData() || m_modalitiesActive[i]->GetHistogramData()->GetNumBin() != m_mdiChild->GetPreferences().HistogramBins)
		{
			m_modalitiesActive[i]->ComputeImageStatistics();
			m_modalitiesActive[i]->ComputeHistogramData(m_mdiChild->GetPreferences().HistogramBins);
		}

		vtkColorTransferFunction *colorFuncCopy = vtkColorTransferFunction::New(); // TODO delete?
		vtkPiecewiseFunction *opFuncCopy = vtkPiecewiseFunction::New(); // TODO delete?
		m_copyTFs[i] = createCopyTf(i, colorFuncCopy, opFuncCopy); //new iASimpleTransferFunction(colorFuncCopy, opFuncCopy);

		m_histograms[i] = new iADiagramFctWidget(this, m_mdiChild);
		QSharedPointer<iAPlot> histogramPlot = QSharedPointer<iAPlot>(
			new	iABarGraphDrawer(m_modalitiesActive[i]->GetHistogramData(), QColor(70, 70, 70, 255)));
		m_histograms[i]->AddPlot(histogramPlot);
		m_histograms[i]->SetTransferFunctions(m_copyTFs[i]->GetColorFunction(), m_copyTFs[i]->GetOpacityFunction());
		m_histograms[i]->updateTrf();
		// }

		// Slicer {
		m_slicerWidgets[i] = new iASimpleSlicerWidget(this);
		m_slicerWidgets[i]->changeModality(m_modalitiesActive[i]);
		m_slicerWidgets[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
		// }

		iAChannelID id = static_cast<iAChannelID>(ch_Meta0 + i);
		iAChannelVisualizationData* chData = new iAChannelVisualizationData();
		m_mdiChild->InsertChannelData(id, chData);
		vtkImageData* imageData = m_modalitiesActive[i]->GetImage();
		vtkColorTransferFunction* ctf = m_modalitiesActive[i]->GetTransfer()->GetColorFunction();
		vtkPiecewiseFunction* otf = m_modalitiesActive[i]->GetTransfer()->GetOpacityFunction();
		ResetChannel(chData, imageData, ctf, otf);
		m_mdiChild->InitChannelRenderer(id, false, true);
	}

	connect((dlg_transfer*)(m_histograms[0]->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction1()));
	connect((dlg_transfer*)(m_histograms[1]->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction2()));
	connect((dlg_transfer*)(m_histograms[2]->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction3()));

	// Pure virtual method
	initialize();

	setSlicerMode(m_slicerMode);
	setSliceNumber(m_sliceNumber);
	
	applyWeights();
	connect((dlg_transfer*)(m_mdiChild->getHistogram()->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(originalHistogramChanged()));
	emit modalitiesChanged(m_modalitiesActive[0], m_modalitiesActive[1], m_modalitiesActive[2]);
}

bool iATripleModalityHistograms::isReady()
{
	return m_modalitiesActive[2];
}


bool iATripleModalityHistograms::containsModality(QSharedPointer<iAModality> modality)
{
	return m_modalitiesActive[0] == modality || m_modalitiesActive[1] == modality || m_modalitiesActive[2] == modality;
}

int iATripleModalityHistograms::getModalitiesCount()
{
	return m_modalitiesActive[2] ? 3 : (m_modalitiesActive[1] ? 2 : (m_modalitiesActive[0] ? 1 : 0));
}

iATransferFunction* iATripleModalityHistograms::createCopyTf(int index, vtkSmartPointer<vtkColorTransferFunction> colorTf, vtkSmartPointer<vtkPiecewiseFunction> opacityFunction)
{
	colorTf->DeepCopy(m_modalitiesActive[index]->GetTransfer()->GetColorFunction());
	opacityFunction->DeepCopy(m_modalitiesActive[index]->GetTransfer()->GetOpacityFunction());
	return new iASimpleTransferFunction(colorTf, opacityFunction);
}

void iATripleModalityHistograms::originalHistogramChanged()
{
	QSharedPointer<iAModality> selected = m_mdiChild->GetModalitiesDlg()->GetModalities()->Get(m_mdiChild->GetModalitiesDlg()->GetSelected());
	int index;
	if (selected == m_modalitiesActive[0]) {
		index = 0;
	}
	else if (selected == m_modalitiesActive[1]) {
		index = 1;
	}
	else if (selected == m_modalitiesActive[2]) {
		index = 2;
	} else {
		return;
	}
	updateCopyTransferFunction(index);
	updateTransferFunction(index);
}

/** Called when the original transfer function changes
  * RESETS THE COPY (admit numerical imprecision when setting the copy values)
  * => effective / weight = copy
  */
void iATripleModalityHistograms::updateCopyTransferFunction(int index)
{
	if (isReady()) {
		double weight = m_weightCur[index];

		// newly set transfer function (set via the histogram)
		QSharedPointer<iAModalityTransfer> effective = m_modalitiesActive[index]->GetTransfer();

		// copy of previous transfer function, to be updated in this method
		iATransferFunction *copy = m_copyTFs[index];

		double valCol[6], valOp[4];
		copy->GetColorFunction()->RemoveAllPoints();
		copy->GetOpacityFunction()->RemoveAllPoints();

		for (int j = 0; j < effective->GetColorFunction()->GetSize(); ++j)
		{
			effective->GetColorFunction()->GetNodeValue(j, valCol);
			effective->GetOpacityFunction()->GetNodeValue(j, valOp);

			if (valOp[1] > weight) {
				valOp[1] = weight;
			}
			double copyOp = valOp[1] / weight;

			effective->GetOpacityFunction()->SetNodeValue(j, valOp);

			copy->GetColorFunction()->AddRGBPoint(valCol[0], valCol[1], valCol[2], valCol[3], valCol[4], valCol[5]);
			copy->GetOpacityFunction()->AddPoint(valOp[0], copyOp, valOp[2], valOp[3]);
		}
	}
}

/** Called when the copy transfer function changes
  * ADD NODES TO THE EFFECTIVE ONLY (clear and repopulate with adjusted effective values)
  * => copy * weight = effective
  */
void iATripleModalityHistograms::updateOriginalTransferFunction(int index)
{
	if (isReady()) {
		double weight = m_weightCur[index];

		// newly set transfer function (set via the histogram)
		QSharedPointer<iAModalityTransfer> effective = m_modalitiesActive[index]->GetTransfer();

		// copy of previous transfer function, to be updated in this method
		iATransferFunction *copy = m_copyTFs[index];

		double valCol[6], valOp[4];
		effective->GetColorFunction()->RemoveAllPoints();
		effective->GetOpacityFunction()->RemoveAllPoints();

		for (int j = 0; j < copy->GetColorFunction()->GetSize(); ++j)
		{
			copy->GetColorFunction()->GetNodeValue(j, valCol);
			copy->GetOpacityFunction()->GetNodeValue(j, valOp);

			valOp[1] = valOp[1] * weight; // index 1 means opacity

			effective->GetColorFunction()->AddRGBPoint(valCol[0], valCol[1], valCol[2], valCol[3], valCol[4], valCol[5]);
			effective->GetOpacityFunction()->AddPoint(valOp[0], valOp[1], valOp[2], valOp[3]);
		}
	}
}

/** Resets the values of all nodes in the effective transfer function using the values present in the
  *     copy of the transfer function, using m_weightCur for the adjustment
  * CHANGES THE NODES OF THE EFFECTIVE ONLY (based on the copy)
  */
void iATripleModalityHistograms::applyWeights()
{
	if (isReady()) {
		for (int i = 0; i < 3; ++i)
		{
			vtkPiecewiseFunction *effective = m_modalitiesActive[i]->GetTransfer()->GetOpacityFunction();
			vtkPiecewiseFunction *copy = m_copyTFs[i]->GetOpacityFunction();

			double pntVal[4];
			for (int j = 0; j < copy->GetSize(); ++j)
			{
				copy->GetNodeValue(j, pntVal);
				pntVal[1] = pntVal[1] * m_weightCur[i]; // index 1 in pntVal means opacity
				effective->SetNodeValue(j, pntVal);
			}
			m_histograms[i]->redraw();

			iAChannelID id = static_cast<iAChannelID>(ch_Meta0 + i);
			//m_mdiChild->UpdateChannelSlicerOpacity(id, m_weightCur[i]);
			m_mdiChild->getSlicerDataXY()->updateChannelMappers();
			m_mdiChild->getSlicerDataXZ()->updateChannelMappers();
			m_mdiChild->getSlicerDataYZ()->updateChannelMappers();
			m_mdiChild->updateSlicers();
		}
	}
}