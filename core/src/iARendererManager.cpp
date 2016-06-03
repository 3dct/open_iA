/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iARendererManager.h"

#include "iARenderer.h"

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkObject.h>
#include <vtkOpenGLRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

#include <cassert>

iARendererManager::iARendererManager()
{
	m_isRedrawn = false;
	m_commonCamera = NULL;
}

void iARendererManager::addToBundle(iARenderer* renderer)
{
	if(!m_commonCamera)
		m_commonCamera = renderer->GetRenderer()->GetActiveCamera();
	else
		renderer->setCamera(m_commonCamera);
	
	m_renderers.append(renderer);
	renderer->GetRenderer()->AddObserver(vtkCommand::EndEvent, this, &iARendererManager::redrawOtherRenderers);
}

bool iARendererManager::removeFromBundle(iARenderer* renderer)
{
	if(!m_renderers.contains(renderer))
	{
		assert(false);
		// iARenderManager::removeFromBundle called with renderer which isn't part of the Bundle!
		return false;
	}
	vtkSmartPointer<vtkCamera> newCam = vtkSmartPointer<vtkCamera>::New();
	newCam->DeepCopy(renderer->GetRenderer()->GetActiveCamera());
	renderer->setCamera(newCam);
	m_renderers.remove(m_renderers.indexOf(renderer));
	return true;
}

void iARendererManager::removeAll()
{
	m_commonCamera = NULL;
	foreach( iARenderer * r, m_renderers)
	{
		removeFromBundle( r );
	}
}

void iARendererManager::redrawOtherRenderers(vtkObject* caller,
											 long unsigned int eventId,
											 void* callData)
{
	if(!m_isRedrawn)
	{
		m_isRedrawn = true;
		for(int i = 0; i < m_renderers.count(); i++)
		{
			if(m_renderers[i]->GetRenderer() != callData) {
				m_renderers[i]->GetRenderWindow()->Render();
			}
		}
		m_isRedrawn = false;
	}
}
