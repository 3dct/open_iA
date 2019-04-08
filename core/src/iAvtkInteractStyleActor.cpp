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
	m_rotationEnabled(false)
{
	std::fill(m_slicerChannel, m_slicerChannel + iASlicerMode::SlicerCount, nullptr);
	InteractionPicker->SetTolerance(100.0);
}

void iAvtkInteractStyleActor::Rotate()
{ //todo disable translation
  //rotate about center
	if (enable3D)
		vtkInteractorStyleTrackballActor::Rotate();
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
	/*if (enable3D)*/
		vtkInteractorStyleTrackballActor::Spin();
}

void iAvtkInteractStyleActor::OnMouseMove()
{
	vtkInteractorStyleTrackballActor::OnMouseMove();

	if (m_rotationEnabled) {
		//this->rotate2d();
		m_rotationEnabled = false; 
	}else updateInteractors();
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

void iAvtkInteractStyleActor::rotate2d()
{
	/*if (!m_slicerChannel[m_currentSliceMode])
		return;*/
	//if 

	DEBUG_LOG("Iam rotating ")
	
	//double const *actorPos = m_slicerChannel[m_currentSliceMode]->actorPosition(); 
	
	vtkRenderWindowInteractor *rwi = this->Interactor;
	/*if (rwi->GetControlKey())
		DEBUG_LOG("Control key pressed")
	else return; */
	//double const *testPos = 
	double displayOrig[3]; 
	
	vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New(); 
	transform = dynamic_cast<vtkTransform*>(m_slicerChannel[m_currentSliceMode]->reslicer()->GetResliceTransform());
	
	double const * testOrigin = m_slicerChannel[m_currentSliceMode]->reslicer()->GetResliceAxesOrigin(); 
	//double const * testPos = transform->GetPosition(); 
	double posCopy[3] = { testOrigin[0], testOrigin[1], testOrigin[2] };
	
	vtkMath::Normalize(posCopy);

	
	this->ComputeWorldToDisplay(posCopy[0], posCopy[1], posCopy[2], displayOrig);
	double newAngle =
		vtkMath::DegreesFromRadians(atan2(rwi->GetEventPosition()[1] - displayOrig[1],
			rwi->GetEventPosition()[0] - displayOrig[0]));

	double oldAngle =
		vtkMath::DegreesFromRadians(atan2(rwi->GetLastEventPosition()[1] - displayOrig[1],
			rwi->GetLastEventPosition()[0] - displayOrig[0]));
	double angle2 = newAngle - oldAngle;
	DEBUG_LOG(QString("old angle %1").arg(oldAngle));
	DEBUG_LOG(QString("New angle %1").arg(newAngle));

	const double angle = 30;  // in degree
	//transform->Translate(-displayOrig[0], -displayOrig[1], -displayOrig[2]);
	

	transform->RotateZ(angle2/*, 0, 0, 1*/);
	//transform->Translate(displayOrig[0], displayOrig[1], displayOrig[2]);
	
	double const *tPos = transform->GetPosition();
	double const *t_Orient = transform->GetOrientation(); 
	
	DEBUG_LOG(QString("Position after %1 %2 %3").arg(tPos[0]).arg(tPos[1]).arg(tPos[2])); 
	DEBUG_LOG(QString("Orientation after %1 %2 %3").arg(t_Orient[0]).arg(t_Orient[1]).arg(t_Orient[2]));

	m_slicerChannel[m_currentSliceMode]->reslicer()->SetResliceTransform(transform); 

	m_slicerChannel[m_currentSliceMode]->reslicer()->Modified(); 
	m_slicerChannel[m_currentSliceMode]->reslicer()->Update();
	
	//update slice planes
	/*for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();*/

	m_volumeRenderer->update();
	emit actorsUpdated();



	//Winkel berechnen zu einer Achse
	/*evtl zu display coordinaten convertieren 
	*Normalisierung
	*drehung um achse mit einem winkel
	*und zurück verschieben bzw tramsform aktualisieren
	*
	*
	*/

	/*
	  // Compute the center of the image
  double center[3];
  center[0] = (bounds[1] + bounds[0]) / 2.0;
  center[1] = (bounds[3] + bounds[2]) / 2.0;
  center[2] = (bounds[5] + bounds[4]) / 2.0;

  // Rotate about the center
  transform->Translate(center[0], center[1], center[2]);
  transform->RotateWXYZ(angle, 0, 0, 1);
  transform->Translate(-center[0], -center[1], -center[2]);

  // Reslice does all of the work
  vtkSmartPointer<vtkImageReslice> reslice =
	vtkSmartPointer<vtkImageReslice>::New();
  reslice->SetInputConnection(reader->GetOutputPort());
  reslice->SetResliceTransform(transform);
  reslice->SetInterpolationModeToCubic();
  reslice->SetOutputSpacing(
	
	*/
	//rotate resclicer;
	//throw std::logic_error("The method or operation is not implemented.");
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

	double *obj_center = this->InteractionProp->GetOrigin();

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

	double scale[3];
	scale[0] = scale[1] = scale[2] = 1.0;

	double **rotate = new double*[1];
	rotate[0] = new double[4];

	rotate[0][0] = newAngle - oldAngle;
	rotate[0][1] = motion_vector[0];
	rotate[0][2] = motion_vector[1];
	rotate[0][3] = motion_vector[2];
	
	motion_vector[0] = 0; 
	motion_vector[1] = 0; 
	motion_vector[2] = 1; 
	// vtkErrorMacro("ObjectCenter\t" << obj_center );
	this->Prop3DTransform(this->InteractionProp,
		obj_center,
		1,
		rotate,
		scale);

	delete[] rotate[0];
	delete[] rotate;
	m_slicerChannel[m_currentSliceMode]->reslicer()->Modified();
	m_slicerChannel[m_currentSliceMode]->reslicer()->Update();

	//update slice planes
	/*for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();*/

	m_volumeRenderer->update();
	emit actorsUpdated();


	/*rwi->Render();*/
}
