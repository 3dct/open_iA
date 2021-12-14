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
#include "iAImNDTModuleInterface.h"

#include "iAImNDTAttachment.h"
#include "iAVREnvironment.h"

// FeatureScout - 3D cylinder visualization
#include "dlg_CSVInput.h"
#include "iA3DCylinderObjectVis.h"
#include "iACsvConfig.h"
#include "iACsvVtkTableCreator.h"

#include <iALog.h>
#include <iAMainWindow.h>


#include <openvr.h>

#include <vtkFloatArray.h>
#include <vtkTable.h>

#include <QAction>
#include <QMenu>
#include <QMessageBox>

void iAImNDTModuleInterface::Initialize()
{
	if (!m_mainWnd)
	{
		return;
	}

	QAction * actionVRInfo = new QAction(tr("Info"), m_mainWnd);
	connect(actionVRInfo, &QAction::triggered, this, &iAImNDTModuleInterface::info);

	QAction * actionVRRender = new QAction(tr("Rendering"), m_mainWnd);
	connect(actionVRRender, &QAction::triggered, this, &iAImNDTModuleInterface::render);
	m_mainWnd->makeActionChildDependent(actionVRRender);

	m_actionVRShowFibers = new QAction(tr("Start Analysis"), m_mainWnd);
	connect(m_actionVRShowFibers, &QAction::triggered, this, &iAImNDTModuleInterface::showFibers);

	m_actionVR_ARView = new QAction(tr("Start AR Environment"), m_mainWnd);
	connect(m_actionVR_ARView, &QAction::triggered, this, &iAImNDTModuleInterface::startARView);
	m_actionVR_ARView->setDisabled(true);

	QMenu* vrMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("ImNDT"), false);
	vrMenu->addAction(actionVRInfo);
	vrMenu->addAction(actionVRRender);
	vrMenu->addAction(m_actionVRShowFibers);
	vrMenu->addAction(m_actionVR_ARView);
}

void iAImNDTModuleInterface::info()
{
	LOG(lvlInfo, QString("VR Information:"));
	LOG(lvlInfo, QString("    Is Runtime installed: %1").arg(vr::VR_IsRuntimeInstalled() ? "yes" : "no"));
	const uint32_t MaxRuntimePathLength = 1024;
	uint32_t actualLength;
#if OPENVR_VERSION_MAJOR > 1 || (OPENVR_VERSION_MAJOR == 1 && OPENVR_VERSION_MINOR > 3)
	char runtimePath[MaxRuntimePathLength];
	vr::VR_GetRuntimePath(runtimePath, MaxRuntimePathLength, &actualLength);
#else // OpenVR <= 1.3.22:
	char const * runtimePath = vr::VR_RuntimePath();
#endif
	LOG(lvlInfo, QString("    OpenVR runtime path: %1").arg(runtimePath));
	LOG(lvlInfo, QString("    Head-mounted display present: %1").arg(vr::VR_IsHmdPresent() ? "yes" : "no"));
	vr::EVRInitError eError = vr::VRInitError_None;
	auto pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);
	if (eError != vr::VRInitError_None)
	{
		LOG(lvlError, QString("    Unable to init VR runtime: %1").arg(vr::VR_GetVRInitErrorAsEnglishDescription(eError)));
	}
	else
	{
		if (pHMD)
		{
			uint32_t width, height;
			pHMD->GetRecommendedRenderTargetSize(&width, &height);
			LOG(lvlInfo, QString("    Head-mounted display present, recommended render target size: %1x%2 ").arg(width).arg(height));
		}
		else
		{
			LOG(lvlInfo, QString("    Head-mounted display could not be initialized: %1").arg(vr::VR_GetVRInitErrorAsEnglishDescription(eError)));
		}
	}
	vr::VR_Shutdown();
}

void iAImNDTModuleInterface::render()
{
	if (!vrAvailable())
	{
		return;
	}
	PrepareActiveChild();
	AttachToMdiChild( m_mdiChild );
}

void iAImNDTModuleInterface::showFibers()
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

	std::map<size_t, std::vector<iAVec3f> > curvedFiberInfo;

	if (csvConfig.visType == iACsvConfig::Cylinders || csvConfig.visType == iACsvConfig::Lines)
	{
		if(!readCurvedFiberInfo(csvConfig.curvedFiberFileName, curvedFiberInfo))
		{
			curvedFiberInfo = std::map<size_t, std::vector<iAVec3f>>();
		}
	}

	if (!vrAvailable())
	{
		return;
	}
	if (m_vrEnv)
	{
		return;
	}
	//Create Environment
	m_vrEnv.reset(new iAVREnvironment());
	connect(m_vrEnv.data(), &iAVREnvironment::finished, this, &iAImNDTModuleInterface::vrDone);
	m_actionVRShowFibers->setText("Stop Show Fibers");

	m_objectTable = creator.table();

	//Create InteractorStyle
	m_style = vtkSmartPointer<iAImNDTInteractorStyle>::New();

	//Create VR Main
	m_vrMain = new iAImNDTMain(m_vrEnv.data(), m_style, m_objectTable, io, csvConfig, curvedFiberInfo);

	m_actionVR_ARView->setEnabled(true);

	// Start Render Loop HERE!
	m_vrEnv->start();

	m_vrEnv.reset(nullptr);
}

void iAImNDTModuleInterface::startARView()
{
	if(m_vrMain->toggleArView())
	{
		m_actionVR_ARView->setText("Stop AR Environment");
	}
	else
	{
		m_actionVR_ARView->setText("Start AR Environment");
	}
	
}

bool iAImNDTModuleInterface::vrAvailable()
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

iAModuleAttachmentToChild * iAImNDTModuleInterface::CreateAttachment( iAMainWindow* mainWnd, iAMdiChild* child)
{
	return new iAImNDTAttachment( mainWnd, child );
}

void iAImNDTModuleInterface::vrDone()
{
	m_actionVRShowFibers->setText("Show Fibers");
	m_actionVR_ARView->setDisabled(true);
}