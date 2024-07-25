// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "iACompVisOptions.h"
#include "iAComp3DWidgetInteractionStyle.h"

//Qt
#include <QDockWidget>

//vtk
#include <vtkSmartPointer.h>

struct iACsvConfig;
class iAColoredPolyObjectVis;
class iAObjectsData;
class iAPolyObjectVisActor;

class iAQVTKWidget;
class iAMainWindow;

//vtk
class vtkTable;
class vtkInteractorObserver;
class vtkRenderer;

class iAComp3DWidget : public QDockWidget
{
	Q_OBJECT

public:

	iAComp3DWidget(iAMainWindow* parent, std::shared_ptr<iAObjectsData> objData, const iACsvConfig& csvConfig);

	void showEvent(QShowEvent* event) override;

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
	void create3DVis(const iACsvConfig& csvConfig);

	/*** Rendering ***/
	std::shared_ptr<iAColoredPolyObjectVis> m_3dvisData;
	std::shared_ptr<iAPolyObjectVisActor> m_3dvisActor;
	std::shared_ptr<iAObjectsData> m_objData;

	iAQVTKWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	QColor m_objectColor;

	vtkSmartPointer<iAComp3DWidgetInteractionStyle> m_interactionStyle;
};
