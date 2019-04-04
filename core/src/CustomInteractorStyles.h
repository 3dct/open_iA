#pragma once

#include "iAConsole.h"
#include "iASlicerMode.h"
#include "mdichild.h"

#include <vtkInteractorStyleTrackballActor.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderWindowInteractor.h>
//#include <vtkProp3D.h>
#include <vtkObjectFactory.h>
#include <vtkCellPicker.h>

#include <QString>

#include <assert.h>

class iAChannelSlicerData;
class iAVolumeRenderer;

// Motion flags

#define VTKIS_WINDOW_LEVEL 1024
#define VTKIS_SLICE        1025

// Style flags

#define VTKIS_IMAGE2D 2
#define VTKIS_IMAGE3D 3
#define VTKIS_IMAGE_SLICING 4


namespace propDefs
{
	struct SliceDefs
	{
		double x; 
		double y;
		double z; 

		void print(QString obj)
		{
			DEBUG_LOG(obj + QString("Coords %1 %2 %3").arg(x).arg(y).arg(z)); 
		}
	};

	
	class PropModifier
	{
	public:
		/*
		static void updatePropOrientation(vtkProp3D *prop, double angle_x, double angle_y, double angle_z)
		{
			 prop->SetOrientation(angle_x, angle_y, angle_z); 
		 }
		

		 static void printProp(vtkProp3D *prop, QString Text, bool printOtherInfo)
		 {
			 if (!prop) { return;  }
			 
			 const double *orientation=prop->GetOrientation(); 
			 const double *position = prop->GetPosition();
			 const double *origin = prop->GetOrigin();

			 DEBUG_LOG(Text + QString("Position  x: %1 y: %2 z: %3").arg(position[0]).arg(position[1]).arg(position[2]));
			
			 if (printOtherInfo) {
				 DEBUG_LOG(Text + QString("Orientation x: %1, y: %2 , z: %3 ").arg(orientation[0]).arg(orientation[1]).arg(orientation[2]));
				 DEBUG_LOG(Text + QString("Origin X %1 y %2 z %3").arg(origin[0]).arg(origin[1]).arg(origin[1]));
			 }
		 }
			*/
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
		//! @}

	//! @{ Conditionally disable zooming via right button dragging

	void Rotate() override
	{
		if (enable3D) vtkInteractorStyleTrackballActor::Rotate();
		return; 
	}
	void Spin() override { if (enable3D) vtkInteractorStyleTrackballActor::Spin(); return;  }

	void iACustomInterActorStyleTrackBall::Pick()
	{
		//printProbOrigin();
		//printPropPosistion();
		this->InvokeEvent(vtkCommand::PickEvent, this);
	}
	void SetInteractionModeToImage2D()
	{
		this->SetInteractionMode(VTKIS_IMAGE2D);
	}
	

	vtkSetClampMacro(InteractionMode, int, VTKIS_IMAGE2D, VTKIS_IMAGE_SLICING);
	vtkGetMacro(InteractionMode, int);

	void initialize(vtkImageData *img, iAVolumeRenderer* volRend, iAChannelSlicerData *slicerChannel[4], int currentMode, MdiChild *mdiChild);
	
	//identify which slicer is used
	void updateInteractors(); 

	//void setSetcurrentSlicer();
signals:
	void actorsUpdated();
protected:

	MdiChild *m_mdiChild; 
	iAVolumeRenderer* m_volumeRenderer;

	iACustomInterActorStyleTrackBall();
	//~iACustomInterActorStyleTrackBall() override;
	//void FindPickedActor(int x, int y);
	//vtkCellPicker *InteractionPicker;

	//void printProbOrigin();
	//void printPropPosistion();
	//void printProbOrientation();

private:

	bool enable3D; /*= false;*/
	/*void synchronizes()*/

	//iASlicerMode activeSliceMode; 

	//3d renderer prop
	vtkImageData *m_image;


	iAChannelSlicerData* m_slicerChannel[3];
	int m_currentSliceMode;

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
