#include "iAXVRAModuleInterface.h"

#include "iAMainWindow.h"
#include <iAModuleDispatcher.h>

#include <QAction>
#include <QMessageBox>

void iAXVRAModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	{
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	}
	QAction * actionTest = new QAction(tr("Test"), m_mainWnd);
	connect(actionTest, &QAction::triggered, this, &iAXVRAModuleInterface::testAction);
	// m_mainWnd->makeActionChildDependent(actionTest);   // uncomment this to enable action only if child window is open
	addToMenuSorted(m_mainWnd->toolsMenu(), actionTest);
	//m_mainWnd->moduleDispatcher().module<iAVRModuleInterface>();
}

void iAXVRAModuleInterface::testAction()
{
	QMessageBox::information(m_mainWnd, "Test Module", "This is the Test Module!");
}