#pragma once

#include "iAConsole.h"
#include "iASlicerMode.h"
#include "mdichild.h"

#include <vtkInteractorStyleTrackballActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkProp3D.h>
#include <vtkObjectFactory.h>
#include <vtkCellPicker.h>

#include <QString>

#include <assert.h>


class iAVolumeRenderer;

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
		double x; 
		double y;
		double z; 

		void print(QString obj) {
			DEBUG_LOG(obj + QString("Coords %1 %2 %3").arg(x).arg(y).arg(z)); 
		}
	};

	
	class PropModifier
	{
	public:
		
		

		static void updatePropOrientation(vtkProp3D *prop, double angle_x, double angle_y, double angle_z)
		{
			 prop->SetOrientation(angle_x, angle_y, angle_z); 
		 }
		

		 static void printProp(vtkProp3D *prop, QString Text)
		 {
			 if (!prop) { return;  }
			 
			 const double *orientation=prop->GetOrientation(); 
			 const double *position = prop->GetPosition();

			 DEBUG_LOG(Text + QString("x: %1 y: %2 z: %3").arg(position[0]).arg(position[1]).arg(position[2]));

		 }

		 static QString printMode(iASlicerMode mode) {
			 QString tmp = "Slicer \t"; 
			 switch (mode)
			 {
			 case YZ:
				 tmp += "yz \t";
				 break;
			 case XY:
				 tmp += "XY \t";
				 break;
			 case XZ:
				 tmp += "xz \t";
				 break;			
			 default:
				 break;
			 }

			 return tmp; 
		 }
	};

}


class iACustomInterActorStyleTrackBall : public QObject, public vtkInteractorStyleTrackballActor
{
	Q_OBJECT
public:

	static iACustomInterActorStyleTrackBall *New();
	vtkTypeMacro(iACustomInterActorStyleTrackBall, vtkInteractorStyleTrackballActor);

  // Event bindings controlling the effects of pressing mouse buttons
  // or moving the mouse.
	virtual void OnMouseMove();
	
	//we need the shift key to 
	virtual void OnLeftButtonDown()
	{
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
	void SetInteractionModeToImage2D()
	{
		this->SetInteractionMode(VTKIS_IMAGE2D);
	}
	
	vtkSetClampMacro(InteractionMode, int, VTKIS_IMAGE2D, VTKIS_IMAGE_SLICING);
	vtkGetMacro(InteractionMode, int);

	vtkProp3D * getCurrentSlicerProp() const
	{
		return this->m_PropCurrentSlicer.prop; 
	}
	
	//one of them is coordinate of current slicer
	void initCoordinates(double x, double y, double z)
	{
		sliceX = x; 
		sliceY = y;
		sliceZ = z; 
	}

	void initializeActors(iAVolumeRenderer *volRend, vtkProp3D *propSlicer1, vtkProp3D *propSlicer2)
	{
		if (!volRend || !propSlicer1 || !propSlicer2)
		{
			DEBUG_LOG("props are null"); 
		}		
		this->set3DProp(volRend);
		this->setSlicer1(propSlicer1);
		this->setSlicer2(propSlicer2);
	}
	
	//modes of other two slicers
	void initModes(iASlicerMode mode_slice1, double coordSlice1, iASlicerMode mode_slice2, double coordSlice2); 
	//currentSlicer
	void setActiveSlicer(vtkProp3D *currentActor, iASlicerMode slice, int activeSliceNr); 

	//identify which slicer is used
	void updateSlicer(); 

	void setMDIChild(MdiChild *mdiChild) {
		m_mdiChild = mdiChild; 
	};

	//void setSetcurrentSlicer();
signals:
	void actorsUpdated();
protected:
	
	double m_currentPos[3] ;
	MdiChild *m_mdiChild; 

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
		m_volumeRenderer = nullptr;
	}

	void setModeSlicer1(iASlicerMode mode, double coord) {
		m_propSlicer1.mode = mode;
		m_propSlicer1.fixedCoord = coord; 
	}

	void setModeSlicer2(iASlicerMode mode, double coord) {
		m_propSlicer2.mode = mode; 
		m_propSlicer2.fixedCoord = coord; 
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

	//3d renderer prop
	iAVolumeRenderer *m_volumeRenderer;


	propDefs::sliceProp m_propSlicer1; 
	propDefs::sliceProp m_propSlicer2;
		
	
	//koordinate of slices
	int sliceX;
	int sliceY;
	int sliceZ; 


	void set3DProp(iAVolumeRenderer * renderer) {
		/*if (!prop3D) { DEBUG_LOG("prop3d is null") };*/
		m_volumeRenderer = renderer;
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
	void operator=(const iACustomInterActorStyleTrackBall&) = delete;  // Not implemented.
	iACustomInterActorStyleTrackBall(const iACustomInterActorStyleTrackBall &) = delete;
};
