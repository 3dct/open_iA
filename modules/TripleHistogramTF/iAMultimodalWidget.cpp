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

#include "iAMultimodalWidget.h"

#include "iASimpleSlicerWidget.h"

#include <charts/iADiagramFctWidget.h>
#include <charts/iAHistogramData.h>
#include <charts/iAPlotTypes.h>
#include <charts/iAProfileWidget.h>
#include <iASlicer.h>
#include <iASlicerWidget.h>
#include <iAChannelVisualizationData.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iAModalityTransfer.h>
#include <iAPreferences.h>
#include <iASlicerData.h>
#include <iASlicerMode.h>
#include <iATransferFunction.h>
#include <dlg_modalities.h>
#include <dlg_transfer.h>
#include <mdichild.h>

#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QString>
#include <QSharedPointer>

// Debug
#include <QDebug>
static const QString DEFAULT_MODALITY_LABELS[3] = { "A", "B", "C" };

//static const char *WEIGHT_FORMAT = "%.10f";
static const QString DISABLED_TEXT_COLOR = "rgb(0,0,0)"; // black
static const QString DISABLED_BACKGROUND_COLOR = "rgba(255,255,255)"; // white

iAMultimodalWidget::iAMultimodalWidget(QWidget* parent, MdiChild* mdiChild, NumberOfModalities num)
	:
	m_numOfMod(num)
{
	m_mdiChild = mdiChild;

	m_disabledLabel = new QLabel();
	m_disabledLabel->setAlignment(Qt::AlignCenter);
	m_disabledLabel->setStyleSheet("background-color: " + DISABLED_BACKGROUND_COLOR + "; color: " + DISABLED_TEXT_COLOR);

	m_slicerModeComboBox = new QComboBox();
	m_slicerModeComboBox->addItem("YZ", iASlicerMode::YZ);
	m_slicerModeComboBox->addItem("XY", iASlicerMode::XY);
	m_slicerModeComboBox->addItem("XZ", iASlicerMode::XZ);

	m_sliceSlider = new QSlider(Qt::Horizontal);
	m_sliceSlider->setMinimum(0);

	for (int i = 0; i < m_numOfMod; i++) {
		m_histograms.push_back(NULL);
		m_slicerWidgets.push_back(NULL);
		m_modalitiesActive.push_back(NULL);
		m_copyTFs.push_back(NULL);
	}

	updateModalities();

	connect(m_slicerModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slicerModeComboBoxIndexChanged(int)));
	connect(m_sliceSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));

	connect(mdiChild->getSlicerDlgXY()->verticalScrollBarXY, SIGNAL(valueChanged(int)), this, SLOT(setSliceXYScrollBar(int)));
	connect(mdiChild->getSlicerDlgXZ()->verticalScrollBarXZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceXZScrollBar(int)));
	connect(mdiChild->getSlicerDlgYZ()->verticalScrollBarYZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceYZScrollBar(int)));

	connect(mdiChild->getSlicerDlgXY()->verticalScrollBarXY, SIGNAL(sliderPressed()), this, SLOT(setSliceXYScrollBar()));
	connect(mdiChild->getSlicerDlgXZ()->verticalScrollBarXZ, SIGNAL(sliderPressed()), this, SLOT(setSliceXZScrollBar()));
	connect(mdiChild->getSlicerDlgYZ()->verticalScrollBarYZ, SIGNAL(sliderPressed()), this, SLOT(setSliceYZScrollBar()));
}



// ----------------------------------------------------------------------------------
// 
// ----------------------------------------------------------------------------------

bool iAMultimodalWidget::setSlicerMode(iASlicerMode slicerMode) {
	if (slicerMode != getSlicerMode()) {
		m_slicerModeComboBox->setCurrentIndex(m_slicerModeComboBox->findData(slicerMode));
		emit slicerModeChangedExternally(slicerMode);
		return true;
	}
	return false;
}

bool iAMultimodalWidget::setSliceNumber(int sliceNumber) {
	if (sliceNumber != getSliceNumber()) {
		m_sliceSlider->setValue(sliceNumber);
		emit sliceNumberChangedExternally(sliceNumber);
		qDebug() << "setSliceNumber" << sliceNumber;
		return true;
	}
	return false;
}

void iAMultimodalWidget::setWeightsProtected(BCoord bCoord, double t)
{
	if (bCoord == m_weights) {
		return;
	}

	m_weights = bCoord;
	applyWeights();
	emit weightsChanged2(t);
	emit weightsChanged3(bCoord);
	emit transferFunctionChanged();
}

void iAMultimodalWidget::setSlicerModeProtected(iASlicerMode slicerMode) {
	if (isReady()) {
		for (QSharedPointer<iASimpleSlicerWidget> slicer : m_slicerWidgets) {
			slicer->setSlicerMode(slicerMode);
		}

		int dimensionIndex;
		int sliceNumber;
		switch (slicerMode)
		{
		case iASlicerMode::YZ:
			dimensionIndex = 0; // X length is in position 0 in the dimensions array
			sliceNumber = m_mdiChild->getSlicerDataYZ()->getSliceNumber();
			break;
		case iASlicerMode::XZ:
			dimensionIndex = 1; // Y length is in position 1 in the dimensions array
			sliceNumber = m_mdiChild->getSlicerDataXZ()->getSliceNumber();
			break;
		case iASlicerMode::XY:
			dimensionIndex = 2; // Z length is in position 2 in the dimensions array
			sliceNumber = m_mdiChild->getSlicerDataXY()->getSliceNumber();
			break;
		default:
			// TODO exception
			return;
		}

		int dimensionLength = m_mdiChild->getImageData()->GetDimensions()[dimensionIndex];
		m_sliceSlider->setMaximum(dimensionLength - 1);
		if (!setSliceNumber(sliceNumber)) {
			sliderValueChanged(sliceNumber);
		}

		emit slicerModeChanged(slicerMode);
	}
}

void iAMultimodalWidget::setSliceNumberProtected(int sliceNumber)
{
	if (isReady()) {
		for (QSharedPointer<iASimpleSlicerWidget> slicer : m_slicerWidgets) {
			slicer->setSliceNumber(sliceNumber);
		}
		emit sliceNumberChanged(sliceNumber);
		qDebug() << "setSliceNumberPrivate" << sliceNumber;
	}
}

void iAMultimodalWidget::updateScrollBars(int newValue)
{
	switch (getSlicerMode())
	{
	case iASlicerMode::YZ:
		m_mdiChild->getSlicerDlgYZ()->verticalScrollBarYZ->setValue(newValue);
		//m_mdiChild->getSlicerYZ()->setSliceNumber(sliceNumber); // Not necessary because setting the scrollbar already does this
		break;
	case iASlicerMode::XZ:
		m_mdiChild->getSlicerDlgXZ()->verticalScrollBarXZ->setValue(newValue);
		//m_mdiChild->getSlicerXZ()->setSliceNumber(sliceNumber);
		break;
	case iASlicerMode::XY:
		m_mdiChild->getSlicerDlgXY()->verticalScrollBarXY->setValue(newValue);
		//m_mdiChild->getSlicerXY()->setSliceNumber(sliceNumber);
		break;
	default:
		// TODO exception
		return;
	}
}

void iAMultimodalWidget::updateTransferFunction(int index)
{
	updateOriginalTransferFunction(index);
	w_slicer(index)->update();
	w_histogram(index)->update();
	emit transferFunctionChanged();
}

// When new modalities are added/removed
void iAMultimodalWidget::updateModalities()
{
	if (m_mdiChild->GetModalities()->size() >= m_numOfMod) {
		bool allModalitiesAreHere = true;
		for (int i = 0; i < m_numOfMod; i++) {
			if (/*NOT*/ ! containsModality(m_mdiChild->GetModality(i))) {
				allModalitiesAreHere = false;
				break;
			}
		}
		if (allModalitiesAreHere) {
			return; // No need to update modalities if all of them are already here!
		}

	} else {
		int i = 0;
		for (; i < m_numOfMod && i < m_mdiChild->GetModalities()->size(); ++i) {
			m_modalitiesActive[i] = m_mdiChild->GetModality(i);
		}
		for (; i < m_numOfMod; i++) {
			m_modalitiesActive[i] = NULL;
		}
		return;
	}

	// Initialize modalities being added
	for (int i = 0; i < m_numOfMod; ++i) {
		m_modalitiesActive[i] = m_mdiChild->GetModality(i);

		// Histogram {
		if (!m_modalitiesActive[i]->GetHistogramData() || m_modalitiesActive[i]->GetHistogramData()->GetNumBin() != m_mdiChild->GetPreferences().HistogramBins)
		{
			m_modalitiesActive[i]->ComputeImageStatistics();
			m_modalitiesActive[i]->ComputeHistogramData(m_mdiChild->GetPreferences().HistogramBins);
		}

		vtkColorTransferFunction *colorFuncCopy = vtkColorTransferFunction::New();
		vtkPiecewiseFunction *opFuncCopy = vtkPiecewiseFunction::New();
		m_copyTFs[i] = createCopyTf(i, colorFuncCopy, opFuncCopy);

		m_histograms[i] = QSharedPointer<iADiagramFctWidget>(new iADiagramFctWidget(nullptr, m_mdiChild));
		QSharedPointer<iAPlot> histogramPlot = QSharedPointer<iAPlot>(
			new	iABarGraphPlot(m_modalitiesActive[i]->GetHistogramData(), QColor(70, 70, 70, 255)));
		m_histograms[i]->addPlot(histogramPlot);
		m_histograms[i]->setTransferFunctions(m_copyTFs[i]->getColorFunction(), m_copyTFs[i]->getOpacityFunction());
		m_histograms[i]->updateTrf();
		// }

		// Slicer {
		resetSlicer(i);
		// }

		iAChannelID id = static_cast<iAChannelID>(ch_Meta0 + i);
		iAChannelVisualizationData* chData = new iAChannelVisualizationData();
		m_mdiChild->InsertChannelData(id, chData);
		vtkImageData* imageData = m_modalitiesActive[i]->GetImage();
		vtkColorTransferFunction* ctf = m_modalitiesActive[i]->GetTransfer()->getColorFunction();
		vtkPiecewiseFunction* otf = m_modalitiesActive[i]->GetTransfer()->getOpacityFunction();
		ResetChannel(chData, imageData, ctf, otf);
		m_mdiChild->InitChannelRenderer(id, false, true);
	}

	connect((dlg_transfer*)(m_histograms[0]->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction1()));
	connect((dlg_transfer*)(m_histograms[1]->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction2()));
	connect((dlg_transfer*)(m_histograms[2]->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction3()));

	applyWeights();
	connect((dlg_transfer*)(m_mdiChild->getHistogram()->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(originalHistogramChanged()));

	emit(modalitiesLoaded_beforeUpdate());

	setSlicerModeProtected(getSlicerMode());
	//setSliceNumber(getSliceNumber()); // Already called in setSlicerMode(iASlicerMode)

	update();
}

void iAMultimodalWidget::resetSlicers() {
	for (int i = 0; i < 3; i++) {
		resetSlicer(i);
	}
}

void iAMultimodalWidget::resetSlicer(int i)
{
	// Slicer is replaced here.
	// Make sure there are no other references to the old iASimpleSlicerWidget
	// referenced by the QSharedPointer!
	m_slicerWidgets[i] = QSharedPointer<iASimpleSlicerWidget>(new iASimpleSlicerWidget(nullptr, true));
	m_slicerWidgets[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	if (m_modalitiesActive[i]) {
		m_slicerWidgets[i]->changeModality(m_modalitiesActive[i]);
		m_slicerWidgets[i]->setSlicerMode(getSlicerMode());
		m_slicerWidgets[i]->setSliceNumber(getSliceNumber());
	}
}

QSharedPointer<iATransferFunction> iAMultimodalWidget::createCopyTf(int index, vtkSmartPointer<vtkColorTransferFunction> colorTf, vtkSmartPointer<vtkPiecewiseFunction> opacityFunction)
{
	colorTf->DeepCopy(m_modalitiesActive[index]->GetTransfer()->getColorFunction());
	opacityFunction->DeepCopy(m_modalitiesActive[index]->GetTransfer()->getOpacityFunction());
	return QSharedPointer<iATransferFunction>(
		new iASimpleTransferFunction(colorTf, opacityFunction));
}

void iAMultimodalWidget::originalHistogramChanged()
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
	}
	else {
		return;
	}
	updateCopyTransferFunction(index);
	updateTransferFunction(index);
}

/** Called when the original transfer function changes
* RESETS THE COPY (admit numerical imprecision when setting the copy values)
* => effective / weight = copy
*/
void iAMultimodalWidget::updateCopyTransferFunction(int index)
{
	if (isReady()) {
		double weight = getWeight(index);

		// newly set transfer function (set via the histogram)
		QSharedPointer<iAModalityTransfer> effective = m_modalitiesActive[index]->GetTransfer();

		// copy of previous transfer function, to be updated in this method
		QSharedPointer<iATransferFunction> copy = m_copyTFs[index];

		double valCol[6], valOp[4];
		copy->getColorFunction()->RemoveAllPoints();
		copy->getOpacityFunction()->RemoveAllPoints();

		for (int j = 0; j < effective->getColorFunction()->GetSize(); ++j)
		{
			effective->getColorFunction()->GetNodeValue(j, valCol);
			effective->getOpacityFunction()->GetNodeValue(j, valOp);

			if (valOp[1] > weight) {
				valOp[1] = weight;
			}
			double copyOp = valOp[1] / weight;

			effective->getOpacityFunction()->SetNodeValue(j, valOp);

			copy->getColorFunction()->AddRGBPoint(valCol[0], valCol[1], valCol[2], valCol[3], valCol[4], valCol[5]);
			copy->getOpacityFunction()->AddPoint(valOp[0], copyOp, valOp[2], valOp[3]);
		}
	}
}

/** Called when the copy transfer function changes
* ADD NODES TO THE EFFECTIVE ONLY (clear and repopulate with adjusted effective values)
* => copy * weight ~= effective
*/
void iAMultimodalWidget::updateOriginalTransferFunction(int index)
{
	if (isReady()) {
		double weight = getWeight(index);

		// newly set transfer function (set via the histogram)
		QSharedPointer<iAModalityTransfer> effective = m_modalitiesActive[index]->GetTransfer();

		// copy of previous transfer function, to be updated in this method
		QSharedPointer<iATransferFunction> copy = m_copyTFs[index];

		double valCol[6], valOp[4];
		effective->getColorFunction()->RemoveAllPoints();
		effective->getOpacityFunction()->RemoveAllPoints();

		for (int j = 0; j < copy->getColorFunction()->GetSize(); ++j)
		{
			copy->getColorFunction()->GetNodeValue(j, valCol);
			copy->getOpacityFunction()->GetNodeValue(j, valOp);

			valOp[1] = valOp[1] * weight; // index 1 means opacity

			effective->getColorFunction()->AddRGBPoint(valCol[0], valCol[1], valCol[2], valCol[3], valCol[4], valCol[5]);
			effective->getOpacityFunction()->AddPoint(valOp[0], valOp[1], valOp[2], valOp[3]);
		}
	}
}

/** Resets the values of all nodes in the effective transfer function using the values present in the
*     copy of the transfer function, using m_weightCur for the adjustment
* CHANGES THE NODES OF THE EFFECTIVE ONLY (based on the copy)
*/
void iAMultimodalWidget::applyWeights()
{
	if (isReady()) {
		for (int i = 0; i < 3; ++i) {
			vtkPiecewiseFunction *effective = m_modalitiesActive[i]->GetTransfer()->getOpacityFunction();
			vtkPiecewiseFunction *copy = m_copyTFs[i]->getOpacityFunction();

			double pntVal[4];
			for (int j = 0; j < copy->GetSize(); ++j)
			{
				copy->GetNodeValue(j, pntVal);
				pntVal[1] = pntVal[1] * getWeight(i); // index 1 in pntVal means opacity
				effective->SetNodeValue(j, pntVal);
			}
			m_histograms[i]->update();

			iAChannelID id = static_cast<iAChannelID>(ch_Meta0 + i);
			m_mdiChild->UpdateChannelSlicerOpacity(id, getWeight(i));
		}

		m_mdiChild->getSlicerDataXY()->updateChannelMappers();
		m_mdiChild->getSlicerDataXZ()->updateChannelMappers();
		m_mdiChild->getSlicerDataYZ()->updateChannelMappers();
		m_mdiChild->updateSlicers();
	}
}



// ----------------------------------------------------------------------------------
// Short methods
// ----------------------------------------------------------------------------------

iASlicerMode iAMultimodalWidget::getSlicerMode()
{
	return (iASlicerMode)m_slicerModeComboBox->currentData().toInt();
}

iASlicerMode iAMultimodalWidget::getSlicerModeAt(int comboBoxIndex)
{
	return (iASlicerMode)m_slicerModeComboBox->itemData(comboBoxIndex).toInt();
}

int iAMultimodalWidget::getSliceNumber()
{
	return m_sliceSlider->value();
}

void iAMultimodalWidget::slicerModeComboBoxIndexChanged(int newIndex)
{
	setSlicerModeProtected(getSlicerModeAt(newIndex));
}

void iAMultimodalWidget::sliderValueChanged(int newValue)
{
	setSliceNumberProtected(newValue);
	updateScrollBars(newValue);
}

// SCROLLBARS (private SLOTS)
void iAMultimodalWidget::setSliceXYScrollBar()
{
	setSlicerMode(iASlicerMode::XY);
}
void iAMultimodalWidget::setSliceXZScrollBar()
{
	setSlicerMode(iASlicerMode::XZ);
}
void iAMultimodalWidget::setSliceYZScrollBar()
{
	setSlicerMode(iASlicerMode::YZ);
}
void iAMultimodalWidget::setSliceXYScrollBar(int sliceNumberXY)
{
	setSliceNumber(sliceNumberXY);
}
void iAMultimodalWidget::setSliceXZScrollBar(int sliceNumberXZ)
{
	setSliceNumber(sliceNumberXZ);
}
void iAMultimodalWidget::setSliceYZScrollBar(int sliceNumberYZ)
{
	setSliceNumber(sliceNumberYZ);
}

QSharedPointer<iAModality> iAMultimodalWidget::getModality(int index)
{
	return m_modalitiesActive[index];
}

BCoord iAMultimodalWidget::getWeights()
{
	return m_weights;
}

double iAMultimodalWidget::getWeight(int i)
{
	return m_weights[i];
}

bool iAMultimodalWidget::isReady()
{
	return m_modalitiesActive[2];
}

bool iAMultimodalWidget::containsModality(QSharedPointer<iAModality> modality)
{
	return m_modalitiesActive[0] == modality || m_modalitiesActive[1] == modality || m_modalitiesActive[2] == modality;
}

int iAMultimodalWidget::getModalitiesCount()
{
	return m_modalitiesActive[2] ? 3 : (m_modalitiesActive[1] ? 2 : (m_modalitiesActive[0] ? 1 : 0));
}