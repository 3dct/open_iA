#include "dlg_VisMainWindow.h"

//iA
#include "mainwindow.h"

dlg_VisMainWindow::dlg_VisMainWindow(QList<csvFileData>* data, iAMultidimensionalScaling* mds, MainWindow* parent) 
	:
	QMainWindow(parent),
	m_data(data),
	m_mds(mds)
{
	//setup mainwindow
	parent->mdiArea->addSubWindow(this);
	setupUi(this);

	//start mds dialog
	startMDSDialog();

	//finish mainwindow setup
	createMenu();
	this->showMaximized();
}

void dlg_VisMainWindow::startMDSDialog()
{
	m_MDSD = new dlg_MultidimensionalScalingDialog(m_data, m_mds);
	if (m_MDSD->exec() != QDialog::Accepted)
	{
		return;
	}
}

QList<csvFileData>* dlg_VisMainWindow::getData()
{
	return m_data;
}

void dlg_VisMainWindow::createMenu()
{
	connect(actionRecalculateMDS, &QAction::triggered, this, &dlg_VisMainWindow::startMDSDialog);
}