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
	
	m_mainW = new dlg_VisMainWindow(mainWin);
	m_mainW->show();

	calculateMDS();
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

void iACompVisMain::calculateMDS()
{
	m_mds = new iAMultidimensionalScaling(m_data);
}