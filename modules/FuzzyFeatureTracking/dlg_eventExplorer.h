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
#pragma once

#include "ui_EventExplorer.h"

#include <iAVtkWidgetFwd.h>

#include <vtkSmartPointer.h>

#include <QDockWidget>

#include <vector>

class dlg_trackingGraph;
class iAFeatureTracking;
class iAVolumeStack;

class vtkEventQtSlotConnect;
class vtkChartXY;
class vtkContextView;
class vtkDoubleArray;
class vtkIntArray;
class vtkMutableDirectedGraph;
class vtkPlot;
class vtkStringArray;
class vtkTable;

class dlg_eventExplorer : public QDockWidget, private Ui_EventExplorer
{
	Q_OBJECT

public:
	dlg_eventExplorer(QWidget *parent, size_t numberOfCharts, int numberOfEventTypes, iAVolumeStack *volumeStack, dlg_trackingGraph* trackingGraph, std::vector<iAFeatureTracking*> trackedFeaturesForwards, std::vector<iAFeatureTracking*> trackedFeaturesBackwards);
	~dlg_eventExplorer();

private slots:
	void comboBoxXSelectionChanged(int s);
	void comboBoxYSelectionChanged(int s);

	void updateOpacityGrid(int v);

	void updateCheckBoxCreation(int c);
	void updateCheckBoxContinuation(int c);
	void updateCheckBoxSplit(int c);
	void updateCheckBoxMerge(int c);
	void updateCheckBoxDissipation(int c);

	void updateCheckBoxLogX(int c);
	void updateCheckBoxLogY(int c);

	void chartMouseButtonCallBack(vtkObject * obj);

private:
	void buildGraph(int id, int layer, int eventType, double uncertainty);
	void buildSubGraph(int id, int layer);
	void updateChartData(int axis, int s);
	void updateChartLogScale(int axis, bool log);
	void updateOpacity(int v, int eventType);

	iAVolumeStack* m_volumeStack;
	size_t m_numberOfCharts;
	int m_numberOfEventTypes;
	int m_plotPositionInVector[5];
	int m_numberOfActivePlots;
	int m_propertyXId;
	int m_propertyYId;
	int m_rgb[5][3];

	std::vector<iAVtkWidget*> m_widgets;
	std::vector<vtkSmartPointer<vtkContextView>> m_contextViews;
	std::vector<vtkSmartPointer<vtkChartXY>> m_charts;
	std::vector<vtkSmartPointer<vtkPlot>> m_plots;
	std::vector<vtkSmartPointer<vtkTable>> m_tables;

	std::vector<iAFeatureTracking*> m_trackedFeaturesForwards;
	std::vector<iAFeatureTracking*> m_trackedFeaturesBackwards;

	dlg_trackingGraph* m_trackingGraph;

	std::vector<std::vector<int>> m_nodes;
	std::map<int, bool> m_visitedNodes;
	std::map<vtkIdType, int> m_nodesToLayers;
	std::map<int, std::map<vtkIdType, int>> m_graphToTableId;
	std::map<int, std::map<vtkIdType, int>> m_tableToGraphId;

	vtkMutableDirectedGraph* m_graph;
	vtkStringArray* m_labels;
	vtkIntArray* m_nodeLayer;
	vtkIntArray* m_colorR;
	vtkIntArray* m_colorG;
	vtkIntArray* m_colorB;
	vtkDoubleArray* m_trackingUncertainty;
	vtkEventQtSlotConnect* m_chartConnections;
};
