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
#include "iARendererManager.h"

#include "iAConsole.h"

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkObject.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

#include <cassert>

iARendererManager::iARendererManager()
{
	m_isRedrawn = false;
	m_commonCamera = nullptr;
}

void iARendererManager::addToBundle(vtkRenderer* renderer)
{
	if (!m_commonCamera)
	{
		m_commonCamera = renderer->GetActiveCamera();
	}
	else
	{
		renderer->SetActiveCamera(m_commonCamera);
	}
	m_renderers.append(renderer);
	renderer->AddObserver(vtkCommand::EndEvent, this, &iARendererManager::redrawOtherRenderers);
}

bool iARendererManager::removeFromBundle(vtkRenderer* renderer)
{
	if(!m_renderers.contains(renderer))
	{
		assert(false);
		DEBUG_LOG("iARenderManager::removeFromBundle called with renderer which isn't part of the Bundle!");
		return false;
	}
	vtkSmartPointer<vtkCamera> newCam = vtkSmartPointer<vtkCamera>::New();
	newCam->DeepCopy(renderer->GetActiveCamera());
	renderer->SetActiveCamera(newCam);
	m_renderers.remove(m_renderers.indexOf(renderer));
	return true;
}

void iARendererManager::removeAll()
{
	m_commonCamera = nullptr;
	for(vtkRenderer * r: m_renderers)
	{
		removeFromBundle( r );
	}
}

void iARendererManager::redrawOtherRenderers(vtkObject* /*caller*/, long unsigned int /*eventId*/, void* callData)
{
	if (!m_isRedrawn)
	{
		m_isRedrawn = true;
		for(int i = 0; i < m_renderers.count(); i++)
		{
			if(m_renderers[i] != callData)
			{
				m_renderers[i]->GetRenderWindow()->Render();
			}
		}
		m_isRedrawn = false;
	}
}
