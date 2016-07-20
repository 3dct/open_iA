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
 
#ifndef DLG_EVENT_EXPLORER_H
#define	DLG_EVENT_EXPLORER_H

#include <QtGui>
#include <QObject>
#include <QList>
#include "ui_EventExplorer.h"
#include "QVTKWidget.h"

#include <qpushbutton.h>
#include <QHBoxLayout>

#include "vtkFloatArray.h"
#include <vtkIntArray.h>
#include "vtkMath.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkChartXY.h"
#include "vtkPlot.h"
#include "vtkTable.h"
#include "vtkPen.h"
#include "vtkTimerLog.h"
#include "vtkIdTypeArray.h"
#include "vtkEventQtSlotConnect.h"

#include "vtkMutableDirectedGraph.h"

#include <vector>

class dlg_trackingGraph;
class iAFeatureTracking;
class iAVolumeStack;

class dlg_eventExplorer : public QDockWidget, private Ui_EventExplorer
{
	Q_OBJECT

public:
	dlg_eventExplorer(QWidget *parent, int numberOfCharts, int numberOfEventTypes, iAVolumeStack *volumeStack, dlg_trackingGraph* trackingGraph, std::vector<iAFeatureTracking*> trackedFeaturesForwards, std::vector<iAFeatureTracking*> trackedFeaturesBackwards);
	~dlg_eventExplorer();

	private slots:
	void comboBoxXSelectionChanged(int s);
	void comboBoxYSelectionChanged(int s);

	void updateOpacityCreation(int v);
	void updateOpacityContinuation(int v);
	void updateOpacitySplit(int v);
	void updateOpacityMerge(int v);
	void updateOpacityDissipation(int v);

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

	QWidget* m_activeChild;
	QVTKWidget* m_chartWidget1;

	iAVolumeStack* m_volumeStack;

	int m_numberOfCharts;
	int m_numberOfEventTypes;
	int m_plotPositionInVector[5];
	int m_numberOfActivePlots;
	int m_propertyXId;
	int m_propertyYId;
	int m_rgb[5][3];

	std::vector<QVTKWidget*> m_widgets;
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

#endif
