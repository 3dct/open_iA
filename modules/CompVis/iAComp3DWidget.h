// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once


//Qt
#include <QDockWidget>

//vtk
#include <vtkSmartPointer.h>

class iAComp3DWidgetInteractionStyle;

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

	iAComp3DWidget(iAMainWindow* parent, std::shared_ptr<iAObjectsData> objData);
	void showEvent(QShowEvent* event) override;
	void renderWidget();
	void resetWidget();
	void drawSelection(std::vector<size_t> selectedIds);

private:
	/*** Rendering ***/
	std::shared_ptr<iAColoredPolyObjectVis> m_3dvisData;
	std::shared_ptr<iAPolyObjectVisActor> m_3dvisActor;
	std::shared_ptr<iAObjectsData> m_objData;

	iAQVTKWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	QColor m_objectColor;

	vtkSmartPointer<iAComp3DWidgetInteractionStyle> m_interactionStyle;
};
