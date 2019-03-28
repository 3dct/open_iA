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
#include <assert.h>


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
		
		//ex slice xy -> z is fixed
		double fixedCoord; 
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

		//update the position while keeping one coordinate fixed, based on slicer mode
		static void updateSlicerPosition(vtkProp3D *prop, iASlicerMode mode, const SliceDefs &sl_defs, QString text) {
			assert(prop && "object is null"); 
			if (!prop) {
				DEBUG_LOG("update Prop failed");
				throw std::invalid_argument("nullpointer of prop"); 

			}	

			double *pos = prop->GetPosition();

			switch (mode)
			{
			case YZ:
				//x is fixed
				pos[0] = sl_defs.fixedCoord; //keep 
				pos[1] += sl_defs.y;
				pos[2] += sl_defs.z;
				prop->SetPosition(pos);
				break;
			case XY:
				//z is fixed
				pos[0] = sl_defs.x; 
				pos[1] += sl_defs.y;
				pos[2] += sl_defs.fixedCoord;//keep 
				prop->SetPosition(pos);			
				break;
			case XZ:
				//y fixed
				pos[0] = sl_defs.x; 
				pos[1] += sl_defs.fixedCoord;//keep 
				pos[2] += sl_defs.z;
				prop->SetPosition(pos);
				break;
				prop->SetPosition(sl_defs.x,sl_defs.fixedCoord, sl_defs.z); 
				break;
			case SlicerModeCount:
				throw std::invalid_argument("invalid slicer mode"); 			
			}

			PropModifier::printProp(prop, text);
		
		}
				
		static void updatePropOrientation(vtkProp3D *prop, double angle_x, double angle_y, double angle_z){
			 prop->SetOrientation(angle_x, angle_y, angle_z); 
		 }
		

		 static void printProp(vtkProp3D *prop, QString Text) {
			 if (!prop) { return;  }
			 
			 const double *orientation=prop->GetOrientation(); 
			 const double *position = prop->GetPosition();

			 DEBUG_LOG(Text + QString("Printing prop pos %1 %2 %3").arg(position[0]).arg(position[1]).arg(position[2]));

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
		if (this->CurrentRenderer == nullptr || this->m_PropCurrentSlicer.prop == nullptr 
			|| this->m_propSlicer1.prop == nullptr || this->m_propSlicer2.prop == nullptr )
		{
			DEBUG_LOG("Either renderer or props are null"); 
			return;
		}

		double picked[3];
		this->Interactor->GetPicker()->GetPickPosition(picked);
		DEBUG_LOG(QString("Picked 1% \t %2 \t %3").arg(picked[0]).arg(picked[1]).arg(picked[2]));

		//connect the components; 
		printProbOrientation();
		printPropPosistion();
		printProbOrigin(); 

		//hier sind die Props noch drin


		assert(this->m_Prop3DSlicer && "prop 3D slicer null");
		assert(this->m_propSlicer1.prop && "prop Slicer 1 null");
		assert(this->m_propSlicer2.prop && "prop Slicer 2 null");

		DEBUG_LOG("uiiii"); 
		updateSlicer(); 
		
		
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
		return this->m_PropCurrentSlicer.prop; 
	}
	

	//one of them is coordinate of current slicer
	void initCoordinates(double x, double y, double z){
		sliceX = x; 
		sliceY = y;
		sliceZ = z; 
		//m_PropCurrentSlicer

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
	void setActiveSlicer(vtkProp3D *currentActor, iASlicerMode slice, int activeSliceNr); 

	//identify which slicer is used
	void updateSlicer(); 

	//void setSetcurrentSlicer();
protected:
	
	double m_currentPos[3] ;

	iACustomInterActorStyleTrackBall();
	//~iACustomInterActorStyleTrackBall() override;
	void FindPickedActor(int x, int y);
	vtkCellPicker *InteractionPicker;

	void printProbOrigin(){
		double *pos = m_PropCurrentSlicer.prop->GetOrigin(); 
		DEBUG_LOG(QString("\nOrigin x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2])); 
	}
	void printPropPosistion() {
		double *pos = m_PropCurrentSlicer.prop->GetPosition();
		DEBUG_LOG(QString("\nPosition %1 %2 %3").arg(pos[0]).arg(pos[1]).arg(pos[2])); 
	}
	void printProbOrientation() {
		double *pos = m_PropCurrentSlicer.prop->GetOrientation();
		DEBUG_LOG(QString("\nPosition x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
	}

private:
	/*void synchronizes()*/

	//setting to null
	void resetProps() {
		m_propSlicer1.prop = nullptr;
		m_propSlicer2.prop = nullptr;
		m_Prop3DSlicer = nullptr;
	}

	void setModeSlicer1(iASlicerMode mode) {
		m_propSlicer1.mode = mode;
	}

	void setModeSlicer2(iASlicerMode mode) {
		m_propSlicer2.mode = mode; 
	}

	void setPropSlicer1(double x, double y, double z) {
		m_propSlicer1.prop->SetPosition(x, y, z);
	}
	void setPropSlicer2(double x, double y, double z){
		m_propSlicer2.prop->SetPosition(x, y, z);
	}

	//iASlicerMode activeSliceMode; 

	//Prop of the current slicer
	propDefs::sliceProp/*vtkProp3D*/ /***/m_PropCurrentSlicer;

	//3d slicer prop
	vtkProp3D *m_Prop3DSlicer;


	propDefs::sliceProp m_propSlicer1; 
	propDefs::sliceProp m_propSlicer2;
		
	
	//koordinate of slices
	int sliceX;
	int sliceY;
	int sliceZ; 


	void set3DProp(vtkProp3D * prop3D) {
		/*if (!prop3D) { DEBUG_LOG("prop3d is null") };*/
		m_Prop3DSlicer = prop3D;
	}

	//+ set coordinate of slicer 1
	void setSlicer1(vtkProp3D * prop1) {
		/*if (!prop1) { DEBUG_LOG("prop sli1 is null") };*/
		m_propSlicer1.prop = prop1;

	}

	void setSlicer2(vtkProp3D * prop2) {
		/*if (!prop2) { DEBUG_LOG("prop sli2 is null") };*/
		m_propSlicer2.prop = prop2;
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



