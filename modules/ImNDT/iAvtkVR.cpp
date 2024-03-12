// Copyright (c) open_iA contributors
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

vtkSmartPointer<iAvtkVRCamera> iAvtkVR::createCamera(Backend b)
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

vtkSmartPointer<iAvtkVRRenderWindow> iAvtkVR::createWindow(Backend b)
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

vtkSmartPointer<iAvtkVRRenderWindowInteractor> iAvtkVR::createInteractor(Backend b)
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

vtkSmartPointer<iAvtkVRRenderer> iAvtkVR::createRenderer(Backend b)
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

void iAvtkVR::setActionManifest(iAvtkVRRenderWindowInteractor* interactor, iAvtkVR::Backend backend)
{
	QString actionManifest;
#ifdef OPENXR_AVAILABLE
	if (backend == iAvtkVR::OpenXR)
	{
		actionManifest = QCoreApplication::applicationDirPath() + "/VR-input-manifests/open_iA_openxr_actions.json";
	}
#endif
#ifdef OPENVR_AVAILABLE
	if (backend == iAvtkVR::OpenVR)
	{
		actionManifest = QCoreApplication::applicationDirPath() + "/VR-input-manifests/vtk_openvr_actions.json";
	}
#endif
	if (actionManifest.isEmpty())
	{
		LOG(lvlError, QString("Could not determine action manifest for backend %1").arg(backendName(backend)));
		return;
	}
	interactor->SetActionManifestFileName(actionManifest.toStdString());
}

QString iAvtkVR::backendName(iAvtkVR::Backend b)
{
	return b == iAvtkVR::OpenVR ? "OpenVR" : "OpenXR";
}

std::vector<iAvtkVR::Backend> const& iAvtkVR::availableBackends()
{
	static std::vector<Backend> backends;
	if (backends.empty())
	{
#ifdef OPENVR_AVAILABLE
		backends.push_back(iAvtkVR::OpenVR);
#endif
#ifdef OPENXR_AVAILABLE
		backends.push_back(iAvtkVR::OpenXR);
#endif
	}
	return backends;
}
