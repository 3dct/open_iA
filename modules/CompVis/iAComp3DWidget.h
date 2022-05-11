#pragma once

//CompVis
#include "iACsvDataStorage.h"
#include "iACompVisOptions.h"
#include "iAComp3DWidgetInteractionStyle.h"

//Qt
#include <QDockWidget>
#include "ui_CompHistogramTable.h"

//vtk
#include "vtkSmartPointer.h"


//CompVis
class iAMainWindow;

//iA
class iAQVTKWidget;
class iACsvIO;
struct iACsvConfig;
class iA3DEllipseObjectVis;
class iA3DCylinderObjectVis;

//vtk
class vtkTable;
class vtkInteractorObserver;
class vtkRenderer;

class iAComp3DWidget : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT

public:

	iAComp3DWidget(
		iAMainWindow* parent, vtkSmartPointer<vtkTable> objectTable, iACsvIO* io, const iACsvConfig* csvConfig);

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
	void drawSelection(std::vector<size_t> selectedIds, QColor color);


private:

	/*** Initialization ***/
	void create3DVis();

	/*** Initialization for Rendering with iAobjectvis***/
	vtkSmartPointer<vtkTable> m_objectTable;
	iACsvIO* m_io;
	const iACsvConfig* m_csvConfig;

	iA3DCylinderObjectVis* m_3dvisCylinders;
	iA3DEllipseObjectVis* m_3dvisEllipses;

	/*** Rendering ***/
	iAQVTKWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	QColor m_objectColor;

	vtkSmartPointer<iAComp3DWidgetInteractionStyle> m_interactionStyle;
};
