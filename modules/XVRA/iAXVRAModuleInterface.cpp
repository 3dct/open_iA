// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAXVRAModuleInterface.h"

#include <iAAABB.h>
#include <iALog.h>

// guibase
#include <iADataSetRenderer.h>
#include <iADataSetViewer.h>
#include <iAMainWindow.h>
#include <iAMdiChild.h>
#include <iAModuleDispatcher.h>
#include <iARenderer.h>

// objectvis
#include <iACsvInputDlg.h>
#include <iACsvConfig.h>
#include <iACsvVtkTableCreator.h>
#include <iAObjectsData.h>
#include <iAObjectVisFactory.h>

// FeatureScout
#include <iAFeatureScoutTool.h>

// ImNDT
#include <iAImNDTModuleInterface.h>

// XVRA
#include <iAFrustumActor.h>

#include <vtkCamera.h>
#include <vtkRenderer.h>
#include <vtkTable.h>    // required to avoid error C2440: 'static_cast': cannot convert from 'vtkObjectBase *const ' to 'T *'

#include <QAction>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>



#include "iAColoredPolyObjectVis.h"

class dlg_FeatureScout;
class iAImNDTModuleInterface;
class iAObjectData;

/*
class iAXVRATool : public iATool
{
private:
	iAFeatureScoutTool* fsTool;
	iAFrustumActor* fsFrustum;
	iAFrustumActor* vrFrustum;
};
*/

iAXVRAModuleInterface::iAXVRAModuleInterface() :
	m_fsFrustum(nullptr),
	m_vrFrustum(nullptr)
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

	m_actionXVRAStart = new QAction(tr("Start XVRA"), m_mainWnd);
	connect(m_actionXVRAStart, &QAction::triggered, this, &iAXVRAModuleInterface::startXVRA);

	QMenu* vrMenu = getOrAddSubMenu(m_mainWnd->toolsMenu(), tr("XVRA"), false);
	vrMenu->addAction(actionXVRAInfo);
	vrMenu->addAction(m_actionXVRAStart);

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
	// If VR environment is currently running, stop it first (maybe reuse?)
	auto vrModule = m_mainWnd->moduleDispatcher().module<iAImNDTModuleInterface>();
	if (vrModule->isImNDTRunning())
	{
		vrModule->stopImNDT();
		m_actionXVRAStart->setText("Start XVRA");
		return;
	}

	// Start csv dialog (-> PolyObject)
	iACsvInputDlg dlg(false);
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

	// Start ImNDT first, since starting new VR environment could fail (e.g. due to no headset available)
	if (!vrModule->startImNDT(objData, csvConfig))
	{
		return;
	}

	// Start FeatureScout
	auto child = m_mainWnd->createMdiChild(false);
	if (!iAFeatureScoutTool::addToChild(child, csvConfig, objData))
	{
		return;
	}
	connect(child, &iAMdiChild::dataSetRendered, this, [this, vrModule, child](size_t dataSetIdx)
	{
		auto fsTool = dynamic_cast<iAFeatureScoutTool*>(child->tools()[iAFeatureScoutTool::ID].get());
		if (dataSetIdx != fsTool->visDataSetIdx())
		{
			return;
		}
		disconnect(child, &iAMdiChild::dataSetRendered, this, nullptr);

		// Add Camera Frustum visualizations:
		auto fsCam = m_mainWnd->activeMdiChild()->renderer()->camera(); // camera from FeatureScout
		auto vrCam = vrModule->getRenderer()->GetActiveCamera();          // camera from VR

		auto bounds = child->dataSetViewer(dataSetIdx)->renderer()->bounds();
		double maxSize = std::max({
			bounds.maxCorner().x() - bounds.minCorner().x(),
			bounds.maxCorner().y() - bounds.minCorner().y(),
			bounds.maxCorner().z() - bounds.minCorner().z(),
		});
		m_vrFrustum = new iAFrustumActor(vrModule->getRenderer(), fsCam, maxSize/10);  // frustum of featurescout shown in vr
		m_fsFrustum = new iAFrustumActor(m_mainWnd->activeMdiChild()->renderer()->renderer(), vrCam, maxSize / 10);  // frustum of vr shown in featurescout

		// set up rendering updates for frustum changes:
		connect(m_fsFrustum, &iAFrustumActor::updateRequired, this, [this, child, dataSetIdx]()
		{
			if (!m_fsFrustum)
			{
				return;
			}
			// trigger an update to renderer if camera indicator has changed
			static auto lastUpdate = QDateTime::currentDateTime();
			auto lastRenTime = child->dataSetViewer(dataSetIdx)->renderer()->vtkRen()->GetLastRenderTimeInSeconds();
			auto now = QDateTime::currentDateTime();
			auto secDiff = lastUpdate.msecsTo(now) / 1000.0;
			// rate-limit updates to only do it in twice the interval of a single rendering run:
			auto updating = secDiff > 8 * lastRenTime;
			//LOG(lvlDebug, QString("Update triggered; lastRenTime: %1 fps: %2; secDiff: %3, updating: %4")
			//	.arg(lastRenTime)
			//	.arg(1 / lastRenTime)
			//	.arg(secDiff)
			//	.arg(updating));
			if (updating)
			{
				lastUpdate = now;
				m_fsFrustum->updateSource();
				child->updateRenderer();
			}
		});
		connect(m_vrFrustum, &iAFrustumActor::updateRequired, vrModule, [this, vrModule]()
		{
			vrModule->queueTask([this]() { m_vrFrustum->updateSource();  });
		});
		m_vrFrustum->show();
		m_fsFrustum->show();

		// set up selection propagation between FeatureScout and VR:
		connect(fsTool, &iAFeatureScoutTool::selectionChanged, vrModule, [vrModule, fsTool]()
		{
			auto sel = fsTool->selection();
			QSignalBlocker block(vrModule);
			vrModule->setSelection(sel);
		});
		connect(vrModule, &iAImNDTModuleInterface::selectionChanged, fsTool, [vrModule, fsTool]()
		{
			auto sel = vrModule->selection();
			QSignalBlocker block(fsTool);
			fsTool->setSelection(sel);
		});

		// set up close / shut down handler for both child and VR environment:
		connect(child, &iAMdiChild::closed, this, [vrModule, this]()
		{
			disconnect(m_fsFrustum);
			m_fsFrustum = nullptr;
			vrModule->stopImNDT();
		});
		connect(vrModule, &iAImNDTModuleInterface::analysisStopped, this, [this, vrModule]()
		{
			disconnect(vrModule, &iAImNDTModuleInterface::analysisStopped, this, nullptr);
			m_actionXVRAStart->setText("Start XVRA");
		});
	});

	m_actionXVRAStart->setText("Stop Analysis");
}
