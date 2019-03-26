#pragma once
#include "vtkInteractorStyleTrackballActor.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkProp3D.h"
#include "vtkObjectFactory.h"
#include "vtkCellPicker.h"
#include "QString"
#include "iAConsole.h"
#include "iASlicerMode.h"


// Motion flags

#define VTKIS_WINDOW_LEVEL 1024
#define VTKIS_SLICE        1025

// Style flags

#define VTKIS_IMAGE2D 2
#define VTKIS_IMAGE3D 3
#define VTKIS_IMAGE_SLICING 4



class iACustomInterActorStyleTrackBall : public vtkInteractorStyleTrackballActor
{
	public:

	static iACustomInterActorStyleTrackBall *New();
	vtkTypeMacro(iACustomInterActorStyleTrackBall, vtkInteractorStyleTrackballActor);

  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
	virtual void OnMouseMove();
	
	//we need the shift key to 
	virtual void OnLeftButtonDown() {

		int x = this->Interactor->GetEventPosition()[0];
		int y = this->Interactor->GetEventPosition()[1];

		this->FindPokedRenderer(x, y);
		this->Interactor->GetPicker()->Pick(x, y, 0, this->GetCurrentRenderer());
		this->FindPickedActor(x, y);
		if (this->CurrentRenderer == nullptr || this->PropCurrentSlicer == nullptr)
		{
			return;
		}

		double picked[3];
		this->Interactor->GetPicker()->GetPickPosition(picked);
		
		//connect the components; 
		printProbOrientation();
		printPropPosistion();
		printProbOrigin(); 
		
		DEBUG_LOG(QString("Picked 1% \t %2 \t %3").arg(picked[0]).arg(picked[1]).arg(picked[2])); 

		if (!this->Interactor->GetShiftKey())
			return;
		vtkInteractorStyleTrackballActor::OnLeftButtonDown();
	}

	virtual void OnLeftButtonUp(); 
	void OnMiddleButtonUp() override;	
	void OnRightButtonUp() override;

	//! @}
	//! @{ Conditionally disable zooming via right button dragging
	void OnRightButtonDown() override
	{
		if (!m_rightButtonDragZoomEnabled)
			return;
		vtkInteractorStyleTrackballActor::OnRightButtonDown();
	}
	
	void SetRightButtonDragZoomEnabled(bool enabled)
	{
		m_rightButtonDragZoomEnabled = enabled;
	}

	void Rotate() override
	{
		return; 
	}
	void Spin() override { return;  }
	
	
	void iACustomInterActorStyleTrackBall::Pick()
	{
		printProbOrigin();
		printPropPosistion();
		this->InvokeEvent(vtkCommand::PickEvent, this);
	}

	void SetInteractionModeToImage2D() {
		this->SetInteractionMode(VTKIS_IMAGE2D);
	}
	
	vtkSetClampMacro(InteractionMode, int, VTKIS_IMAGE2D, VTKIS_IMAGE_SLICING);
	vtkGetMacro(InteractionMode, int);

	vtkProp3D * getCurrentSlicerProp() const{
		return this->PropCurrentSlicer; 
	}
	
	//TODO miteinander verknüpfen

	void setActiveSlicer(iASlicerMode slice, int SliceNumber) {
		switch (slice) {
		case iASlicerMode::XY: sliceZ = SliceNumber; break;
		case iASlicerMode::XZ: sliceY = SliceNumber; break;
		case iASlicerMode::YZ: sliceX = SliceNumber; break;
		default: throw std::invalid_argument("Invalid slice option"); break; 
				
		}
	}
	
	void initializeActors(vtkProp3D *propSlicer3D, vtkProp3D *propSlicer1, vtkProp3D *propSlicer2) {
		if (!propSlicer3D || !propSlicer1 || propSlicer2) {
		
			DEBUG_LOG("props are null"); 
		}
		
		this->set3DProp(propSlicer3D);
		this->setSlicer1(propSlicer1);
		this->setSlicer2(propSlicer2);
	}


protected:
	
	double m_currentPos[3] ;

	iACustomInterActorStyleTrackBall();
	//~iACustomInterActorStyleTrackBall() override;
	void FindPickedActor(int x, int y);
	vtkCellPicker *InteractionPicker;

	
	void printProbOrigin(){
		double *pos = PropCurrentSlicer->GetOrigin(); 
		DEBUG_LOG(QString("\nOrigin x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2])); 
	}

	void printPropPosistion() {
		double *pos = PropCurrentSlicer->GetPosition();
		DEBUG_LOG(QString("\nPosition %1 %2 %3").arg(pos[0]).arg(pos[1]).arg(pos[2])); 
	}

	void printProbOrientation() {
		double *pos = PropCurrentSlicer->GetOrientation();
		DEBUG_LOG(QString("\nPosition x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
	}

private:
	
	/*void getOrientation(double *values, vtkProp3D *visprop) const { 
		values = visprop->GetOrientation();
	}

	void getPosition(double *values, vtkProp3D *visprop) const {
		values = visprop->GetPosition();
	}*/


	//setting to null
	void resetProps() {
		propSlicer1 = nullptr;
		propSlicer2 = nullptr;
		Prop3DSlicer = nullptr;
	}

	iASlicerMode activeSlicer; 
	//Prop of the current slicer
	vtkProp3D *PropCurrentSlicer;

	//3d slicer prop
	vtkProp3D *Prop3DSlicer;

	//3d slicer prop
	vtkProp3D *propSlicer1;
	vtkProp3D *propSlicer2;


	//koordinate of slices
	int sliceX;
	int sliceY;
	int sliceZ; 


	void set3DProp(vtkProp3D * prop3D) {
		/*if (!prop3D) { DEBUG_LOG("prop3d is null") };*/
		Prop3DSlicer = prop3D;
	}

	//+ set coordinate of slicer 1
	void setSlicer1(vtkProp3D * prop1) {
		/*if (!prop1) { DEBUG_LOG("prop sli1 is null") };*/
		propSlicer1 = prop1;

	}

	void setSlicer2(vtkProp3D * prop2) {
		/*if (!prop2) { DEBUG_LOG("prop sli2 is null") };*/
		propSlicer2 = prop2;
	}


	/*vtkInteractorStyleTrackballActor(const iACustomInterActorStyleTrackBall&) = delete;*/
	void operator=(const iACustomInterActorStyleTrackBall&) = delete;

	/*iACustomInterActorStyle(const iACustomInterActorStyle&);*/
	bool m_rightButtonDragZoomEnabled = false;
	int InteractionMode;

	/*double XViewRightVector[3];
	double XViewUpVector[3];
	double YViewRightVector[3];
	double YViewUpVector[3];
	double ZViewRightVector[3];
	double ZViewUpVector[3];
*/
	// Not implemented.
	//void operator=(const iACustomInterActorStyle&);  // Not implemented.
};


//vtkStandardNewMacro(iACustomInterActorStyleTrackBall);


