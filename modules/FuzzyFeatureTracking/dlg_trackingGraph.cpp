/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "dlg_trackingGraph.h"

#include "iAVtkGraphDrawer.h"
// VTK
#include <vtkGraphItem.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#ifdef _MSC_VER
#include <windows.h>
#endif


const int MAX_ITERATIONS		= 24;
const double BACKGROUND[3]		= {1, 1, 1};


/**
 * Constructor
 */
dlg_trackingGraph::dlg_trackingGraph(QWidget *parent) : QDockWidget(parent)
{
	setupUi(this);

	// create qt graph widget
	graphWidget = new QVTKWidget();
	this->horizontalLayout->addWidget(graphWidget);

	// create graph
	m_graph = vtkSmartPointer<vtkMutableDirectedGraph>::New();

	m_graphItem = vtkSmartPointer<iATrackingGraphItem>::New();
	m_graphItem->SetGraph(m_graph);

	m_trans = vtkSmartPointer<vtkContextTransform>::New();
	m_trans->SetInteractive(true);
	m_trans->AddItem(m_graphItem);

	m_contextScene = vtkSmartPointer<vtkContextScene>::New();
	m_contextScene->AddItem(m_trans);

	m_actor = vtkSmartPointer<vtkContextActor>::New();
	m_actor->SetScene(m_contextScene);

	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	m_renderer->SetBackground(BACKGROUND[0], BACKGROUND[1], BACKGROUND[2]);
	m_renderer->AddActor(m_actor);

	m_renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
	m_renderWindow->AddRenderer(m_renderer);

	graphWidget->SetRenderWindow(m_renderWindow);

	m_interactorStyle = vtkSmartPointer<vtkContextInteractorStyle>::New();
	m_interactorStyle->SetScene(m_contextScene);

	m_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	m_interactor->SetInteractorStyle(m_interactorStyle);
	m_interactor->SetRenderWindow(m_renderWindow);
	m_interactor->Start();

	m_renderWindow->Render();
}

dlg_trackingGraph::~dlg_trackingGraph()
{

}

void dlg_trackingGraph::updateGraph(vtkMutableDirectedGraph* g, int nunRanks, map<vtkIdType, int> nodesToLayers, map<int, map<vtkIdType, int>> graphToTableId)
{
	if(g->GetNumberOfVertices() < 1) return;

	this->m_graph = g;
	this->m_nodesToLayers = nodesToLayers;

	vtkNew<vtkPoints> points;	
	iAVtkGraphDrawer graphDrawer;
	//graphDrawer.setMaxIteration(MAX_ITERATIONS);
	graphDrawer.createLayout(points.GetPointer(), m_graph, m_renderWindow->GetSize(), nunRanks);
	m_graph->SetPoints(points.GetPointer());
	
	m_graphItem->SetGraph(m_graph);
	m_graphItem->Update();
	m_renderWindow->Render();
}
