#include "CustomInterActorStyles.h"

#include "iAVolumeRenderer.h"

#include <vtkVolume.h>

#include <limits>


namespace
{
	//update the position while keeping one coordinate fixed, based on slicer mode
	void updatePropPosition(vtkProp3D *prop, iASlicerMode mode, const propDefs::SliceDefs &sl_defs, QString text) {
		assert(prop && "object is null");
		if (!prop) {
			DEBUG_LOG("update Prop failed");
			throw std::invalid_argument("null pointer of prop");

		}

		double *pos = prop->GetPosition();

		switch (mode)
		{
		case YZ:
			//x is fixed
			pos[0] = sl_defs.fixedCoord; //keep 
			pos[1] = sl_defs.y;
			pos[2] = sl_defs.z;
			prop->SetPosition(pos);
			break;
		case XY:
			//z is fixed
			pos[0] = sl_defs.x;
			pos[1] = sl_defs.y;
			pos[2] = sl_defs.fixedCoord;//keep 
			prop->SetPosition(pos);
			break;
		case XZ:
			//y fixed
			pos[0] = sl_defs.x;
			pos[1] = sl_defs.fixedCoord;//keep 
			pos[2] = sl_defs.z;
			prop->SetPosition(pos);
			break;
			/*prop->SetPosition(sl_defs.x,sl_defs.fixedCoord, sl_defs.z);
			break;*/
		case SlicerModeCount:
			throw std::invalid_argument("invalid slicer mode");
		}

		propDefs::PropModifier::printProp(prop, text);
	}
}

vtkStandardNewMacro(iACustomInterActorStyleTrackBall);

iACustomInterActorStyleTrackBall::iACustomInterActorStyleTrackBall() {

	InteractionMode = 0;
	this->m_PropCurrentSlicer.prop = nullptr;
	this->m_volumeRenderer = nullptr; 
	this->m_propSlicer1.prop = nullptr;
	this->m_propSlicer2.prop = nullptr; 
	this->m_mdiChild = nullptr; 

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

	case VTKIS_PAN: {
		this->FindPokedRenderer(x, y);
		this->Pan();
		this->InvokeEvent(vtkCommand::InteractionEvent, nullptr);
		break;
	}
}

//	int x = this->Interactor->GetEventPosition()[0];
//	int y = this->Interactor->GetEventPosition()[1];
//
//	this->FindPokedRenderer(x, y);
//	this->Interactor->GetPicker()->Pick(x, y, 0, this->GetCurrentRenderer());
//	this->FindPickedActor(x, y);
//	if (this->CurrentRenderer == nullptr || this->m_PropCurrentSlicer.prop == nullptr
//		|| this->m_propSlicer1.prop == nullptr || this->m_propSlicer2.prop == nullptr)
//	{
//		DEBUG_LOG("Either renderer or props are null");
//		return;
//	}
//
//	double picked[3];
//	this->Interactor->GetPicker()->GetPickPosition(picked);
//	DEBUG_LOG(QString("Picked 1% \t %2 \t %3").arg(picked[0]).arg(picked[1]).arg(picked[2]));
//
//	//connect the components; 
//	printProbOrientation();
//	printPropPosistion();
//	printProbOrigin();
//
//
//
//
//	assert(this->m_volumeRenderer && "prop 3D slicer null");
//	assert(this->m_propSlicer1.prop && "prop Slicer 1 null");
//	assert(this->m_propSlicer2.prop && "prop Slicer 2 null");
//
//
//	updateSlicer();
//
//
//	if (!this->Interactor->GetShiftKey()){
//		return;
//		vtkInteractorStyleTrackballActor::OnMouseMove();
//	}

	// Call parent to handle all other states and perform additional work

	




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
	updatePropPosition(m_propSlicer1.prop, m_PropCurrentSlicer.mode, sl_defs, "Slicer1"); 
	//update slicer 2; 
	updatePropPosition(m_propSlicer2.prop, m_PropCurrentSlicer.mode, sl_defs, "Slicer2");
	//update slicer 3D; 
	
	updatePropPosition(m_volumeRenderer->GetVolume().Get(), m_PropCurrentSlicer.mode, sl_defs, "Slicer3d");
	if (this->m_mdiChild)
	{
		m_volumeRenderer->Update();
		emit actorsUpdated();
		DEBUG_LOG("views updated");
	}
}




