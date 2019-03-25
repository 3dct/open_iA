#pragma once
#include "vtkInteractorStyleTrackballActor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkObjectFactory.h"


// Motion flags

#define VTKIS_WINDOW_LEVEL 1024
#define VTKIS_SLICE        1025

// Style flags

#define VTKIS_IMAGE2D 2
#define VTKIS_IMAGE3D 3
#define VTKIS_IMAGE_SLICING 4



class iACustomInterActorStyleTrackBall : public vtkInteractorStyleTrackballCamera
{
	
	public:

	static iACustomInterActorStyleTrackBall *New();
	vtkTypeMacro(iACustomInterActorStyleTrackBall, vtkInteractorStyleTrackballCamera);
	//iACustomInterActorStyle();
	// Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
	virtual void OnMouseMove();


	//we need the shift key to 
	virtual void OnLeftButtonDown() {
		if (!this->Interactor->GetShiftKey())
			return;
		vtkInteractorStyleTrackballCamera::OnLeftButtonDown();

	}
	/*virtual void OnLeftButtonUp();
	virtual void OnMiddleButtonDown();
	virtual void OnMiddleButtonUp();*/

	//! @}
	//! @{ Conditionally disable zooming via right button dragging
	void OnRightButtonDown() override
	{
		if (!m_rightButtonDragZoomEnabled)
			return;
		vtkInteractorStyleTrackballCamera::OnRightButtonDown();
	}
	
	
	/*virtual void OnRightButtonUp();
	virtual void OnMouseWheelForward();
	virtual void OnMouseWheelBackward();*/

	void SetRightButtonDragZoomEnabled(bool enabled)
	{
		m_rightButtonDragZoomEnabled = enabled;
	}


	void Rotate() override
	{
		return; 
	}


protected:
	iACustomInterActorStyleTrackBall();
	~iACustomInterActorStyleTrackBall() override;
	//	~iACustomInterActorStyle();
	//
		/*double MotionFactor;*/

		/*virtual void Dolly(double factor);*/

private:

	/*iACustomInterActorStyle(const iACustomInterActorStyle&);*/
	bool m_rightButtonDragZoomEnabled = false;
	int InteractionMode;

	double XViewRightVector[3];
	double XViewUpVector[3];
	double YViewRightVector[3];
	double YViewUpVector[3];
	double ZViewRightVector[3];
	double ZViewUpVector[3];

	// Not implemented.
	//void operator=(const iACustomInterActorStyle&);  // Not implemented.
};


vtkStandardNewMacro(iACustomInterActorStyleTrackBall);


