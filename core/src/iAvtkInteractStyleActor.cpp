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
#include "mdichild.h"
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
#include <vtkProp3D.h>

#include <vtkTransformFilter.h>  //TOOD clean this if transform does not work

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
	}
				
	
}

	vtkStandardNewMacro(iAvtkInteractStyleActor);

iAvtkInteractStyleActor::iAvtkInteractStyleActor():
	m_volumeRenderer(nullptr),
	m_mdiChild(nullptr),
	enable3D(false),
	m_rotationEnabled(true)
{
	m_transform3D = vtkSmartPointer<vtkTransform>::New();
	

	m_SliceRotateTransform = vtkSmartPointer<vtkTransform>::New(); 
	m_transformFilter = vtkSmartPointer<vtkTransformFilter>::New(); //TDOO Remove transform filter

	for (int i = 0; i < 3; i++) {
		m_sliceTranslationTransform[i] = vtkSmartPointer < vtkTransform>::New();
	}
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
		//this->custom2DRotate(); 
		
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

	if (this->Interactor->GetShiftKey())
		updateInteractors();
	else if (m_rotationEnabled) {
		this->rotate2D(); 
		m_rotationEnabled = false;
	}
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
	//for 3d

	double transformposOut[3];
	if (enable3D)
	{
		double const * pos = m_volumeRenderer->position();
		if (pos[0] == 0 && pos[1] == 0 && pos[2] == 0)
			return;

		//new coords based on current volume renderer position
		updateCoords(origin, pos, m_currentSliceMode);
		m_volumeRenderer->volume()->SetPosition(0, 0, 0);
	}
	else //update 2d Slicer
	{
		if (!m_slicerChannel[m_currentSliceMode])
			return;
		double const * pos = m_slicerChannel[m_currentSliceMode]->actorPosition();
		if (pos[0] == 0 && pos[1] == 0 && pos[2] == 0)
			return;

		//probably take translation of reslicer, and initial coordinates put to transform
		//eg. take matrix of slicer, put it into transform, and shift
		//coordinates come from image origin
		

		//for (const auto &currentSlicer: )

	/*	translateSlicerActor( origin, pos, transformposOut, m_currentSliceMode);
		m_slicerChannel[m_currentSliceMode]->setActorPosition(transformposOut[0], transformposOut[1], transformposOut[2]); */
		//originally doing the two things below

		//m_slicerChannel[m_currentSliceMode]->reslicer()->SetResliceAxesOrigin(pos);
		//m_slicerChannel[m_currentSliceMode]->reslicer()->SetResliceAxes(m_sliceTranslationTransform[m_currentSliceMode]->GetMatrix());
		//m_slicerChannel[m_currentSliceMode]->reslicer()->Update();
		updateCoords(origin, pos, m_currentSliceMode);

		m_slicerChannel[m_currentSliceMode]->setActorPosition(0, 0, 0);
		//and use origin to update it
	}
	//m_volumeRenderer->volume()->SetOrigin(transformposOut);
	
	m_image->SetOrigin(/*transformposOut*/origin);  //< update image origin
	
	//update reslicer
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();

	m_volumeRenderer->update();
	emit actorsUpdated();
}



void iAvtkInteractStyleActor::rotate2D()
{
	//should work in this way: 
	/*
	*1 perform perform rotation to interactor
	*2  update 3d volume
	*3 update slicer
	*/


	DEBUG_LOG("Rotation");
	if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
	{
		return;
	}

	vtkRenderWindowInteractor *rwi = this->Interactor;

	if (!m_image) { DEBUG_LOG(QString("Error on rotation %1").arg(m_currentSliceMode)); return;  }

	// Get the axis to rotate around = vector from eye to origin
	double const *imageBounds = this->m_image->GetBounds();
	DEBUG_LOG(QString("Image Center"));
	double imageCenter[3];
	imageCenter[0] = (imageBounds[1] + imageBounds[0]) / 2.0f;
	imageCenter[1] = (imageBounds[3] + imageBounds[2]) / 2.0f;
	imageCenter[2] = (imageBounds[5] + imageBounds[4]) / 2.0f;


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


	double relativeAngle = newAngle - oldAngle;
	double scale[3];
	scale[0] = scale[1] = scale[2] = 1.0;

	double **rotate = new double*[1];
	rotate[0] = new double[4];

	motion_vector[0] = 0;
	motion_vector[1] = 0;
	motion_vector[2] = 1;

	rotate[0][0] = newAngle - oldAngle;
	rotate[0][1] = motion_vector[0];
	rotate[0][2] = motion_vector[1];
	rotate[0][3] = motion_vector[2];

	DEBUG_LOG(QString("relative angle %1").arg(rotate[0][0]));

	//rotate interactor of 2d thing
	this->Prop3DTransform(this->InteractionProp,
		obj_center,
		1,
		rotate,
		scale);

	delete[] rotate[0];
	delete[] rotate;

	double const * orientXYZ = this->InteractionProp->GetOrientation();
	double const * oriWXYZ = this->InteractionProp->GetOrientationWXYZ();

	DEBUG_LOG(QString("Orientation XYZ %1 %2 %3").arg(orientXYZ[0]).arg(orientXYZ[1]).arg(orientXYZ[2]));
	DEBUG_LOG(QString("Orientation WXYZ %1 %2 %3 %4").arg(oriWXYZ[0]).arg(oriWXYZ[1]).arg(oriWXYZ[2]).arg(oriWXYZ[3]));

	QString mode = slicerModeString(m_currentSliceMode);
	DEBUG_LOG(QString("Mode %1 %2").arg(mode.toStdString().c_str()).arg(m_currentSliceMode));
	double const * spacing = m_image->GetSpacing();

	double const *volCenter = m_volumeRenderer->volume()->GetCenter();

	double const *orientationBefore = m_volumeRenderer->volume()->GetOrientation();
	DEBUG_LOG(QString("Orientation before %1 %2 %3").arg(orientationBefore[0]).arg(orientationBefore[1]).arg(orientationBefore[2]));


	
	//TransformReslicer(obj_center/*imageCenter*/, relativeAngle); 

	Update3DTransform(volCenter/*imageCenter*/, spacing, relativeAngle);
	/*double *const Rendorientation = m_volumeRenderer->volume()->GetOrientation();
	DEBUG_LOG(QString("orientation %1 %2. %3").arg(Rendorientation[0]).arg(Rendorientation[1]).arg(Rendorientation[2
	]));*/

	double *const Rendposition = m_volumeRenderer->volume()->GetPosition();
	DEBUG_LOG(QString("orientation %1 %2. %3").arg(Rendposition[0]).arg(Rendposition[1]).arg(Rendposition[2
	]));
	
	double angle = newAngle - oldAngle; 


	/*perform rotation to reslicer*/
	TransformReslicerExperimental(obj_center, angle);
	


	//m_sliceTranslationTransform->
	//m_slicerChannelRotate->setTransform(m_transform3D)

	//m_SliceRotateTransform->SetMatrix(m_transform3D->GetMatrix()); 

	//m_slicerChannel->setTransform()
	//transform an die reslicer weitergeben

	double const *orientationAfter = m_volumeRenderer->volume()->GetOrientation();
	DEBUG_LOG(QString("Orientation after %1 %2 %3").arg(orientationAfter[0]).arg(orientationAfter[1]).arg(orientationAfter[2]));;

	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();

	// da muss die Rotation weitergegeben werden

	m_volumeRenderer->update();
	emit actorsUpdated();


	
}


void iAvtkInteractStyleActor::UpdateReslicerTransform2D(double *const Rendposition, const double *orientation, const double *imageCenter)
{
	m_SliceRotateTransform->SetInput(m_transform3D);	
	m_SliceRotateTransform->PostMultiply();
	m_SliceRotateTransform->Translate(Rendposition);


	switch (m_currentSliceMode)
	{
	case iASlicerMode::XY:
		m_SliceRotateTransform->Translate(Rendposition[0], Rendposition[1], 0);
		break;
	case iASlicerMode::XZ:
		m_SliceRotateTransform->Translate(Rendposition[0], 0, Rendposition[2]);
		/*origin[0] += pos[0];
		origin[2] += pos[1];*/
		break;
	case iASlicerMode::YZ:
		m_SliceRotateTransform->Translate(0, Rendposition[1], Rendposition[2]);

		/*origin[1] += pos[0];
		origin[2] += pos[1];*/
		break;
	case iASlicerMode::SlicerCount:
		/*origin[0] += pos[0];
		origin[1] += pos[1];
		origin[2] += pos[2];*/
		break;
	}

	//m
	//m_slicerChannel[m_currentSliceMode]
	//m_SliceRotateTransform->RotateWXYZ()
	/*const double *imageCenter*/
	//setconst double *imageCenter

	m_slicerChannel[m_currentSliceMode]->reslicer()->SetResliceAxes(m_SliceRotateTransform->GetMatrix());


	//get orientation and transform from the reslicer

	m_slicerChannel[m_currentSliceMode]->reslicer()->Update();
}

void iAvtkInteractStyleActor::translateSlicerActor(const double *origin, const double *pos, double *posOut,  const int sliceMode)
{
	DEBUG_LOG(QString("Slicermode ").arg(sliceMode));
	//double test[3] = { 1,1,1 }; 
	//if (!(pos /*&& posOut*/)) return; 
	//m_sliceTranslationTransform->GetMatrix(this->m_volumeRenderer->volume()->GetMatrix()/*this->m_slicerChannel[m_currentSliceMode]->getImageActor()->GetMatrix()*/);
	/*m_sliceTranslationTransform[m_currentSliceMode]->set*/
	m_sliceTranslationTransform[sliceMode]->PostMultiply();
	m_sliceTranslationTransform[sliceMode]->Translate(origin);
	switch (sliceMode) {
	case iASlicerMode::XY:
		DEBUG_LOG("Mode xy");
		m_sliceTranslationTransform[sliceMode]->Translate(/*origin[0]+*/pos[0], /*origin[1]+*/pos[1], 0);
		break;
		/*	origin[0] += pos[0];
			origin[1] += pos[1];*/

	case iASlicerMode::XZ:
		DEBUG_LOG("Mode xz");
		m_sliceTranslationTransform[sliceMode]->Translate(/*origin[0]+*/pos[0], 0,/* origin[2]+*/pos[1]);
		/*origin[0] += pos[0];
		origin[2] += pos[1];*/
		break;
	case iASlicerMode::YZ:
		DEBUG_LOG("Mode yz"); 
		m_sliceTranslationTransform[sliceMode]->Translate(0, /*origin[1]+*/ pos[0], /*origin[2]+*/ pos[1]);
		break;
	}	/*origin[1] += pos[0];
		origin[2] += pos[1];*/
	//m_sliceTranslationTransform->Translate(pos[0], pos[1], 0); 
	//m_sliceTranslationTransform->Translate(origin); 
	m_sliceTranslationTransform[sliceMode]->Update();

	//posOut = m_sliceTranslationTransform[sliceMode]->GetOrientation(); 
	
	m_sliceTranslationTransform[sliceMode]->MultiplyPoint(pos, posOut);
	/*DEBUG_LOG(QString("Interactor pos %1 %2 %3").arg(pos[0]).arg(pos[1]).arg(pos[2]));
	DEBUG_LOG(QString("output pos %1 %2 %3").arg(posOut[0], posOut[1], posOut[2]));*/ 
	
	//this->m_slicerChannel[m_currentSliceMode]->reslicer()/*getImageActor()*/->SetResliceTransform(m_sliceTranslationTransform);
	///m_SliceTransform->Translate()
	

}


void iAvtkInteractStyleActor::Update3DTransform(const double * imageCenter, const double * spacing, double relativeAngle)
{
	
	//m_volumeRenderer->volume()->GetOrientation(volOrientation);
	//double const *orientation = m_volumeRenderer->volume()->GetOrientation(); 


	
	//use angle from previous volume
	//m_transform3D->Identity(); 
	m_transform3D->SetMatrix(m_volumeRenderer->volume()->GetMatrix()); 
	m_transform3D->PostMultiply();
	m_transform3D->Translate(-imageCenter[0]/**spacing[0]*/, -imageCenter[1]/**spacing[1]*/, -imageCenter[2]/**spacing[0]*/);
	
	//d/*ouble tmpAngle = 0.0f; */
	
	switch (m_currentSliceMode) {
	case YZ: 
		//angle = relativeAngle + orientation[0];
		m_transform3D->RotateX(relativeAngle); break;
	case XZ: 
		//angle = relativeAngle + orientation[1];
		m_transform3D->RotateY(relativeAngle); break;
	case XY: 
		//angle = relativeAngle + orientation[0];
		m_transform3D->RotateZ(relativeAngle); break;
	default: break;

	}

	m_transform3D->Translate(imageCenter[0]/**spacing[0]*/, imageCenter[1]/**spacing[1]*/, imageCenter[2]/**spacing[2]*/);
	//m_transform3D->Update(); 
	double volOrientation[3];

	//m_transform3D->GetOrientation(volOrientation);

	//m_volumeRenderer->volume()->SetOrientation(volOrientation);
	/*if (!m_image) DEBUG_LOG("Image is null ptr");*/

	//DEBUG_LOG(m_image->GetDimensions())
	//vtkSmartPointer<vtkTransformFilter> transformFilter = vtkSmartPointer<vtkTransformFilter>::New();
	//transformFilter->SetInput(m_image);
	//m_transformFilter->SetInputData(m_image/*->GetData()*/);
	
	//m/*_transformFilter->SetInformation(m_image->GetInformation());
	//not working below with transform filter
	
	/*m_transformFilter->SetTransform(m_transform3D);
	m_transformFilter->UpdateDataObject();
	m_transformFilter->Update();

	m_image =dynamic_cast<vtkImageData * >( m_transformFilter->GetOutputDataObject(0));

	if (!m_image) { DEBUG_LOG("img is null"); return;  }
	m_volumeRenderer->update(); */
	//*/	
	/*m_volumeRenderer->setUpdateImage(m_transformFilter); */
	//m_volumeRenderer->update(); 
	m_volumeRenderer->volume()->SetUserTransform(m_transform3D);
	m_volumeRenderer->update(); 
}

void iAvtkInteractStyleActor::TransformReslicerExperimental(double * obj_center, double rotationAngle)
{

	if (!m_image) { DEBUG_LOG(QString("image is null %1").arg(m_currentSliceMode)); return;  }
	
	//which transformation should be used
	m_SliceRotateTransform->SetMatrix(m_volumeRenderer->volume()->GetMatrix()/*this->InteractionProp->GetMatrix()*/);
	m_SliceRotateTransform->PostMultiply();
	m_SliceRotateTransform->Translate(-obj_center[0] /** spacing[0]*/, -obj_center[1]  /*spacing[1]*/, 0 /** spacing[2]*/);

	m_SliceRotateTransform->RotateZ(rotationAngle);
	m_SliceRotateTransform->Translate(obj_center[0] /** spacing[0]*/, obj_center[1] /** spacing[1]*/,0/** spacing[2]*/);
	
	m_SliceRotateTransform->Inverse(); 


	const int sliceNumber = m_mdiChild->sliceNumber(m_currentSliceMode);  /*m_slicerChannel[m_currentSliceMode]->reslicer()->GetNumber*/
	const int sliceMode = m_currentSliceMode; 
	double ofs[3] = { 0.0, 0.0, 0.0 };


	updateReslicerRotationTransformation(sliceMode, ofs, sliceNumber);
	
	m_volumeRenderer->update(); 

}

void iAvtkInteractStyleActor::updateReslicerRotationTransformation(const int sliceMode, double * ofs, const int sliceNumber)
{
	{
		m_slicerChannel[/*0*/sliceMode]->reslicer()->SetInputData(m_image);
		double const * spacing = m_image->GetSpacing();
		double const * origin = m_image->GetOrigin(); //origin bei null
		int slicerZAxisIdx = mapSliceToGlobalAxis(m_currentSliceMode, iAAxisIndex::Z);
		ofs[slicerZAxisIdx] = sliceNumber * spacing[slicerZAxisIdx];
		m_slicerChannel[/*0*/sliceMode]->reslicer()->SetResliceAxesOrigin(origin[0] + ofs[0], origin[1] + ofs[1], origin[2] + ofs[2]);


		/*rotate2->Inverse(); */
		/*m_slicerChannel[m]*/
		m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->SetOutputExtent(m_image->GetExtent());
		m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->SetOutputOrigin(origin);
		m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->SetOutputSpacing(m_image->GetSpacing());
		m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->Update();

		/*m_slicerChannel[m_currentSliceMode]->reslicer()->*/

		/*m_slicerChannel[m_currentSliceMode]->reslicer()->Update();*/
		//m_slicerChannel[m_currentSliceMode]->reslicer()->SetResliceAxes(m_SliceTransform->GetMatrix());
		m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->SetResliceTransform(m_SliceRotateTransform);
		m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->UpdateWholeExtent();
		m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->SetInterpolationModeToLinear();

		m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->Update();
	}
}
