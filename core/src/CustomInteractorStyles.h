#pragma once
#include "vtkInteractorStyleTrackballActor.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"


class iACustomInterActorStyleTrackBall : public vtkInteractorStyleTrackballActor /*evtl interactorstyleImage*/ {
	
	public:

	static iACustomInterActorStyleTrackBall *New();
	vtkTypeMacro(iACustomInterActorStyleTrackBall, vtkInteractorStyleTrackballActor);
	//iACustomInterActorStyle();
	// Description:
  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
	virtual void OnMouseMove() {

		return;
	}

	virtual void OnLeftButtonDown() {
		if (!this->Interactor->GetShiftKey())
			return;
		vtkInteractorStyle::OnLeftButtonDown();

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
		vtkInteractorStyle::OnRightButtonDown();
	}
	/*virtual void OnRightButtonUp();
	virtual void OnMouseWheelForward();
	virtual void OnMouseWheelBackward();*/

	void SetRightButtonDragZoomEnabled(bool enabled)
	{
		m_rightButtonDragZoomEnabled = enabled;
	}


protected:

	//	~iACustomInterActorStyle();
	//
		/*double MotionFactor;*/

		/*virtual void Dolly(double factor);*/

private:

	/*iACustomInterActorStyle(const iACustomInterActorStyle&);*/
	bool m_rightButtonDragZoomEnabled = false;
	// Not implemented.
	//void operator=(const iACustomInterActorStyle&);  // Not implemented.
};


vtkStandardNewMacro(iACustomInterActorStyleTrackBall);