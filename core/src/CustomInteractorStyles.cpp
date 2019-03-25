#include "CustomInterActorStyles.h"


//vtkStandardNewMacro(iACustomInterActorStyleTrackBall);

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

iACustomInterActorStyleTrackBall::iACustomInterActorStyleTrackBall(){

	InteractionMode = 0; 
}