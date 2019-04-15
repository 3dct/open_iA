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
#include "iATripleModalityWidget.h"

#include "iABarycentricContextRenderer.h"
#include "iABarycentricTriangleWidget.h"
#include "iASimpleSlicerWidget.h"
#include "RightBorderLayout.h"

#include <charts/iADiagramFctWidget.h>
#include <charts/iAHistogramData.h>
#include <charts/iAPlotTypes.h>
#include <charts/iAProfileWidget.h>
#include <dlg_modalities.h>
#include <dlg_slicer.h>
#include <iAChartFunctionTransfer.h>
#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iAModalityTransfer.h>
#include <iAPreferences.h>
#include <iASlicer.h>
#include <mdichild.h>

#include <vtkCamera.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QString>

// Debug
#include <QDebug>

//static const char *WEIGHT_FORMAT = "%.10f";
static const QString DISABLED_TEXT_COLOR = "rgb(0,0,0)"; // black
static const QString DISABLED_BACKGROUND_COLOR = "rgba(255,255,255)"; // white

iATripleModalityWidget::iATripleModalityWidget(QWidget * parent, MdiChild *mdiChild, Qt::WindowFlags f /*= 0 */) :
	QWidget(parent, f)
{
	m_mdiChild = mdiChild;

	m_disabledLabel = new QLabel();
	m_disabledLabel->setAlignment(Qt::AlignCenter);
	m_disabledLabel->setStyleSheet("background-color: " + DISABLED_BACKGROUND_COLOR + "; color: " + DISABLED_TEXT_COLOR);

	/*
	mdiChild->slicer(iASlicerMode::XY)->GetImageActor()->SetOpacity(0.0);
	mdiChild->slicer(iASlicerMode::XZ)->GetImageActor()->SetOpacity(0.0);
	mdiChild->slicer(iASlicerMode::YZ)->GetImageActor()->SetOpacity(0.0);
	*/
	for (int i=0; i<3; ++i)
		mdiChild->slicer(i)->channel(0)->setActorOpacity(0.0);

	//setStyleSheet("background-color:red"); // test spacing/padding/margin

	m_triangleRenderer = new iABarycentricContextRenderer();
	m_triangleWidget = new iABarycentricTriangleWidget();
	m_triangleWidget->setModality1label(DEFAULT_MODALITY_LABELS[0]);
	m_triangleWidget->setModality2label(DEFAULT_MODALITY_LABELS[1]);
	m_triangleWidget->setModality3label(DEFAULT_MODALITY_LABELS[2]);
	m_triangleWidget->setTriangleRenderer(m_triangleRenderer);

	m_slicerModeComboBox = new QComboBox();
	m_slicerModeComboBox->addItem("YZ", iASlicerMode::YZ);
	m_slicerModeComboBox->addItem("XY", iASlicerMode::XY);
	m_slicerModeComboBox->addItem("XZ", iASlicerMode::XZ);

	m_sliceSlider = new QSlider(Qt::Horizontal);
	m_sliceSlider->setMinimum(0);

	connect(m_triangleWidget, SIGNAL(weightChanged(BCoord)), this, SLOT(triangleWeightChanged(BCoord)));
	connect(m_slicerModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxIndexChanged(int)));
	connect(m_sliceSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));

	connect(mdiChild->slicerDockWidget(iASlicerMode::XY)->verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setSliceXYScrollBar(int)));
	connect(mdiChild->slicerDockWidget(iASlicerMode::XZ)->verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setSliceXZScrollBar(int)));
	connect(mdiChild->slicerDockWidget(iASlicerMode::YZ)->verticalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(setSliceYZScrollBar(int)));
	connect(mdiChild->slicerDockWidget(iASlicerMode::XY)->verticalScrollBar, SIGNAL(sliderPressed()), this, SLOT(setSliceXYScrollBar()));
	connect(mdiChild->slicerDockWidget(iASlicerMode::XZ)->verticalScrollBar, SIGNAL(sliderPressed()), this, SLOT(setSliceXZScrollBar()));
	connect(mdiChild->slicerDockWidget(iASlicerMode::YZ)->verticalScrollBar, SIGNAL(sliderPressed()), this, SLOT(setSliceYZScrollBar()));
}

iATripleModalityWidget::~iATripleModalityWidget()
{
	if (m_copyTFs[0]) delete m_copyTFs[0];
	if (m_copyTFs[1]) delete m_copyTFs[1];
	if (m_copyTFs[2]) delete m_copyTFs[2];
	delete m_triangleRenderer;
}

iASlicerMode iATripleModalityWidget::getSlicerMode()
{
	return (iASlicerMode) m_slicerModeComboBox->currentData().toInt();
}

iASlicerMode iATripleModalityWidget::getSlicerModeAt(int comboBoxIndex)
{
	return (iASlicerMode)m_slicerModeComboBox->itemData(comboBoxIndex).toInt();
}

int iATripleModalityWidget::getSliceNumber()
{
	return m_sliceSlider->value();
}

void iATripleModalityWidget::triangleWeightChanged(BCoord newWeight)
{
	setWeightPrivate(newWeight);
}

void iATripleModalityWidget::comboBoxIndexChanged(int newIndex)
{
	setSlicerModePrivate(getSlicerModeAt(newIndex));
}

void iATripleModalityWidget::sliderValueChanged(int newValue)
{
	setSliceNumberPrivate(newValue);
	updateScrollBars(newValue);
}

// SCROLLBARS (private SLOTS) {
void iATripleModalityWidget::setSliceXYScrollBar()
{
	setSlicerMode(iASlicerMode::XY);
}
void iATripleModalityWidget::setSliceXZScrollBar()
{
	setSlicerMode(iASlicerMode::XZ);
}
void iATripleModalityWidget::setSliceYZScrollBar()
{
	setSlicerMode(iASlicerMode::YZ);
}
void iATripleModalityWidget::setSliceXYScrollBar(int sliceNumberXY)
{
	setSliceNumber(sliceNumberXY);
}
void iATripleModalityWidget::setSliceXZScrollBar(int sliceNumberXZ)
{
	setSliceNumber(sliceNumberXZ);
}
void iATripleModalityWidget::setSliceYZScrollBar(int sliceNumberYZ)
{
	setSliceNumber(sliceNumberYZ);
}
// }

// PUBLIC SETTERS { -------------------------------------------------------------------------
bool iATripleModalityWidget::setWeight(BCoord bCoord)
{
	if (bCoord != getWeight()) {
		m_triangleWidget->setWeight(bCoord);
		return true;
	}
	return false;
}
bool iATripleModalityWidget::setSlicerMode(iASlicerMode slicerMode)
{
	if (slicerMode != getSlicerMode()) {
		m_slicerModeComboBox->setCurrentIndex(m_slicerModeComboBox->findData(slicerMode));
		return true;
	}
	return false;
}
bool iATripleModalityWidget::setSliceNumber(int sliceNumber)
{
	if (sliceNumber != getSliceNumber()) {
		m_sliceSlider->setValue(sliceNumber);
		return true;
	}
	return false;
}
// } ----------------------------------------------------------------------------------------

// PRIVATE SETTERS { ------------------------------------------------------------------------
void iATripleModalityWidget::setWeightPrivate(BCoord bCoord)
{
	if (bCoord == m_weightCur) {
		return;
	}

	m_weightCur = bCoord;
	applyWeights();
	emit weightChanged(bCoord);
	emit transferFunctionChanged();
}
void iATripleModalityWidget::setSlicerModePrivate(iASlicerMode slicerMode)
{
	if (isReady())
	{
		m_slicerWidgets[0]->setSlicerMode(slicerMode);
		m_slicerWidgets[1]->setSlicerMode(slicerMode);
		m_slicerWidgets[2]->setSlicerMode(slicerMode);

		int dimensionIndex = mapSliceToGlobalAxis(slicerMode, iAAxisIndex::Z);
		int sliceNumber    = m_mdiChild->slicer(slicerMode)->sliceNumber();
		int dimensionLength = m_mdiChild->imageData()->GetDimensions()[dimensionIndex];
		m_sliceSlider->setMaximum(dimensionLength - 1);
		if (!setSliceNumber(sliceNumber)) {
			sliderValueChanged(sliceNumber);
		}

		emit slicerModeChanged(slicerMode);
	}
}
void iATripleModalityWidget::setSliceNumberPrivate(int sliceNumber)
{
	if (isReady()) {
		m_slicerWidgets[0]->setSliceNumber(sliceNumber);
		m_slicerWidgets[1]->setSliceNumber(sliceNumber);
		m_slicerWidgets[2]->setSliceNumber(sliceNumber);
		emit sliceNumberChanged(sliceNumber);
	}
}
// } ----------------------------------------------------------------------------------------

void iATripleModalityWidget::updateScrollBars(int newValue)
{
	m_mdiChild->slicerDockWidget(getSlicerMode())->verticalScrollBar->setValue(newValue);
}

void iATripleModalityWidget::updateTransferFunction(int index)
{
	updateOriginalTransferFunction(index);
	m_slicerWidgets[index]->update();
	m_histograms[index]->update();
	emit transferFunctionChanged();
}

void iATripleModalityWidget::setModalityLabel(QString label, int index)
{
	// TODO change triangle's labels
}


QSharedPointer<iAModality> iATripleModalityWidget::getModality(int index)
{
	return m_modalitiesActive[index];
}

BCoord iATripleModalityWidget::getWeight()
{
	return m_triangleWidget->getWeight();
}

double iATripleModalityWidget::getWeight(int index)
{
	return m_weightCur[index];
}

// When new modalities are added/removed
void iATripleModalityWidget::updateModalities()
{
	if (m_mdiChild->modalities()->size() >= 3) {
		if (containsModality(m_mdiChild->modality(0)) &&
			containsModality(m_mdiChild->modality(1)) &&
			containsModality(m_mdiChild->modality(2))) {

			return;
		}
	} else {
		int i = 0;
		for (; i < ModalityNumber && i < m_mdiChild->modalities()->size(); ++i) {
			m_modalitiesActive[i] = m_mdiChild->modality(i);
		}
		for (; i < ModalityNumber; i++) { // Loop is not executed - and probably not intended to be?
			m_modalitiesActive[i] = nullptr;
			//if (m_weightLabels[i]) delete m_weightLabels[i];
			//if (m_modalityLabels[i]) delete m_modalityLabels[i];
			//if (m_slicerWidgets[i]) delete m_slicerWidgets[i];
			//if (m_histograms[i]) delete m_histograms[i];
		}
		return;
	}

	// Initialize modalities being added
	for (int i = 0; i < ModalityNumber; ++i)
	{
		m_modalitiesActive[i] = m_mdiChild->modality(i);

		// Histogram {
		if (!m_modalitiesActive[i]->histogramData() || m_modalitiesActive[i]->histogramData()->numBin() != m_mdiChild->preferences().HistogramBins)
		{
			m_modalitiesActive[i]->computeImageStatistics();
			m_modalitiesActive[i]->computeHistogramData(m_mdiChild->preferences().HistogramBins);
		}

		vtkColorTransferFunction *colorFuncCopy = vtkColorTransferFunction::New(); // TODO delete?
		vtkPiecewiseFunction *opFuncCopy = vtkPiecewiseFunction::New(); // TODO delete?
		m_copyTFs[i] = createCopyTf(i, colorFuncCopy, opFuncCopy); //new iASimpleTransferFunction(colorFuncCopy, opFuncCopy);

		m_histograms[i] = new iADiagramFctWidget(nullptr, m_mdiChild);
		QSharedPointer<iAPlot> histogramPlot = QSharedPointer<iAPlot>(
			new	iABarGraphPlot(m_modalitiesActive[i]->histogramData(), QColor(70, 70, 70, 255)));
		m_histograms[i]->addPlot(histogramPlot);
		m_histograms[i]->setTransferFunctions(m_copyTFs[i]->colorTF(), m_copyTFs[i]->opacityTF());
		m_histograms[i]->updateTrf();
		// }

		// Slicer {
		m_slicerWidgets[i] = new iASimpleSlicerWidget(nullptr, isSlicerInteractionEnabled());
		m_slicerWidgets[i]->changeModality(m_modalitiesActive[i]);
		m_slicerWidgets[i]->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
		// }

		m_channelIDs[i] = m_mdiChild->createChannel();
		iAChannelData* chData = m_mdiChild->channelData(m_channelIDs[i]);
		vtkImageData* imageData = m_modalitiesActive[i]->image();
		vtkColorTransferFunction* ctf = m_modalitiesActive[i]->transfer()->colorTF();
		vtkPiecewiseFunction* otf = m_modalitiesActive[i]->transfer()->opacityTF();
		chData->setData(imageData, ctf, otf);
		// TODO: initialize channel?
		m_mdiChild->initChannelRenderer(m_channelIDs[i], false, true);
	}

	m_triangleWidget->setModalities(getModality(0)->image(), getModality(1)->image(), getModality(2)->image());
	m_triangleWidget->update();

	connect((iAChartTransferFunction*)(m_histograms[0]->functions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction1()));
	connect((iAChartTransferFunction*)(m_histograms[1]->functions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction2()));
	connect((iAChartTransferFunction*)(m_histograms[2]->functions()[0]), SIGNAL(Changed()), this, SLOT(updateTransferFunction3()));

	setSlicerModePrivate(getSlicerMode());
	//setSliceNumber(getSliceNumber()); // Already called in setSlicerMode(iASlicerMode)
	
	applyWeights();
	connect((iAChartTransferFunction*)(m_mdiChild->histogram()->functions()[0]), SIGNAL(Changed()), this, SLOT(originalHistogramChanged()));

	// Pure virtual method
	initialize();
}

bool iATripleModalityWidget::isReady()
{
	return m_modalitiesActive[2];
}


bool iATripleModalityWidget::containsModality(QSharedPointer<iAModality> modality)
{
	return m_modalitiesActive[0] == modality || m_modalitiesActive[1] == modality || m_modalitiesActive[2] == modality;
}

int iATripleModalityWidget::getModalitiesCount()
{
	return m_modalitiesActive[2] ? 3 : (m_modalitiesActive[1] ? 2 : (m_modalitiesActive[0] ? 1 : 0));
}

iATransferFunction* iATripleModalityWidget::createCopyTf(int index, vtkSmartPointer<vtkColorTransferFunction> colorTf, vtkSmartPointer<vtkPiecewiseFunction> opacityFunction)
{
	colorTf->DeepCopy(m_modalitiesActive[index]->transfer()->colorTF());
	opacityFunction->DeepCopy(m_modalitiesActive[index]->transfer()->opacityTF());
	return new iASimpleTransferFunction(colorTf, opacityFunction);
}

void iATripleModalityWidget::originalHistogramChanged()
{
	QSharedPointer<iAModality> selected = m_mdiChild->modalities()->get(m_mdiChild->modalitiesDockWidget()->selected());
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
void iATripleModalityWidget::updateCopyTransferFunction(int index)
{
	if (isReady()) {
		double weight = m_weightCur[index];

		// newly set transfer function (set via the histogram)
		QSharedPointer<iAModalityTransfer> effective = m_modalitiesActive[index]->transfer();

		// copy of previous transfer function, to be updated in this method
		iATransferFunction *copy = m_copyTFs[index];

		double valCol[6], valOp[4];
		copy->colorTF()->RemoveAllPoints();
		copy->opacityTF()->RemoveAllPoints();

		for (int j = 0; j < effective->colorTF()->GetSize(); ++j)
		{
			effective->colorTF()->GetNodeValue(j, valCol);
			effective->opacityTF()->GetNodeValue(j, valOp);

			if (valOp[1] > weight) {
				valOp[1] = weight;
			}
			double copyOp = valOp[1] / weight;

			effective->opacityTF()->SetNodeValue(j, valOp);

			copy->colorTF()->AddRGBPoint(valCol[0], valCol[1], valCol[2], valCol[3], valCol[4], valCol[5]);
			copy->opacityTF()->AddPoint(valOp[0], copyOp, valOp[2], valOp[3]);
		}
	}
}

/** Called when the copy transfer function changes
  * ADD NODES TO THE EFFECTIVE ONLY (clear and repopulate with adjusted effective values)
  * => copy * weight = effective
  */
void iATripleModalityWidget::updateOriginalTransferFunction(int index)
{
	if (isReady()) {
		double weight = m_weightCur[index];

		// newly set transfer function (set via the histogram)
		QSharedPointer<iAModalityTransfer> effective = m_modalitiesActive[index]->transfer();

		// copy of previous transfer function, to be updated in this method
		iATransferFunction *copy = m_copyTFs[index];

		double valCol[6], valOp[4];
		effective->colorTF()->RemoveAllPoints();
		effective->opacityTF()->RemoveAllPoints();

		for (int j = 0; j < copy->colorTF()->GetSize(); ++j)
		{
			copy->colorTF()->GetNodeValue(j, valCol);
			copy->opacityTF()->GetNodeValue(j, valOp);

			valOp[1] = valOp[1] * weight; // index 1 means opacity

			effective->colorTF()->AddRGBPoint(valCol[0], valCol[1], valCol[2], valCol[3], valCol[4], valCol[5]);
			effective->opacityTF()->AddPoint(valOp[0], valOp[1], valOp[2], valOp[3]);
		}
	}
}

/** Resets the values of all nodes in the effective transfer function using the values present in the
  *     copy of the transfer function, using m_weightCur for the adjustment
  * CHANGES THE NODES OF THE EFFECTIVE ONLY (based on the copy)
  */
void iATripleModalityWidget::applyWeights()
{
	if (isReady()) {
		for (int i = 0; i < ModalityNumber; ++i)
		{
			vtkPiecewiseFunction *effective = m_modalitiesActive[i]->transfer()->opacityTF();
			vtkPiecewiseFunction *copy = m_copyTFs[i]->opacityTF();

			double pntVal[4];
			for (int j = 0; j < copy->GetSize(); ++j)
			{
				copy->GetNodeValue(j, pntVal);
				pntVal[1] = pntVal[1] * m_weightCur[i]; // index 1 in pntVal means opacity
				effective->SetNodeValue(j, pntVal);
			}
			m_histograms[i]->update();

			//m_mdiChild->updateChannelOpacity(m_channelIDs[i], m_weightCur[i]);
			for (int i=0; i<3; ++i)
				m_mdiChild->slicer(i)->updateChannelMappers();
			m_mdiChild->updateSlicers();
		}
	}
}
