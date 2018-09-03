// Copied from the tutorial

#include "iATripleHistogramTFModuleInterface.h"

#include "mainwindow.h"
#include "mdichild.h"
#include "dlg_TripleHistogramTF.h"

#include <QMessageBox>

void iATripleHistogramTFModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	QMenu * filtersMenu = m_mainWnd->getToolsMenu();  // alternatively, you can use getToolsMenu() here if you want to add a tool
	QAction * actionTest = new QAction( m_mainWnd );
	actionTest->setText( QApplication::translate( "MainWindow", "Triple Histogram Transfer Function", 0 ) );
	AddActionToMenuAlphabeticallySorted(filtersMenu,  actionTest, true ); // "By specifying false in the third parameter to AddActionToMenuAlphabeticallySorted we say that the menu entry should not depend on the availability of an open file (if you say true here, the menu entry will only be enabled if a file is open)"
	connect( actionTest, SIGNAL( triggered() ), this, SLOT(MenuItemSelected() ) ); // "The added menu entry is linked to the method TestAction via the call to the connect method (inherited from QObject). In the TestAction we just open a simple information message box"
}

void iATripleHistogramTFModuleInterface::MenuItemSelected()
{
	PrepareActiveChild();
	thtf = new dlg_TripleHistogramTF(m_mdiChild);

	//m_mdiChild->addDockWidget(Qt::BottomDockWidgetArea, thtf);
	m_mdiChild->tabifyDockWidget(m_mdiChild->logs, thtf);

	thtf->show();
	thtf->raise();
}