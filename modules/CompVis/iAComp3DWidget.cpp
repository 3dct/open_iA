// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAComp3DWidget.h"

//CompVis
#include "iAMainWindow.h"

//iA
#include "iAQVTKWidget.h"
#include "iACsvConfig.h"
#include "iAObjectVis.h"
#include "iAEllipsoidObjectVis.h"
#include "iACylinderObjectVis.h"
#include "iAPolyObjectVisActor.h"
#include "iAVec3.h"

#include <QVBoxLayout>

#include <vtkInteractorObserver.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkTable.h>

#include <map>
#include <vector>
#include <algorithm>


iAComp3DWidget::iAComp3DWidget(
	iAMainWindow* parent, vtkSmartPointer<vtkTable> objectTable, std::shared_ptr<QMap<uint, uint>> columnMapping, const iACsvConfig& csvConfig) :
	QDockWidget(parent),
	m_objectColor(QColor(140, 140, 140, 255)),
	m_interactionStyle(vtkSmartPointer<iAComp3DWidgetInteractionStyle>::New())
{
	setupUi(this);

	auto layout = new QVBoxLayout;
	dockWidgetContents->setLayout(layout);

	m_qvtkWidget = new iAQVTKWidget(this);
	layout->addWidget(m_qvtkWidget);

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	addRendererToWidget(m_renderer);

	//interaction
	initializeInteraction();

	//rendering
	create3DVis(objectTable, columnMapping, csvConfig);
}

/*************** Rendering ****************************/
void iAComp3DWidget::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);
	
	renderWidget();
}

void iAComp3DWidget::renderWidget()
{
	m_qvtkWidget->renderWindow()->GetInteractor()->Render();
}

void iAComp3DWidget::addRendererToWidget(vtkSmartPointer<vtkRenderer> renderer)
{
	m_qvtkWidget->renderWindow()->AddRenderer(renderer);
}

void iAComp3DWidget::setInteractorStyleToWidget(vtkSmartPointer<vtkInteractorObserver> style)
{
	m_qvtkWidget->interactor()->SetInteractorStyle(style);
}

void iAComp3DWidget::removeAllRendererFromWidget()
{
	vtkRendererCollection* rendererList = m_qvtkWidget->renderWindow()->GetRenderers();

	vtkCollectionSimpleIterator sit;
	rendererList->InitTraversal(sit);
	for (int i = 0; i < rendererList->GetNumberOfItems(); i++)
	{
		vtkRenderer* currRend = rendererList->GetNextRenderer(sit);
		m_qvtkWidget->renderWindow()->RemoveRenderer(currRend);
	}
}

/*************** Initialization ****************************/
void iAComp3DWidget::create3DVis(vtkSmartPointer<vtkTable> objectTable, std::shared_ptr<QMap<uint, uint>> columnMapping, const iACsvConfig& csvConfig)
{
	if (csvConfig.visType == iAObjectVisType::Cylinder)
	{
		int cylinderQuality = csvConfig.cylinderQuality;
		size_t segmentSkip = csvConfig.segmentSkip;
		m_3dvisData = std::make_shared<iACylinderObjectVis>(
			std::make_shared<iAObjectsData>(objectTable, columnMapping),
			m_objectColor,
			std::map<size_t, std::vector<iAVec3f>>(),	// empty curved fiber info
			cylinderQuality,
			segmentSkip);
	}
	else if (csvConfig.visType == iAObjectVisType::Ellipsoid)
	{
		m_3dvisData = std::make_shared<iAEllipsoidObjectVis>(
			std::make_shared<iAObjectsData>(objectTable, columnMapping),
			m_objectColor);
	}
	m_3dvisActor = std::make_shared<iAPolyObjectVisActor>(m_renderer.Get(), m_3dvisData.get());
	m_3dvisActor->show();
}

void iAComp3DWidget::initializeInteraction()
{
	m_interactionStyle->setVisualization(this);
	m_interactionStyle->SetDefaultRenderer(m_renderer);
	setInteractorStyleToWidget(m_interactionStyle);
}

/*************** Update ****************************/

void iAComp3DWidget::resetWidget()
{
	//reset selection by rendering an empty vector of selectionIds
	m_3dvisData->renderSelection(std::vector<size_t>(), 0, m_objectColor, nullptr);
	renderWidget();
}

void iAComp3DWidget::drawSelection(std::vector<size_t> selectedIds)
{
	std::sort(selectedIds.begin(), selectedIds.end());
	const std::vector<size_t> constSelectedIds(selectedIds);
	m_3dvisData->renderSelection(constSelectedIds, 0, m_objectColor, nullptr);
	renderWidget();
}
