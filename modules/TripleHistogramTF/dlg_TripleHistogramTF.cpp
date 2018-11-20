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

#include "iAHistogramStack.h"
#include "iAHistogramTriangle.h"

#include "iAVolumeRenderer.h"
#include "iAModalityTransfer.h"

#include <vtkCamera.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

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
	// Test ^^^
	//-----------------------------------------------
	

	QWidget *histogramsWidget = new QWidget(dockWidgetContents);
	m_stackedLayout = new QStackedLayout(histogramsWidget);
	m_stackedLayout->setStackingMode(QStackedLayout::StackOne);

	m_disabledLabel = new QLabel();
	m_disabledLabel->setAlignment(Qt::AlignCenter);

	m_histogramStack = new iAHistogramTriangle(dockWidgetContents, mdiChild);
	m_histogramStack->setModalityLabel(DEFAULT_LABELS[0], 0);
	m_histogramStack->setModalityLabel(DEFAULT_LABELS[1], 1);
	m_histogramStack->setModalityLabel(DEFAULT_LABELS[2], 2);
	m_histogramStack->updateModalities();

	m_stackedLayout->addWidget(m_histogramStack);
	m_stackedLayout->addWidget(m_disabledLabel);
	m_stackedLayout->setCurrentIndex(1);

	//Connect
	connect(m_histogramStack, SIGNAL(transferFunctionChanged()), this, SLOT(updateTransferFunction()));
	connect(mdiChild->GetModalitiesDlg(), SIGNAL(ModalitiesChanged()), this, SLOT(updateModalities()));
	// }

	updateDisabledLabel();
	updateModalities();

	auto appendFilter = vtkSmartPointer<vtkImageAppendComponents>::New();
	appendFilter->SetInputData(m_histogramStack->getModality(0)->GetImage());
	appendFilter->AddInputData(m_histogramStack->getModality(1)->GetImage());
	appendFilter->AddInputData(m_histogramStack->getModality(2)->GetImage());
	appendFilter->Update();

	combinedVol = vtkSmartPointer<vtkVolume>::New();
	auto combinedVolProp = vtkSmartPointer<vtkVolumeProperty>::New();
	combinedVolProp->SetInterpolationTypeToLinear();
	combinedVolProp->SetColor(0, m_histogramStack->getModality(0)->GetTransfer()->getColorFunction());
	combinedVolProp->SetScalarOpacity(0, m_histogramStack->getModality(0)->GetTransfer()->getOpacityFunction());
	combinedVolProp->SetColor(1, m_histogramStack->getModality(1)->GetTransfer()->getColorFunction());
	combinedVolProp->SetScalarOpacity(1, m_histogramStack->getModality(1)->GetTransfer()->getOpacityFunction());
	combinedVolProp->SetColor(2, m_histogramStack->getModality(2)->GetTransfer()->getColorFunction());
	combinedVolProp->SetScalarOpacity(2, m_histogramStack->getModality(2)->GetTransfer()->getOpacityFunction());
	combinedVol->SetProperty(combinedVolProp);

	combinedVolMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	combinedVolMapper->SetBlendModeToComposite();
	combinedVolMapper->SetInputData(appendFilter->GetOutput());
	combinedVolMapper->Update();
	combinedVol->SetMapper(combinedVolMapper);
	combinedVol->Update();

	combinedVolRenderer = vtkSmartPointer<vtkRenderer>::New();
	combinedVolRenderer->GetActiveCamera()->ParallelProjectionOn();
	combinedVolRenderer->SetLayer(1);
	combinedVolRenderer->AddVolume(combinedVol);
	combinedVolRenderer->ResetCamera();

	for (int i = 0; i < 3; ++i)
	{
		QSharedPointer<iAVolumeRenderer> renderer = m_histogramStack->getModality(i)->GetRenderer();
		renderer->Remove();
	}
	mdiChild->getRenderer()->AddRenderer(combinedVolRenderer);
}

dlg_TripleHistogramTF::~dlg_TripleHistogramTF()
{
}

// SLOTS {
void dlg_TripleHistogramTF::updateTransferFunction()
{
	m_mdiChild->redrawHistogram();
	m_mdiChild->getRenderer()->update();
}
void dlg_TripleHistogramTF::updateModalities()
{
	m_histogramStack->updateModalities();
	if (m_histogramStack->getModalitiesCount() >= 3) {
		m_stackedLayout->setCurrentIndex(0);
	} else {
		updateDisabledLabel();
		m_stackedLayout->setCurrentIndex(1);
	}
}
// }

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