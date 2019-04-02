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
	this->m_volumeRenderer = nullptr; 
	std::fill(m_propSlicer, m_propSlicer + iASlicerMode::SlicerCount, nullptr);
	this->m_mdiChild = nullptr; 

	//this->InteractionPicker = vtkCellPicker::New();
	// TODO : check how this is done in other places
	this->InteractionPicker->SetTolerance(100.0);
	enable3D = false; 
}

void iACustomInterActorStyleTrackBall::OnMouseMove()
{
	vtkInteractorStyleTrackballActor::OnMouseMove();

	if (this->m_volumeRenderer == nullptr || this->m_propSlicer[0] == nullptr
		|| this->m_propSlicer[1] == nullptr || this->m_propSlicer[2] == nullptr)
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

/*
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
*/

void iACustomInterActorStyleTrackBall::initialize(iAVolumeRenderer *volRend, vtkProp3D *propSlicer[3], int currentMode, MdiChild *mdiChild)
{
	m_volumeRenderer = volRend;
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		m_propSlicer[i] = propSlicer[i];
	m_currentSliceMode = currentMode;
	enable3D = (m_currentSliceMode == iASlicerMode::SlicerCount);
	if (enable3D)
		this->SetInteractionMode(VTKIS_IMAGE3D);
	m_mdiChild = mdiChild;
}

void iACustomInterActorStyleTrackBall::updateSlicer()
{
	//getting coords from 3d slicer
	
	//coords // !!dritte immer null in 2d; 

	//test data for the slicers
	//const double testPosYZ[3] = { 195.904, -112.318, 0 };// testPropYZ->GetPosition();
	//const double testPosXZ[3] = { 163.221, -32.6442, 0 };
	//const double testPosXY[3] = { -153.34, 4.79187, 0 };
	//

	//coords initialized from the current slicer; 
	slice_coords coords(m_volumeRenderer->GetPosition());

	if (!enable3D)
	{
		const double *posCurrentSlicer = m_propSlicer[m_currentSliceMode]->GetPosition();
		coords.updateCoords(posCurrentSlicer, static_cast<iASlicerMode>(m_currentSliceMode));
	}
	DEBUG_LOG(QString("Current Slicer: ") + getSlicerModeString(m_currentSliceMode));
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (i != m_currentSliceMode)
		{
			coords.toCoords(m_propSlicer[i], static_cast<iASlicerMode>(i), false);
			propDefs::PropModifier::printProp(m_propSlicer[i], getSlicerModeString(i));
		}
	}
	if (!enable3D)
	{
		coords.toCoords(m_volumeRenderer->GetVolume(), static_cast<iASlicerMode>(m_currentSliceMode), true);
		propDefs::PropModifier::printProp(m_volumeRenderer->GetVolume(), "3d renderer");
	}

	if (this->m_mdiChild)
	{
		m_volumeRenderer->Update();
		emit actorsUpdated();
		DEBUG_LOG("views updated");
	}
}

void iACustomInterActorStyleTrackBall::printProbOrigin()
{
	double const * const pos = m_currentSliceMode != iASlicerMode::SlicerCount ?
		m_propSlicer[m_currentSliceMode]->GetOrigin() :
		m_volumeRenderer->GetVolume()->GetOrigin();
	DEBUG_LOG(QString("\nOrigin x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
}

void iACustomInterActorStyleTrackBall::printPropPosistion() {
	double const * const pos = m_currentSliceMode != iASlicerMode::SlicerCount ?
		m_propSlicer[m_currentSliceMode]->GetPosition() :
		m_volumeRenderer->GetVolume()->GetPosition();
	DEBUG_LOG(QString("\nPosition %1 %2 %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
}

void iACustomInterActorStyleTrackBall::printProbOrientation() {
	double const * const pos = m_currentSliceMode != iASlicerMode::SlicerCount ?
		m_propSlicer[m_currentSliceMode]->GetOrientation() :
		m_volumeRenderer->GetVolume()->GetOrientation();
	DEBUG_LOG(QString("\nOrientation x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
}
