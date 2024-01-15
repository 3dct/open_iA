// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARenderObserver.h"

#include <vtkMath.h>
#include <vtkPlane.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkWorldPointPicker.h>

#include <iAToolsVTK.h>    // for setCamPosition

std::vector<double> slicerNormal(int mode)
{
	std::vector<double> normal(3, 0.0);
	normal[mode] = 1.0;
	return normal;
}

iARenderObserver::iARenderObserver(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren, std::array<vtkPlane*, 3> planes):
	m_pRen(pRen),
	m_pIren(pIren),
	m_pWorldPicker(vtkSmartPointer<vtkWorldPointPicker>::New()),
	m_planes(planes)
{}

void iARenderObserver::Execute(vtkObject * caller, unsigned long eid, void *  callData)
{
	if (eid == vtkCommand::KeyPressEvent)
	{
		char keyCode = m_pIren->GetKeyCode();
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
	for (vtkCommand * listener : m_listener)
	{
		listener->Execute(caller, eid, callData);
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

void iARenderObserver::AddListener(vtkCommand* listener)
{
	m_listener.push_back(listener);
}

vtkRenderWindowInteractor* iARenderObserver::GetInteractor()
{
	return m_pIren;
}

vtkWorldPointPicker* iARenderObserver::GetWorldPicker()
{
	return m_pWorldPicker;
}
