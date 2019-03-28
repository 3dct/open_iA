#include "CustomInterActorStyles.h"
#include <limits>


vtkStandardNewMacro(iACustomInterActorStyleTrackBall);

iACustomInterActorStyleTrackBall::iACustomInterActorStyleTrackBall() {

	InteractionMode = 0;
	this->m_PropCurrentSlicer.prop = nullptr;
	this->m_Prop3DSlicer = nullptr; 
	this->m_propSlicer1.prop = nullptr;
	this->m_propSlicer2.prop = nullptr; 

	this->InteractionPicker = vtkCellPicker::New();
	this->InteractionPicker->SetTolerance(100.0);
	m_currentPos[0] = std::numeric_limits<double>::min();
	m_currentPos[1] = std::numeric_limits<double>::min();
	m_currentPos[2] = std::numeric_limits<double>::min();
}

void iACustomInterActorStyleTrackBall::OnMouseMove()
{
	int x = this->Interactor->GetEventPosition()[0];
	int y = this->Interactor->GetEventPosition()[1];

	switch (this->State)
	{
		/*case VTKIS_WINDOW_LEVEL:
			this->FindPokedRenderer(x, y);
			this->WindowLevel();
			this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
			break;*/

	case VTKIS_PICK:
		this->FindPokedRenderer(x, y);
		this->Pick();
		this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
		break;

	/*case VTKIS_SLICE:
		this->FindPokedRenderer(x, y);
		this->Slice();
		this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
		break;*/
	}

	// Call parent to handle all other states and perform additional work

	this->Superclass::OnMouseMove();


}

void iACustomInterActorStyleTrackBall::OnLeftButtonUp()
{
	switch (this->State)
	{
	case VTKIS_PAN:
		this->EndPan();
		break;

	case VTKIS_SPIN:
		this->EndSpin();
		break;

	case VTKIS_ROTATE:
		this->EndRotate();
		break;
	}

	if (this->Interactor)
	{
		this->ReleaseFocus();
	}
}



void iACustomInterActorStyleTrackBall::OnMiddleButtonUp()
{

}

void iACustomInterActorStyleTrackBall::OnRightButtonUp()
{

}

void iACustomInterActorStyleTrackBall::FindPickedActor(int x, int y)
{

	this->InteractionPicker->Pick(x, y, 0.0, this->CurrentRenderer);
	//Image actor
	vtkProp *prop = this->InteractionPicker->GetViewProp();
	if (prop != nullptr)
	{
		this->m_PropCurrentSlicer.prop = vtkProp3D::SafeDownCast(prop);
	}
	else
	{
		this->m_PropCurrentSlicer.prop = nullptr;
	}
}

void iACustomInterActorStyleTrackBall::initModes(iASlicerMode mode_slice1, iASlicerMode mode_slice2)
{
	setModeSlicer1(mode_slice1);
	setModeSlicer2(mode_slice2); 
}

void iACustomInterActorStyleTrackBall::setActiveSlicer(vtkProp3D *currentActor, iASlicerMode slice, int activeSliceNr)
{
	if (!currentActor) {
		throw std::invalid_argument("slice actor is null");
	}

	m_PropCurrentSlicer.prop = currentActor; 
	m_PropCurrentSlicer.fixedCoord = activeSliceNr; 
	m_PropCurrentSlicer.mode = slice; 

}

void iACustomInterActorStyleTrackBall::updateSlicer()
{

	//position;-> 2 mal die propdefs
	propDefs::sliceProp propSlice1; 
	propDefs::sliceProp propSlice2;
	propDefs::SliceDefs sl_defs; 
	
	//coords
	const double *pos = m_PropCurrentSlicer.prop->GetPosition();
	
	sl_defs.fixedCoord = m_PropCurrentSlicer.fixedCoord; 
	sl_defs.x = pos[0];
	sl_defs.y = pos[1];
	sl_defs.z = pos[2]; 

	DEBUG_LOG(QString("New positins %1 %2 %3, Fixed %4").arg(sl_defs.x).arg(sl_defs.y).arg(sl_defs.y).arg(sl_defs.fixedCoord));

	//zb current slice mode xy -> z is fixed

	//update slicer 1 - props are null why??? 
	propDefs::PropModifier::updateSlicerPosition(m_propSlicer1.prop, m_PropCurrentSlicer.mode, sl_defs, "Slicer1"); 
	//update slicer 2; 
	propDefs::PropModifier::updateSlicerPosition(m_propSlicer2.prop, m_PropCurrentSlicer.mode, sl_defs, "Slicer2");
	//update slicer 3D; 
	
	propDefs::PropModifier::updateSlicerPosition(m_Prop3DSlicer, m_PropCurrentSlicer.mode, sl_defs, "Slicer3d");
	
}




