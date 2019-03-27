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


namespace propDefs {
	
	struct sliceProp {
		vtkProp3D *prop; 
		iASlicerMode mode;
	};

	struct SliceDefs {
		double fixedCoord;
		double x; 
		double y;
		double z; 
	};

	
	class PropModifier
	{
	public:

		 void updateProp(vtkProp3D *prop, iASlicerMode mode, const SliceDefs &sl_defs) {
			switch (mode)
			{
			case YZ:
				prop->SetPosition(sl_defs.fixedCoord, sl_defs.y, sl_defs.z);
				break;
			case XY:
				prop->SetPosition(sl_defs.x, sl_defs.y, sl_defs.fixedCoord);
				break;
			case XZ:
				prop->SetPosition(sl_defs.x,sl_defs.fixedCoord, sl_defs.z); 
				break;
			case SlicerModeCount:
				throw std::invalid_argument("invalid slicer mode"); 			
			}
		
		}
		
	};

}


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
		if (this->CurrentRenderer == nullptr || this->m_PropCurrentSlicer == nullptr)
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
		return this->m_PropCurrentSlicer; 
	}
	

	//one of them is coordinate of current slicer
	void initCoordinates(double x, double y, double z){
		sliceX = x; 
		sliceY = y;
		sliceZ = z; 
	
	}
	void initializeActors(vtkProp3D *propSlicer3D, vtkProp3D *propSlicer1, vtkProp3D *propSlicer2) {
		
		
		
		if (!propSlicer3D || !propSlicer1 || propSlicer2) {
		
			DEBUG_LOG("props are null"); 
		}
		
		this->set3DProp(propSlicer3D);
		this->setSlicer1(propSlicer1);
		this->setSlicer2(propSlicer2);
	}
	

	//modes of other two slicers
	void initModes(iASlicerMode mode_slice1, iASlicerMode mode_slice2); 
	//currentSlicer
	void setActiveSlicer(vtkProp3D *currentActor, iASlicerMode slice/*, int SliceNumber*/); 

	//identify which slicer is used
	void updateSlicer(); 

protected:
	
	double m_currentPos[3] ;

	iACustomInterActorStyleTrackBall();
	//~iACustomInterActorStyleTrackBall() override;
	void FindPickedActor(int x, int y);
	vtkCellPicker *InteractionPicker;

	void printProbOrigin(){
		double *pos = m_PropCurrentSlicer->GetOrigin(); 
		DEBUG_LOG(QString("\nOrigin x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2])); 
	}
	void printPropPosistion() {
		double *pos = m_PropCurrentSlicer->GetPosition();
		DEBUG_LOG(QString("\nPosition %1 %2 %3").arg(pos[0]).arg(pos[1]).arg(pos[2])); 
	}
	void printProbOrientation() {
		double *pos = m_PropCurrentSlicer->GetOrientation();
		DEBUG_LOG(QString("\nPosition x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
	}

private:
	/*void synchronizes()*/

	//setting to null
	void resetProps() {
		propSlicer1.prop = nullptr;
		propSlicer2.prop = nullptr;
		Prop3DSlicer = nullptr;
	}

	void setModeSlicer1(iASlicerMode mode) {
		propSlicer1.mode = mode;
	}

	void setModeSlicer2(iASlicerMode mode) {
		propSlicer2.mode = mode; 
	}

	void setPropSlicer1(double x, double y, double z) {
		propSlicer1.prop->SetPosition(x, y, z);
	}
	void setPropSlicer2(double x, double y, double z){
		propSlicer2.prop->SetPosition(x, y, z);
	}

	iASlicerMode activeSliceMode; 

	//Prop of the current slicer
	vtkProp3D *m_PropCurrentSlicer;

	//3d slicer prop
	vtkProp3D *Prop3DSlicer;


	propDefs::sliceProp propSlicer1; 
	propDefs::sliceProp propSlicer2;
		


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
		propSlicer1.prop = prop1;

	}

	void setSlicer2(vtkProp3D * prop2) {
		/*if (!prop2) { DEBUG_LOG("prop sli2 is null") };*/
		propSlicer2.prop = prop2;
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



