// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "iACompVisOptions.h"
#include "iAComp3DWidgetInteractionStyle.h"
#include "ui_CompHistogramTable.h"

//Qt
#include <QDockWidget>

//vtk
#include <vtkSmartPointer.h>


//CompVis
class iAMainWindow;

//iA
class iAQVTKWidget;
struct iACsvConfig;
class iAColoredPolyObjectVis;
class iAPolyObjectVisActor;

//vtk
class vtkTable;
class vtkInteractorObserver;
class vtkRenderer;

class iAComp3DWidget : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT

public:

	iAComp3DWidget(
		iAMainWindow* parent, vtkSmartPointer<vtkTable> objectTable, std::shared_ptr<QMap<uint, uint>> columnMapping, const iACsvConfig& csvConfig);

	void showEvent(QShowEvent* event);

	/*** Initialization ***/
	void initializeInteraction();

	/*** Rendering ***/
	void renderWidget();
	void addRendererToWidget(vtkSmartPointer<vtkRenderer> renderer);
	void setInteractorStyleToWidget(vtkSmartPointer<vtkInteractorObserver> style);
	void removeAllRendererFromWidget();

	/*** Update ***/
	void resetWidget();
	void drawSelection(std::vector<size_t> selectedIds);


private:

	/*** Initialization ***/
	void create3DVis(vtkSmartPointer<vtkTable> objectTable, std::shared_ptr<QMap<uint, uint>> columnMapping, const iACsvConfig& csvConfig);

	/*** Rendering ***/
	std::shared_ptr<iAColoredPolyObjectVis> m_3dvisData;
	std::shared_ptr<iAPolyObjectVisActor> m_3dvisActor;

	iAQVTKWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	QColor m_objectColor;

	vtkSmartPointer<iAComp3DWidgetInteractionStyle> m_interactionStyle;
};
