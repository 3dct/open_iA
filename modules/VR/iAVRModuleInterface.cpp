/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <dlg_commoninput.h>
#include <iAConsole.h>
#include <iAModality.h>
#include <iAModalityTransfer.h>
#include <iAVolumeRenderer.h>
#include <mainwindow.h>
#include <mdichild.h>

#include <openvr.h>

#include <vtkFloatArray.h>
#include <vtkTable.h>

#include <QMessageBox>

void iAVRModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;

	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu* vrMenu = getMenuWithTitle(toolsMenu, tr("VR"), false);

	QAction * actionVRInfo = new QAction(tr("Info"), nullptr);
	AddActionToMenuAlphabeticallySorted(vrMenu, actionVRInfo, false);
	connect(actionVRInfo, &QAction::triggered, this, &iAVRModuleInterface::info);

	QAction * actionVRRender = new QAction(tr("Rendering"), nullptr);
	AddActionToMenuAlphabeticallySorted(vrMenu, actionVRRender, true);
	connect(actionVRRender, &QAction::triggered, this, &iAVRModuleInterface::render);

	QAction * actionVRShowFibers = new QAction(tr("Show Fibers"), nullptr);
	AddActionToMenuAlphabeticallySorted(vrMenu, actionVRShowFibers, false);
	connect(actionVRShowFibers, &QAction::triggered, this, &iAVRModuleInterface::showFibers);

	QAction * actionVRStop = new QAction(tr("Stop VR"), nullptr);
	AddActionToMenuAlphabeticallySorted(vrMenu, actionVRStop, false);
	//actionVRStop->setEnabled(false);
	connect(actionVRStop, &QAction::triggered, this, &iAVRModuleInterface::stop);
}

void iAVRModuleInterface::info()
{
	DEBUG_LOG(QString("VR Information:"));
	DEBUG_LOG(QString("    Head-mounted display present: %1").arg(vr::VR_IsHmdPresent() ? "yes" : "no"));
	DEBUG_LOG(QString("    Is Runtime installed: %1").arg(vr::VR_IsRuntimeInstalled() ? "yes" : "no"));
	DEBUG_LOG(QString("    OpenVR runtime path: %1").arg(vr::VR_RuntimePath()));

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
		return;
	PrepareActiveChild();
	AttachToMdiChild( m_mdiChild );
}

void iAVRModuleInterface::showFibers()
{
	if (!vrAvailable())
		return;
	dlg_CSVInput dlg(false);
	if (dlg.exec() != QDialog::Accepted)
		return;
	iACsvConfig csvConfig = dlg.getConfig();
	if (csvConfig.visType == iACsvConfig::UseVolume)
		return;

	iACsvVtkTableCreator creator;
	iACsvIO io;
	if (!io.loadCSV(creator, csvConfig))
		return;

	if (!vrAvailable())
		return;
	if (m_vrEnv)
		return;
	m_vrEnv.reset(new iAVREnvironment());
	//actionVRStop->setEnabled(true);

	m_objectTable = creator.getTable();

	m_cylinderVis.reset(new iA3DCylinderObjectVis(m_vrEnv->renderer(), m_objectTable, io.getOutputMapping(), QColor(255, 0, 0), 12));
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

void iAVRModuleInterface::stop()
{
	m_vrEnv->stop();
}

iAModuleAttachmentToChild * iAVRModuleInterface::CreateAttachment( MainWindow* mainWnd, iAChildData childData )
{
	return new iAVRAttachment( mainWnd, childData );
}
