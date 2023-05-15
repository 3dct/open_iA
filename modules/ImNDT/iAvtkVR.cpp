// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <iAvtkVR.h>

#include "iALog.h"

// required for VTK >= 9.1.0 for actual implementations:
#ifdef OPENVR_AVAILABLE
#include <vtkOpenVRCamera.h>
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#endif

#ifdef OPENXR_AVAILABLE
#include <vtkOpenXRCamera.h>
#include <vtkOpenXRRenderer.h>
#include <vtkOpenXRRenderWindow.h>
#include <vtkOpenXRRenderWindowInteractor.h>
#endif

#include <QCoreApplication>

vtkSmartPointer<iAvtkVRCamera> iAVRObjectFactory::createCamera(Backend b)
{
#ifdef OPENVR_AVAILABLE
	if (b == OpenVR)
	{
		return vtkSmartPointer<vtkOpenVRCamera>::New();

	}
#endif
#ifdef OPENXR_AVAILABLE
	if (b == OpenXR)
	{
		return vtkSmartPointer<vtkOpenXRCamera>::New();
	}
#endif
	LOG(lvlError, QString("The chosen backend %1 is invalid or not available!").arg(b));
	return {};
}

vtkSmartPointer<iAvtkVRRenderWindow> iAVRObjectFactory::createWindow(Backend b)
{
#ifdef OPENVR_AVAILABLE
	if (b == OpenVR)
	{
		return vtkSmartPointer<vtkOpenVRRenderWindow>::New();

	}
#endif
#ifdef OPENXR_AVAILABLE
	if (b == OpenXR)
	{
		return vtkSmartPointer<vtkOpenXRRenderWindow>::New();
	}
#endif
	LOG(lvlError, QString("The chosen backend %1 is invalid or not available!").arg(b));
	return {};
}

vtkSmartPointer<iAvtkVRRenderWindowInteractor> iAVRObjectFactory::createInteractor(Backend b)
{
#ifdef OPENVR_AVAILABLE
	if (b == OpenVR)
	{
		return vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New();

	}
#endif
#ifdef OPENXR_AVAILABLE
	if (b == OpenXR)
	{
		return vtkSmartPointer<vtkOpenXRRenderWindowInteractor>::New();
	}
#endif
	LOG(lvlError, QString("The chosen backend %1 is invalid or not available!").arg(b));
	return {};
}

vtkSmartPointer<iAvtkVRRenderer> iAVRObjectFactory::createRenderer(Backend b)
{
#ifdef OPENVR_AVAILABLE
	if (b == OpenVR)
	{
		return vtkSmartPointer<vtkOpenVRRenderer>::New();

	}
#endif
#ifdef OPENXR_AVAILABLE
	if (b == OpenXR)
	{
		return vtkSmartPointer<vtkOpenXRRenderer>::New();
	}
#endif
	LOG(lvlError, QString("The chosen backend %1 is invalid or not available!").arg(b));
	return {};
}

void iAVRObjectFactory::setActionManifest(iAvtkVRRenderWindowInteractor* interactor)
{
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)
	// TODO: move to interactor retrieval?
#if OPENXR_AVAILABLE
	QString actionManifest = QCoreApplication::applicationDirPath() + "/VR-input-manifests/open_iA_openxr_actions.json";
#else
	QString actionManifest = QCoreApplication::applicationDirPath() + "/VR-input-manifests/vtk_openvr_actions.json";
#endif
	interactor->SetActionManifestFileName(actionManifest.toStdString());
#else
	Q_UNUSED(interactor);
#endif
}