#include "CustomInterActorStyles.h"
#include <limits>


vtkStandardNewMacro(iACustomInterActorStyleTrackBall);

iACustomInterActorStyleTrackBall::iACustomInterActorStyleTrackBall() {

	InteractionMode = 0;
	this->m_PropCurrentSlicer = nullptr;
	this->Prop3DSlicer = nullptr; 
	this->propSlicer1 = nullptr;
	this->propSlicer2 = nullptr; 

	this->InteractionPicker = vtkCellPicker::New();
	/*PickTolerance = 100.0;*/
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
		this->m_PropCurrentSlicer = vtkProp3D::SafeDownCast(prop);
	}
	else
	{
		this->m_PropCurrentSlicer = nullptr;
	}
}

void iACustomInterActorStyleTrackBall::setActiveSlicer(vtkProp3D *currentSlicer, iASlicerMode sliceMode, int SliceNumber)
{
	if (!currentSlicer) {
		throw std::invalid_argument("slice actor is null");
	}

	m_PropCurrentSlicer = currentSlicer; 

	switch (sliceMode) {
		case iASlicerMode::XY: sliceZ = SliceNumber; break;
		case iASlicerMode::XZ: sliceY = SliceNumber; break;
		case iASlicerMode::YZ: sliceX = SliceNumber; break;
		default: throw std::invalid_argument("Invalid slice option"); break;

	}

	activeSliceMode = sliceMode; 
}
