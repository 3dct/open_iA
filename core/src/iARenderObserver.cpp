/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iARenderObserver.h"

#include <vtkImageData.h>
#include <vtkActorCollection.h>
#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>

#include <QTextStream>

iARenderObserver::iARenderObserver(vtkRenderer* pRen,
	vtkRenderer* pLabelRen,
	vtkRenderWindowInteractor* pIren,
	vtkPicker* pPicker,
	vtkTransform* pTrans,
	vtkImageData* pImageData,
	vtkPlane* plane1,
	vtkPlane* plane2,
	vtkPlane* plane3,
	vtkCellLocator * cellLocator )
{
	m_pRen = pRen;
	m_pLabelRen = pLabelRen;
	m_pIren = pIren;
	m_pPicker = pPicker;
	m_pTrans = pTrans;
	m_pImageData = pImageData;
	m_pPlane1 = plane1;
	m_pPlane2 = plane2;
	m_pPlane3 = plane3;
	m_pcellLocator = cellLocator;

	m_pLine = vtkLineSource::New();
	m_pProbe = vtkProbeFilter::New();

	rotate = false;
	mode = 0; pos[0] = 0; pos[1] = 0; pos[2] = 0;
	speed = 1.0;
	scale = 1.0;

	m_pWorldPicker = vtkWorldPointPicker::New();
}

void iARenderObserver::ReInitialize( vtkRenderer* pRen, vtkRenderer* pLabelRen, vtkRenderWindowInteractor* pIren, vtkPicker* pPicker, vtkTransform* pTrans, vtkImageData* pImageData, vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3, vtkCellLocator *cellLocator )
{
	m_pRen = pRen;
	m_pLabelRen = pLabelRen;
	m_pIren = pIren;
	m_pPicker = pPicker;
	m_pTrans = pTrans;
	m_pImageData = pImageData;
	m_pPlane1 = plane1;
	m_pPlane2 = plane2;
	m_pPlane3 = plane3;
	m_pcellLocator = cellLocator;
}


iARenderObserver::~iARenderObserver()
{
	m_pLine->Delete();
	m_pProbe->Delete();
	m_pWorldPicker->Delete();
}

void iARenderObserver::Execute(vtkObject * caller,
	unsigned long eid,
	void *  callData)
{
	char keyCode = m_pIren->GetKeyCode();
	char* keySym = m_pIren->GetKeySym();
	// TODO: check original intention of this code
	//       VTK seems to have done a major change to how keycode works...
	switch (eid)
	{
		case vtkCommand::LeftButtonPressEvent:
		{
			PickWithWorldPicker();
			double* pos = m_pWorldPicker->GetPickPosition();
			double* spacing = m_pImageData->GetSpacing();
			int x = static_cast<int>(pos[0] / spacing[0]),
				y = static_cast<int>(pos[1] / spacing[1]),
				z = static_cast<int>(pos[2] / spacing[2]);
			emit Clicked(x, y, z);
			break;
		}
		case vtkCommand::KeyPressEvent:
		{
			if (keyCode == 'a' || keyCode == 'c')
			{
				emit InteractorModeSwitched(keyCode);
			}
			if (keyCode == '\t') {
				mode++; if (mode > 1) mode = 0;
			}

			if (mode == 0)
			{
				int dims[3];
				double spacing[3];
				m_pImageData->GetDimensions(dims);
				m_pImageData->GetSpacing(spacing);

				if (!keySym || strlen(keySym) == 0)
					return;
				keyCode = keySym[0];
				switch (keyCode)
				{
				case 'x':
				{
					m_pPlane1->SetNormal(-m_pPlane1->GetNormal()[0], -m_pPlane1->GetNormal()[1], -m_pPlane1->GetNormal()[2]);
				}
				break;
				case 'y':
				{
					m_pPlane2->SetNormal(-m_pPlane2->GetNormal()[0], -m_pPlane2->GetNormal()[1], -m_pPlane2->GetNormal()[2]);
				}
				break;
				case 'z':
				{
					m_pPlane3->SetNormal(-m_pPlane3->GetNormal()[0], -m_pPlane3->GetNormal()[1], -m_pPlane3->GetNormal()[2]);
				}
				break;
				case 'r':
				{
					pos[0] = 0; pos[1] = 0; pos[2] = 0; CheckPos(2);

					m_pPlane1->SetOrigin(0, 0, 0);
					m_pPlane2->SetOrigin(0, 0, 0);
					m_pPlane3->SetOrigin(0, 0, 0);
					m_pPlane1->SetNormal(1, 0, 0);
					m_pPlane2->SetNormal(0, 1, 0);
					m_pPlane3->SetNormal(0, 0, 1);

					// TODO: avoid duplication with iARenderer setCamPosition!
					vtkCamera* cam = m_pRen->GetActiveCamera();
					cam->SetViewUp(0, 0, 1);
					cam->SetPosition(1, 1, 1);
					cam->SetFocalPoint(0, 0, 0);
					m_pRen->ResetCamera();
				}
				break;
				}
			}
			else if (mode == 1)
			{
				if (m_pIren->GetControlKey())
					rotate = !rotate;

				// TODO: check: double-use of code '\t' (also used for mode switching, see above)
				if (keyCode == '\t')
				{
					if (speed == 10.0) speed = 1.0;
					else speed = 10.0;
				}

				if (!keySym || strlen(keySym) == 0)
					return;
				keyCode = keySym[0];
				switch (keyCode)
				{
				case '0':
				{
					double origin[3];
					PickVolume(origin);

					vtkSmartPointer<vtkMatrix4x4> matrix = vtkSmartPointer<vtkMatrix4x4>::New();
					matrix->DeepCopy(m_pTrans->GetMatrix());
					matrix->SetElement(0, 3, origin[0]);
					matrix->SetElement(1, 3, origin[1]);
					matrix->SetElement(2, 3, origin[2]);
					m_pTrans->SetMatrix(matrix);
				}
				break;
				case '1':
				{
					double pickedAxis[3];
					PickVolume(pickedAxis);

					SetAxis(X_AXIS, pickedAxis);
				}
				break;
				case '2':
				{
					double pickedAxis[3];
					PickVolume(pickedAxis);

					SetAxis(Y_AXIS, pickedAxis);
				}
				break;
				case '3':
				{
					double pickedAxis[3];
					PickVolume(pickedAxis);

					SetAxis(Z_AXIS, pickedAxis);
				}
				break;
				case '4':
					scale *= 0.8;
					m_pTrans->Scale(0.8, 0.8, 0.8);
					break;
				case '5':
					scale *= 1.2;
					m_pTrans->Scale(1.2, 1.2, 1.2);
					break;
				}

				if (rotate)
				{
					switch (keyCode)
					{
					case 'x': m_pTrans->RotateWXYZ(speed, 1.0, 0.0, 0.0); break;
					case 'X': m_pTrans->RotateWXYZ(-speed, 1.0, 0.0, 0.0); break;
					case 'y': m_pTrans->RotateWXYZ(speed, 0.0, 1.0, 0.0); break;
					case 'Y': m_pTrans->RotateWXYZ(-speed, 0.0, 1.0, 0.0); break;
					case 'z': m_pTrans->RotateWXYZ(speed, 0.0, 0.0, 1.0); break;
					case 'Z': m_pTrans->RotateWXYZ(-speed, 0.0, 0.0, 1.0); break;
					}
				}
				else
				{
					switch (keyCode)
					{
					case 'x': m_pTrans->Translate(speed, 0.0, 0.0); break;
					case 'X': m_pTrans->Translate(-speed, 0.0, 0.0); break;
					case 'y': m_pTrans->Translate(0.0, speed, 0.0); break;
					case 'Y': m_pTrans->Translate(0.0, -speed, 0.0); break;
					case 'z': m_pTrans->Translate(0.0, 0.0, speed); break;
					case 'Z': m_pTrans->Translate(0.0, 0.0, -speed); break;
					}
				}
			}
			break;
		}
	}
	for (vtkCommand * listener : m_listener)
	{
		listener->Execute(caller, eid, callData);
	}
	m_pRen->Render();
	m_pIren->Render();
}


void iARenderObserver::PickVolume(double point[3])
{
	int i;
	double p1World[4], p2World[4], pickPosition[4];
	double selectionX, selectionY, selectionZ;
	double cameraPos[4], cameraFP[4];
	double *displayCoords, *worldCoords;
	double *clipRange;
	double ray[3], rayLength;
	double tF, tB;
	double cameraDOP[3];

	vtkCamera *camera = m_pRen->GetActiveCamera();

	// Do the volume pick.

	// Get camera focal point and position. Convert to display (screen)
	// coordinates. We need a depth value for z-buffer.
	camera->GetPosition(cameraPos);
	cameraPos[3] = 1.0;
	camera->GetFocalPoint(cameraFP);
	cameraFP[3] = 1.0;

	m_pRen->SetWorldPoint(cameraFP[0],cameraFP[1],cameraFP[2],cameraFP[3]);
	m_pRen->WorldToDisplay();
	displayCoords = m_pRen->GetDisplayPoint();
	selectionZ = displayCoords[2];

	// Convert the selection point into world coordinates.
	//
	selectionX = m_pIren->GetEventPosition()[0];
	selectionY = m_pIren->GetEventPosition()[1];
	m_pRen->SetDisplayPoint(selectionX, selectionY, selectionZ);
	m_pRen->DisplayToWorld();
	worldCoords = m_pRen->GetWorldPoint();
	if ( worldCoords[3] == 0.0 )
		return;

	for (i=0; i < 3; i++)
		pickPosition[i] = worldCoords[i] / worldCoords[3];


	//  Compute the ray endpoints.  The ray is along the line running from
	//  the camera position to the selection point, starting where this line
	//  intersects the front clipping plane, and terminating where this
	//  line intersects the back clipping plane.
	for (i=0; i<3; i++)
		ray[i] = pickPosition[i] - cameraPos[i];

	for (i=0; i<3; i++)
		cameraDOP[i] = cameraFP[i] - cameraPos[i];

	vtkMath::Normalize(cameraDOP);

	if (( rayLength = vtkMath::Dot(cameraDOP,ray)) == 0.0 )
		return;

	clipRange = camera->GetClippingRange();

	if ( camera->GetParallelProjection() )
	{
		tF = clipRange[0] - rayLength;
		tB = clipRange[1] - rayLength;
		for (i=0; i<3; i++)
		{
			p1World[i] = pickPosition[i] + tF*cameraDOP[i];
			p2World[i] = pickPosition[i] + tB*cameraDOP[i];
		}
	}
	else
	{
		tF = clipRange[0] / rayLength;
		tB = clipRange[1] / rayLength;
		for (i=0; i<3; i++)
		{
			p1World[i] = cameraPos[i] + tF*ray[i];
			p2World[i] = cameraPos[i] + tB*ray[i];
		}
	}
	p1World[3] = p2World[3] = 1.0;

	m_pLine->SetPoint1(p1World);
	m_pLine->SetPoint2(p2World);
	m_pLine->SetResolution(50000);
	m_pLine->Update();

	m_pProbe->SetSourceData(m_pImageData);
	m_pProbe->SetInputConnection(m_pLine->GetOutputPort());
	m_pProbe->Update();

	double max = 0, derivative = 0;
	int maxindex = -1;

	for (i = 1; i < m_pProbe->GetOutput()->GetNumberOfPoints(); i++)
	{
		derivative = m_pProbe->GetOutput()->GetPointData()->GetScalars()->GetComponent(i, 0) - m_pProbe->GetOutput()->GetPointData()->GetScalars()->GetComponent(i-1, 0);
		if (derivative > max)
		{
			if (maxindex != -1)
				max = derivative;
			if (derivative < -1000)
				break;
			maxindex = i;
		}
	}

	if (maxindex != -1)
	{
		point[0] = m_pProbe->GetOutput()->GetPoint(maxindex)[0];
		point[1] = m_pProbe->GetOutput()->GetPoint(maxindex)[1];
		point[2] = m_pProbe->GetOutput()->GetPoint(maxindex)[2];


	}
}


void iARenderObserver::SetAxis(Axis axis, double pickedAxis[3])
{
	double trans[3] = {
		m_pTrans->GetMatrix()->GetElement(0, 3),
		m_pTrans->GetMatrix()->GetElement(1, 3),
		m_pTrans->GetMatrix()->GetElement(2, 3)
	};

	pickedAxis[0] -= trans[0];
	pickedAxis[1] -= trans[1];
	pickedAxis[2] -= trans[2];

	double secondAxis[3] = {
		m_pTrans->GetMatrix()->GetElement(0, (axis+1)%3),
		m_pTrans->GetMatrix()->GetElement(1, (axis+1)%3),
		m_pTrans->GetMatrix()->GetElement(2, (axis+1)%3)
	};

	double thirdAxis[3];
	vtkMath::Cross(secondAxis, pickedAxis, thirdAxis);
	vtkMath::Cross(pickedAxis, thirdAxis, secondAxis);

	vtkMath::Normalize(pickedAxis);
	vtkMath::Normalize(secondAxis);
	vtkMath::Normalize(thirdAxis);

	pickedAxis[0] *= scale; pickedAxis[1] *= scale; pickedAxis[2] *= scale;
	secondAxis[0] *= scale; secondAxis[1] *= scale; secondAxis[2] *= scale;
	thirdAxis[0] *= scale; thirdAxis[1] *= scale; thirdAxis[2] *= scale;

	m_pTrans->GetMatrix()->SetElement(0, axis-1, pickedAxis[0]);
	m_pTrans->GetMatrix()->SetElement(1, axis-1, pickedAxis[1]);
	m_pTrans->GetMatrix()->SetElement(2, axis-1, pickedAxis[2]);

	m_pTrans->GetMatrix()->SetElement(0, (axis+1)%3, secondAxis[0]);
	m_pTrans->GetMatrix()->SetElement(1, (axis+1)%3, secondAxis[1]);
	m_pTrans->GetMatrix()->SetElement(2, (axis+1)%3, secondAxis[2]);

	m_pTrans->GetMatrix()->SetElement(0, axis%3, thirdAxis[0]);
	m_pTrans->GetMatrix()->SetElement(1, axis%3, thirdAxis[1]);
	m_pTrans->GetMatrix()->SetElement(2, axis%3, thirdAxis[2]);
}


void iARenderObserver::CheckPos(int dim)
{
	int dims[3];
	m_pImageData->GetDimensions(dims);

	if (dim == 0){
		if ( pos[0] < 0 ) pos[0] = dims[0]-1;
		if ( pos[0] > dims[0]-1 ) pos[0] = 0;
	} else if (dim == 1) {
		if ( pos[1] < 0 ) pos[1] = dims[1]-1;
		if ( pos[1] > dims[1]-1 ) pos[1] = 0;
	} else {
		if ( pos[2] < 0 ) pos[2] = dims[2]-1;
		if ( pos[2] > dims[2]-1 ) pos[2] = 0;
	}
}


void iARenderObserver::PickWithWorldPicker()
{
	m_pRen->Render();
	m_pWorldPicker->Pick(
		m_pIren->GetEventPosition()[0],
		m_pIren->GetEventPosition()[1],
		0,  // always zero.
		m_pRen);
}


iARenderObserver * iARenderObserver::New( vtkRenderer* pRen,
	vtkRenderer* pLabelRen,
	vtkRenderWindowInteractor* pIren,
	vtkPicker* pPicker,
	vtkTransform* pTrans,
	vtkImageData* pImageData,
	vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3,
	vtkCellLocator *cellLocator )
{
	return new iARenderObserver(pRen,
		pLabelRen,
		pIren,
		pPicker,
		pTrans,
		pImageData,
		plane1,	plane2,	plane3,
		cellLocator);
}


void iARenderObserver::AddListener(vtkCommand* listener)
{
	m_listener.push_back(listener);
}


int iARenderObserver::GetMode()
{
	return mode;
}

vtkCellLocator * iARenderObserver::GetCellLocator()
{
	return m_pcellLocator;
}
vtkRenderWindowInteractor* iARenderObserver::GetInteractor()
{
	return m_pIren;
}
vtkImageData* iARenderObserver::GetImageData()
{
	return m_pImageData;
}
vtkRenderer* iARenderObserver::GetLabelRenderer()
{
	return m_pLabelRen;
}
vtkPicker* iARenderObserver::GetPicker()
{
	return m_pPicker;
}
vtkWorldPointPicker* iARenderObserver::GetWorldPicker()
{
	return m_pWorldPicker;
}
