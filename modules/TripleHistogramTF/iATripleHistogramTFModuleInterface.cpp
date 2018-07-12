// Copied from the tutorial

#include "iATripleHistogramTFModuleInterface.h"

#include "mainwindow.h"

#include <QMessageBox>

void iATripleHistogramTFModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	QMenu * filtersMenu = m_mainWnd->getToolsMenu();  // alternatively, you can use getToolsMenu() here if you want to add a tool
	QAction * actionTest = new QAction( m_mainWnd );
	actionTest->setText( QApplication::translate( "MainWindow", "Test", 0 ) );
	AddActionToMenuAlphabeticallySorted(filtersMenu,  actionTest, true ); // "By specifying false in the third parameter to AddActionToMenuAlphabeticallySorted we say that the menu entry should not depend on the availability of an open file (if you say true here, the menu entry will only be enabled if a file is open)"
	connect( actionTest, SIGNAL( triggered() ), this, SLOT( TestAction() ) ); // "The added menu entry is linked to the method TestAction via the call to the connect method (inherited from QObject). In the TestAction we just open a simple information message box"
}

void iATripleHistogramTFModuleInterface::TestAction()
{
	//QMessageBox::information(m_mainWnd, "Test Module", "This is the Test Module!");
	PrepareActiveChild();
	//QDir datasetsDir = m_mdiChild->getFilePath();
	//datasetsDir.setNameFilters(QStringList("*.mhd"));
	thtf = new dlg_TripleHistogramTF(m_mdiChild);
	m_mdiChild->addDockWidget(Qt::BottomDockWidgetArea, thtf);
	thtf->raise();
}