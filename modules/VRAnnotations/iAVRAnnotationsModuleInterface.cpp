// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRAnnotationsModuleInterface.h"

#include <iALog.h>

// guibase
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>
#include <iAvtkSourcePoly.h>

// ImNDT
#include <iAImNDTModuleInterface.h>

#include <vtkLineSource.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkVectorText.h>
#include <vtkVRFollower.h>

#include <QTimer>

//! vtkCaptionWidget but for VR - place text on a 3D position as well (there is no "screen" to stick to...)
//! required updates:
//      - keep source marker in sync with object transform (position/rotation)
//      - keep text rotated towards user -> should be automatically done by vtkBillboardTextActor3D
struct iAVRCaption {
	iAVRCaption(QString const & text, QColor const & textColor, iAVec3d const & markedPos, iAVec3d const & captionPos, vtkRenderer* ren)
	{
		// using vtkVectorText to avoid using freetype, as that would lead to crash with our two-thread rendering setup
		// https://discourse.vtk.org/t/crash-with-two-rendering-threads-using-text-rendering-freetype/15159
		m_textSrc = vtkSmartPointer<vtkVectorText>::New();
		auto cp = captionPos.data();
		m_textSrc->SetText(text.toLatin1());  // no text property and no utf-8 text for that one
		//m_textSrc->SetInput(text.toUtf8());
		//m_textSrc->GetTextProperty()->SetColor(textColor.redF(), textColor.greenF(), textColor.blueF());
		//m_textSrc->GetTextProperty()->SetOpacity(textColor.alphaF());
		//m_textSrc->GetTextProperty()->SetBackgroundColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF());
		//m_textSrc->GetTextProperty()->SetBackgroundOpacity(bgColor.alphaF());
		// set large font size but scale down to match dataset boundaries:
		//m_textSrc->GetTextProperty()->SetFontSize(20);
		//m_textSrc->SetScale(0.1);                                           // TBD: make scene/screen size specific? make such that it has a (corrected, DPI-considering )pixel size of ~10-20
		m_textMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_textMapper->SetInputConnection(m_textSrc->GetOutputPort());
		m_follower = vtkSmartPointer<vtkVRFollower>::New();
		m_follower->SetMapper(m_textMapper);
		m_follower->GetProperty()->SetColor(textColor.redF(), textColor.greenF(), textColor.blueF());
		m_follower->SetCamera(ren->GetActiveCamera());
		m_follower->SetPosition(cp[0], cp[1], cp[2]);

		ren->AddActor(m_follower);

		auto mp = markedPos.data();
		m_marker.source->SetCenter(mp[0], mp[1], mp[2]);
		m_marker.source->SetRadius(0.1);                                 // TBD: make dataset size specific! a few voxels?
		m_marker.actor->GetProperty()->SetColor(textColor.redF(), textColor.greenF(), textColor.blueF());
		ren->AddActor(m_marker.actor);

		m_line.setPoint(1, cp[0], cp[1], cp[2]);
		m_line.setPoint(1, mp[0], mp[1], mp[2]);
		m_line.actor->GetProperty()->SetColor(textColor.redF(), textColor.greenF(), textColor.blueF());
		ren->AddActor(m_line.actor);
		// TBD: marker glyph (arrow)?
	}
	iAvtkSourcePoly<vtkSphereSource> m_marker;
	iAvtkSourcePoly<vtkLineSource> m_line;

	// use vtkVectorText here, to avoid problems (crashes) due to freetype library not being thread-safe
	vtkSmartPointer<vtkVectorText> m_textSrc;
	vtkSmartPointer<vtkPolyDataMapper> m_textMapper;
	vtkSmartPointer<vtkVRFollower> m_follower;
};

iAVRAnnotationsModuleInterface::~iAVRAnnotationsModuleInterface() = default;  // required for unique_ptr

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
		// push annotations to VR if existing; but only after a little delay
		// (wait for VR to be fully initialized)
		// maybe find a better way than just waiting? could there be a
		//QTimer::singleShot(std::chrono::milliseconds(500), this, [this] {
		recreateVRAnnotations();
		//});
	});
	connect(vrModule, &iAImNDTModuleInterface::vrStopped, this, [this, vrModule]
	{
		LOG(lvlDebug, QString("VR stopped"));
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
	{
		auto ren = vrModule->getRenderer();  // in order to not remove widgets while they are still being rendered
		for (auto const & t : m_textActors)
		{
			ren->RemoveActor(t->m_follower);
			ren->RemoveActor(t->m_marker.actor);
			ren->RemoveActor(t->m_line.actor);
		}
		m_textActors.clear();  //< remove existing annotations
		for (auto childAnnots : m_annotations)
		{
			for (auto a : childAnnots.second)
			{
				iAVec3d captionPos;
				m_textActors.push_back(std::make_shared<iAVRCaption>(
					a.m_name, a.m_color, a.m_coord, captionPos, ren));
			}
		}
	});
}
