/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#pragma once

// Qt
#include <QDockWidget>
#include <QWidget>
// iA
#include "ui_TrackingGraph.h"
#include "iATrackingGraphItem.h"
// VTK
#include <QVTKWidget.h>
#include <vtkSmartPointer.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkContextInteractorStyle.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkContextTransform.h>
#include <vtkContextActor.h>
#include <vtkMutableDirectedGraph.h>
#include <vtkContextScene.h>
// std
#include <vector>
#include <map>

using namespace std;

class dlg_trackingGraph : public QDockWidget, private Ui_TrackingGraph
{
	Q_OBJECT

public:
			dlg_trackingGraph(QWidget* parent);
			~dlg_trackingGraph();

	void	updateGraph(vtkMutableDirectedGraph* g, int nunRanks, map<vtkIdType, int> nodesToLayers, map<int, map<vtkIdType, int>> graphToTableId);

private: 
	QVTKWidget*		graphWidget;

	vtkSmartPointer<vtkMutableDirectedGraph>	m_graph;
	vtkSmartPointer<iATrackingGraphItem>		m_graphItem;
	vtkSmartPointer<vtkContextActor>			m_actor;
	vtkSmartPointer<vtkContextTransform>		m_trans;
	vtkSmartPointer<vtkRenderer>				m_renderer;
	vtkSmartPointer<vtkContextScene>			m_contextScene;
	vtkSmartPointer<vtkRenderWindow>			m_renderWindow;
	vtkSmartPointer<vtkContextInteractorStyle>	m_interactorStyle;
	vtkSmartPointer<vtkRenderWindowInteractor>	m_interactor;

	map<vtkIdType, int>					m_nodesToLayers;
	map<int, map<vtkIdType, int>>		m_graphToTableId;
};
