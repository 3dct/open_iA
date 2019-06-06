#include "iAdaptiveThresholdModuleInterface.h"
#include "dlg_AdaptiveThreshold.h"

#include "mainwindow.h"

#include <QMessageBox>

void iAdaptiveThresholdModuleInterface::Initialize()
{
	if (!m_mainWnd)    // if m_mainWnd is not set, we are running in command line mode
	    return;        // in that case, we do not do anything as we can not add a menu entry there
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();  // alternatively, you can use getToolsMenu() here if you want to add a tool
	QAction * actionTest = new QAction( m_mainWnd );
	actionTest->setText( QApplication::translate( "MainWindow", "AdaptiveThresholding", 0 ) );
	AddActionToMenuAlphabeticallySorted(toolsMenu,  actionTest, false );
	connect( actionTest, SIGNAL( triggered() ), this, SLOT( TestAction() ) );
}

void iAdaptiveThresholdModuleInterface::TestAction()
{
	AdaptiveThreshold dlg_thres;
	dlg_thres.initChart();
	if (dlg_thres.exec() != QDialog::Accepted)
		return;
	/*QMessageBox::information(m_mainWnd, "Test Module", "This is the Test Module!");*/
}