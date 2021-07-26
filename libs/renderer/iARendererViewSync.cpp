/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "iARendererViewSync.h"

#include "iALog.h"

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkObject.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

#include <cassert>

namespace
{
	void copyCameraParams(vtkCamera* dstCam, vtkCamera* srcCam)
	{
		dstCam->SetViewUp(srcCam->GetViewUp());
		dstCam->SetPosition(srcCam->GetPosition());
		dstCam->SetFocalPoint(srcCam->GetFocalPoint());
		dstCam->SetClippingRange(srcCam->GetClippingRange());
		if (srcCam->GetParallelProjection())
		{
			dstCam->SetParallelScale(srcCam->GetParallelScale());
		}
	}
}

iARendererViewSync::iARendererViewSync(bool sharedCamera) :
	m_updateInProgress(false),
	m_commonCamera(nullptr),
	m_sharedCamera(sharedCamera)
{
}

iARendererViewSync::~iARendererViewSync()
{
	removeAll();
}

void iARendererViewSync::addToBundle(vtkRenderer* renderer)
{
	if (m_sharedCamera)
	{
		if (!m_commonCamera)
		{
			m_commonCamera = renderer->GetActiveCamera();
		}
		else
		{
			renderer->SetActiveCamera(m_commonCamera);
		}
	}
	else
	{
		if (m_rendererObserverTags.size() > 0)
		{
			auto sourceCam = m_rendererObserverTags.keys()[0]->GetActiveCamera();
			copyCameraParams(renderer->GetActiveCamera(), sourceCam);
			renderer->GetActiveCamera()->SetParallelProjection(sourceCam->GetParallelProjection());
		}
	}
	auto observeTag = renderer->AddObserver(vtkCommand::EndEvent, this, &iARendererViewSync::redrawOtherRenderers);
	m_rendererObserverTags.insert(renderer, observeTag);
}

bool iARendererViewSync::removeFromBundle(vtkRenderer* renderer)
{
	if(!m_rendererObserverTags.contains(renderer))
	{
		assert(false);
		LOG(lvlWarn, "iARenderManager::removeFromBundle called with renderer which isn't part of the Bundle!");
		return false;
	}
	vtkSmartPointer<vtkCamera> newCam = vtkSmartPointer<vtkCamera>::New();
	newCam->DeepCopy(renderer->GetActiveCamera());
	renderer->SetActiveCamera(newCam);
	renderer->RemoveObserver(m_rendererObserverTags[renderer]);
	m_rendererObserverTags.remove(renderer);
	return true;
}

void iARendererViewSync::removeAll()
{
	m_commonCamera = nullptr;
	for (auto  r: m_rendererObserverTags.keys())
	{
		removeFromBundle( r );
	}
}

void iARendererViewSync::redrawOtherRenderers(vtkObject* caller, long unsigned int /*eventId*/, void* /*callData*/)
{
	if (m_updateInProgress || !caller)
	{
		return;
	}
	m_updateInProgress = true;	// thread-safety?
	auto sourceCam = ((vtkRenderer*)caller)->GetActiveCamera();
	for (auto r: m_rendererObserverTags.keys())
	{
		if (r == caller)
		{
			continue;
		}
		if (!m_sharedCamera)
		{
			copyCameraParams(r->GetActiveCamera(), sourceCam);
		}
		if (r->GetRenderWindow())
		{	// don't update renderers already removed from render window:
			r->GetRenderWindow()->Render();
		}
	}
	m_updateInProgress = false;
}
