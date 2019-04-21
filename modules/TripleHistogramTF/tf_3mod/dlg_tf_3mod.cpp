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

#include "dlg_tf_3mod.h"

#include "iATripleModalityWidget.h"
#include "iABarycentricContextRenderer.h"
#include "iAHistogramStack.h"
#include "iAHistogramTriangle.h"

#include <dlg_modalities.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iAModalityTransfer.h>
#include <iARenderer.h>
#include <iASlicerData.h>
#include <iAVolumeRenderer.h>
#include <mdichild.h>

#include <vtkCamera.h>
#include <vtkImageAppendComponents.h>
#include <vtkImageData.h>
#include <vtkRenderer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageActor.h>

#include <QLabel>
#include <QSplitter>
#include <QStackedLayout>
// Debug
#include <QDebug>

const static QString DEFAULT_LABELS[3] = { "A", "B", "C" };

dlg_tf_3mod::dlg_tf_3mod(MdiChild * mdiChild /*= 0*/, Qt::WindowFlags f /*= 0 */)
	:
	//TripleHistogramTFConnector(mdiChild, f), m_mdiChild(mdiChild)
	QDockWidget("Triple Histogram Transfer Function", mdiChild, f),
	m_mdiChild(mdiChild)
{

	// Initialize dock widget
	setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetVerticalTitleBar);

	resize(779, 501);

	//QWidget *dockWidgetContents = new QWidget();
	QSplitter *dockWidgetContents = new QSplitter(Qt::Horizontal);
	setWidget(dockWidgetContents);
	dockWidgetContents->setObjectName(QStringLiteral("dockWidgetContents"));

	QWidget *histogramsWidget = new QWidget(dockWidgetContents);
	m_stackedLayout = new QStackedLayout(histogramsWidget);
	m_stackedLayout->setStackingMode(QStackedLayout::StackOne);

	m_disabledLabel = new QLabel();
	m_disabledLabel->setAlignment(Qt::AlignCenter);

	m_tripleModalityWidget = new iATripleModalityWidget(dockWidgetContents, mdiChild);

	m_stackedLayout->addWidget(m_tripleModalityWidget);
	m_stackedLayout->addWidget(m_disabledLabel);
	m_stackedLayout->setCurrentIndex(1);

	//Connect
	connect(m_tripleModalityWidget, SIGNAL(transferFunctionChanged()), this, SLOT(updateTransferFunction()));
	connect(mdiChild->GetModalitiesDlg(), SIGNAL(ModalitiesChanged()), this, SLOT(updateModalities()));
	// }

	updateDisabledLabel();
	updateModalities();
}

// SLOTS {
void dlg_tf_3mod::updateTransferFunction()
{
	m_mdiChild->redrawHistogram();
	m_mdiChild->getRenderer()->update();
}
void dlg_tf_3mod::updateModalities()
{
	m_tripleModalityWidget->updateModalities();
	if (m_tripleModalityWidget->getModalitiesCount() < 3) {
		updateDisabledLabel();
		m_stackedLayout->setCurrentIndex(1);
		return;
	}

	m_stackedLayout->setCurrentIndex(0);

	auto appendFilter = vtkSmartPointer<vtkImageAppendComponents>::New();
	appendFilter->SetInputData(m_tripleModalityWidget->getModality(0)->GetImage());
	appendFilter->AddInputData(m_tripleModalityWidget->getModality(1)->GetImage());
	appendFilter->AddInputData(m_tripleModalityWidget->getModality(2)->GetImage());
	appendFilter->Update();

	m_combinedVol = vtkSmartPointer<vtkVolume>::New();
	auto combinedVolProp = vtkSmartPointer<vtkVolumeProperty>::New();
	combinedVolProp->SetInterpolationTypeToLinear();

	combinedVolProp->SetColor(0, m_tripleModalityWidget->getModality(0)->GetTransfer()->getColorFunction());
	combinedVolProp->SetScalarOpacity(0, m_tripleModalityWidget->getModality(0)->GetTransfer()->getOpacityFunction());

	combinedVolProp->SetColor(1, m_tripleModalityWidget->getModality(1)->GetTransfer()->getColorFunction());
	combinedVolProp->SetScalarOpacity(1, m_tripleModalityWidget->getModality(1)->GetTransfer()->getOpacityFunction());

	combinedVolProp->SetColor(2, m_tripleModalityWidget->getModality(2)->GetTransfer()->getColorFunction());
	combinedVolProp->SetScalarOpacity(2, m_tripleModalityWidget->getModality(2)->GetTransfer()->getOpacityFunction());

	m_combinedVol->SetProperty(combinedVolProp);

	m_combinedVolMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	m_combinedVolMapper->SetBlendModeToComposite();
	m_combinedVolMapper->SetInputData(appendFilter->GetOutput());
	m_combinedVolMapper->Update();
	m_combinedVol->SetMapper(m_combinedVolMapper);
	m_combinedVol->Update();

	m_combinedVolRenderer = vtkSmartPointer<vtkRenderer>::New();
	m_combinedVolRenderer->SetActiveCamera(m_mdiChild->getRenderer()->getCamera());
	m_combinedVolRenderer->GetActiveCamera()->ParallelProjectionOn();
	m_combinedVolRenderer->SetLayer(1);
	m_combinedVolRenderer->AddVolume(m_combinedVol);
	//m_combinedVolRenderer->ResetCamera();

	for (int i = 0; i < 3; ++i)
	{
		QSharedPointer<iAVolumeRenderer> renderer = m_tripleModalityWidget->getModality(i)->GetRenderer();
		renderer->Remove();
	}
	m_mdiChild->getRenderer()->AddRenderer(m_combinedVolRenderer);

	m_mdiChild->getSlicerDataXY()->GetImageActor()->SetOpacity(0.0);
	m_mdiChild->getSlicerDataXZ()->GetImageActor()->SetOpacity(0.0);
	m_mdiChild->getSlicerDataYZ()->GetImageActor()->SetOpacity(0.0);

	m_mdiChild->getSlicerDataXY()->SetManualBackground(1.0, 1.0, 1.0);
	m_mdiChild->getSlicerDataXZ()->SetManualBackground(1.0, 1.0, 1.0);
	m_mdiChild->getSlicerDataYZ()->SetManualBackground(1.0, 1.0, 1.0);
}
// }

void dlg_tf_3mod::updateDisabledLabel()
{
	int count = m_tripleModalityWidget->getModalitiesCount();
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