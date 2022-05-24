/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iA3DLineObjectVis.h"
#include "iA3DObjectFactory.h"
#include "iACsvConfig.h"
#include "iACsvVtkTableCreator.h"

#include <iALog.h>
#include <iAMainWindow.h>


#include <openvr.h>

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

	QAction * actionVRInfo = new QAction(tr("VR Info"), m_mainWnd);
	connect(actionVRInfo, &QAction::triggered, this, &iAImNDTModuleInterface::info);

	QAction * actionVRRender = new QAction(tr("Rendering"), m_mainWnd);
	connect(actionVRRender, &QAction::triggered, this, &iAImNDTModuleInterface::render);
	m_mainWnd->makeActionChildDependent(actionVRRender);


	m_actionVRStartAnalysis = new QAction(tr("Start Analysis"), m_mainWnd);
	connect(m_actionVRStartAnalysis, &QAction::triggered, this, &iAImNDTModuleInterface::startAnalysis);


	QMenu* vrMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("ImNDT"), false);
	vrMenu->addAction(actionVRInfo);
	vrMenu->addAction(actionVRRender);

	vrMenu->addAction(m_actionVRStartAnalysis);
}

void iAImNDTModuleInterface::info()
{
	QString infoTxt("ImNDT: Immersive Workspace for the Analysis of Multidimensional Material Data From Non-Destructive Testing \n\n Actions with Right Controller: \n (1) Picking is detected at the bottom inner edge of the controller and activated by pressing the trigger inside cube in the MiM or the fiber model. \n (2) Multi-selection is made possible by holding one grip and then picking by triggering the right controller. To confirm the selection release the grip. Deselection by selecting the already selected cube again. \n (3) Pressing the trigger outside an object resets any highlighting. \n (4) Pressing the Application Button switches between similarity network and fiber model (only possible if MiM is present). \n (5) Picking two cubes (Multi-selection) in the similarity network opens the Histo-book through which the user can switch between the distributions by holding the right trigger - swiping left or right and releasing it. \n (6) The Trackpad changes the octree level or feature (both only possible if MiM is present) and resets the Fiber Model. Octree Level can be adjusted by pressing up (higher/detailed level) and down (lower/coarser level) on the trackpad. Feature can be changed by pressing right (next feature) and left (previous feature) on the trackpad. \n\n Actions with Left Controller: \n (1) Pressing the Application Button shows or hides the MiM. \n (2) Pressing the trigger changes the displacement mode. \n (3) The Trackpad changes the applied displacement (only possible if MiM is present) or the Jaccard index (only possible if similarity network is shown). Displacement can be adjusted by pressing up (shift away from center) and down (merge together) on the trackpad. Jaccard index can be changed by pressing right (shows higher similarity) and left (shows lower similarity) on the trackpad. \n (4) By holding one grip and then pressing the trigger on the right controller the AR View can be turned on/off. \n\n Actions using Both Controllers: \n  (1) Pressing a grid button on both controllers zooms or translates the world. The zoom can be controlled by pulling controllers apart (zoom in) or together (zoom out). To translate the world both controllers pull in the same direction (grab and pull closer or away). \n");
	QMessageBox::information(m_mainWnd, "ImNDT Module", infoTxt);

}

void iAImNDTModuleInterface::vrInfo()
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


void iAImNDTModuleInterface::startAnalysis()
{

	if (!m_vrMain)
	{
		//Create VR Main
		if (!loadImNDT()) return;

		ImNDT(m_polyObject, m_objectTable, m_io, m_csvConfig);

		connect(m_vrEnv.data(), &iAVREnvironment::finished, this, &iAImNDTModuleInterface::vrDone);
		m_actionVRStartAnalysis->setText("Stop Analysis");
	}
	else
	{
		m_vrEnv->stop();
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

// Start ImNDT with pre-loaded data
void iAImNDTModuleInterface::ImNDT(QSharedPointer<iA3DColoredPolyObjectVis> polyObject, vtkSmartPointer<vtkTable> objectTable, iACsvIO io, iACsvConfig csvConfig)
{
	if (!m_vrEnv)
		m_vrEnv.reset(new iAVREnvironment());

	if (m_vrEnv->isRunning())
	{
		m_vrEnv->stop();
		return;
	}
	if (!vrAvailable())
	{
		return;
	}

	//Create InteractorStyle
	m_style = vtkSmartPointer<iAImNDTInteractorStyle>::New();

	//Create VR Main
	//TODO: CHECK IF PolyObject is not Volume OR NoVis
	m_polyObject = polyObject;
	m_objectTable = objectTable;
	m_vrMain = new iAImNDTMain(m_vrEnv.data(), m_style, m_polyObject.data(), m_objectTable, io, csvConfig);
	connect(m_vrMain, &iAImNDTMain::selectionChanged, this, &iAImNDTModuleInterface::selectionChanged);

	// Start Render Loop HERE!
	m_vrEnv->start();
}

vtkRenderer* iAImNDTModuleInterface::getRenderer()
{
	return m_vrEnv->renderer();
}


bool iAImNDTModuleInterface::loadImNDT()
{
	dlg_CSVInput dlg(false);
	if (dlg.exec() != QDialog::Accepted)
	{
		return false;
	}
	m_csvConfig = dlg.getConfig();

	iACsvVtkTableCreator creator;

	if (!m_io.loadCSV(creator, m_csvConfig))
	{
		return false;
	}

	std::map<size_t, std::vector<iAVec3f> > curvedFiberInfo;

	if (m_csvConfig.visType == iACsvConfig::Cylinders || m_csvConfig.visType == iACsvConfig::Lines)
	{
		if (!readCurvedFiberInfo(m_csvConfig.curvedFiberFileName, curvedFiberInfo))
		{
			curvedFiberInfo = std::map<size_t, std::vector<iAVec3f>>();
		}
	}

	m_objectTable = creator.table();

	//Create PolyObject
	m_polyObject = create3DObjectVis(
		m_csvConfig.visType, m_objectTable, m_io.getOutputMapping(), QColor(140, 140, 140, 255), curvedFiberInfo)
					   .dynamicCast<iA3DColoredPolyObjectVis>();
	if (!m_polyObject)
	{
		LOG(lvlError, "Invalid 3D object visualization!");
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
	delete m_vrMain;
	m_vrMain = nullptr;
	m_actionVRStartAnalysis->setText("Start Analysis");
}