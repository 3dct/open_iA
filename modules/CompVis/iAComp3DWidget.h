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
struct iACsvConfig;
class iA3DColoredPolyObjectVis;
class iA3DPolyObjectActor;

//vtk
class vtkTable;
class vtkInteractorObserver;
class vtkRenderer;

class iAComp3DWidget : public QDockWidget, public Ui_CompHistogramTable
{
	Q_OBJECT

public:

	iAComp3DWidget(
		iAMainWindow* parent, vtkSmartPointer<vtkTable> objectTable, QSharedPointer<QMap<uint, uint>> columnMapping, const iACsvConfig& csvConfig);

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
	void create3DVis(vtkSmartPointer<vtkTable> objectTable, QSharedPointer<QMap<uint, uint>> columnMapping, const iACsvConfig& csvConfig);

	/*** Rendering ***/
	std::shared_ptr<iA3DColoredPolyObjectVis> m_3dvisData;
	std::shared_ptr<iA3DPolyObjectActor> m_3dvisActor;

	iAQVTKWidget* m_qvtkWidget;
	vtkSmartPointer<vtkRenderer> m_renderer;

	QColor m_objectColor;

	vtkSmartPointer<iAComp3DWidgetInteractionStyle> m_interactionStyle;
};
