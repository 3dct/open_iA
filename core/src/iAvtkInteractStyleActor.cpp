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

#include <vtkAlgorithmOutput.h>
#include <vtkPolyDataMapper.h>
#include <vtkCubeSource.h>
//#include <vtkPlaneSource.h>
#include <vtkSphereSource.h>
#include <vtkActor.h>
#include <vtkRenderer.h>
#include <vtkProperty.h>
//end TODO remove

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

namespace polyTransform {
	enum vismod
	{
		x, y, z
	};
}


	vtkStandardNewMacro(iAvtkInteractStyleActor);

iAvtkInteractStyleActor::iAvtkInteractStyleActor():
	m_volumeRenderer(nullptr),
	m_mdiChild(nullptr),
	enable3D(false),
	m_rotationEnabled(true)
{

	try {
		m_transform3D = vtkSmartPointer<vtkTransform>::New();

		for (int i = 0; i < 3; i++) {
			m_SliceInteractorTransform[i] = vtkSmartPointer<vtkTransform>::New();
		}
		std::fill(m_slicerChannel, m_slicerChannel + iASlicerMode::SlicerCount, nullptr);
		InteractionPicker->SetTolerance(100.0);
	}
	catch (std::bad_alloc &ba) {
		DEBUG_LOG("Error in Memory reservation"); 
	}
	
	
	
}

void iAvtkInteractStyleActor::initializeAndRenderPolyData(uint thickness)
{
	if (!m_image) return; 
	

	DEBUG_LOG("init cube"); 
	try {
		m_CubeSource_X = vtkSmartPointer<vtkCubeSource>::New();
		m_cubeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_cubeActor = vtkSmartPointer<vtkActor>::New();

		m_SphereSourceCenter = vtkSmartPointer<vtkSphereSource>::New();
		m_SphereMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_SphereActor = vtkSmartPointer<vtkActor>::New();
		m_cubeActor->GetProperty()->SetColor(1, 0, 0); //(R, G, B);

		m_cubeActor->GetProperty()->SetOpacity(1.0); //(R, G, B);

		m_cubeMapper->SetInputConnection(m_CubeSource_X->GetOutputPort());
		m_cubeActor->SetMapper(m_cubeMapper);
		m_cubeXTransform = vtkSmartPointer<vtkTransform>::New(); 
		m_RefTransform = vtkSmartPointer<vtkTransform>::New(); 

	


		m_SphereMapper->SetInputConnection(m_SphereSourceCenter->GetOutputPort());
		m_SphereActor->SetMapper(m_SphereMapper);
		m_SphereActor->GetProperty()->SetColor(1, 1, 1); //(R, G, B);

		m_SphereActor->GetProperty()->SetOpacity(0.7); //(R, G, B);

		const double * bounds = m_image->GetBounds();
		const double *spacing = m_image->GetSpacing();
		//DEBUG_LOG(QString("img bounds %1 %2 %3 %4 %5 %6").arg(bounds[0]).arg(bounds[1]).arg(bounds[2]).arg(bounds[3]).arg(bounds[4]).arg(bounds[5]));
		double imageCenter[3];
		imageCenter[0] = (bounds[1] + bounds[0]) / 2.0f;
		imageCenter[1] = (bounds[3] + bounds[2]) / 2.0f;
		imageCenter[2] = (bounds[5] + bounds[4]) / 2.0f;
		m_SphereSourceCenter->SetCenter(imageCenter);
		m_SphereSourceCenter->SetRadius(50.0);


	
		m_CubeSource_X->Update();
		
		m_CubeSource_X->SetCenter(imageCenter);
		m_CubeSource_X->SetBounds(bounds[0], bounds[1]/*+300*/, 0+imageCenter[1], thickness*spacing[1]+imageCenter[1], bounds[4]/*+300*/, bounds[5])/*+300)*/;
		m_CubeSource_X->Update();


		
		//this->rotatePolydata(m_cubeXTransform, m_cubeActor, imageCenter, 30.0, 1); 
		this->createReferenceObject(imageCenter, spacing, 2, bounds, 1); 
		
		this->translatePolydata(m_RefTransform, m_RefCubeActor, 0, 100, 0); 
		this->translatePolydata(m_RefTransform, m_RefCubeActor, 0, -100, 0);
		this->rotatePolydata(m_cubeXTransform, m_cubeActor, imageCenter, 30, 1); 
		this->translatePolydata(m_cubeXTransform, m_cubeActor, 0, 100, 0); 
		//this->translatePolydata(m_cubeXTransform, 0, 100, 0); 
		//m_cubeSource->SetBounds(bounds[0], bounds[1], )
		

		if (m_mdiChild && m_volumeRenderer) {

			m_volumeRenderer->getCurrentRenderer()->AddActor(m_cubeActor);
			m_volumeRenderer->currentRenderer()->AddActor(m_SphereActor);

			m_volumeRenderer->update(); 
		}
	}
	catch (std::bad_alloc &ba) {
		DEBUG_LOG("could not reserve memory for sphereactor");
	}
}

void iAvtkInteractStyleActor::rotateSlicerProp(vtkSmartPointer<vtkTransform> &transform, double *center, double angle, vtkProp3D *prop, uint mode)
{
	vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
	mat = prop->GetUserMatrix();
	if (mat) {
		DEBUG_LOG("Matrix successfull");
		transform->SetMatrix(mat);
	}

	transform->PostMultiply();
	transform->Translate(-center[0], -center[1], -center[2]);
	switch (mode)
	{
	case 0: transform->RotateX(angle); break;
	case 1: transform->RotateY(angle); break;
	case 2: transform->RotateZ(angle); break;

	default:
		break;
	}

	transform->Translate(center[0], center[1], center[2]);

	transform->Update();
	prop->SetUserTransform(transform);
	m_volumeRenderer->update();
}

void iAvtkInteractStyleActor::createReferenceObject(double /*const */* center, double const *spacing, uint thickness, const double *bounds, uint mode) 
{

	try {
		m_RefCubeSource = vtkSmartPointer<vtkCubeSource>::New();
		m_RefCubeMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_RefCubeActor = vtkSmartPointer<vtkActor>::New();
		m_RefCubeMapper->SetInputConnection(m_RefCubeSource->GetOutputPort());
		m_RefCubeActor->SetMapper(m_RefCubeMapper);
		m_RefCubeSource->SetCenter(center);
		m_RefCubeActor->GetProperty()->SetColor(0, 0.7, 0); //(R, G, B);

		m_RefCubeActor->GetProperty()->SetOpacity(0.5); //(R, G, B);
		

		//mode 0: x, 1: y, 2:z
		switch (mode)
		{
		case 0: m_RefCubeSource->SetBounds(0 + center[0], thickness*spacing[0] + center[0], bounds[2], bounds[3],
			bounds[4], bounds[5]); break;

		case 1: m_RefCubeSource->SetBounds(bounds[0], bounds[1]/*+300*/,
			0 + center[1], thickness*spacing[1] + center[1], bounds[4]/*+300*/, bounds[5])/*+300)*/; break;
		case 2:
			m_RefCubeSource->SetBounds(bounds[0], bounds[1]/*+300*/,
				bounds[2], bounds[3], 0 + center[2], thickness*spacing[2] + center[0]); break;
		default:
			break;
		}



		if (m_mdiChild && m_volumeRenderer) {
			m_volumeRenderer->getCurrentRenderer()->AddActor(m_RefCubeActor);
			m_volumeRenderer->update();
		}

	}
	catch (std::bad_alloc &ba) {
		DEBUG_LOG("could not reserve memory for sphereactor");

	}
}

void iAvtkInteractStyleActor::translatePolydata(vtkSmartPointer<vtkTransform> &polTransform, vtkSmartPointer<vtkActor> &polyActor, double X, double Y, double Z)
{
	m_cubeXTransform->SetMatrix(m_cubeActor->GetMatrix()); 
	vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
	mat = polyActor->GetUserMatrix();
	if (mat) {
		DEBUG_LOG("Matrix successfull");
		polTransform->SetMatrix(mat);
	}
	
	polTransform->Translate(X, Y, Z);
	polTransform->Update();
	polyActor->SetUserTransform(polTransform);
	m_volumeRenderer->update(); 
}

void iAvtkInteractStyleActor::rotatePolydata(vtkSmartPointer<vtkTransform> &polTransform, vtkSmartPointer<vtkActor> &polyActor, const double *center, double angle, uint mode)
{

	vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New(); 
	mat = polyActor->GetUserMatrix();
	if (mat) {
		//DEBUG_LOG("Matrix successfull");
		polTransform->SetMatrix(mat); 
	}

	polTransform->PostMultiply();
	polTransform->Translate(-center[0], -center[1], -center[2]);
	switch (mode)
	{
	case 0: polTransform->RotateX(angle); break; 
	case 1: polTransform->RotateY(angle); break;
	case 2: polTransform->RotateZ(angle); break; 

	default:
		break;
	}
	
	polTransform->Translate(center[0], center[1], center[2]);
	
	polTransform->Update();
	polyActor->SetUserTransform(polTransform);
	m_volumeRenderer->update();
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

	if (currentMode == 0) {
		initializeAndRenderPolyData(5); 
	}
	//just a test cube for one slcier; 


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

		//first translate interaction prop
		//this->interaction prop

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
		
		//original image
		
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
	//2d rotation should work in this way: 
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

	if (!m_image) { DEBUG_LOG(QString("Error on rotation %1").arg(m_currentSliceMode)); return; }

	// Get the axis to rotate around = vector from eye to origin

	
	double const *imageBounds = this->m_image->GetBounds();
	DEBUG_LOG(QString("Image Center"));
	double imageCenter[3];//image center
	imageCenter[0] = (imageBounds[1] + imageBounds[0]) / 2.0f;
	imageCenter[1] = (imageBounds[3] + imageBounds[2]) / 2.0f;
	imageCenter[2] = (imageBounds[5] + imageBounds[4]) / 2.0f;

	//center of interaction prop of current slicer 
	double *sliceProbCenter = this->InteractionProp->GetCenter();
	double disp_obj_center[3];
	double relativeAngle = 0.0f;
	
	relativeAngle = computeDisplayRotationAngle(sliceProbCenter, disp_obj_center, rwi, relativeAngle);

	
	//end original code
	this->rotateSlicerProp(m_SliceInteractorTransform[m_currentSliceMode], sliceProbCenter, relativeAngle, this->InteractionProp,2);

	double const * orientXYZ = this->InteractionProp->GetOrientation();
	double const * oriWXYZ = this->InteractionProp->GetOrientationWXYZ();

	DEBUG_LOG(QString("Orientation interactor XYZ %1 %2 %3 %4").arg(oriWXYZ[0]).arg(oriWXYZ[1]).arg(oriWXYZ[2]).arg(oriWXYZ[3]));


	QString mode = slicerModeString(m_currentSliceMode);
	DEBUG_LOG(QString("Mode %1 %2").arg(mode.toStdString().c_str()).arg(m_currentSliceMode));
	double const * spacing = m_image->GetSpacing();

	double const *volCenter = m_volumeRenderer->volume()->GetCenter();

	double const *orientationBefore = m_volumeRenderer->volume()->GetOrientation();
	//DEBUG_LOG(QString("Orientation before %1 %2 %3").arg(orientationBefore[0]).arg(orientationBefore[1]).arg(orientationBefore[2]));



	//TransformReslicer(obj_center/*imageCenter*/, relativeAngle); 


	//this we need obviously
	Update3DTransform(volCenter/*imageCenter*/, spacing, relativeAngle);
	/*double *const Rendorientation = m_volumeRenderer->volume()->GetOrientation();
	DEBUG_LOG(QString("orientation %1 %2. %3").arg(Rendorientation[0]).arg(Rendorientation[1]).arg(Rendorientation[2
	]));*/

	double *const Rendposition = m_volumeRenderer->volume()->GetPosition();
	/*DEBUG_LOG(QString("orientation %1 %2. %3").arg(Rendposition[0]).arg(Rendposition[1]).arg(Rendposition[2
	]));*/

	double angle = newAngle - oldAngle;




	//evtl die Orientation an die Transform weiter geben als INPut //TODO

	//custom center
	const int* imgExtend = m_image->GetExtent();
	double cutstomCenter[3];

	//this seems to be the same like image center? 
	cutstomCenter[0] = m_image->GetOrigin()[0] + spacing[0] * 0.5 *(imgExtend[0] + imgExtend[1]);
	cutstomCenter[1] = m_image->GetOrigin()[1] + spacing[1] * 0.5 *(imgExtend[2] + imgExtend[3]);
	cutstomCenter[2] = m_image->GetOrigin()[2] + spacing[2] * 0.5 *(imgExtend[4] + imgExtend[5]);

	DEBUG_LOG(QString("Custom center is %1 %2 %3").arg(cutstomCenter[0]).arg(cutstomCenter[1]).arg(cutstomCenter[2]));
	DEBUG_LOG(QString("VolCenter is %1 %2 %3").arg(volCenter[0]).arg(volCenter[1]).arg(volCenter[2]));
	DEBUG_LOG(QString("ImageCenter is %1 %2 %3").arg(imageCenter[0]).arg(imageCenter[1]).arg(imageCenter[2]));


	//TransformReslicerExperimental(imageCenter, angle, spacing, m_currentSliceMode);
	//this->rotatePolydata(m_cubeXTransform, m_cubeActor, imageCenter, angle,	1); 



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


double iAvtkInteractStyleActor::computeDisplayRotationAngle(double * sliceProbCenter, double * disp_obj_center, vtkRenderWindowInteractor * rwi, double relativeAngle)
{
	//drag an angle, calculate angle from coords, objectCenter comes from interaction prop 
	this->ComputeWorldToDisplay(sliceProbCenter[0], sliceProbCenter[1], sliceProbCenter[2],
		disp_obj_center);

	double newAngle =
		vtkMath::DegreesFromRadians(atan2(rwi->GetEventPosition()[1] - disp_obj_center[1],
			rwi->GetEventPosition()[0] - disp_obj_center[0]));

	double oldAngle =
		vtkMath::DegreesFromRadians(atan2(rwi->GetLastEventPosition()[1] - disp_obj_center[1],
			rwi->GetLastEventPosition()[0] - disp_obj_center[0]));

	//perform rotation of interaction prop of the slicer
	relativeAngle = newAngle - oldAngle;	return relativeAngle;
}

void iAvtkInteractStyleActor::UpdateReslicerTranslateTransform2D(double *const Rendposition, const double *orientation, const double *imageCenter, int sliceMode)
{
	m_SliceInteractorTransform[sliceMode]->SetInput(m_transform3D);	
	m_SliceInteractorTransform[sliceMode]->PostMultiply();
	m_SliceInteractorTransform[sliceMode]->Translate(Rendposition);


	switch (m_currentSliceMode)
	{
	case iASlicerMode::XY:
		m_SliceInteractorTransform[sliceMode]->Translate(Rendposition[0], Rendposition[1], 0);
		break;
	case iASlicerMode::XZ:
		m_SliceInteractorTransform[sliceMode]->Translate(Rendposition[0], 0, Rendposition[2]);
		/*origin[0] += pos[0];
		origin[2] += pos[1];*/
		break;
	case iASlicerMode::YZ:
		m_SliceInteractorTransform[sliceMode]->Translate(0, Rendposition[1], Rendposition[2]);

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

	m_slicerChannel[m_currentSliceMode]->reslicer()->SetResliceAxes(m_SliceInteractorTransform[sliceMode]->GetMatrix());


	//get orientation and transform from the reslicer

	m_slicerChannel[m_currentSliceMode]->reslicer()->Update();
}




void iAvtkInteractStyleActor::Update3DTransform(const double * imageCenter, const double * spacing, double relativeAngle)
{
	
	//m_volumeRenderer->volume()->GetOrientation(volOrientation);
	//double const *orientation = m_volumeRenderer->volume()->GetOrientation(); 


	
	//use angle from previous volume
	//m_transform3D->Identity(); 
	m_transform3D->SetMatrix(m_volumeRenderer->volume()->GetMatrix()); 
	m_transform3D->PostMultiply();
	m_transform3D->Translate(-imageCenter[0]/**spacing[0]*/, -imageCenter[1],/**spacing[1]*/ -imageCenter[2])/**spacing[2])*/;
	
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

	m_transform3D->Translate(imageCenter[0],/**spacing[0],*/ imageCenter[1]/**spacing[1]*/, imageCenter[2]/**spacing[2]*/);
	//m_transform3D->Inverse();
	
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
	//m_volumeRenderer->volume()->SetUserMatrix(m_transform3D->GetMatrix());
	m_volumeRenderer->update(); 
}

void iAvtkInteractStyleActor::TransformReslicerExperimental(double const * obj_center, double rotationAngle, double const *spacing, int sliceMode)
{

	if (!m_image) { DEBUG_LOG(QString("image is null, slicemode %1").arg(sliceMode)); return;  }
	

	/*this->center[0] = this->origin[0] + this->spacing[0] * 0.5 *
		(this->extent[0] + this->extent[1]);

	this->center[1] = this->origin[1] + this->spacing[1] * 0.5 *
		(this->extent[2] + this->extent[3]);

	this->center[2] = this->origin[2] + this->spacing[2] * 0.5 *
		(this->extent[4] + this->extent[5]);*/


	//which transformation should be used
	//rotate reslicer
	
	//m_SliceRotateTransform[sliceMode]->Identity(); 
	//m_SliceRotateTransform->Orie
	
	
	m_SliceInteractorTransform[sliceMode]->PostMultiply();
	//m_SliceRotateTransform[sliceMode]->Translate(-obj_center[0] /** spacing[0]*/, -obj_center[1]  /*spacing[1]*/, 0 /** spacing[2]*/);

	m_SliceInteractorTransform[sliceMode]->Translate(-obj_center[0],-obj_center[1],/**spacing[1]**spacing[2]*/-obj_center[2]);
	//m_SliceRotateTransform[sliceMode]->Translate(obj_center[0],-obj_center[1],/**spacing[1]**spacing[2]*/-obj_center[2]);

	//switch (sliceMode) {
	//case YZ:
		//m_SliceRotateTransform[sliceMode]->Translate(0, -obj_center[1], -obj_center[2]); //-obj_center[0] /** spacing[0]*/, -obj_center[1]  /*spacing[1]*/, /** spacing[2]*/);
		m_SliceInteractorTransform[sliceMode]->RotateZ(rotationAngle);
		//m_SliceRotateTransform[sliceMode]->Translate(0, obj_center[1], obj_center[2]);
		//m_SliceRotateTransform[sliceMode]->Translate(0, obj_center[1], obj_center[2]);
	//	break;
	//case XZ:
		//m_SliceRotateTransform[sliceMode]->Translate(-obj_center[0], 0, -obj_center[2]); //-obj_center[0] /** spacing[0]*/, -obj_center[1]  /*spacing[1]*/, /** spacing[2]*/);
		//m_SliceRotateTransform[sliceMode]->RotateY(rotationAngle);
		//m_SliceRotateTransform[sliceMode]->Translate(obj_center[0], 0, obj_center[2]);
	//	break;
	//case XY:
		//m_SliceRotateTransform[sliceMode]->Translate(-obj_center[0],-obj_center[1], 0); //-obj_center[0] /** spacing[0]*/, -obj_center[1]  /*spacing[1]*/, /** spacing[2]*/);
	//	m_SliceRotateTransform[sliceMode]->RotateZ(rotationAngle);
		//m_SliceRotateTransform[sliceMode]->Translate(obj_center[0],obj_center[1], 0);
	//	break;
	//}
	/*default: break;
	}*/
	//m_SliceRotateTransform[sliceMode]->Inverse();
	//m_SliceRotateTransform[sliceMode]->Translate(obj_center[0] /** spacing[0]*/, obj_center[1] /** spacing[1]*/,0/** spacing[2]*/);
	m_SliceInteractorTransform[sliceMode]->Translate(obj_center[0], obj_center[1],/*0*//**spacing[1]*//*,0*//**spacing[2]*/ obj_center[2]);
	m_SliceInteractorTransform[sliceMode]->Inverse(); 


	const int sliceNumber = m_mdiChild->sliceNumber(sliceMode);  /*m_slicerChannel[m_currentSliceMode]->reslicer()->GetNumber*/
	//const int sliceMode = m_currentSliceMode; 
	double ofs[3] = { 0.0, 0.0, 0.0 };

	//for (int i = 0; i < iASlicerMode::SlicerCount; ++i) {
	//	int sliceMode = i; 
	//	int sliceNumber = m_mdiChild->sliceNumber(sliceMode);
	//	updateReslicerRotationTransformation2d(sliceMode, ofs, sliceNumber);
	//}
	
	updateReslicerRotationTransformation2d(sliceMode, ofs, sliceNumber);
	/*int sliceNumber2 = m_mdiChild->sliceNumber(1);
	int sliceNumber3 = m_mdiChild->sliceNumber(2);


	updateReslicerRotationTransformation2d(1, ofs, sliceNumber2);
	updateReslicerRotationTransformation2d(2, ofs, sliceNumber3);*/
	m_volumeRenderer->update(); 

}

void iAvtkInteractStyleActor::updateReslicerRotationTransformation2d(const int sliceMode, double * ofs, const int sliceNumber)
{

	
		m_slicerChannel[sliceMode]->reslicer()->SetInputData(m_image);
		double const * spacing = m_image->GetSpacing();
		double const * origin = m_image->GetOrigin(); //origin bei null
		int slicerZAxisIdx = mapSliceToGlobalAxis(sliceMode, iAAxisIndex::Z/*iAAxisIndex::Z*/);

		//ist das immer die Z-Achse? 
		ofs[slicerZAxisIdx] = sliceNumber * spacing[slicerZAxisIdx];
		m_slicerChannel[sliceMode]->reslicer()->SetResliceAxesOrigin(origin[0] + ofs[0], origin[1] + ofs[1], origin[2] + ofs[2]);


		
		m_slicerChannel[sliceMode]->reslicer()->SetOutputExtent(m_image->GetExtent());
		//m_slicerChannel[sliceMode]->reslicer()->SetOutputOrigin(origin[0] + ofs[0], origin[1] + ofs[1], origin[2] + ofs[2]);
		m_slicerChannel[sliceMode]->reslicer()->SetOutputSpacing(m_image->GetSpacing());
		m_slicerChannel[sliceMode]->reslicer()->Update();
	/*	m_slicerChannel[sliceMode]->reslicer()->SetOutputDimensionality(2);*/
	//geht offensichtlich nur über das rotate transform
		
		//m_slicerChannel[sliceMode]->reslicer()->SetResliceAxes(m_SliceRotateTransform[sliceMode]->GetMatrix());
		m_slicerChannel[sliceMode]->reslicer()->SetResliceTransform(m_SliceInteractorTransform[sliceMode]);
		//m_slicerChannel[sliceMode]->reslicer()->UpdateWholeExtent();
		m_slicerChannel[sliceMode]->reslicer()->SetInterpolationModeToLinear();

		m_slicerChannel[sliceMode]->reslicer()->Update();
		//m_image = /*dynamic_cast<vtkImageData* >(*/m_slicerChannel[/*0*//*m_currentSliceMode*/sliceMode]->reslicer()->GetInformationInput()/*)*/;
	
}
