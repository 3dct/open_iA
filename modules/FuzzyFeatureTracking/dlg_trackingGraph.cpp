// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "dlg_trackingGraph.h"

#include "iATrackingGraphItem.h"
#include "iAVtkGraphDrawer.h"

#include <iAQVTKWidget.h>

#include <vtkContextActor.h>
#include <vtkContextInteractorStyle.h>
#include <vtkContextScene.h>
#include <vtkContextTransform.h>
#include <vtkContextView.h>
#include <vtkGraphItem.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>

const double BACKGROUND[3]		= {1, 1, 1};

dlg_trackingGraph::dlg_trackingGraph(QWidget *parent) :
	QDockWidget(parent),
	m_graphWidget(new iAQVTKWidget()),
	m_graphItem(vtkSmartPointer<iATrackingGraphItem>::New())
{
	setupUi(this);
	vtkNew<vtkContextTransform> trans;
	trans->SetInteractive(true);
	trans->AddItem(m_graphItem.GetPointer());
	vtkNew<vtkContextActor> actor;
	vtkNew<vtkContextView> contextView;
	auto contextScene = contextView->GetScene();
	contextScene->AddItem(trans.GetPointer());
	actor->SetScene(contextScene);
	vtkNew<vtkRenderer> renderer;
	renderer->SetBackground(BACKGROUND[0], BACKGROUND[1], BACKGROUND[2]);
	renderer->AddActor(actor.GetPointer());
	auto renWin = m_graphWidget->renderWindow();
	auto interactor = m_graphWidget->interactor();
	this->horizontalLayout->addWidget(m_graphWidget);
	renWin->AddRenderer(renderer.GetPointer());
	contextView->SetRenderWindow(renWin);
	vtkNew<vtkContextInteractorStyle> interactorStyle;
	interactorStyle->SetScene(contextScene);
	interactor->SetInteractorStyle(interactorStyle.GetPointer());
	contextView->SetInteractor(interactor);
	renWin->Render();
}

void dlg_trackingGraph::updateGraph(vtkSmartPointer<vtkMutableDirectedGraph> graph, size_t numRanks)
{
	if (graph->GetNumberOfVertices() < 1)
	{
		return;
	}
	vtkNew<vtkPoints> points;
	iAVtkGraphDrawer graphDrawer;
	auto renWin = m_graphWidget->renderWindow();
	graphDrawer.createLayout(points.GetPointer(), graph, renWin->GetSize(), numRanks);
	graph->SetPoints(points.GetPointer());

	m_graphItem->SetGraph(graph);
	m_graphItem->Update();
	renWin->Render();
	m_graphWidget->update();
}
