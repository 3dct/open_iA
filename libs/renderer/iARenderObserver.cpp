// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARenderObserver.h"

#include <vtkCamera.h>
#include <vtkCellLocator.h>
#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkTransform.h>
#include <vtkWorldPointPicker.h>

#include <iAToolsVTK.h>    // for setCamPosition

std::vector<double> slicerNormal(int mode, size_t size)
{
	std::vector<double> normal(size, 0.0);
	normal[mode] = 1.0;
	return normal;
}

iARenderObserver::iARenderObserver(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren,
	vtkTransform* pTrans, std::array<vtkPlane*, 3> planes):
	m_pRen(pRen),
	m_pIren(pIren),
	m_pTrans(pTrans),
	m_pLine(vtkSmartPointer<vtkLineSource>::New()),
	m_pProbe(vtkSmartPointer<vtkProbeFilter>::New()),
	m_pWorldPicker(vtkSmartPointer<vtkWorldPointPicker>::New()),
	m_planes(planes),
	mode(0),
	speed(1.0),
	scale(1.0)
{
}

void iARenderObserver::ReInitialize(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren,
	vtkTransform* pTrans, std::array<vtkPlane*, 3> planes)
{
	m_pRen = pRen;
	m_pIren = pIren;
	m_pTrans = pTrans;
	m_planes = planes;
}

//iARenderObserver* iARenderObserver::New(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren,
//	vtkTransform* pTrans, vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3)
//{
//	return new iARenderObserver(pRen, pIren, pTrans, plane1, plane2, plane3);
//}

void iARenderObserver::Execute(vtkObject * caller, unsigned long eid, void *  callData)
{
	switch (eid)
	{
		/*
		case vtkCommand::LeftButtonPressEvent:
		{
			if (!m_pImageData)
				return;
			PickWithWorldPicker();
			double* pickPos = m_pWorldPicker->GetPickPosition();
			double* spacing = m_pImageData->GetSpacing();
			int x = static_cast<int>(pickPos[0] / spacing[0]),
				y = static_cast<int>(pickPos[1] / spacing[1]),
				z = static_cast<int>(pickPos[2] / spacing[2]);
			emit clicked(x, y, z);
			break;
		}
		*/
		case vtkCommand::KeyPressEvent:
		{
			char keyCode = m_pIren->GetKeyCode();
			emit keyPressed(keyCode);
			if (keyCode == '\t')
			{
				mode = (mode + 1) % 2;
			}
			if (mode == 0)
			{
				switch (keyCode)
				{
					case 'x':
						m_planes[0]->SetNormal(-m_planes[0]->GetNormal()[0], -m_planes[0]->GetNormal()[1], -m_planes[0]->GetNormal()[2]);
						break;
					case 'y':
						m_planes[1]->SetNormal(-m_planes[1]->GetNormal()[0], -m_planes[1]->GetNormal()[1], -m_planes[1]->GetNormal()[2]);
						break;
					case 'z':
						m_planes[2]->SetNormal(-m_planes[2]->GetNormal()[0], -m_planes[2]->GetNormal()[1], -m_planes[2]->GetNormal()[2]);
						break;
					case 'r':
					{
						for (int m = 0; m < 3; ++m)
						{
							m_planes[m]->SetOrigin(0, 0, 0);
							m_planes[0]->SetNormal(slicerNormal(m).data());
						}
						setCamPosition(m_pRen->GetActiveCamera(), iACameraPosition::Iso);
						m_pRen->ResetCamera();
						break;
					}
				}
				m_pIren->Render();
			}
			// ability to move second coordinate axis; required for registration
			else if (mode == 1)
			{
				switch (keyCode)
				{
					case 's':
					{
						speed = (speed == 10.0) ? 1.0 : 10.0;
						break;
					}
					// TODO NEWIO: verify how this should work, and whether we still need it (or if we need it here)
					/*
					case '0':
					{
						double origin[3];
						PickVolume(origin);

						vtkNew<vtkMatrix4x4> matrix;
						matrix->DeepCopy(m_pTrans->GetMatrix());
						matrix->SetElement(0, 3, origin[0]);
						matrix->SetElement(1, 3, origin[1]);
						matrix->SetElement(2, 3, origin[2]);
						m_pTrans->SetMatrix(matrix.GetPointer());
						break;
					}
					case '1':
					{
						double pickedAxis[3];
						PickVolume(pickedAxis);
						SetAxis(X_AXIS, pickedAxis);
						break;
					}
					case '2':
					{
						double pickedAxis[3];
						PickVolume(pickedAxis);
						SetAxis(Y_AXIS, pickedAxis);
						break;
					}
					case '3':
					{
						double pickedAxis[3];
						PickVolume(pickedAxis);
						SetAxis(Z_AXIS, pickedAxis);
						break;
					}
					*/
					case '4':
						scale *= 0.8;
						m_pTrans->Scale(0.8, 0.8, 0.8);
						break;
					case '5':
						scale *= 1.2;
						m_pTrans->Scale(1.2, 1.2, 1.2);
						break;
					}
					bool rotate = m_pIren->GetControlKey();
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
}

void iARenderObserver::PickVolume(double point[3])
{   // TODO NEWIO: verify whether we need functionality - for registration maybe?
	/*
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
	*/
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

void iARenderObserver::PickWithWorldPicker()
{
	m_pRen->Render();
	m_pWorldPicker->Pick(
		m_pIren->GetEventPosition()[0],
		m_pIren->GetEventPosition()[1],
		0,  // always zero.
		m_pRen);
}

void iARenderObserver::AddListener(vtkCommand* listener)
{
	m_listener.push_back(listener);
}

int iARenderObserver::GetMode()
{
	return mode;
}

vtkRenderWindowInteractor* iARenderObserver::GetInteractor()
{
	return m_pIren;
}

vtkWorldPointPicker* iARenderObserver::GetWorldPicker()
{
	return m_pWorldPicker;
}
