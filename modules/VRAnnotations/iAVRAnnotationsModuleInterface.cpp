// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRAnnotationsModuleInterface.h"

#include <iALog.h>

// guibase
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>

// ImNDT
#include <iAImNDTModuleInterface.h>

#include <vtkCaptionWidget.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>

iAVRAnnotationsModuleInterface::iAVRAnnotationsModuleInterface()
{}

void iAVRAnnotationsModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	{
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	}

	// react on
	//     - labelling tool attached
	connect(m_mainWnd, &iAMainWindow::childCreated, this, [this](iAMdiChild* child)
	{
		connect(child, &iAMdiChild::toolAdded, this, [this, child](QString const& id)
		{
			if (id == iAAnnotationTool::Name)
			{
				LOG(lvlDebug, QString("Annotations started."));
				auto tool = getTool<iAAnnotationTool>(child);
				connect(tool, &iAAnnotationTool::annotationsUpdated, this, [this, child](std::vector<iAAnnotation> const& a)
				{
					m_annotations[child] = a;
					LOG(lvlDebug, QString("Annotations updated: %1").arg(a.size()));
					recreateVRAnnotations();
					// TODO: for better performance, handle annotation changes specifically, instead of always recreating all!
					// annotations were either:
					//     - added
					//     - renamed
					//     - removed
					//     - visibility changed
				});
			}
		});
		/*
		connect(child, &iAMdiChild::toolRemoved, this, [this, child](QString const& id)
		{
			if (id == iAAnnotationTool::Name)
			{
				LOG(lvlDebug, QString("Annotations stopped."));
			}
		});
		*/
		connect(child, &iAMdiChild::closed, this, [this, child]
		{
			auto it = m_annotations.find(child);
			if (it != m_annotations.end())
			{
				m_annotations.erase(it);
				recreateVRAnnotations();
			}
		});
	});
	//     - VR environment started
	// TODO: check - probably depends on ImNDT module being loaded first ??
	auto vrModule = m_mainWnd->moduleDispatcher().module<iAImNDTModuleInterface>();
	connect(vrModule, &iAImNDTModuleInterface::vrStarted, this, [this, vrModule]
	{
		LOG(lvlDebug, QString("VR started"));
		recreateVRAnnotations();
		// push annotations to VR if existing
	});
	connect(vrModule, &iAImNDTModuleInterface::vrStopped, this, [this, vrModule]
	{
		LOG(lvlDebug, QString("VR stopped"));
		recreateVRAnnotations();
		// cleanup?
	});
}

void iAVRAnnotationsModuleInterface::recreateVRAnnotations()
{
	auto vrModule = m_mainWnd->moduleDispatcher().module<iAImNDTModuleInterface>();
	if (!vrModule->isVRRunning())
	{
		return;
	}
	vrModule->queueTask([this, vrModule]    // this needs to run in the context of the VR thread
	{                                       // in order to not remove widgets while they are still being rendered
		for (auto c : m_captionWidgets)
		{
			c->SetInteractor(nullptr);
		}
		m_captionWidgets.clear();  //< remove existing annotations
		auto ren = vrModule->getRenderer();
		for (auto childAnnots : m_annotations)
		{
			for (auto a : childAnnots.second)
			{
				auto captionWidget = create3DWidget(ren->GetRenderWindow()->GetInteractor(), a);
				captionWidget->SetEnabled(true);
				m_captionWidgets.push_back(captionWidget);
			}
		}
	});
}