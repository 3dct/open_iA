#include "iACompVisModuleInterface.h"

//testing
#include "iAConsole.h"

#include "dlg_CSVReader.h"
#include "iACompVisMain.h"

#include <mdichild.h>
#include "mainwindow.h"

#include <QMessageBox>


void iACompVisModuleInterface::Initialize()
{
	if (!m_mainWnd)  // if m_mainWnd is not set, we are running in command line mode
	{
		return;  // in that case, we do not do anything as we can not add a menu entry there
	}

	QMenu* toolsMenu = m_mainWnd->toolsMenu();
	QAction* actionCompVis = new QAction(QObject::tr("CompVis"), nullptr);
	AddActionToMenuAlphabeticallySorted(toolsMenu, actionCompVis, false);
	connect(actionCompVis, SIGNAL(triggered()), this, SLOT(CompVis()));
}

void iACompVisModuleInterface::CompVis()
{
	iACompVisMain* main = new iACompVisMain(m_mainWnd);
}