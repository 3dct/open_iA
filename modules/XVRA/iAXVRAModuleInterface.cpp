// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAXVRAModuleInterface.h"

#include <iALog.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>
#include <iARenderer.h>

// objectvis
#include <dlg_CSVInput.h>
#include <iACsvConfig.h>
#include <iACsvVtkTableCreator.h>
#include <iAObjectsData.h>
#include <iAObjectVisFactory.h>

// FeatureScout
#include <dlg_FeatureScout.h>
#include <iAFeatureScoutToolbar.h>

// ImNDT
#include <iAImNDTModuleInterface.h>
#include <iAFrustumActor.h>

#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkTable.h>    // required to avoid error C2440: 'static_cast': cannot convert from 'vtkObjectBase *const ' to 'T *'

#include <QAction>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>


iAXVRAModuleInterface::iAXVRAModuleInterface() :
	m_fsMain(nullptr),
	fsFrustum(nullptr),
	vrFrustum(nullptr),
	m_updateRequired(false)
{
}

void iAXVRAModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	{
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	}
	QAction* actionXVRAInfo = new QAction(tr("Info"), m_mainWnd);
	connect(actionXVRAInfo, &QAction::triggered, this, &iAXVRAModuleInterface::info);

	QAction * actionXVRAStart = new QAction(tr("Start XVRA"), m_mainWnd);
	connect(actionXVRAStart, &QAction::triggered, this, &iAXVRAModuleInterface::startXVRA);


	QMenu* vrMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("XVRA"), false);
	vrMenu->addAction(actionXVRAInfo);
	vrMenu->addAction(actionXVRAStart);

}

void iAXVRAModuleInterface::info()
{
	QString infoTxt("Cross-Virtuality Analysis of Rich X-Ray Computed Tomography Data for Materials Science Applications"
		"\n\n"
		"Actions with Right Controller:\n"
		" (1) Picking is detected at the bottom inner edge of the controller and activated by pressing the trigger inside cube in the MiM or the fiber model.\n"
		" (2) Multi-selection is made possible by holding one grip and then picking by triggering the right controller. To confirm the selection release the grip. Deselection by selecting the already selected cube again.\n"
		" (3) Pressing the trigger outside an object resets any highlighting.\n"
		" (4) Pressing the Application Button switches between similarity network and fiber model (only possible if MiM is present).\n"
		" (5) Picking two cubes (Multi-selection) in the similarity network opens the Histo-book through which the user can switch between the distributions by holding the right trigger - swiping left or right and releasing it.\n"
		" (6) The Trackpad changes the octree level or feature (both only possible if MiM is present) and resets the Fiber Model. Octree Level can be adjusted by pressing up (higher/detailed level) and down (lower/coarser level) on the trackpad. Feature can be changed by pressing right (next feature) and left (previous feature) on the trackpad."
		"\n\n"
		"Actions with Left Controller:\n"
		" (1) Pressing the Application Button shows or hides the MiM.\n"
		" (2) Pressing the trigger changes the displacement mode.\n"
		" (3) The Trackpad changes the applied displacement (only possible if MiM is present) or the Jaccard index (only possible if similarity network is shown). Displacement can be adjusted by pressing up (shift away from center) and down (merge together) on the trackpad. Jaccard index can be changed by pressing right (shows higher similarity) and left (shows lower similarity) on the trackpad.\n"
		" (4) By holding one grip and then pressing the trigger on the right controller the AR View can be turned on/off."
		"\n\n"
		"Actions using Both Controllers:\n"
		" (1) Pressing a grid button on both controllers zooms or translates the world. The zoom can be controlled by pulling controllers apart (zoom in) or together (zoom out). To translate the world both controllers pull in the same direction (grab and pull closer or away).\n");
	LOG(lvlInfo, infoTxt);
	QMessageBox::information(m_mainWnd, "XVRA Module", infoTxt);
}

void iAXVRAModuleInterface::startXVRA()
{
	/***** Start csv dialog (-> PolyObject) *****/

	dlg_CSVInput dlg(false);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	iACsvConfig csvConfig = dlg.getConfig();
	auto objData = loadObjectsCSV(csvConfig);
	if (!objData)
	{
		return;
	}
	// Create PolyObject visualization
	m_polyObject = std::dynamic_pointer_cast<iAColoredPolyObjectVis>(createObjectVis(objData.get(), QColor(140, 140, 140, 255)));
	if (!m_polyObject)
	{
		LOG(lvlError, "Invalid 3D object visualization!");
		return;
	}

	// Start VR
	auto vrMain = m_mainWnd->moduleDispatcher().module<iAImNDTModuleInterface>();
	if (!vrMain->ImNDT(objData, m_polyObject, csvConfig))
	{
		return;
	}

	// Start FeatureScout (after VR, since starting VR could fail due to VR already running!)
	auto child = m_mainWnd->createMdiChild(false);
	// TODO: use iAFeatureScoutTool instead of dlg_FeatureScout directly?
	m_fsMain = new dlg_FeatureScout(child, csvConfig.objectType, csvConfig.fileName, objData.get(), m_polyObject.get());
	iAFeatureScoutToolbar::addForChild(m_mainWnd, child);

	// Add Camera Frustum visualizations:
	vtkSmartPointer<vtkCamera> fsCam = m_mainWnd->activeMdiChild()->renderer()->camera(); // camera from FeatureScout
	vtkSmartPointer<vtkCamera> vrCam = vrMain->getRenderer()->GetActiveCamera();          // camera from VR

	double const* bounds = m_polyObject->bounds();
	double maxSize = std::max({bounds[1] - bounds[0], bounds[3] - bounds[2], bounds[5] - bounds[4]});
	vrFrustum = new iAFrustumActor(vrMain->getRenderer(), fsCam, maxSize/10);  // frustum of featurescout shown in vr
	fsFrustum = new iAFrustumActor(m_mainWnd->activeMdiChild()->renderer()->renderer(), vrCam, maxSize / 10);  // frustum of vr shown in featurescout

	m_updateRenderer.callOnTimeout([this, child]()
	{
		if (m_updateRequired)
		{	// trigger an update to renderer if camera indicator has changed
			child->updateRenderer();
		}
		m_updateRequired = false;
	});
	m_updateRenderer.setInterval(200);	// maximum update interval
	connect(fsFrustum, &iAFrustumActor::updateRequired, [this]() { m_updateRequired = true; });
	m_updateRenderer.start();
	vrFrustum->show();
	fsFrustum->show();

	// set up selection propagation from VR to FeatureScout
	connect(vrMain, &iAImNDTModuleInterface::selectionChanged, m_fsMain, &dlg_FeatureScout::selectionChanged3D);

}
