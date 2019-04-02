#include "CustomInterActorStyles.h"

#include "iAVolumeRenderer.h"

#include <vtkVolume.h>
#include <vtkProp3D.h>

#include <limits>


namespace
{
	////update the position while keeping one coordinate fixed, based on slicer mode
	//void updatePropPosition(vtkProp3D *prop, iASlicerMode mode, const propDefs::SliceDefs &sl_defs, 
	//	QString text, bool enablePrinting) 
	//{
	//	assert(prop && "object is null");
	//	if (!prop) {
	//		DEBUG_LOG("update Prop failed");
	//		throw std::invalid_argument("null pointer of prop");

	//	}

	//	double *pos = prop->GetPosition();

	//	switch (mode)
	//	{
	//	case YZ:
	//		//x is fixed
	//		pos[0] = sl_defs.fixedCurrentSlicerCoord; //keep 
	//		pos[1] = sl_defs.x;
	//		pos[2] = sl_defs.y;
	//		prop->SetPosition(pos);
	//		break;
	//	case XY:
	//		//z is fixed
	//		pos[0] = sl_defs.x;
	//		pos[1] = sl_defs.y;
	//		pos[2] = sl_defs.fixedCurrentSlicerCoord;//keep 
	//		prop->SetPosition(pos);
	//		break;
	//	case XZ:
	//		//y fixed
	//		pos[0] = sl_defs.x;
	//		pos[1] = sl_defs.fixedCurrentSlicerCoord;//keep 
	//		pos[2] = sl_defs.y;
	//		prop->SetPosition(pos);
	//		break;
	//	default:
	//		throw std::invalid_argument("invalid slicer mode");
	//	}

	//	if (enablePrinting); 
	//		propDefs::PropModifier::printProp(prop, text);
	//}

	
	class slice_coords {
	public:

		slice_coords(double const * pos)
		{
			x = pos[0];
			y = pos[1];
			z = pos[2];
		}

		//convert position to coords
		void toCoords(vtkProp3D *prop, iASlicerMode mode, bool update3D)
		{	
			double pos[3];
			prop->GetPosition(pos);
			if (!update3D)
			{
				pos[2] = 0;
				switch (mode) {
				case iASlicerMode::XY:
					pos[0] = x;
					pos[1] = y;
					break;
				case iASlicerMode::YZ:
					pos[0] = y;
					pos[1] = z;
					break;
				case iASlicerMode::XZ:
					pos[0] = x;
					pos[1] = z;
					break;
				}
			}
			else
			{
				pos[0] = x;
				pos[1] = y;
				pos[2] = z;
			}
			prop->SetPosition(pos); 

		}

		void updateCoords(const double *pos, iASlicerMode mode) {
			switch (mode)
			{
			case iASlicerMode::XY:
				x = pos[0];
				y = pos[1];
				break;
			case iASlicerMode::XZ:
				x = pos[0];
				z = pos[1];
				break;
			case iASlicerMode::YZ:
				y = pos[0];
				z = pos[1];
			}
		}

		

		QString toString() {
			return QString("x %1 y %2 z %3").arg(x).arg(y).arg(z);

		}

	private:
		slice_coords() = delete; 
		double x;
		double y;
		double z;

	};
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
	/*m_PropCurrentSlicer.fixedCoord = std::numeric_limits<double>::min(); 
	m_propSlicer1.fixedCoord = std::numeric_limits<double>::min();
	m_propSlicer2.fixedCoord = std::numeric_limits<double>::min(); */

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
	//getting coords from 3d slicer
	
	//coords // !!dritte immer null in 2d; 
	const double *posCurrentSlicer = m_PropCurrentSlicer.prop->GetPosition();

	
	iASlicerMode customYZ = iASlicerMode::YZ;
	iASlicerMode customXZ= iASlicerMode::XZ;
	iASlicerMode customXY = iASlicerMode::XY;


	//test data for the slicers
	//const double testPosYZ[3] = { 195.904, -112.318, 0 };// testPropYZ->GetPosition();
	//const double testPosXZ[3] = { 163.221, -32.6442, 0 };
	//const double testPosXY[3] = { -153.34, 4.79187, 0 };
	//

	//coords initialized from the current slicer; 
	slice_coords coords(m_volumeRenderer->GetPosition());
	coords.updateCoords(posCurrentSlicer, m_PropCurrentSlicer.mode);
	coords.toCoords(m_propSlicer1.prop, m_propSlicer1.mode, false);
	coords.toCoords(m_propSlicer2.prop, m_propSlicer2.mode, false);	
	
	QString modeCurrent = propDefs::PropModifier::printMode(m_PropCurrentSlicer.mode);
	DEBUG_LOG(QString("current slicer ")  + modeCurrent);

	//getting coordinate and mode of the other slicer
	QString str_mode1 = propDefs::PropModifier::printMode(m_propSlicer1.mode);
	propDefs::PropModifier::printProp(m_propSlicer1.prop, str_mode1); 
	QString str_mode2 = propDefs::PropModifier::printMode(m_propSlicer2.mode);

	propDefs::PropModifier::printProp(m_propSlicer2.prop, str_mode2); 
		
	coords.toCoords(m_volumeRenderer->GetVolume(), m_PropCurrentSlicer.mode, true);
	propDefs::PropModifier::printProp(m_volumeRenderer->GetVolume(), "3d renderer");
		
	if (this->m_mdiChild)
	{
		m_volumeRenderer->Update();
		emit actorsUpdated();
		DEBUG_LOG("views updated");
	}
}




