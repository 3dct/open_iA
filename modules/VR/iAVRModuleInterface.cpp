/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVRModuleInterface.h"

#include "iAVRAttachment.h"
#include "iAVREnvironment.h"

// FeatureScout - 3D cylinder visualization
#include "dlg_CSVInput.h"
#include "iA3DCylinderObjectVis.h"
#include "iACsvConfig.h"
#include "iACsvVtkTableCreator.h"

#include <iAConsole.h>
#include <mainwindow.h>

#include <openvr.h>

#include <vtkFloatArray.h>
#include <vtkTable.h>

#include <QMessageBox>

void iAVRModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}
	QMenu * toolsMenu = m_mainWnd->toolsMenu();
	QMenu* vrMenu = getMenuWithTitle(toolsMenu, tr("VR"), false);

	QAction * actionVRInfo = new QAction(tr("Info"), nullptr);
	AddActionToMenuAlphabeticallySorted(vrMenu, actionVRInfo, false);
	connect(actionVRInfo, &QAction::triggered, this, &iAVRModuleInterface::info);

	QAction * actionVRRender = new QAction(tr("Rendering"), nullptr);
	AddActionToMenuAlphabeticallySorted(vrMenu, actionVRRender, true);
	connect(actionVRRender, &QAction::triggered, this, &iAVRModuleInterface::render);

	m_actionVRShowFibers = new QAction(tr("Show Fibers"), nullptr);
	AddActionToMenuAlphabeticallySorted(vrMenu, m_actionVRShowFibers, false);
	connect(m_actionVRShowFibers, &QAction::triggered, this, &iAVRModuleInterface::showFibers);
}

void iAVRModuleInterface::info()
{
	DEBUG_LOG(QString("VR Information:"));
	DEBUG_LOG(QString("    Is Runtime installed: %1").arg(vr::VR_IsRuntimeInstalled() ? "yes" : "no"));
	const uint32_t MaxRuntimePathLength = 1024;
	uint32_t actualLength;
#if OPENVR_VERSION_MAJOR > 1 || (OPENVR_VERSION_MAJOR == 1 && OPENVR_VERSION_MINOR > 3)
	char runtimePath[MaxRuntimePathLength];
	vr::VR_GetRuntimePath(runtimePath, MaxRuntimePathLength, &actualLength);
#else // OpenVR <= 1.3.22:
	char const * runtimePath = vr::VR_RuntimePath();
#endif
	DEBUG_LOG(QString("    OpenVR runtime path: %1").arg(runtimePath));
	DEBUG_LOG(QString("    Head-mounted display present: %1").arg(vr::VR_IsHmdPresent() ? "yes" : "no"));
	vr::EVRInitError eError = vr::VRInitError_None;
	auto pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None)
	{
		DEBUG_LOG(QString("    Unable to init VR runtime: %1").arg(vr::VR_GetVRInitErrorAsEnglishDescription(eError)));
	}
	else
	{
		if (pHMD)
		{
			uint32_t width, height;
			pHMD->GetRecommendedRenderTargetSize(&width, &height);
			DEBUG_LOG(QString("    Head-mounted display present, recommended render target size: %1x%2 ").arg(width).arg(height));
		}
		else
		{
			DEBUG_LOG(QString("    Head-mounted display could not be initialized: %1").arg(vr::VR_GetVRInitErrorAsEnglishDescription(eError)));
		}
	}
	vr::VR_Shutdown();
}

void iAVRModuleInterface::render()
{
	if (!vrAvailable())
	{
		return;
	}
	PrepareActiveChild();
	AttachToMdiChild( m_mdiChild );
}

void iAVRModuleInterface::showFibers()
{
	if (m_vrEnv)
	{
		m_vrEnv->stop();
		return;
	}
	if (!vrAvailable())
	{
		return;
	}
	dlg_CSVInput dlg(false);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	iACsvConfig csvConfig = dlg.getConfig();
	if (csvConfig.visType == iACsvConfig::UseVolume)
	{
		return;
	}

	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
	{
		return;
	}

	if (!vrAvailable())
	{
		return;
	}
	if (m_vrEnv)
	{
		return;
	}
	m_vrEnv.reset(new iAVREnvironment());
	connect(m_vrEnv.data(), &iAVREnvironment::finished, this, &iAVRModuleInterface::vrDone);
	m_actionVRShowFibers->setText("Stop Show Fibers");

	m_objectTable = creator.table();

	m_cylinderVis.reset(new iA3DCylinderObjectVis(m_vrEnv->renderer(), m_objectTable, io.getOutputMapping(), QColor(255, 0, 0), std::map<size_t, std::vector<iAVec3f> >() ));
	m_cylinderVis->show();

	m_vrEnv->start();

	m_vrEnv.reset(nullptr);
}

bool iAVRModuleInterface::vrAvailable()
{
	if (!vr::VR_IsRuntimeInstalled())
	{
		QMessageBox::warning(m_mainWnd, "VR", "VR runtime not found. Please install Steam and SteamVR!");
		return false;
	}
	if (!vr::VR_IsHmdPresent())
	{
		QMessageBox::warning(m_mainWnd, "VR", "No VR device found. Make sure your HMD device is plugged in and turned on!");
		return false;
	}
	return true;
}

iAModuleAttachmentToChild * iAVRModuleInterface::CreateAttachment( MainWindow* mainWnd, MdiChild* child)
{
	return new iAVRAttachment( mainWnd, child );
}

void iAVRModuleInterface::vrDone()
{
	m_actionVRShowFibers->setText("Show Fibers");
}