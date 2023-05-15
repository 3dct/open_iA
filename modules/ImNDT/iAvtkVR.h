// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>
#include <vtkVersion.h>

#include <QString>

#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)
	#include <vtkVRCamera.h>
	#include <vtkVRRenderer.h>
	#include <vtkVRRenderWindow.h>
	using iAvtkVRCamera = vtkVRCamera;
	using iAvtkVRRenderer = vtkVRRenderer;
	using iAvtkVRRenderWindow = vtkVRRenderWindow;
	#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 2, 0)
	#include <vtkVRRenderWindowInteractor.h>
		using iAvtkVRRenderWindowInteractor = vtkVRRenderWindowInteractor;
	#else
	#include <vtkOpenVRRenderWindowInteractor.h>
		using iAvtkVRRenderWindowInteractor = vtkOpenVRRenderWindowInteractor;
	#endif
#else
#include <vtkOpenVRCamera.h>
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>
using iAvtkVRCamera = vtkOpenVRCamera;
using iAvtkVRRenderer = vtkOpenVRRenderer;
using iAvtkVRRenderWindow = vtkOpenVRRenderWindow;
using iAvtkVRRenderWindowInteractor = vtkOpenVRRenderWindowInteractor;
#endif

class iAVRObjectFactory
{
public:
	enum Backend {
		OpenVR,
		OpenXR
	};
	static vtkSmartPointer<iAvtkVRCamera> createCamera(Backend b);
	static vtkSmartPointer<iAvtkVRRenderWindow> createWindow(Backend b);
	static vtkSmartPointer<iAvtkVRRenderWindowInteractor> createInteractor(Backend b);
	static vtkSmartPointer<iAvtkVRRenderer> createRenderer(Backend b);
	static void setActionManifest(iAvtkVRRenderWindowInteractor* interactor);
	static QString backendName(Backend b);
};