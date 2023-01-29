// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARendererViewSync.h"

#include "iALog.h"

#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkObject.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>

#include <cassert>
#include <set>

namespace
{
	void copyCameraParams(vtkCamera* dstCam, vtkCamera* srcCam)
	{
		dstCam->SetViewUp(srcCam->GetViewUp());
		dstCam->SetPosition(srcCam->GetPosition());
		dstCam->SetFocalPoint(srcCam->GetFocalPoint());
		dstCam->SetClippingRange(srcCam->GetClippingRange());
		if (srcCam->GetParallelProjection() != dstCam->GetParallelProjection())
		{
			dstCam->SetParallelProjection(srcCam->GetParallelProjection());
		}
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
	m_renderers.push_back(renderer);
	if (m_sharedCamera)
	{
		if (!m_commonCamera)
		{
			m_commonCamera = renderer->GetActiveCamera();
		}
		else
		{
			renderer->SetActiveCamera(m_commonCamera);
			return; // no need to add observer to same camera more than once
		}
	}
	else
	{
		if (m_rendererObserverTags.size() > 0)
		{
			auto sourceCam = m_rendererObserverTags.keys()[0];
			copyCameraParams(renderer->GetActiveCamera(), sourceCam);
			renderer->GetActiveCamera()->SetParallelProjection(sourceCam->GetParallelProjection());
		}
	}
	auto observeTag = renderer->GetActiveCamera()->AddObserver(vtkCommand::ModifiedEvent, this, &iARendererViewSync::redrawOtherRenderers);

	m_rendererObserverTags.insert(renderer->GetActiveCamera(), observeTag);
}

bool iARendererViewSync::removeFromBundle(vtkRenderer* renderer, bool resetCamera)
{
	auto it = std::find(m_renderers.begin(), m_renderers.end(), renderer);
	if (it == m_renderers.end())
	{
		assert(false);
		LOG(lvlWarn, "iARenderManager::removeFromBundle called with renderer which isn't part of the Bundle!");
		return false;
	}
	m_renderers.erase(it);
	if (!m_sharedCamera || m_renderers.size() == 0)
	{
		auto cam = renderer->GetActiveCamera();
		cam->RemoveObserver(m_rendererObserverTags[cam]);
		m_rendererObserverTags.remove(cam);
	}
	if (resetCamera && m_sharedCamera)
	{
		vtkSmartPointer<vtkCamera> newCam = vtkSmartPointer<vtkCamera>::New();
		newCam->DeepCopy(renderer->GetActiveCamera());
		renderer->SetActiveCamera(newCam);
	}
	return true;
}

void iARendererViewSync::removeAll()
{
	m_commonCamera = nullptr;
	for (auto r : m_renderers)
	{
		removeFromBundle(r, false);
	}
}

bool iARendererViewSync::sharedCamera() const
{
	return m_sharedCamera;
}

void iARendererViewSync::redrawOtherRenderers(vtkObject* caller, long unsigned int /*eventId*/, void* /*callData*/)
{
	if (m_updateInProgress || !caller)
	{
		return;
	}
	m_updateInProgress = true;  // thread-safety?
	auto sourceCam = static_cast<vtkCamera*>(caller);
	std::set<vtkRenderWindow*> updatedWindows;
	if (!m_sharedCamera)
	{
		for (auto r : m_renderers)
		{
			if (r->GetActiveCamera() == caller)
			{   // the window from which the call originated doesn't need to be updated:
				updatedWindows.insert(r->GetRenderWindow());
			}
			else
			{   // for all others, update camera:
				copyCameraParams(r->GetActiveCamera(), sourceCam);
			}
		}
	}
	// Update all render windows:
	for (auto r : m_renderers)
	{
		auto rw = r->GetRenderWindow();
		// no update if renderer was already removed from render window, or if that window was already updated
		if (rw && updatedWindows.find(rw) == updatedWindows.end())
		{
			r->GetRenderWindow()->Render();
			updatedWindows.insert(r->GetRenderWindow());
		}
	}
	m_updateInProgress = false;
}
