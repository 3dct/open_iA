#pragma once
//vtk
#include "vtkInteractorStyleTrackballCamera.h"

//Debug
#include "iALog.h"

//CompVis
//#include "iACompUniformTable.h"
//#include "iACompHistogramVis.h"
class iACompHistogramVis;

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

	//general zooming in executed by the camera
	void generalZoomIn();
	//general zooming out executed by the camera
	void generalZoomOut();

	iACompHistogramVis* m_main;

	//is always 1 -> only needed for changing the zoom for the camera
	double m_zoomLevel;
	//if ture --> zooming is active, otherwise ordering according to selected bins is active
	bool m_zoomOn;

};


