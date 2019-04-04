#include "CustomInterActorStyles.h"

#include "iAChannelSlicerData.h"
#include "iAVolumeRenderer.h"

#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkVolume.h>
#include <vtkProp3D.h>

#include <limits>


namespace
{
	
	class slice_coords {
	public:

		slice_coords(double const * pos)
		{
			x = pos[0];
			y = pos[1];
			z = pos[2];
		}
		
		//set the coords based on a slicer mode, keep other one fixed
		void updateCoords(const double *pos, iASlicerMode mode) {
			switch (mode)
			{
			case iASlicerMode::XY:
				x += pos[0];
				y += pos[1];
				break;
			case iASlicerMode::XZ:
				x += pos[0];
				z += pos[1];
				break;
			case iASlicerMode::YZ:
				y += pos[0];
				z += pos[1];
				break;
			case iASlicerMode::SlicerCount:
				x += pos[0];
				y += pos[1];
				z += pos[2];
				break;
			}
		}

		

		QString toString() {
			return QString("x %1 y %2 z %3").arg(x).arg(y).arg(z);

		}

		double x;
		double y;
		double z;
	private:
		slice_coords() = delete; 

	};
}

vtkStandardNewMacro(iACustomInterActorStyleTrackBall);

iACustomInterActorStyleTrackBall::iACustomInterActorStyleTrackBall() {

	InteractionMode = 0;
	this->m_volumeRenderer = nullptr; 
	std::fill(m_slicerChannel, m_slicerChannel + iASlicerMode::SlicerCount, nullptr);
	this->m_mdiChild = nullptr; 

	//this->InteractionPicker = vtkCellPicker::New();
	// TODO : check how this is done in other places
	this->InteractionPicker->SetTolerance(100.0);
	enable3D = false; 
}

void iACustomInterActorStyleTrackBall::OnMouseMove()
{
	vtkInteractorStyleTrackballActor::OnMouseMove();

	//double picked[3];
	//this->Interactor->GetPicker()->GetPickPosition(picked);
	//DEBUG_LOG(QString("Picked 1% \t %2 \t %3").arg(picked[0]).arg(picked[1]).arg(picked[2]));

	//connect the components; 
	
	updateInteractors();
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

void iACustomInterActorStyleTrackBall::initialize(vtkImageData *img, iAVolumeRenderer* volRend, iAChannelSlicerData *slicerChannel[3], int currentMode, MdiChild *mdiChild)
{
	m_image = img;
	m_volumeRenderer = volRend;
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		m_slicerChannel[i] = slicerChannel[i];
	m_currentSliceMode = currentMode;
	enable3D = (m_currentSliceMode == iASlicerMode::SlicerCount);
	if (enable3D)
		this->SetInteractionMode(VTKIS_IMAGE3D);
	if (!mdiChild)
		DEBUG_LOG("MdiChild not set!");
	m_mdiChild = mdiChild;
}

void iACustomInterActorStyleTrackBall::updateInteractors()
{
	//getting coords from 3d slicer
	
	//coords // !!dritte immer null in 2d; 

	//test data for the slicers
	//const double testPosYZ[3] = { 195.904, -112.318, 0 };// testPropYZ->GetPosition();
	//const double testPosXZ[3] = { 163.221, -32.6442, 0 };
	//const double testPosXY[3] = { -153.34, 4.79187, 0 };
	//

	//coords initialized from the image origin;
	slice_coords coords(m_image->GetOrigin());

	DEBUG_LOG(QString("Move: %1; InteractionMode: %2").arg(getSlicerModeString(m_currentSliceMode)).arg(InteractionMode) );
	DEBUG_LOG(QString("  Old origin: %1, %2, %3").arg(coords.x).arg(coords.y).arg(coords.z));
	
	/*relative movement of object
	*we  have a current slicer, from this 2D-coords we take as reference coords for registration
	*reset the position of the image actor*/
	if (!enable3D)
	{
		if (!m_slicerChannel[m_currentSliceMode])
			return;
		const double *posCurrentSlicer = m_slicerChannel[m_currentSliceMode]->actorPosition();
		DEBUG_LOG(QString("  Pos %1 slicer: %2, %3, %4").arg(getSlicerModeString(m_currentSliceMode)).arg(posCurrentSlicer[0]).arg(posCurrentSlicer[1]).arg(posCurrentSlicer[2]));
		coords.updateCoords(posCurrentSlicer, static_cast<iASlicerMode>(m_currentSliceMode));
		m_slicerChannel[m_currentSliceMode]->setActorPosition(0, 0, 0);
	}
	else //in 3d case
	{
		double const * pos = m_volumeRenderer->position();
		DEBUG_LOG(QString("  Pos 3D renderer: %1, %2, %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
		coords.updateCoords(pos, static_cast<iASlicerMode>(m_currentSliceMode));
		m_volumeRenderer->volume()->SetPosition(0, 0, 0);
	}
	DEBUG_LOG(QString("  New origin: %1, %2, %3").arg(coords.x).arg(coords.y).arg(coords.z));


	//update image origin;
	m_image->SetOrigin(coords.x, coords.y, coords.z );
	
	//update slice planes
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();
	
		
	m_volumeRenderer->update();
	emit actorsUpdated();
}
/*
void iACustomInterActorStyleTrackBall::printProbOrigin()
{
	if (m_currentSliceMode != iASlicerMode::SlicerCount && !m_slicerChannel[m_currentSliceMode])
		return;
	double const * const pos = m_currentSliceMode != iASlicerMode::SlicerCount ?
		m_slicerChannel[m_currentSliceMode]->GetOrigin() :
		m_volumeRenderer->GetVolume()->GetOrigin();
	DEBUG_LOG(QString("\nOrigin x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
}

void iACustomInterActorStyleTrackBall::printPropPosistion()
{
	if (m_currentSliceMode != iASlicerMode::SlicerCount && !m_slicerChannel[m_currentSliceMode])
		return;
	double const * const pos = m_currentSliceMode != iASlicerMode::SlicerCount ?
		m_slicerChannel[m_currentSliceMode]->GetPosition() :
		m_volumeRenderer->GetVolume()->GetPosition();
	DEBUG_LOG(QString("\nPosition %1 %2 %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
}

void iACustomInterActorStyleTrackBall::printProbOrientation()
{
	if (m_currentSliceMode != iASlicerMode::SlicerCount && !m_slicerChannel[m_currentSliceMode])
		return;
	double const * const pos = m_currentSliceMode != iASlicerMode::SlicerCount ?
		m_slicerChannel[m_currentSliceMode]->GetOrientation() :
		m_volumeRenderer->GetVolume()->GetOrientation();
	DEBUG_LOG(QString("\nOrientation x %1 y %2 z %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
}
*/