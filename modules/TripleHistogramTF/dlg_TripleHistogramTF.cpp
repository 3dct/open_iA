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
 
#include "dlg_TripleHistogramTF.h"

#include <qsplitter.h>

#include "dlg_modalities.h"
#include "iAModalityList.h"
#include "iARenderer.h"
#include "iASlicerData.h"
#include "iABarycentricContextRenderer.h"

#include <vtkImageData.h>

// Debug
#include "qdebug.h"

const static QString DEFAULT_LABELS[3] = { "A", "B", "C" };

dlg_TripleHistogramTF::dlg_TripleHistogramTF(MdiChild * mdiChild /*= 0*/, Qt::WindowFlags f /*= 0 */) :
	//TripleHistogramTFConnector(mdiChild, f), m_mdiChild(mdiChild)
	QDockWidget("Triple Histogram Transfer Function", mdiChild, f),
	m_mdiChild(mdiChild)
{

	// Initialize dock widget
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetVerticalTitleBar);

	// Set up connects
	// ...

	//-----------------------------------------------
	// Test vvv // TODO: remove comments
	resize(779, 501);

	//QWidget *dockWidgetContents = new QWidget();
	QSplitter *dockWidgetContents = new QSplitter(Qt::Horizontal);
	setWidget(dockWidgetContents);
	dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));

	//RightBorderLayout *mainLayout = new RightBorderLayout(dockWidgetContents, RightBorderLayout::Right);
	//m_gridLayout = new QHBoxLayout(dockWidgetContents);
	//m_gridLayout->setSpacing(0);
	//m_gridLayout->setObjectName(QStringLiteral("horizontalLayout_2"));
	//m_gridLayout->setContentsMargins(0, 0, 0, 0);

	QWidget *optionsContainer = new QWidget();
	//optionsContainer->setStyleSheet("background-color:blue"); // test spacing/padding/margin
	optionsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	QHBoxLayout *optionsContainerLayout = new QHBoxLayout(optionsContainer);
	// Test ^^^
	//-----------------------------------------------

	m_triangleRenderer = new iABarycentricContextRenderer();
	m_triangleWidget = new iABarycentricTriangleWidget(dockWidgetContents);
	m_triangleWidget->setModality1label(DEFAULT_LABELS[0]);
	m_triangleWidget->setModality2label(DEFAULT_LABELS[1]);
	m_triangleWidget->setModality3label(DEFAULT_LABELS[2]);
	m_triangleWidget->setTriangleRenderer(m_triangleRenderer);

	m_slicerModeComboBox = new QComboBox(optionsContainer);
	m_slicerModeComboBox->addItem("YZ", iASlicerMode::YZ);
	m_slicerModeComboBox->addItem("XY", iASlicerMode::XY);
	m_slicerModeComboBox->addItem("XZ", iASlicerMode::XZ);

	m_sliceSlider = new QSlider(Qt::Horizontal, optionsContainer);
	m_sliceSlider->setMinimum(0);

	optionsContainerLayout->addWidget(m_slicerModeComboBox);
	optionsContainerLayout->addWidget(m_sliceSlider);

	QWidget *histogramsWidget = new QWidget(dockWidgetContents);
	m_stackedLayout = new QStackedLayout(histogramsWidget);
	m_stackedLayout->setStackingMode(QStackedLayout::StackOne);

	m_disabledLabel = new QLabel();
	m_disabledLabel->setAlignment(Qt::AlignCenter);

	m_histogramStack = new iAHistogramStack(dockWidgetContents, mdiChild);
	m_histogramStack->setModalityLabel(DEFAULT_LABELS[0], 0);
	m_histogramStack->setModalityLabel(DEFAULT_LABELS[1], 1);
	m_histogramStack->setModalityLabel(DEFAULT_LABELS[2], 2);
	m_histogramStack->updateModalities();

	m_stackedLayout->addWidget(m_histogramStack);
	m_stackedLayout->addWidget(m_disabledLabel);
	m_stackedLayout->setCurrentIndex(1);

	QWidget *leftWidget = new QWidget();
	QVBoxLayout *leftWidgetLayout = new QVBoxLayout(leftWidget);
	leftWidgetLayout->setSpacing(1);
	leftWidgetLayout->setMargin(0);
	leftWidgetLayout->addWidget(optionsContainer);
	leftWidgetLayout->addWidget(histogramsWidget);

	dockWidgetContents->addWidget(leftWidget);
	dockWidgetContents->addWidget(m_triangleWidget);

	//m_mdiChild->getRaycaster()->setTransferFunction(m_transferFunction);
	//m_mdiChild->getSlicerXY()->setTransferFunction(m_transferFunction);
	//...

	updateSlicerMode();
	setWeight(m_triangleWidget->getControlPointCoordinates());

	// Does not work. TODO: fix
	/*mdiChild->getSlicerXY()->reInitialize(
		mdiChild->getImageData(),
		vtkTransform::New(), // no way of getting current transform, so create a new one // TODO: fix?
		m_histogramStack->getTransferFunction()); // here is where the magic happens ;)*/

	//Connect
	connect(m_triangleWidget, SIGNAL(weightChanged(BCoord)), this, SLOT(setWeight(BCoord)));
	connect(m_slicerModeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSlicerMode()));
	connect(m_sliceSlider, SIGNAL(valueChanged(int)), this, SLOT(sliderValueChanged(int)));

	connect(m_histogramStack, SIGNAL(transferFunctionChanged()), this, SLOT(updateTransferFunction()));
	connect(m_histogramStack, SIGNAL(modalitiesChanged(QSharedPointer<iAModality> modality1, QSharedPointer<iAModality> modality2, QSharedPointer<iAModality> modality3)), this, SLOT(modalityAddedToStack(QSharedPointer<iAModality> modality1, QSharedPointer<iAModality> modality2, QSharedPointer<iAModality> modality3)));

	connect(mdiChild->GetModalitiesDlg(), SIGNAL(ModalitiesChanged()), this, SLOT(modalitiesChanged()));

	connect(mdiChild->getSlicerDlgXY()->verticalScrollBarXY, SIGNAL(valueChanged(int)), this, SLOT(setSliceXYScrollBar(int)));
	connect(mdiChild->getSlicerDlgXZ()->verticalScrollBarXZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceXZScrollBar(int)));
	connect(mdiChild->getSlicerDlgYZ()->verticalScrollBarYZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceYZScrollBar(int)));

	connect(mdiChild->getSlicerDlgXY()->verticalScrollBarXY, SIGNAL(sliderPressed()), this, SLOT(setSliceXYScrollBar()));
	connect(mdiChild->getSlicerDlgXZ()->verticalScrollBarXZ, SIGNAL(sliderPressed()), this, SLOT(setSliceXZScrollBar()));
	connect(mdiChild->getSlicerDlgYZ()->verticalScrollBarYZ, SIGNAL(sliderPressed()), this, SLOT(setSliceYZScrollBar()));
	// }

	updateDisabledLabel();
}

dlg_TripleHistogramTF::~dlg_TripleHistogramTF()
{
	delete m_triangleRenderer;
}

void dlg_TripleHistogramTF::setWeight(BCoord bCoord)
{
	m_histogramStack->setWeight(bCoord);
}

void dlg_TripleHistogramTF::updateTransferFunction()
{
	m_mdiChild->redrawHistogram();
	m_mdiChild->getRenderer()->update();
}

void dlg_TripleHistogramTF::modalitiesChanged()
{
	m_histogramStack->updateModalities();
	if (m_histogramStack->getModalitiesCount() >= 3) {
		m_stackedLayout->setCurrentIndex(0);
		m_triangleWidget->setModalities(m_histogramStack->getModality(0)->GetImage(), m_histogramStack->getModality(1)->GetImage(), m_histogramStack->getModality(2)->GetImage());
	} else {
		updateDisabledLabel();
		m_stackedLayout->setCurrentIndex(1);
	}
	m_triangleWidget->update();
}

// ---------------------------------------------------------------------------------------------------------------------------
// PRIVATE METHODS
// ---------------------------------------------------------------------------------------------------------------------------

// SCROLLBARS (private SLOTS) {
void dlg_TripleHistogramTF::setSliceXYScrollBar()
{
	setSlicerMode(iASlicerMode::XY);
}
void dlg_TripleHistogramTF::setSliceXZScrollBar()
{
	setSlicerMode(iASlicerMode::XZ);
}
void dlg_TripleHistogramTF::setSliceYZScrollBar()
{
	setSlicerMode(iASlicerMode::YZ);
}
void dlg_TripleHistogramTF::setSliceXYScrollBar(int sliceNumberXY)
{
	//setSliceNumber(sliceNumberXY);
	m_sliceSlider->setValue(sliceNumberXY);
}
void dlg_TripleHistogramTF::setSliceXZScrollBar(int sliceNumberXZ)
{
	//setSliceNumber(sliceNumberXZ);
	m_sliceSlider->setValue(sliceNumberXZ);
}
void dlg_TripleHistogramTF::setSliceYZScrollBar(int sliceNumberYZ)
{
	//setSliceNumber(sliceNumberYZ);
	m_sliceSlider->setValue(sliceNumberYZ);
}
// }

// PRIVATE SLOT
void dlg_TripleHistogramTF::updateSlicerMode()
{
	setSlicerMode((iASlicerMode)m_slicerModeComboBox->currentData().toInt());
}

void dlg_TripleHistogramTF::setSlicerMode(iASlicerMode slicerMode)
{
	if (slicerMode != m_slicerMode) {
		m_slicerModeComboBox->setCurrentIndex(m_slicerModeComboBox->findData(slicerMode));
		m_slicerMode = slicerMode;

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

		m_histogramStack->setSlicerMode(slicerMode, dimensionLength);

		modalitiesChanged();

		m_sliceSlider->setMaximum(dimensionLength - 1);
		//int value = dimensionLength / 2;
		m_sliceSlider->setValue(sliceNumber);
		setSliceNumber(sliceNumber);
	}
}

void dlg_TripleHistogramTF::setSliceNumber(int sliceNumber)
{
	m_histogramStack->setSliceNumber(sliceNumber);
}

// PRIVATE SLOT
void dlg_TripleHistogramTF::sliderValueChanged(int sliceNumber)
{
	setSliceNumber(sliceNumber);
	updateMainSlicers(sliceNumber);
}

void dlg_TripleHistogramTF::updateMainSlicers(int sliceNumber) {
	switch (m_slicerMode)
	{
	case iASlicerMode::YZ:
		m_mdiChild->getSlicerDlgYZ()->verticalScrollBarYZ->setValue(sliceNumber);
		//m_mdiChild->getSlicerYZ()->setSliceNumber(sliceNumber); // Not necessary because setting the scrollbar already does this
		break;
	case iASlicerMode::XZ:
		m_mdiChild->getSlicerDlgXZ()->verticalScrollBarXZ->setValue(sliceNumber);
		//m_mdiChild->getSlicerXZ()->setSliceNumber(sliceNumber);
		break;
	case iASlicerMode::XY:
		m_mdiChild->getSlicerDlgXY()->verticalScrollBarXY->setValue(sliceNumber);
		//m_mdiChild->getSlicerXY()->setSliceNumber(sliceNumber);
		break;
	default:
		// TODO exception
		return;
	}
}

void dlg_TripleHistogramTF::updateDisabledLabel()
{
	int count = m_histogramStack->getModalitiesCount();
	QString modalit_y_ies_is_are = count == 2 ? "modality is" : "modalities are";
	//QString nameA = count >= 1 ? m_modalitiesActive[0]->GetName() : "missing";
	//QString nameB = count >= 2 ? m_modalitiesActive[1]->GetName() : "missing";
	//QString nameC = modalitiesCount >= 3 ? m_modalitiesAvailable[2]->GetName() : "missing";
	m_disabledLabel->setText(
		"Unable to set up this widget.\n" +
		QString::number(3 - count) + " " + modalit_y_ies_is_are + " missing.\n"/* +
		"\n" +
		"Modality " + DEFAULT_MODALITY_LABELS[0] + ": " + nameA + "\n" +
		"Modality " + DEFAULT_MODALITY_LABELS[1] + ": " + nameB + "\n" +
		"Modality " + DEFAULT_MODALITY_LABELS[2] + ": missing"*/
	);
}