#pragma once
#include "iAConsole.h"
#include "iAModuleInterface.h"
#include "iAPolygonPrimitivesModuleInterface.h"
#include "dlg_PolygonPrimitives.h"
#include <mdichild.h>

void iAPolygonPrimitivesModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
		return;        // in that case, we do not do anything as we can not add a menu entry there
	QMenu* filtersMenu = m_mainWnd->filtersMenu();  // alternatively, you can use getToolsMenu() here if you want to add a tool
	QAction* actionTest = new QAction(m_mainWnd);
	actionTest->setText(QApplication::translate("MainWindow", "Test", 0));
	AddActionToMenuAlphabeticallySorted(filtersMenu, actionTest, false);
	connect(actionTest, SIGNAL(triggered()), this, SLOT(TestAction()));
}

void iAPolygonPrimitivesModuleInterface::TestAction()
{
	/*auto child = m_mainWnd->activeMdiChild();
	if (!child) {
		DEBUG_LOG("current child is null");
		return;
	}*/

	PolygonPrimitives dlg_primitives; 

	if (dlg_primitives.exec() != QDialog::Accepted) {
	
		return; 
	}



	QMessageBox::information(m_mainWnd, "Test Module", "This is the Test Module!");
}