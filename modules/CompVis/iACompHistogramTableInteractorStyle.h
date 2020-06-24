#pragma once

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkSmartPointer.h"
#include "iACompHistogramTableData.h"

#include <map>
#include <vector>
#include "vtkActor.h"
#include "vtkIdTypeArray.h"

class iACompHistogramTable;
class vtkRenderer;
class iACompVisMain;
class vtkPropPicker;

namespace Pick
{
using PickedMap = std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>;
}

class iACompHistogramTableInteractorStyle : public vtkInteractorStyleTrackballCamera
{
   public:
	static iACompHistogramTableInteractorStyle* New();
	vtkTypeMacro(iACompHistogramTableInteractorStyle, vtkInteractorStyleTrackballCamera);

	virtual void OnLeftButtonDown();
	virtual void OnMiddleButtonDown();
	virtual void OnRightButtonDown();
	virtual void OnMouseWheelForward();
	virtual void OnMouseWheelBackward();
	virtual void OnKeyPress();
	virtual void OnKeyRelease();

	//init iACompHistogramTable
	void setIACompHistogramTable(iACompHistogramTable* visualization);
	void setIACompVisMain(iACompVisMain* main);

   protected:
	iACompHistogramTableInteractorStyle();

   private:	
	
	//general zooming in executed by the camera
	void generalZoomIn();
	//general zooming out executed by the camera
	void generalZoomOut();

	//linear zooming in over all bins of the histograms
	void linearZoomInHistogram();
	//linear zooming out over all bins of the histograms
	void linearZoomOutHistogram();

	//non linear zooming in - zooming in on currently selected bin(s)/row(s)
	void nonLinearZoomIn();
	//non linear zooming out - zooming out on currently selected bin(s)/row(s)
	void nonLinearZoomOut();

	void updateOtherCharts();

	//set the picklist for the propPicker to only pick original row actors
	void setPickList(std::vector<vtkSmartPointer<vtkActor>>* originalRowActors);

	iACompHistogramTable* m_visualization;
	//is a u
	Pick::PickedMap* m_picked;

	//controls whether the linear or non-linear zoom on the histogram is activated
	//true --> non-linear zoom is activated, otherwise linear zoom
	bool m_controlBinsInZoomedRows;
	//controls if the number of bins is modified or if the point representation should be drawn
	//true --> point representation is drawn
	bool m_pointRepresentationOn;

	//is always 1 -> only needed for changing the zoom for the camera
	double m_zoomLevel;


	QList<bin::BinType*>* m_zoomedRowData;

	iACompVisMain* m_main;

	vtkSmartPointer<vtkPropPicker> m_actorPicker;
};
