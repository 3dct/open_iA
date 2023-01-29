// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "ui_TrackingGraph.h"

#include <vtkSmartPointer.h>

#include <QDockWidget>

class iATrackingGraphItem;

class iAQVTKWidget;

class vtkMutableDirectedGraph;

class dlg_trackingGraph : public QDockWidget, private Ui_TrackingGraph
{
	Q_OBJECT

public:
	dlg_trackingGraph(QWidget* parent);
	void updateGraph(vtkSmartPointer<vtkMutableDirectedGraph> g, size_t numRanks);

private:
	iAQVTKWidget* m_graphWidget;
	vtkSmartPointer<iATrackingGraphItem> m_graphItem;
};
