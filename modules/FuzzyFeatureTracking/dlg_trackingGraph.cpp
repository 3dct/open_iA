/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "dlg_trackingGraph.h"

#include "iATrackingGraphItem.h"
#include "iAVtkGraphDrawer.h"
#include "iAVtkWidget.h"

#include <vtkContextActor.h>
#include <vtkContextInteractorStyle.h>
#include <vtkContextScene.h>
#include <vtkContextTransform.h>
#include <vtkContextView.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkGraphItem.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

const double BACKGROUND[3]		= {1, 1, 1};

dlg_trackingGraph::dlg_trackingGraph(QWidget *parent) :
	QDockWidget(parent),
	m_graphWidget(new iAVtkWidget()),
	m_graphItem(vtkSmartPointer<iATrackingGraphItem>::New())
{
	m_graphWidget->setFormat(iAVtkWidget::defaultFormat());
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
	auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	m_graphWidget->SetRenderWindow(renWin);
	auto interactor = m_graphWidget->GetInteractor();
#else
	m_graphWidget->setRenderWindow(renWin);
	auto interactor = m_graphWidget->interactor();
#endif
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
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	auto renWin = m_graphWidget->GetRenderWindow();
#else
	auto renWin = m_graphWidget->renderWindow();
#endif
	graphDrawer.createLayout(points.GetPointer(), graph, renWin->GetSize(), numRanks);
	graph->SetPoints(points.GetPointer());

	m_graphItem->SetGraph(graph);
	m_graphItem->Update();
	renWin->Render();
	m_graphWidget->update();
}
