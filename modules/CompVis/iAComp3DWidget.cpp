// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAComp3DWidget.h"

//CompVis
#include "iAComp3DWidgetInteractionStyle.h"

//iA
#include <iACsvConfig.h>
#include <iACylinderObjectVis.h>
#include <iAEllipsoidObjectVis.h>
#include <iAMainWindow.h>
#include <iAObjectsData.h>
#include <iAPolyObjectVisActor.h>
#include <iAQVTKWidget.h>

#include <vtkInteractorObserver.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

#include <QVBoxLayout>

#include <algorithm>
#include <vector>


iAComp3DWidget::iAComp3DWidget(iAMainWindow* parent, std::shared_ptr<iAObjectsData> objData) :
	QDockWidget(parent),
	m_objData(objData),
	m_objectColor(QColor(140, 140, 140, 255)),
	m_interactionStyle(vtkSmartPointer<iAComp3DWidgetInteractionStyle>::New())
{
	setWidget(new QWidget);
	widget()->setLayout(new QVBoxLayout);
	widget()->layout()->setContentsMargins(0, 0, 0, 0);
	setFeatures(NoDockWidgetFeatures);
	m_qvtkWidget = new iAQVTKWidget(this);
	widget()->layout()->addWidget(m_qvtkWidget);

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	m_qvtkWidget->renderWindow()->AddRenderer(m_renderer);

	m_interactionStyle->setVisualization(this);
	m_interactionStyle->SetDefaultRenderer(m_renderer);
	m_qvtkWidget->interactor()->SetInteractorStyle(m_interactionStyle);

	// create 3D object visualization:
	if (m_objData->m_visType == iAObjectVisType::Cylinder)
	{
		m_3dvisData = std::make_shared<iACylinderObjectVis>(m_objData.get(), m_objectColor);
	}
	else if (m_objData->m_visType == iAObjectVisType::Ellipsoid)
	{
		m_3dvisData = std::make_shared<iAEllipsoidObjectVis>(m_objData.get(), m_objectColor);
	}
	m_3dvisActor = std::make_shared<iAPolyObjectVisActor>(m_renderer.Get(), m_3dvisData.get());
	m_3dvisActor->show();
}


/*************** Rendering ****************************/

void iAComp3DWidget::showEvent(QShowEvent* event)
{
	QDockWidget::showEvent(event);

	renderWidget();
}

void iAComp3DWidget::renderWidget()
{
	m_qvtkWidget->renderWindow()->Render();
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
