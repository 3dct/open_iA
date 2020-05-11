/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "mdichild.h"

#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkCommand.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkProp3D.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTransform.h>

vtkStandardNewMacro(iAvtkInteractStyleActor);

iAvtkInteractStyleActor::iAvtkInteractStyleActor() :
	m_mdiChild(nullptr), m_volumeRenderer(nullptr), m_is3D(false), m_rotationEnabled(false)
{
	m_transform3D = vtkSmartPointer<vtkTransform>::New();
	for (int i = 0; i < 3; ++i)
	{
		m_sliceInteractorTransform[i] = vtkSmartPointer<vtkTransform>::New();
		m_reslicerTransform[i] = vtkSmartPointer<vtkTransform>::New();
		m_currentVolRendererPosition[i] = 0.0;
		m_imageRefOrientation[i] = 0.0;
	}
	std::fill(m_slicerChannel, m_slicerChannel + iASlicerMode::SlicerCount, nullptr);
	InteractionPicker->SetTolerance(100.0);
}

void iAvtkInteractStyleActor::set3DOrientation(
	vtkSmartPointer<vtkTransform>& transform, double const* center, double angle, vtkProp3D* prop, uint mode)
{
	vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New();
	mat = prop->GetUserMatrix();
	if (mat)
	{
		transform->SetMatrix(mat);
	}
	transformationMode myMode = static_cast<transformationMode>(mode);
	rotateAroundAxis(transform, center, myMode, angle);
	prop->SetPosition(transform->GetPosition());
	prop->SetOrientation(transform->GetOrientation());
}

void iAvtkInteractStyleActor::rotateAroundAxis(
	vtkSmartPointer<vtkTransform>& transform, double const* center, transformationMode mode, double angle)
{
	if (!transform)
	{
		return;
	}

	transform->PostMultiply();
	transform->Translate(-center[0], -center[1], -center[2]);
	switch (mode)
	{
	case transformationMode::x:
		transform->RotateX(angle);
		break;
	case transformationMode::y:
		transform->RotateY(angle);
		break;
	case transformationMode::z:
		transform->RotateZ(angle);
		break;
	}

	transform->Translate(center[0], center[1], center[2]);
	transform->Update();
}

void iAvtkInteractStyleActor::setRefOrientation(double const* orientation)
{
	if (!orientation)
	{
		return;
	}
	for (int i = 0; i < 3; i++)
	{
		m_imageRefOrientation[i] = orientation[i];
	}
}

void iAvtkInteractStyleActor::Rotate()
{
	m_rotationEnabled = true;
}

void iAvtkInteractStyleActor::Spin()
{
	if (m_is3D)
	{
		vtkInteractorStyleTrackballActor::Spin();
	}
}

void iAvtkInteractStyleActor::OnMouseMove()
{
	vtkInteractorStyleTrackballActor::OnMouseMove();
	if (Interactor->GetShiftKey())
	{  // translation
		translate();
	}
	else if (m_rotationEnabled)
	{
		if (m_is3D)
		{
			rotate3D();
		}
		else
		{
			rotate2D();
		}
		m_rotationEnabled = false;
	}
	if (!m_is3D)
	{
		SetCurrentRenderer(nullptr);
	}
}

void iAvtkInteractStyleActor::initialize(vtkImageData* img, iAVolumeRenderer* volRend,
	iAChannelSlicerData* slicerChannel[3], int currentMode, MdiChild* mdiChild)
{
	if (!img)
	{
		DEBUG_LOG("no valid image!");
	}

	m_image = img;
	m_volumeRenderer = volRend;
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		m_slicerChannel[i] = slicerChannel[i];
	}
	m_currentSliceMode = currentMode;
	m_is3D = (m_currentSliceMode == iASlicerMode::SlicerCount);
	if (!mdiChild)
	{
		DEBUG_LOG("MdiChild not set!");
	}
	m_mdiChild = mdiChild;
	m_image->GetSpacing(m_imageSpacing);

	if (!m_is3D)
	{
		setPreviousSlicesActorPosition(m_slicerChannel[m_currentSliceMode]->actorPosition());
	}
}

void iAvtkInteractStyleActor::translate()
{
	double origin[3];
	m_image->GetOrigin(origin);

	if (m_is3D)
	{
		double const* posVolNew = m_volumeRenderer->position();
		if (posVolNew[0] == 0 && posVolNew[1] == 0 && posVolNew[2] == 0)
		{
			return;
		}

		//old - new
		double relMovement[3] = {0, 0, 0};

		relMovement[0] = m_currentVolRendererPosition[0] - posVolNew[0];
		relMovement[1] = m_currentVolRendererPosition[1] - posVolNew[1];
		relMovement[2] = m_currentVolRendererPosition[2] - posVolNew[2];

		if ((relMovement[0] == 0.0) && (relMovement[1] == 0.0) && (relMovement[2] == 0.0))
		{  // no movement
			return;
		}

		set2DTranslation(m_reslicerTransform[0], m_slicerChannel[0]->reslicer(), relMovement, m_image->GetSpacing());
		set2DTranslation(m_reslicerTransform[1], m_slicerChannel[1]->reslicer(), relMovement, m_image->GetSpacing());
		set2DTranslation(m_reslicerTransform[2], m_slicerChannel[2]->reslicer(), relMovement, m_image->GetSpacing());

		//store current position of renderer
		setPreviousVolActorPosition(posVolNew);
		//m_volumeRenderer->updateBoundingBox();
	}
	else  //update 2d Slicer
	{
		if (!m_slicerChannel[m_currentSliceMode])
		{
			return;
		}
		double sliceActorPos[3];
		auto tmpslicepos = m_slicerChannel[m_currentSliceMode]->actorPosition();
		std::copy(tmpslicepos, tmpslicepos + 3, sliceActorPos);
		double spacing[3];
		m_image->GetSpacing(spacing);

		double absMovementXYZ[3] = {0, 0, 0};
		prepareMovementCoords(absMovementXYZ, sliceActorPos, false);

		if ((absMovementXYZ[0] == 0.0) && (absMovementXYZ[1] == 0.0) && (absMovementXYZ[2] == 0.0))
		{
			return;
		}
		set3DTranslation(absMovementXYZ, sliceActorPos);
	}
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (i != m_currentSliceMode && m_slicerChannel[i])
		{
			m_slicerChannel[i]->updateReslicer();
		}
	}

	m_volumeRenderer->update();
	emit actorsUpdated();
}

void iAvtkInteractStyleActor::set3DTranslation(double const* movementXYZ, double const* sliceActorPos)
{
	double const* volRendPos = m_volumeRenderer->volume()->GetPosition();

	double slicerMoventXYZ[3];
	for (int i = 0; i < 3; i++)
	{
		slicerMoventXYZ[i] = -movementXYZ[i];
	}

	double OutPos[3] = {0, 0, 0};
	m_transform3D->Translate(movementXYZ);
	m_transform3D->Update();
	m_transform3D->TransformPoint(volRendPos, OutPos);
	m_volumeRenderer->volume()->SetPosition(m_transform3D->GetPosition());  //not via setting transform!
	//m_volumeRenderer->updateBoundingBox();
	double volRendPosafter[3] = {0, 0, 0};
	m_volumeRenderer->volume()->GetPosition(volRendPosafter);
	setPreviousSlicesActorPosition(sliceActorPos);  //setting to original
	setPreviousVolActorPosition(volRendPosafter);
	m_slicerChannel[m_currentSliceMode]->setActorPosition(0, 0, 0);

	for (int i = 0; i < 3; i++)
	{
		set2DTranslation(
			m_reslicerTransform[i], m_slicerChannel[i]->reslicer(), slicerMoventXYZ, m_image->GetSpacing());
	}
}

void iAvtkInteractStyleActor::prepareMovementCoords(double* movement, double const* sliceActorPos, bool relativeMovement)
{
	double oldPos[2] = {0, 0};
	if (relativeMovement)
	{
		oldPos[0] = m_currentSliceActorPosition[0];
		oldPos[1] = m_currentSliceActorPosition[1];
	}

	switch (m_currentSliceMode)
	{
	case iASlicerMode::XY:
		movement[0] = sliceActorPos[0] - oldPos[0];
		movement[1] = sliceActorPos[1] - oldPos[1];
		break;
	case iASlicerMode::XZ:
		movement[0] = sliceActorPos[0] - oldPos[0];
		movement[2] = sliceActorPos[1] - oldPos[1];
		break;
	case iASlicerMode::YZ:
		movement[1] = sliceActorPos[0] - oldPos[0];
		movement[2] = sliceActorPos[1] - oldPos[1];
		break;
	}
}

void iAvtkInteractStyleActor::rotate2D()
{
	if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
	{
		return;
	}

	vtkRenderWindowInteractor* rwi = this->Interactor;
	if (!m_image)
	{
		DEBUG_LOG(QString("Error on rotation %1").arg(m_currentSliceMode));
		return;
	}

	double const* imageBounds = m_image->GetBounds();

	double imageCenter[3];  //image center
	imageCenter[0] = (imageBounds[1] + imageBounds[0]) / 2.0;
	imageCenter[1] = (imageBounds[3] + imageBounds[2]) / 2.0;
	imageCenter[2] = (imageBounds[5] + imageBounds[4]) / 2.0;

	double* sliceProbCenter = this->InteractionProp->GetCenter();
	double disp_obj_center[3];
	double relativeAngle = 0.0;

	computeDisplayRotationAngle(sliceProbCenter, disp_obj_center, rwi, relativeAngle);

	transformationMode rotationDir;
	switch (m_currentSliceMode)
	{
	default:
		DEBUG_LOG("Invalid slicer mode");  // intentional fall-through - C++ 17: [[fallthrough]]
	case iASlicerMode::YZ:
		rotationDir = transformationMode::x;
		break;
	case iASlicerMode::XZ:
		rotationDir = transformationMode::y;
		relativeAngle *= -1.0;
		break;
	case iASlicerMode::XY:
		rotationDir = transformationMode::z;
		break;
	}

	set2DOrientation(m_reslicerTransform[m_currentSliceMode], m_slicerChannel[m_currentSliceMode]->reslicer(),
		rotationDir, imageCenter, -relativeAngle, m_image->GetSpacing());

	for (int i = 0; i < 3; ++i)
	{
		if (i == m_currentSliceMode)
		{
			continue;
		}
		set2DOrientation(m_reslicerTransform[i], m_slicerChannel[i]->reslicer(), rotationDir, imageCenter, relativeAngle,
			m_image->GetSpacing());
	}
	set3DOrientation(m_transform3D, m_volumeRenderer->volume()->GetCenter(), relativeAngle,
		m_volumeRenderer->volume(), m_currentSliceMode);
	setPreviousVolActorPosition(m_volumeRenderer->volume()->GetPosition());
	setPreviousSlicesActorPosition(m_slicerChannel[m_currentSliceMode]->actorPosition());

	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (i != m_currentSliceMode && m_slicerChannel[i])
		{
			m_slicerChannel[i]->updateReslicer();
		}
	}

	m_volumeRenderer->update();
	emit actorsUpdated();
}

void iAvtkInteractStyleActor::rotate3D()
{
	if (this->CurrentRenderer == nullptr || this->InteractionProp == nullptr)
	{
		return;
	}

	setRefOrientation(m_volumeRenderer->volume()->GetOrientation());
	vtkRenderWindowInteractor* rwi = this->Interactor;

	vtkCamera* cam = this->CurrentRenderer->GetActiveCamera();

	// First get the origin of the assembly
	double* obj_center = this->InteractionProp->GetCenter();

	// GetLength gets the length of the diagonal of the bounding box
	double boundRadius = this->InteractionProp->GetLength() * 0.5;
	//below copyied from superclass
	// Get the view up and view right vectors
	double view_up[3], view_look[3], view_right[3];

	cam->OrthogonalizeViewUp();
	cam->ComputeViewPlaneNormal();
	cam->GetViewUp(view_up);
	vtkMath::Normalize(view_up);
	cam->GetViewPlaneNormal(view_look);
	vtkMath::Cross(view_up, view_look, view_right);
	vtkMath::Normalize(view_right);

	// Get the furtherest point from object position+origin
	double outsidept[3];

	outsidept[0] = obj_center[0] + view_right[0] * boundRadius;
	outsidept[1] = obj_center[1] + view_right[1] * boundRadius;
	outsidept[2] = obj_center[2] + view_right[2] * boundRadius;
	// Convert them to display coord
	double disp_obj_center[3];

	this->ComputeWorldToDisplay(obj_center[0], obj_center[1], obj_center[2], disp_obj_center);

	this->ComputeWorldToDisplay(outsidept[0], outsidept[1], outsidept[2], outsidept);

	double radius = sqrt(vtkMath::Distance2BetweenPoints(disp_obj_center, outsidept));
	double nxf = (rwi->GetEventPosition()[0] - disp_obj_center[0]) / radius;

	double nyf = (rwi->GetEventPosition()[1] - disp_obj_center[1]) / radius;

	double oxf = (rwi->GetLastEventPosition()[0] - disp_obj_center[0]) / radius;

	double oyf = (rwi->GetLastEventPosition()[1] - disp_obj_center[1]) / radius;

	if (((nxf * nxf + nyf * nyf) <= 1.0) && ((oxf * oxf + oyf * oyf) <= 1.0))
	{
		double newXAngle = vtkMath::DegreesFromRadians(asin(nxf));
		double newYAngle = vtkMath::DegreesFromRadians(asin(nyf));
		double oldXAngle = vtkMath::DegreesFromRadians(asin(oxf));
		double oldYAngle = vtkMath::DegreesFromRadians(asin(oyf));

		double scale[3];
		scale[0] = scale[1] = scale[2] = 1.0;

		double** rotate = new double*[2];

		rotate[0] = new double[4];
		rotate[1] = new double[4];

		rotate[0][0] = newXAngle - oldXAngle;
		rotate[0][1] = view_up[0];
		rotate[0][2] = view_up[1];
		rotate[0][3] = view_up[2];

		rotate[1][0] = oldYAngle - newYAngle;
		rotate[1][1] = view_right[0];
		rotate[1][2] = view_right[1];
		rotate[1][3] = view_right[2];

		//rotate 3d volume
		this->Prop3DTransform(this->InteractionProp, obj_center, 2, rotate, scale);

		double const* orientationAfter = m_volumeRenderer->volume()->GetOrientation();

		//translate in XYZ
		double relRotation[3] = {0, 0, 0};
		for (int i = 0; i < 3; i++)
		{
			relRotation[i] = m_imageRefOrientation[i] - orientationAfter[i];
		}

		for (uint i = 0; i < 3; i++)
		{
			rotateReslicerXYZ(
				resliceTransform(i), reslicer(i), relRotation, 2u, m_image->GetCenter(), m_imageSpacing);
		}

		double const* posVol = m_volumeRenderer->volume()->GetPosition();
		setPreviousVolActorPosition(posVol);

		delete[] rotate[0];
		delete[] rotate[1];
		delete[] rotate;

		rwi->Render();

		for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		{
			if (i != m_currentSliceMode && m_slicerChannel[i])
			{
				m_slicerChannel[i]->updateReslicer();
			}
		}

		m_volumeRenderer->update();
		emit actorsUpdated();
	}
}

void iAvtkInteractStyleActor::setPreviousSlicesActorPosition(double const* pos)
{
	m_currentSliceActorPosition[0] = pos[0];
	m_currentSliceActorPosition[1] = pos[1];
	m_currentSliceActorPosition[2] = pos[2];
}

void iAvtkInteractStyleActor::setPreviousVolActorPosition(double const* pos)
{
	m_currentVolRendererPosition[0] = pos[0];
	m_currentVolRendererPosition[1] = pos[1];
	m_currentVolRendererPosition[2] = pos[2];
}

void iAvtkInteractStyleActor::computeDisplayRotationAngle(
	double* sliceProbCenter, double* disp_obj_center, vtkRenderWindowInteractor* rwi, double& relativeAngle)
{
	this->ComputeWorldToDisplay(sliceProbCenter[0], sliceProbCenter[1], sliceProbCenter[2], disp_obj_center);

	double newAngle = vtkMath::DegreesFromRadians(
		atan2(rwi->GetEventPosition()[1] - disp_obj_center[1], rwi->GetEventPosition()[0] - disp_obj_center[0]));

	double oldAngle = vtkMath::DegreesFromRadians(atan2(
		rwi->GetLastEventPosition()[1] - disp_obj_center[1], rwi->GetLastEventPosition()[0] - disp_obj_center[0]));

	relativeAngle = newAngle - oldAngle;
}

void iAvtkInteractStyleActor::set2DTranslation(
	vtkSmartPointer<vtkTransform>& transform, vtkImageReslice* reslice, double const* position, double* spacing)
{
	if ((!reslice) && (!transform))
	{
		return;
	}
	reslice->SetOutputDimensionality(2);
	transform->Translate(position[0], position[1], position[2]);
	reslice->SetResliceTransform(transform);
	reslice->SetInputData(m_image);
	reslice->SetOutputExtent(m_image->GetExtent());
	reslice->AutoCropOutputOff();
	reslice->SetOutputSpacing(spacing);
	reslice->SetOutputOrigin(m_image->GetOrigin());
	reslice->SetInterpolationModeToCubic();
	reslice->Update();
}

void iAvtkInteractStyleActor::set2DOrientation(vtkSmartPointer<vtkTransform>& transform, vtkImageReslice* reslicer,
	transformationMode mode, double const* center, double angle, double const* spacing)
{
	if (!reslicer || !transform || angle == 0.0)
	{
		return;
	}
	rotateAroundAxis(transform, center, mode, angle);
	prepareReslicer(reslicer, transform, spacing);
}

void iAvtkInteractStyleActor::prepareReslicer(
	vtkImageReslice* reslicer, vtkSmartPointer<vtkTransform> transform, double const* spacing)
{
	reslicer->SetResliceTransform(transform);
	reslicer->SetInputData(m_image);
	reslicer->SetOutputExtent(m_image->GetExtent());
	reslicer->AutoCropOutputOff();
	reslicer->SetOutputSpacing(spacing);
	reslicer->SetOutputOrigin(m_image->GetOrigin());
	reslicer->SetInterpolationModeToCubic();
	reslicer->Update();
}

void iAvtkInteractStyleActor::rotateReslicerXYZ(vtkSmartPointer<vtkTransform> transform, vtkImageReslice* reslicer,
	double const* rotXYZ, uint rotationMode, double const* center, double const* spacing)
{
	if (!transform)
	{
		return;
	}

	transform->PostMultiply();
	transform->Translate(-center[0], -center[1], -center[2]);
	switch (rotationMode)
	{
	case 0:
		transform->RotateX(rotXYZ[0]);
		break;
	case 1:
		transform->RotateX(rotXYZ[0]);
		transform->RotateY(rotXYZ[1]);
		break;
	case 2:
		transform->RotateX(rotXYZ[0]);
		transform->RotateY(rotXYZ[1]);
		transform->RotateZ(rotXYZ[2]);
	default:
		break;
	}

	transform->Translate(center[0], center[1], center[2]);
	prepareReslicer(reslicer, transform, spacing);
}

vtkImageReslice* iAvtkInteractStyleActor::reslicer(uint mode)
{
	if (mode >= 3)
	{
		return nullptr;
	}
	return m_slicerChannel[mode]->reslicer();
}

vtkSmartPointer<vtkTransform> iAvtkInteractStyleActor::resliceTransform(uint mode)
{
	if (mode >= 3)
	{
		vtkSmartPointer<vtkTransform> test = vtkSmartPointer<vtkTransform>();
		return test;
	}
	return m_reslicerTransform[mode];
}