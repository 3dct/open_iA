#include "CustomInterActorStyles.h"

#include "iAVolumeRenderer.h"

#include <vtkVolume.h>

#include <limits>


namespace
{
	//update the position while keeping one coordinate fixed, based on slicer mode
	void updatePropPosition(vtkProp3D *prop, iASlicerMode mode, const propDefs::SliceDefs &sl_defs, 
		QString text, bool enablePrinting) 
	{
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
			pos[0] = sl_defs.fixedCurrentSlicerCoord; //keep 
			pos[1] = sl_defs.x;
			pos[2] = sl_defs.y;
			prop->SetPosition(pos);
			break;
		case XY:
			//z is fixed
			pos[0] = sl_defs.x;
			pos[1] = sl_defs.y;
			pos[2] = sl_defs.fixedCurrentSlicerCoord;//keep 
			prop->SetPosition(pos);
			break;
		case XZ:
			//y fixed
			pos[0] = sl_defs.x;
			pos[1] = sl_defs.fixedCurrentSlicerCoord;//keep 
			pos[2] = sl_defs.y;
			prop->SetPosition(pos);
			break;
		default:
			throw std::invalid_argument("invalid slicer mode");
		}

		if (enablePrinting); 
			propDefs::PropModifier::printProp(prop, text);
	}


	//replace one coordinate
	void updateSliceDefs(propDefs::SliceDefs &sl_defs, const iASlicerMode mode, const double coord) {
		switch (mode)
		{
		case YZ:
			sl_defs.x = coord; 
			break;
		case XZ:
			sl_defs.y = coord; 
			break;
		case XY:
			sl_defs.z = coord; 
			break;
		default:
			break;
		}
	
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
	m_PropCurrentSlicer.fixedCoord = std::numeric_limits<double>::min(); 
	m_propSlicer1.fixedCoord = std::numeric_limits<double>::min();
	m_propSlicer2.fixedCoord = std::numeric_limits<double>::min(); 

}

void iACustomInterActorStyleTrackBall::OnMouseMove()
{
	vtkInteractorStyleTrackballActor::OnMouseMove();

	if (this->m_volumeRenderer == nullptr || this->m_PropCurrentSlicer.prop == nullptr
		|| this->m_propSlicer1.prop == nullptr || this->m_propSlicer2.prop == nullptr)
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

	assert(this->m_volumeRenderer && "prop 3D slicer null");
	assert(this->m_propSlicer1.prop && "prop Slicer 1 null");
	assert(this->m_propSlicer2.prop && "prop Slicer 2 null");
	   
	updateSlicer();
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

void iACustomInterActorStyleTrackBall::initModes(iASlicerMode mode_slice1, double coordSlice1, iASlicerMode mode_slice2, double coordSlice2)
{
	setModeSlicer1(mode_slice1, coordSlice1);
	setModeSlicer2(mode_slice2, coordSlice2); 
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
	propDefs::sliceProp propSlice1; 
	propDefs::sliceProp propSlice2;
	propDefs::SliceDefs slicer1_defs; 
	propDefs::SliceDefs slicer2_defs;


	
	//coords // dritte immer null in 2d; 
	const double *pos = m_PropCurrentSlicer.prop->GetPosition();
	
	slicer1_defs.fixedCurrentSlicerCoord = m_PropCurrentSlicer.fixedCoord; 
	slicer1_defs.x = pos[0];
	slicer1_defs.y = pos[1];
	slicer1_defs.z = pos[2];

	slicer2_defs = slicer1_defs; //copy

	DEBUG_LOG(QString("New positions \t%1 \t%2 \t%3, Fixed %4").arg(slicer1_defs.x).arg(slicer1_defs.y).
		arg(slicer1_defs.z).arg(slicer1_defs.fixedCurrentSlicerCoord));

	
	//zb current slice mode xy -> z is fixed
	//others are 

	//keeping 2 coordinates fixed, 1 ->die des aktuellen sliceviews



	//getting coordinate and mode of the other slicer
	
		//update slicer 1 
	updateSliceDefs(slicer1_defs, m_propSlicer1.mode, m_propSlicer1.fixedCoord);
	slicer1_defs.print(); 
	updatePropPosition(m_propSlicer1.prop, m_PropCurrentSlicer.mode, slicer1_defs, "Slicer1", true); 
	
	//update slicer 2; 
	updateSliceDefs(slicer2_defs, m_propSlicer2.mode, m_propSlicer2.fixedCoord); 

	updatePropPosition(m_propSlicer2.prop, m_PropCurrentSlicer.mode, slicer2_defs, "Slicer2", true);
	slicer2_defs.print(); 
	
	//update slicer 3D; 
	
	const double *pos3d = m_volumeRenderer->GetVolume().Get()->GetPosition();

	
	//assing 3rd of the 3d thing coordinate to a slicer
	switch (m_PropCurrentSlicer.mode) {
	
	case(iASlicerMode::YZ):
		slicer1_defs.fixedCurrentSlicerCoord = pos3d[0];
		break;
	case (iASlicerMode::XZ): 
		slicer1_defs.fixedCurrentSlicerCoord = pos3d[1];
		break; 
	case (iASlicerMode::XY):
		slicer1_defs.fixedCurrentSlicerCoord = pos3d[2]; 
		break;	
	}

	//here muss noch die richtige dritte coordinate rein
	updatePropPosition(m_volumeRenderer->GetVolume().Get(), m_PropCurrentSlicer.mode, slicer1_defs, "Slicer3d", false);
	if (this->m_mdiChild)
	{
		m_volumeRenderer->Update();
		emit actorsUpdated();
		DEBUG_LOG("views updated");
	}
}




