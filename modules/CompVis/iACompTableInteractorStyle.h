#pragma once
//vtk
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkIdTypeArray.h"

//Debug
#include "iALog.h"

//C++
#include <map>
#include <vector>

//Qt
#include <QString>

//CompVis
class iACompHistogramVis;

namespace Pick
{
	using PickedMap = std::map<vtkSmartPointer<vtkActor>, std::vector<vtkIdType>*>;

	void copyPickedMap(PickedMap* input, PickedMap* result);
	void debugPickedMap(PickedMap* input);
}

class iACompTableInteractorStyle: public vtkInteractorStyleTrackballCamera
{
public:
	static iACompTableInteractorStyle* New();
	vtkTypeMacro(iACompTableInteractorStyle, vtkInteractorStyleTrackballCamera);

	//initialization
	void setIACompHistogramVis(iACompHistogramVis* main);

	virtual void OnLeftButtonDown();
	virtual void OnLeftButtonUp();

	virtual void OnMouseMove();

	virtual void OnMiddleButtonDown();
	virtual void OnRightButtonDown();
	virtual void OnMouseWheelForward();
	virtual void OnMouseWheelBackward();
	virtual void OnKeyPress();
	virtual void OnKeyRelease();

	virtual void Pan();

protected:
	iACompTableInteractorStyle();

	/*** Interaction Camera Zoom ***/
	//general zooming in executed by the camera
	void generalZoomIn();
	//general zooming out executed by the camera
	void generalZoomOut();

	/*** Interaction Picking ***/
	virtual void storePickedActorAndCell(vtkSmartPointer<vtkActor> pickedA, vtkIdType id);

	iACompHistogramVis* m_main;

	//is always 1 -> only needed for changing the zoom for the camera
	double m_zoomLevel;
	//if ture --> zooming is active, otherwise ordering according to selected bins is active
	bool m_zoomOn;

	//stores for each actor a vector for each picked cell according to their vtkIdType
	Pick::PickedMap* m_picked;
	//store for reinitialization after minimization etc. of application
	Pick::PickedMap* m_pickedOld;
};


