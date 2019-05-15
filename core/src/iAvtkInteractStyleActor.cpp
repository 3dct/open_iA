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
#include <vtkLineSource.h>

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
			m_ReslicerTransform[i] = vtkSmartPointer<vtkTransform>::New(); 
		}
		std::fill(m_slicerChannel, m_slicerChannel + iASlicerMode::SlicerCount, nullptr);
		InteractionPicker->SetTolerance(100.0);

		for (int i = 0; i < 3; i++) {
			m_currentVolRendererPosition[i] = 0.0f;
		}
		/*m_currentActorPosition = new double[3]; */

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
		m_cubeActor->SetDragable(0); 
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
		m_SphereSourceCenter->SetRadius(5.0);
		
	
		m_CubeSource_X->Update();
		
		m_CubeSource_X->SetCenter(imageCenter);
		m_CubeSource_X->SetBounds(bounds[0], bounds[1]/*+300*/, 0+imageCenter[1], thickness*spacing[1]+imageCenter[1], bounds[4]/*+300*/, bounds[5])/*+300)*/;
		m_CubeSource_X->Update();


		
		//this->rotatePolydata(m_cubeXTransform, m_cubeActor, imageCenter, 30.0, 1); 
	/*	this->createReferenceObject(imageCenter, spacing, 2, bounds, 1); 
		this->createAndInitLines(bounds, imageCenter);*/

		
		/*this->translatePolydata(m_RefTransform, m_RefCubeActor, 0, 100, 0); 
		this->translatePolydata(m_RefTransform, m_RefCubeActor, 0, -100, 0);*/
		//this->rotatePolydata(m_cubeXTransform, m_cubeActor, imageCenter, 30, 1); 
		//this->translatePolydata(m_cubeXTransform, m_cubeActor, 0, 100, 0); 
		//this->translatePolydata(m_cubeXTransform, 0, 100, 0); 
		//m_cubeSource->SetBounds(bounds[0], bounds[1], )
		

		if (m_mdiChild && m_volumeRenderer) {

			//m_volumeRenderer->getCurrentRenderer()->AddActor(m_cubeActor);
			m_volumeRenderer->currentRenderer()->AddActor(m_SphereActor);
			m_volumeRenderer->update(); 
		}
	}
	catch (std::bad_alloc &ba) {
		DEBUG_LOG("could not reserve memory for sphereactor");
	}
}

void iAvtkInteractStyleActor::rotateInterActorProp(vtkSmartPointer<vtkTransform> &transform,  double const *center, double angle, vtkProp3D *prop, uint mode)
{
	vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
	mat = prop->GetUserMatrix();
	if (mat) {
		//DEBUG_LOG("Matrix successfull");
		transform->SetMatrix(mat);
	}

	rotateAroundAxis(transform, center, mode, angle);

	prop->SetUserTransform(transform);
	m_volumeRenderer->update();
}

void iAvtkInteractStyleActor::translateInterActor(vtkSmartPointer<vtkTransform> &transform, vtkImageActor *actor, double const *position, uint mode)
{

	double relMovement[3] = { 0, 0, 0 };
	//double tempPos[3] = { position[0], position[1], position[2] }; 
	double tempPos[3] = { 0, 0, 0}; 
	/*
	switch (mode)
	{
	case iASlicerMode::XY:

		tempPos[0] = position[0];
		tempPos[1] = position[1];
		break;
	case iASlicerMode::XZ:
		tempPos[0] = position[0];
		tempPos[2] = position[2];
		break;
	case iASlicerMode::YZ:
		tempPos[1] = position[1];
		tempPos[2] = position[2];
		break;

	default:
		break;
	}*/
	/*


	switch (m_currentSliceMode) {
	case iASlicerMode::XY:
		relMovement[0] = position[0];
		relMovement[1] = position[1];
		break;
	case iASlicerMode::XZ:
		relMovement[0] = position[0];
		relMovement[1] = position[2];
		break;
	case iASlicerMode::YZ:
		relMovement[0] = position[1];
		relMovement[1] = position[2];
		break;
	}*/

	//DEBUG_LOG(QString("rel movement position %1 %2 %3").arg(relMovement[0]).arg(relMovement[1]).arg(relMovement[2]));
	
	//transform->PostMultiply();
	vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
	//mat = actor->GetMatrix();
	//if (mat) {
	//	//DEBUG_LOG("Matrix successfull");
	//	transform->SetMatrix(mat);
	//}

	//switch(mode){
	////*double relMovement[3] = { 0, 0, 0 };
	///*case(iASlicerMode::): transform->Translate(0, position[1], 0); break;
	//transform->Translate(0, 0, position[2]);*/
	//}
	//pos0 -> x;
	//pos1 -> y
	//pos2 ->Z

	//switch (mode) {
	//case iASlicerMode::XY:
	//	DEBUG_LOG("mode xy");
	//	relMovement[0] = tempPos[0] /*+actorPosition[0]*/;
	//	relMovement[1] = tempPos[1] /*+actorPosition[1]*/;
	//	break;
	//case iASlicerMode::XZ:
	//	//if from yz z-> dann nur die z variable
	//	//if xy dann nur y variable
	//	DEBUG_LOG("mode xz");
	//	relMovement[0] = tempPos[0] /*+ actorPosition[0]*/;
	//	//DEBUG_LOG(QString("position x %1").arg(relMovement[0]));

	//	relMovement[1] = tempPos[2] /*+ actorPosition[1]*/;
	//	//DEBUG_LOG(QString("position z %1").arg(relMovement[1]));
	//	break;
	//case iASlicerMode::YZ:
	//	DEBUG_LOG("mode yz");
	//	relMovement[0] = tempPos[1] /*+ actorPosition[0]*/;
	//	relMovement[1] = tempPos[2] /*+ actorPosition[1]*/;
	//	break;
	//}

	//DEBUG_LOG(QString("rel movement x, y, z %1 %2 %3").arg(relMovement[0]).arg(relMovement[1]).arg(relMovement[2]));
	/*double x, y, z; */
	//DEBUG_LOG(QString("Position  %1 %2 %3").arg(position[0]).arg(position[1]).arg(position[2])); 
	//vtkProp3D * 
	//double const *actorPos = actor->GetPosition(); 
	TranslateActorMovement(actor,mode,  transform, position);

}

void iAvtkInteractStyleActor::TranslateActorMovement(vtkImageActor * actor, uint mode, vtkSmartPointer<vtkTransform> & transform, double const * position)
{
	if (!actor)   {
		return; 
	}

	double actorPosition[3];	
	actor->GetPosition(actorPosition);
	

	switch (m_currentSliceMode)
	{
	case iASlicerMode::XY:
		if (mode == iASlicerMode::XZ) {
			transform->Translate(position[0] + actorPosition[0], 0, 0);

		}
		else if (mode == iASlicerMode::YZ) {
			transform->Translate(position[1] + actorPosition[1], 0, 0);
		}

		break;

	case iASlicerMode::XZ:
		if (mode == iASlicerMode::YZ) {
			transform->Translate(0, position[2] + actorPosition[2], 0);

		}
		else if (mode == iASlicerMode::XY) {
			transform->Translate(position[0], 0, 0);
		}


		break;
	case iASlicerMode::YZ:
		if (mode == iASlicerMode::XY) {
			transform->Translate(0, position[1], 0);

		}
		else if (mode == iASlicerMode::XZ) {
			transform->Translate(0, position[2], 0);
		}


		break;
	 default:
		
		break;
		


	
	}

	//transform->Translate(relMovement);
	transform->Update();
	//transform->MultiplyPoint(actorPosition)
	
	actor->SetUserTransform(transform);
	actor->Update();
	
	
}

void iAvtkInteractStyleActor::rotateAroundAxis(vtkSmartPointer<vtkTransform> & transform, double const * center, uint mode, double angle)
{
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
}

void iAvtkInteractStyleActor::rotateReslicer(vtkSmartPointer<vtkTransform> &transform, vtkImageReslice *reslicer,  double const *center, uint mode, double angle)
{
	
	this->rotateAroundAxis(transform, center, mode, angle);  //transform is ready; 
	

	//m_slicerChannel[sliceMode]->reslicer()->SetInputData(m_image);
	double const * spacing = m_image->GetSpacing();
	double const * origin = m_image->GetOrigin(); //origin bei null

	//TODO change axes; depending on slicer mode by switch caes
	int slicerZAxisIdx = mapSliceToGlobalAxis(mode, iAAxisIndex::Z/*iAAxisIndex::Z*/);

	//ist das immer die Z-Achse? 
	double ofs[3] = { 0.0, 0.0, 0.0 };
	const int sliceNumber = m_mdiChild->sliceNumber(mode);
	ofs[slicerZAxisIdx] = sliceNumber * spacing[slicerZAxisIdx];
	//m_slicerChannel[mode]->reslicer()->SetResliceAxesOrigin(origin[0] + ofs[0], origin[1] + ofs[1], origin[2] + ofs[2]);
	

	m_slicerChannel[mode]->reslicer()->SetOutputExtent(m_image->GetExtent());
	//m_slicerChannel[sliceMode]->reslicer()->SetOutputOrigin(origin[0] + ofs[0], origin[1] + ofs[1], origin[2] + ofs[2]);
	m_slicerChannel[mode]->reslicer()->SetOutputSpacing(spacing);
	m_slicerChannel[mode]->reslicer()->Update();
	/*	m_slicerChannel[sliceMode]->reslicer()->SetOutputDimensionality(2);*/
	//geht offensichtlich nur über das rotate transform

		//m_slicerChannel[sliceMode]->reslicer()->SetResliceAxes(m_SliceRotateTransform[sliceMode]->GetMatrix());
	m_slicerChannel[mode]->reslicer()->SetResliceTransform(m_SliceInteractorTransform[mode]);
	//m_slicerChannel[sliceMode]->reslicer()->UpdateWholeExtent();
	m_slicerChannel[mode]->reslicer()->SetInterpolationModeToLinear();

	m_slicerChannel[mode]->reslicer()->Update();
	
	

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
		m_RefCubeActor->SetDragable(0);

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

void iAvtkInteractStyleActor::createAndInitLines(double const *bounds, double const * center)
{ 
	if ((!m_mdiChild) && (!m_volumeRenderer)) return; 
	 try {
		 vtkSmartPointer<vtkLineSource> refLine[3]; 
		 vtkSmartPointer<vtkActor> refActor[3];
		 vtkSmartPointer<vtkPolyDataMapper> refMapper[3]; 

		for (int i = 0; i < 3; i++)
		{
			refLine[i] = vtkSmartPointer<vtkLineSource>::New();
			refActor[i] = vtkSmartPointer<vtkActor>::New();
			refMapper[i] = vtkSmartPointer<vtkPolyDataMapper>::New();

			refMapper[i]->SetInputConnection(refLine[i]->GetOutputPort());
			refActor[i]->SetMapper(refMapper[i]);
			
		}

		this->initLine(refLine[0], refActor[0], center, bounds[0], bounds[1], 0);
		this->initLine(refLine[1], refActor[1], center, bounds[2], bounds[3], 1);
		this->initLine(refLine[2], refActor[2], center, bounds[4], bounds[5], 2);
		
		for (int i =0; i < 3; i++)
		{
			m_volumeRenderer->getCurrentRenderer()->AddActor(refActor[i]);
		}

		m_volumeRenderer->update();
		

	}catch (std::bad_alloc &ba) {
		DEBUG_LOG("Mem error in line creation"); 
	}
}

void iAvtkInteractStyleActor::initLine(vtkSmartPointer<vtkLineSource> &line, vtkSmartPointer<vtkActor>& lineActor, double const * center, double min, double max, uint sliceMode)
{
	if ((!line) & (!lineActor)) return;
	if (sliceMode > 2) return; 
	double dist_half = (min + max) / 2.0f; 

	double color[3] = { 0, 0, 0 };
	double point1[3] = { center[0],center[1],center[2] };
	double point2[3] = { center[0],center[1],center[2] };

	color[sliceMode] = 1;
	point1[sliceMode] = center[sliceMode]-dist_half;
	point2[sliceMode] = center[sliceMode]+dist_half;
	
	line->SetPoint1(point1); 
	line->SetPoint2(point2);
	lineActor->GetProperty()->SetColor(color); 
	lineActor->GetProperty()->SetOpacity(0.82); 
	lineActor->GetProperty()->SetLineWidth(4);
	lineActor->SetDragable(0); 
	lineActor->SetPickable(0); 
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



	//mouse move with shift key = translation
	if (this->Interactor->GetShiftKey()) {
		updateInteractors();
	}
	else if (m_rotationEnabled) { // do rotation of the slicer on mouse move
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


	//initialize pos of currentSlicer
	if (!enable3D) {
		setPreivousActorPosition(this->m_slicerChannel[m_currentSliceMode]->actorPosition());

	}

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
		double const * posVol = m_volumeRenderer->position();
		if (posVol[0] == 0 && posVol[1] == 0 && posVol[2] == 0)
			return;

		//ggf muss das an alle actor weiter gegeben werden		
		//new coords based on current volume renderer position
		/*original code*/
		/*updateCoords(origin, posVol, m_currentSliceMode);
		m_volumeRenderer->volume()->SetPosition(0, 0, 0);*/
		/*end original code*/

		//similar thing like update coords; 
		//original coords += positions; 

	

		double relMovement[3] = { 0, 0, 0 };
		relMovement[0] = posVol[0] - m_currentVolRendererPosition[0];
		relMovement[1] = posVol[1] - m_currentVolRendererPosition[1];
		relMovement[2] = posVol[2] - m_currentVolRendererPosition[2];

		//prepare transform for every slicer 
		//relative translation of the interactor

		auto mat0 = m_slicerChannel[0]->imageActor()->GetUserMatrix();
		auto mat1 = m_slicerChannel[1]->imageActor()->GetUserMatrix();
		auto mat2 = m_slicerChannel[2]->imageActor()->GetUserMatrix();

		if (mat0) {
			m_SliceInteractorTransform[0]->SetMatrix(mat0);
			DEBUG_LOG("Mat xy 0k");
		}
		if (mat1) {
			m_SliceInteractorTransform[1]->SetMatrix(mat1);
			DEBUG_LOG("Mat xz 0k");

		}if (mat2) {
			m_SliceInteractorTransform[2]->SetMatrix(mat2);
		}

		//translate from 3d to 2d
		
		//for the actors

		//m_SliceInteractorTransform[0]->Translate(relMovement[0], relMovement[1], 0);//relative movement for xy
		//m_SliceInteractorTransform[1]->Translate(relMovement[0], relMovement[2], 0);//rel movement xz
		//m_SliceInteractorTransform[2]->Translate(relMovement[1], relMovement[2], 0); //rel movement yz
		//m_slicerChannel[0]->imageActor()->SetUserTransform(m_SliceInteractorTransform[0]);
		//m_slicerChannel[1]->imageActor()->SetUserTransform(m_SliceInteractorTransform[1]);
		//m_slicerChannel[2]->imageActor()->SetUserTransform(m_SliceInteractorTransform[2]);

		double trans_xy[3] = { relMovement[0], relMovement[1],0 };
	/*	double trans_xz[3] = { relMovement[0], 0,  relMovement[2] };
		double trans_yz[3] = {0, relMovement[1],  relMovement[2] };*/
		double trans_z[3] = { 0,0 , relMovement[2] };

		//reslicer plane XZ moving by xy 
		//reslicer plane yz moving by xy
		//reslicer plane xy moving by z
		
		this->TranslateReslicer(m_ReslicerTransform[0], m_slicerChannel[0]->reslicer(), trans_xy, m_image->GetSpacing(), 0, m_image->GetCenter()); 
		this->TranslateReslicer(m_ReslicerTransform[1], m_slicerChannel[1]->reslicer(), trans_xy, m_image->GetSpacing(), 1, m_image->GetCenter());
		this->TranslateReslicer(m_ReslicerTransform[2], m_slicerChannel[2]->reslicer(), trans_z, m_image->GetSpacing(), 2, m_image->GetCenter());
		//angenommen das geht dann den reslicer transformieren


		//nur statt dem den reslicer transformieren

		//store current position of renderer

		m_currentVolRendererPosition[0] = posVol[0];
		m_currentVolRendererPosition[1] = posVol[1];
		m_currentVolRendererPosition[2] = posVol[2];
		//come from 3d 
		//update reslicers // actors

	}
	else //update 2d Slicer
	{
		if (!m_slicerChannel[m_currentSliceMode])
			return;

		//This is a translation of current slicer
		DEBUG_LOG(QString("iam translating %1").arg(m_currentSliceMode));

		//takes position of the current actor //currentActor is translated
		double sliceActorPos[3];
		auto tmpslicepos = m_slicerChannel[m_currentSliceMode]->actorPosition();
		std::copy(tmpslicepos, tmpslicepos + 3, sliceActorPos);

		if (sliceActorPos[0] == 0 && sliceActorPos[1] == 0 && sliceActorPos[2] == 0) //no movement
			return;

		double spacing[3];
		m_image->GetSpacing(spacing);


		// //mapping machen yz -> x, y	xy -> x, y		xz->x, y
		////currentSlice pos, mode 
		////stub

		//new code
		//translateInterActor(m_SliceInteractorTransform, TODO, sliceActorPos, mode);
		//updateCoords(origin, sliceActorPos, m_currentSliceMode);
		//0 -> xy
		//1 -> xz
		//2 -> yz

		//prepare the coords thinking of relative movement in display coords
		double movement[3] = { 0,0,0 };

		
		//output is movement
		DEBUG_LOG(QString("previous position %1 %2 %3 ")
			.arg(m_currentSliceActorPosition[0]).arg(m_currentSliceActorPosition[1]).arg(m_currentSliceActorPosition[2]));
		prepareCoordsXYZ(movement, sliceActorPos);

		//DEBUG_LOG(QString("previous actor coords %1 %2 %3").arg("")
		DEBUG_LOG(QString("current slice actor pos %1 %2 %3").arg(sliceActorPos[0]).arg(sliceActorPos[1]).arg(sliceActorPos[2]));
		DEBUG_LOG(QString("x y z %1 %2 %3").arg(movement[0]).arg(movement[1]).arg(movement[2]));


		//start from slice mode 0 which is slice YZ
		int mode0_yz = m_currentSliceMode;


		DEBUG_LOG(QString("movement %1 %2 %3").arg(movement[0]).arg(movement[1]).arg(movement[2]));		

		//this works
		/*for (int i = 0; i < 3; i++) {
			if (i < m_currentSliceMode) continue;
			TranslateActor(movement, 0);

		


		this->setPreivousActorPosition(sliceActorPos); //setting to original
		}*/
		
		//this->TranslateReslicer(m_ReslicerTransform[m_currentSliceMode], m_slicerChannel[m_currentSliceMode]->reslicer(),0,testMovement, 0, 0, testMovement, 0, m_image->GetCenter());
		double reslPos[3] = { 0, 20, 0 };
		// 
		//this->TranslateReslicer(m_ReslicerTransform[m_currentSliceMode], m_slicerChannel[m_currentSliceMode]->reslicer(), reslPos, spacing, m_currentSliceMode, m_image->GetCenter());
		testMovement += 10 * spacing[0];
		DEBUG_LOG(QString("%1").arg(testMovement));
		int test_mode = 1; 
		
		//then translate 3d
			//translate the volume renderer
		vtkSmartPointer<vtkMatrix4x4> matVol = vtkSmartPointer<vtkMatrix4x4>::New();
		matVol = m_volumeRenderer->volume()->GetUserMatrix();
		if (matVol)
			m_transform3D->SetMatrix(matVol);
		m_transform3D->Translate(movement[0] * spacing[0], movement[1] * spacing[1], movement[2] * spacing[2]);
		m_volumeRenderer->volume()->SetUserTransform(m_transform3D);

		//end experimental

		//position vom actor abspeichern; 
		//perform translation based on an previous position

		//end new code
		
		//new code
		//should be same as coords translate current interactor
		

		//first translate interaction prop
		//this->interaction prop
				
		//original image

		//begin original code
		//sets New Origin of the image
	//	updateCoords(origin, sliceActorPos, m_currentSliceMode);

		//m_slicerChannel[m_currentSliceMode]->setActorPosition(0, 0, 0);
		//end original code

		//and use origin to update it
	}

	//begin original code
	//m_volumeRenderer->volume()->SetOrigin(transformposOut);

	////m_image->SetOrigin(/*transformposOut*//*origin)*/;  //< update image origin
	//end original code

	

	//update reslicer
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();

	m_volumeRenderer->update();
	emit actorsUpdated();
	
}

void iAvtkInteractStyleActor::TranslateActor(double const * movement, uint mode)
{
	DEBUG_LOG(QString("movement %1 %2 %3 ").arg(movement[0]).arg(movement[1]).arg(movement[2]));
	//mode YZ ->translate xz and xz
	//mode xy ->translate xz, yz
	//mode xz ->tranlate xy, yz
	
	switch (mode)
		{

		case iASlicerMode::XY:

			DEBUG_LOG("Translation yz Translation XZ");

			performTranslationTransform(m_SliceInteractorTransform[0], m_slicerChannel[0]->imageActor(), movement, 0);
			performTranslationTransform(m_SliceInteractorTransform[1], m_slicerChannel[1]->imageActor(), movement, 1);
			break;

		case iASlicerMode::XZ:
			DEBUG_LOG("Translation xy, Translation xy");
			performTranslationTransform(m_SliceInteractorTransform[0], m_slicerChannel[0]->imageActor(), movement, 0);
			performTranslationTransform(m_SliceInteractorTransform[2], m_slicerChannel[2]->imageActor(), movement, 2);
			break;

		case iASlicerMode::YZ:
			DEBUG_LOG("Translation xy, Translation xz");
			performTranslationTransform(m_SliceInteractorTransform[1], m_slicerChannel[1]->imageActor(), movement, 1);
			performTranslationTransform(m_SliceInteractorTransform[2], m_slicerChannel[2]->imageActor(), movement, 2);
			break;

		default:
			DEBUG_LOG("no interaction");
			break;
		}
			
}

void iAvtkInteractStyleActor::performTranslationTransform(vtkSmartPointer<vtkTransform> &transform, vtkImageActor *actor, double const *relMovement, uint mode)
	{
		
		DEBUG_LOG(QString("translation vector %1 %2 %3").arg(relMovement[0]).arg(relMovement[1]).arg(relMovement[2])); 

		if (!relMovement) return;
		/*vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
		mat = actor->GetUserMatrix();
		if (mat)
			transform->SetMatrix(mat);*/

		switch (mode) {
		case iASlicerMode::XY:
			DEBUG_LOG("translate xy");
			transform->Translate(relMovement[0], relMovement[1], 0);

			break;
		case iASlicerMode::XZ:
			DEBUG_LOG("translate xz");
			transform->Translate(relMovement[0], relMovement[2], 0);
			break;

		case iASlicerMode::YZ:
			DEBUG_LOG("translate yz");
			transform->Translate(relMovement[1], relMovement[2], 0);
			break;

		default:
			break;

		}

		transform->Update();
		//transform[0]->Translate(relMovement[0], relMovement[1], 0);
		////rel movement xz
		//transform[1]->Translate(relMovement[1], relMovement[2], 0);
		////rel movement yz
		//transform[2]->Translate(relMovement[1], relMovement[2], 0);
		double posOut[3] = { 0,0, 0, };
		
		transform->GetPosition(posOut);//m_currentSliceActorPosition, posOut);
		actor->SetPosition(posOut); 
		//actor->SetUserTransform(transform);
		/*m_slicerChannel[1]->imageActor()->SetUserTransform(m_SliceInteractorTransform[1]);
		m_slicerChannel[2]->imageActor()->SetUserTransform(m_SliceInteractorTransform[2]);*/
		actor->Update();
	}



void iAvtkInteractStyleActor::prepareCoordsXYZ(double * movement, double const * sliceActorPos)
{
	switch (m_currentSliceMode) {
	case iASlicerMode::XY:
		movement[0] = sliceActorPos[0] - m_currentSliceActorPosition[0];
		movement[1] = sliceActorPos[1] - m_currentSliceActorPosition[1];
		break;
	case iASlicerMode::XZ:
		movement[0] = sliceActorPos[0] - m_currentSliceActorPosition[0];
		movement[2] = sliceActorPos[1] - m_currentSliceActorPosition[1];
		break;
	case iASlicerMode::YZ:
		movement[1] = sliceActorPos[0] - m_currentSliceActorPosition[0];
		movement[2] = sliceActorPos[1] - m_currentSliceActorPosition[1];
		break;
	}
	DEBUG_LOG(QString("prepareCoords %1 %2 %3").arg(movement[0]).arg(movement[1]).arg(movement[2]));
}

void iAvtkInteractStyleActor::rotate2D()
{
	//2d rotation should work in this way: 
	/*
	*1 perform perform rotation to interactor
	*2  update 3d volume -> yes
	*3 update slicer
	*/


	//DEBUG_LOG("Rotation");
	if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
	{
		return;
	}

	vtkRenderWindowInteractor *rwi = this->Interactor;
	if (!m_image) { DEBUG_LOG(QString("Error on rotation %1").arg(m_currentSliceMode)); return; }

	// Get the axis to rotate around = vector from eye to origin
	double const *imageBounds = this->m_image->GetBounds();
	//DEBUG_LOG(QString("Image Center"));
	double imageCenter[3];//image center
	imageCenter[0] = (imageBounds[1] + imageBounds[0]) / 2.0f;
	imageCenter[1] = (imageBounds[3] + imageBounds[2]) / 2.0f;
	imageCenter[2] = (imageBounds[5] + imageBounds[4]) / 2.0f;

	//center of interaction prop of current slicer 
	double *sliceProbCenter = this->InteractionProp->GetCenter();
	double disp_obj_center[3];
	double relativeAngle = 0.0f;
	
	//2d, rotate Z; 
	computeDisplayRotationAngle(sliceProbCenter, disp_obj_center, rwi, relativeAngle);
	this->rotateInterActorProp(m_SliceInteractorTransform[m_currentSliceMode], sliceProbCenter, relativeAngle, this->InteractionProp,2);

	QString mode = slicerModeString(m_currentSliceMode);
	//DEBUG_LOG(QString("Mode %1 %2").arg(mode.toStdString().c_str()).arg(m_currentSliceMode));
	double const * spacing = m_image->GetSpacing();
	double const *volImageCenter = m_volumeRenderer->volume()->GetCenter();
	double const *orientationBefore = m_volumeRenderer->volume()->GetOrientation();
	   
	//DEBUG_LOG(QString("Orientation before %1 %2 %3").arg(orientationBefore[0]).arg(orientationBefore[1]).arg(orientationBefore[2]));
	//rotate in 3d interactor; 
			
	//rotate around axis based on the spacing needed  //otherwise multiply center with spacing??
	this->rotateInterActorProp(m_transform3D, volImageCenter, relativeAngle, m_volumeRenderer->volume(), m_currentSliceMode); 
			
		   
	//evtl die Orientation an die Transform weiter geben als INPut //TODO

	//custom center
	const int* imgExtend = m_image->GetExtent();
	double cutstomCenter[3];

	//this seems to be the same like image center? 
	cutstomCenter[0] = m_image->GetOrigin()[0] + spacing[0] * 0.5 *(imgExtend[0] + imgExtend[1]);
	cutstomCenter[1] = m_image->GetOrigin()[1] + spacing[1] * 0.5 *(imgExtend[2] + imgExtend[3]);
	cutstomCenter[2] = m_image->GetOrigin()[2] + spacing[2] * 0.5 *(imgExtend[4] + imgExtend[5]);

	//DEBUG_LOG(QString("Custom center is %1 %2 %3").arg(cutstomCenter[0]).arg(cutstomCenter[1]).arg(cutstomCenter[2]));
	//DEBUG_LOG(QString("VolCenter is %1 %2 %3").arg(volImageCenter[0]).arg(volImageCenter[1]).arg(volImageCenter[2]));
	//DEBUG_LOG(QString("ImageCenter is %1 %2 %3").arg(imageCenter[0]).arg(imageCenter[1]).arg(imageCenter[2]));
	

	//das brauchen wir nicht; 
	//TransformReslicerExperimental(imageCenter, angle, spacing, m_currentSliceMode);
	//this->rotatePolydata(m_cubeXTransform, m_cubeActor, imageCenter, angle,	1); 
	//transform an die reslicer weitergeben

	//handle transform to reslicer
	/*we need coordinates of the reslicer as input
	*perform transform onto it similar to above; visualize this by a slicer plane; 
	*set extend stuff + interpolation mode
	*/



	double const *orientationAfter = m_volumeRenderer->volume()->GetOrientation();
	//DEBUG_LOG(QString("Orientation after %1 %2 %3").arg(orientationAfter[0]).arg(orientationAfter[1]).arg(orientationAfter[2]));;

	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		if (i != m_currentSliceMode && m_slicerChannel[i])
			m_slicerChannel[i]->updateReslicer();

	// da muss die Rotation weitergegeben werden

	m_volumeRenderer->update();
	emit actorsUpdated();


	
}


void iAvtkInteractStyleActor::computeDisplayRotationAngle(double * sliceProbCenter, double * disp_obj_center, vtkRenderWindowInteractor * rwi, double &relativeAngle)
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
	relativeAngle = newAngle - oldAngle;	
}

void iAvtkInteractStyleActor::TranslateReslicer(vtkSmartPointer<vtkTransform> &transform, vtkImageReslice *reslice, double const *position, double *spacing, int sliceMode, double const * mageCenter)
{
	if (!reslice)
		return; 

	//vtkSmartPointer<vtkMatrix4x4> mat = m_slicerChannel[sliceMode]->reslicer()->GetResliceAxes();
	//if (mat) {
	//	DEBUG_LOG("Setting input matrix");
	//	transform->SetMatrix(mat);
	//}

	double value = 0; 


	double const *origin = m_image->GetOrigin(); 

	int slicerZAxisIdx = mapSliceToGlobalAxis(sliceMode, iAAxisIndex::X/*iAAxisIndex::Z*/);
	double ofs[3] = { 0.0, 0.0, 0.0 };
	const int sliceNumber = m_mdiChild->sliceNumber(sliceMode);
	ofs[slicerZAxisIdx] = sliceNumber * spacing[slicerZAxisIdx];
	
	reslice->SetOutputDimensionality(2);
	
	transform->Translate(position[0], position[1], position[2]);
	
	reslice->SetResliceTransform(transform);	
	reslice->SetInputData(m_image);
	reslice->SetOutputExtent(m_image->GetExtent());
	reslice->AutoCropOutputOff(); 

	
	//reslice->SetOutputOrigin(origin[0] + ofs[0], origin[1] + ofs[1], origin[2] + ofs[2]);
	//m_slicerChannel[sliceMode]->reslicer()->SetOutputOrigin(mageCenter);
	//reslice->SetOutputOrigin(mageCenter);
	/*reslice->SetResliceAxes(mat); */
	reslice->SetOutputSpacing(spacing);
	reslice->SetOutputOrigin(m_image->GetOrigin());
	//reslice->SetResliceAxesOrigin(origin[0] + ofs[0], origin[1] + ofs[1], origin[2] + ofs[2]);
	
	
		
	reslice->SetInterpolationModeToCubic();
	reslice->Update();
		

	

	

}




//void iAvtkInteractStyleActor::translateSlicerActor(const double *origin, const double *pos, double *posOut, const int sliceMode)
//{
//
//}

//void iAvtkInteractStyleActor::Update3DTransform(const double * imageCenter, const double * spacing, double relativeAngle)
//{
//	
//	//m_volumeRenderer->volume()->GetOrientation(volOrientation);
//	//double const *orientation = m_volumeRenderer->volume()->GetOrientation(); 
//
//
//	
//	//use angle from previous volume
//	//m_transform3D->Identity(); 
//	m_transform3D->SetMatrix(m_volumeRenderer->volume()->GetMatrix()); 
//	m_transform3D->PostMultiply();
//	m_transform3D->Translate(-imageCenter[0]/**spacing[0]*/, -imageCenter[1],/**spacing[1]*/ -imageCenter[2])/**spacing[2])*/;
//		
//	
//	switch (m_currentSliceMode) {
//	case YZ: 
//		//angle = relativeAngle + orientation[0];
//		m_transform3D->RotateX(relativeAngle); break;
//	case XZ: 
//		//angle = relativeAngle + orientation[1];
//		m_transform3D->RotateY(relativeAngle); break;
//	case XY: 
//		//angle = relativeAngle + orientation[0];
//		m_transform3D->RotateZ(relativeAngle); break;
//	default: break;
//
//	}
//
//	m_transform3D->Translate(imageCenter[0],/**spacing[0],*/ imageCenter[1]/**spacing[1]*/, imageCenter[2]/**spacing[2]*/);
//	
//	double volOrientation[3];
//
//	
//	
//
//	//m_image =dynamic_cast<vtkImageData * >( m_transformFilter->GetOutputDataObject(0));
//
//	//if (!m_image) { DEBUG_LOG("img is null"); return;  }
//	//m_volumeRenderer->update(); */
//	//*/	
//	
//	m_volumeRenderer->volume()->SetUserTransform(m_transform3D);
//	
//	m_volumeRenderer->update(); 
//}

//void iAvtkInteractStyleActor::TransformReslicerExperimental(double const * obj_center, double rotationAngle, double const *spacing, int sliceMode)
//{
//
//	if (!m_image) { DEBUG_LOG(QString("image is null, slicemode %1").arg(sliceMode)); return;  }
//	
//	m_SliceInteractorTransform[sliceMode]->PostMultiply();
//	m_SliceInteractorTransform[sliceMode]->Translate(-obj_center[0],-obj_center[1],/**spacing[1]**spacing[2]*/-obj_center[2]);
//	m_SliceInteractorTransform[sliceMode]->RotateZ(rotationAngle);
//	m_SliceInteractorTransform[sliceMode]->Translate(obj_center[0], obj_center[1],/*0*//**spacing[1]*//*,0*//**spacing[2]*/ obj_center[2]);
//	m_SliceInteractorTransform[sliceMode]->Inverse(); 
//	const int sliceNumber = m_mdiChild->sliceNumber(sliceMode);  /*m_slicerChannel[m_currentSliceMode]->reslicer()->GetNumber*/
//	//const int sliceMode = m_currentSliceMode; 
//	double ofs[3] = { 0.0, 0.0, 0.0 };
//	updateReslicerRotationTransformation2d(sliceMode, ofs, sliceNumber);
//	
//	m_volumeRenderer->update(); 
//
//}

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
