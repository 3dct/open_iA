#include "iAXVRAModuleInterface.h"

#include "iAMainWindow.h"
#include <iAModuleDispatcher.h>

#include <QAction>
#include <QMenu>
#include <QMessageBox>

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
	vrMenu->addAction(actionXVRAStart);
	vrMenu->addAction(actionXVRAInfo);

	// m_mainWnd->makeActionChildDependent(actionTest);   // uncomment this to enable action only if child window is open
	//m_mainWnd->moduleDispatcher().module<iAVRModuleInterface>();
}

void iAXVRAModuleInterface::info()
{
	QMessageBox::information(m_mainWnd, "XVRA Module", "Cross-Virtuality Analysis of Rich X-Ray Computed Tomography Data for Materials Science Applications");
}

void iAXVRAModuleInterface::startXVRA()
{
	//Start csv dialog (-> PolyObject)
	//Start Featurescout
	//Start VR
}