#pragma once

#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkSmartPointer.h"

#include <map>
#include <vector>
#include "vtkActor.h"
#include "vtkIdTypeArray.h"

class iACompHistogramTable;
class vtkRenderer;

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

   protected:
	iACompHistogramTableInteractorStyle();

   private:	
	//highlight the selected cells with an outline
	void highlightSelectedCell(vtkSmartPointer<vtkActor> pickedActor, vtkIdType pickedCellId);
	//dehighlight the selected cells with an outline 
	//(necessary that the renderer only contains the datarows for further calculations)
	void removeHighlightedCells();

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

	//stores the actors added to display the border of the selected cells
	//have to be removed before any calculation for zooming can take place!
	std::vector<vtkSmartPointer<vtkActor>>* m_highlighingActors;
};
