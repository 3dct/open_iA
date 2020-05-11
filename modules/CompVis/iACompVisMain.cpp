#include "iACompVisMain.h"

//testing
#include "iAConsole.h"

//CompVis
#include "dlg_CSVReader.h"
#include "iAMultidimensionalScaling.h"

//QT
#include <QMessageBox>

iACompVisMain::iACompVisMain(MainWindow* mainWin)
{
	loadData();
	
	initializeMDS();

	m_mainW = new dlg_VisMainWindow(m_data, m_mds, mainWin);
	m_mainW->show();

}

void iACompVisMain::loadData()
{
	dlg_CSVReader* dlg = new dlg_CSVReader();

	if (dlg->exec() != QDialog::Accepted)
	{
		return;
	}

	m_data = dlg->getCsvDataStorage();
}

void iACompVisMain::initializeMDS()
{
	m_mds = new iAMultidimensionalScaling(m_data);
}