/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAvtkInteractStyleActor.h"

#include "iAChannelSlicerData.h"
#include "iAConsole.h"
#include "iASlicerMode.h"
#include "iAVolumeRenderer.h"

#include <vtkCellPicker.h>
#include <vtkCommand.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkVolume.h>
#include <vtkTransform.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>

namespace
{
	//set the coords based on a slicer mode, keep other one fixed
	void updateCoords(double* origin, double const *pos, int mode)
	{
		//DEBUG_LOG(QString("  Pos: %1, %2, %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
		switch (mode)
		{
		case iASlicerMode::XY:
			origin[0] += pos[0];
			origin[1] += pos[1];
			break;
		case iASlicerMode::XZ:
			origin[0] += pos[0];
			origin[2] += pos[1];
			break;
		case iASlicerMode::YZ:
			origin[1] += pos[0];
			origin[2] += pos[1];
			break;
		case iASlicerMode::SlicerCount:
			origin[0] += pos[0];
			origin[1] += pos[1];
			origin[2] += pos[2];
			break;
		}
		//DEBUG_LOG(QString("  New origin: %1, %2, %3").arg(origin[0]).arg(origin[1]).arg(origin[2]));
	}
}

vtkStandardNewMacro(iAvtkInteractStyleActor);

iAvtkInteractStyleActor::iAvtkInteractStyleActor():
	m_volumeRenderer(nullptr),
	m_mdiChild(nullptr),
	enable3D(false),
	m_rotationEnabled(true)
{
	std::fill(m_slicerChannel, m_slicerChannel + iASlicerMode::SlicerCount, nullptr);
	InteractionPicker->SetTolerance(100.0);
}

void iAvtkInteractStyleActor::Rotate()
{ //todo disable translation
  //rotate about center
	if (enable3D) {
		DEBUG_LOG("Rotate 3d"); 
		vtkInteractorStyleTrackballActor::Rotate();
	}
	else {
		m_rotationEnabled = true; 
		this->custom2DRotate(); 
		/*vtkInteractorStyleTrackballActor::Rotate();*/
		/*v*/
		//this->rotate2d(); 
	}
}

void iAvtkInteractStyleActor::Spin()
{
	if (enable3D) {
		DEBUG_LOG("spin")
		vtkInteractorStyleTrackballActor::Spin();
	}
}

void iAvtkInteractStyleActor::OnMouseMove()
{
	vtkInteractorStyleTrackballActor::OnMouseMove();

	//if (m_rotationEnabled) { /*{*/
	//	this->custom2DRotate();		//this->rotate2d();
	//	m_rotationEnabled = false;
	//}
	//}else 

	if (this->Interactor->GetShiftKey())
		updateInteractors();
}

void iAvtkInteractStyleActor::initialize(vtkImageData *img, iAVolumeRenderer* volRend,
	iAChannelSlicerData *slicerChannel[3], int currentMode, MdiChild *mdiChild)
{
	m_image = img;
	m_volumeRenderer = volRend;
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		m_slicerChannel[i] = slicerChannel[i];
	m_currentSliceMode = currentMode;
	enable3D = (m_currentSliceMode == iASlicerMode::SlicerCount);
	if (!mdiChild)
		DEBUG_LOG("MdiChild not set!");
	m_mdiChild = mdiChild;
}

void iAvtkInteractStyleActor::updateInteractors()
{
	// DEBUG_LOG(QString("Move: %1").arg(enable3D ? "3D" : getSlicerModeString(m_currentSliceMode)));

	//coords initialized from the image origin;
	double origin[3];
	m_image->GetOrigin(origin);
	// DEBUG_LOG(QString("  Old origin: %1, %2, %3").arg(origin[0]).arg(origin[1]).arg(origin[2]));
	
	// relative movement of object - we take the position the object was moved to
	// add that to the origin of the image, and reset the position
	if (enable3D)
	{
		double const * pos = m_volumeRenderer->position();
		if (pos[0] == 0 && pos[1] == 0 && pos[2] == 0)
			return;
		updateCoords(origin, pos, m_currentSliceMode);
		m_volumeRenderer->volume()->SetPosition(0, 0, 0);
	}
	else
	{
		if (!m_slicerChannel[m_currentSliceMode])
			return;
		double const * pos = m_slicerChannel[m_currentSliceMode]->actorPosition();
		if (pos[0] == 0 && pos[1] == 0 && pos[2] == 0)
			return;
		updateCoords(origin, pos, m_currentSliceMode);
		m_slicerChannel[m_currentSliceMode]->setActorPosition(0, 0, 0);
	}
	m_image->SetOrigin(origin);  //< update image origin
	
	//update slice planes
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();

	m_volumeRenderer->update();
	emit actorsUpdated();
}



void iAvtkInteractStyleActor::custom2DRotate()
{
	DEBUG_LOG("Rotation");
	if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
	{
		return;
	}

	vtkRenderWindowInteractor *rwi = this->Interactor;
	/*vtkCamera *cam = this->CurrentRenderer->GetActiveCamera();
*/
	// Get the axis to rotate around = vector from eye to origin
	double const *imageBounds = this->m_image->GetBounds();
	DEBUG_LOG(QString("Image Center")); 
	double imageCenter[3]; 
	imageCenter[0] = (imageBounds[1] + imageBounds[0]) / 2.0f;
	imageCenter[1] = (imageBounds[3] + imageBounds[2]) / 2.0;
	imageCenter[2] = (imageBounds[5] + imageBounds[4]) / 2.0; 
	
	
	double *obj_center = this->InteractionProp->GetCenter();
	DEBUG_LOG(QString("Image Center %1 %2 %3").arg(imageCenter[0]).arg(imageCenter[1]).arg(imageCenter[2]));
	DEBUG_LOG(QString("Prop Center %1 %2 %3").arg(obj_center[0]).arg(obj_center[1]).arg(obj_center[2]));

	double motion_vector[3];
	double view_point[3];
		
	double disp_obj_center[3];

	this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2],
		disp_obj_center);

	double newAngle =
		vtkMath::DegreesFromRadians(atan2(rwi->GetEventPosition()[1] - disp_obj_center[1],
			rwi->GetEventPosition()[0] - disp_obj_center[0]));

	double oldAngle =
		vtkMath::DegreesFromRadians(atan2(rwi->GetLastEventPosition()[1] - disp_obj_center[1],
			rwi->GetLastEventPosition()[0] - disp_obj_center[0]));

	//this->InteractionProp->SetOrientation(0, 0, newAngle);
	double relativeAngle = newAngle - oldAngle;
	double scale[3];
	scale[0] = scale[1] = scale[2] = 1.0;

	double **rotate = new double*[1];
	rotate[0] = new double[4];

	motion_vector[0] = 0;
	motion_vector[1] = 0;
	motion_vector[2] = 1;

	//DEBUG_LOG(QString("new Angle %1").arg(newAngle));
	//DEBUG_LOG(QString("old Angle %1").arg(oldAngle));

	rotate[0][0] = newAngle - oldAngle;
	rotate[0][1] = motion_vector[0];
	rotate[0][2] = motion_vector[1];
	rotate[0][3] = motion_vector[2];

	DEBUG_LOG(QString("relative angle %1").arg(rotate[0][0])); 

	this->Prop3DTransform(this->InteractionProp,
		obj_center/*obj_center*/,
		1,
		rotate,
		scale);

	delete[] rotate[0];
	delete[] rotate;

	/*vtkSmartPointer<vtkTransform> transform =
		vtkSmartPointer<vtkTransform>::New();
*/

	// Rotate about the center
	/*transform->Translate(disp_obj_center[0], disp_obj_center[1], disp_obj_center[2]);
	transform->RotateZ(newAngle-oldAngle);*/
	/*transform->Translate(disp_obj_center[0], disp_obj_center[1], disp_obj_center[2]);*/

	double const * orientXYZ = this->InteractionProp->GetOrientation();
	double const * oriWXYZ = this->InteractionProp->GetOrientationWXYZ();

	DEBUG_LOG(QString("Orientation XYZ %1 %2 %3").arg(orientXYZ[0]).arg(orientXYZ[1]).arg(orientXYZ[2]));
	DEBUG_LOG(QString("Orientation WXYZ %1 %2 %3 %4").arg(oriWXYZ[0]).arg(oriWXYZ[1]).arg(oriWXYZ[2]).arg(oriWXYZ[3])); 
	

	/*
	*based on slicer mode 
	*/

	
	QString mode = slicerModeString(m_currentSliceMode);
	DEBUG_LOG(QString("Mode %1 %2").arg(mode.toStdString().c_str()).arg(m_currentSliceMode)); 
	double const * spacing = m_image->GetSpacing(); 
	
	Update3DTransform(imageCenter, spacing, relativeAngle);



	

	//m_slicerChannel[m_currentSliceMode]->reslicer()->SetOutputExtent(this->m_image->GetExtent());
	//m_slicerChannel[m_currentSliceMode]->reslicer()->SetOutputSpacing(this->m_image->GetSpacing());
	//m_slicerChannel[m_currentSliceMode]->reslicer()->SetOutputOrigin(this->m_image->GetOrigin()); 
	m_slicerChannel[m_currentSliceMode]->reslicer()->Update(); 
	m_slicerChannel[m_currentSliceMode]->reslicer()->SetResliceAxes(this->InteractionProp->GetMatrix());

	m_slicerChannel[m_currentSliceMode]->reslicer()->SetInterpolationModeToLinear();

	m_slicerChannel[m_currentSliceMode]->reslicer()->Modified();
	//m_slicerChannel[m_currentSliceMode]->reslicer()->UpdateWholeExtent();

	//this->InteractionProp->getTr
	//slice anpassen

	//this->InteractionProp->getTra
	
		
		//TBA
	/*this->InteractionProp->GetUserTransform()->GetMatrix();
	reslice->SetResliceTransform(transform);*/
	//m_slicerChannel[m_currentSliceMode]->reslicer()->Modified();
	//m_slicerChannel[m_currentSliceMode]->reslicer()->Update();
	
	//this->InteractionProp->Tr
	//update slice planes
	/*for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();*/

	// da muss die Rotation weitergegeben werden

	m_volumeRenderer->update();
	emit actorsUpdated();


	/*rwi->Render();*/
}


void iAvtkInteractStyleActor::Update3DTransform(double * imageCenter, const double * spacing, double relativeAngle)
{
	double volOrientation[3];
	//m_volumeRenderer->volume()->GetOrientation(volOrientation);

	vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();;transform->PostMultiply(); 
	//transform->SetMatrix(m_volumeRenderer->volume()->GetMatrix());
	transform->Translate(-imageCenter[0]/**spacing[0]*/, -imageCenter[1]/**spacing[1]*/, -imageCenter[2]/**spacing[0]*/);
	


	switch (m_currentSliceMode) {
	case YZ: transform->RotateX(relativeAngle); break;
	case XZ: transform->RotateY(relativeAngle); break;
	case XY: transform->RotateZ(relativeAngle);  break;
	default: break;
	}

	transform->Translate(imageCenter[0]/**spacing[0]*/, imageCenter[1]/**spacing[1]*/, imageCenter[2]/**spacing[2]*/);
	m_volumeRenderer->volume()->SetUserTransform(transform);
	m_volumeRenderer->update(); 
}

void iAvtkInteractStyleActor::TransformReslicer(double * obj_center, double const * spacing, double newAngle, double oldAngle)
{
	vtkSmartPointer<vtkTransform> rotate2 = vtkSmartPointer<vtkTransform>::New();
	//rotate2->PostMultiply(); 
	rotate2->Translate(obj_center[1] * spacing[0], obj_center[1] * spacing[1], obj_center[2] * spacing[2]);
	/*rotate2->getO*/
	rotate2->RotateZ(newAngle - oldAngle);
	rotate2->Translate(-obj_center[0] * spacing[0], -obj_center[1] * spacing[1], -obj_center[2] * spacing[2]);
	//rotate2->Inverse(); 
	//this->InteractionProp->SetUserMatrix(rotate2->GetMatrix()); 


	/*rotate2->Inverse(); */

	m_slicerChannel[m_currentSliceMode]->reslicer()->SetOutputExtent(m_image->GetExtent());
	m_slicerChannel[m_currentSliceMode]->reslicer()->SetOutputOrigin(m_image->GetOrigin());
	m_slicerChannel[m_currentSliceMode]->reslicer()->SetOutputSpacing(m_image->GetSpacing());
	m_slicerChannel[m_currentSliceMode]->reslicer()->Update();

	/*m_slicerChannel[m_currentSliceMode]->reslicer()->Update();*/
	m_slicerChannel[m_currentSliceMode]->reslicer()->SetResliceAxes(rotate2->GetMatrix());
	m_slicerChannel[m_currentSliceMode]->reslicer()->UpdateWholeExtent();
	m_slicerChannel[m_currentSliceMode]->reslicer()->SetInterpolationModeToLinear();

	//m_slicerChannel[m_currentSliceMode]->reslicer()->Update();
}