/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "dlg_VisMainWindow.h"

//iA
#include "iAMainWindow.h"
#include "iACompVisMain.h"
#include "iACompVisOptions.h"


dlg_VisMainWindow::dlg_VisMainWindow(iACsvDataStorage* dataStorage, iAMultidimensionalScaling* mds, iAMainWindow* parent, iACompVisMain* main, bool computeMDSFlag) :
	QMainWindow(parent), 
	m_main(main),
	m_dataStorage(dataStorage),
	m_data(dataStorage->getData()), 
	m_mds(mds),
	m_computeMDSFlag(computeMDSFlag),
	m_failed(false)
{
	//setup iAMainWindow
	parent->addSubWindow(this);
	setupUi(this);

	
	if (m_computeMDSFlag)
	{
		//start mds dialog
		startMDSDialog();
		m_dataStorage->setMDSData(m_mds->getResultMatrix());
	}
	else
	{
		//when univariate data is loaded, MDSbutton is deactivated
		menuMDS->setDisabled(true);
	}
	
	//finish iAMainWindow setup
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

void dlg_VisMainWindow::recalculateMDS()
{
	//recompute MDS
	startMDSDialog();

	m_dataStorage->setMDSData(m_mds->getResultMatrix());
	
	//reset all charts
	m_main->reinitializeCharts(m_dataStorage);
}

void dlg_VisMainWindow::updateMDS(iAMultidimensionalScaling* newMds)
{
	m_mds = newMds;
}

QList<csvFileData>* dlg_VisMainWindow::getData()
{
	return m_data;
}

bool dlg_VisMainWindow::failed() const
{
	return m_failed;
}

void dlg_VisMainWindow::createMenu()
{
	//activate MDS recalculation
	connect(actionRecalculateMDS, &QAction::triggered, this, &dlg_VisMainWindow::recalculateMDS);
	
	//activate ordering in histogram table
	connect(actionAscending_to_Number_of_Objects, &QAction::triggered, this, &dlg_VisMainWindow::reorderHistogramTableAscending);
	connect(actionDescending_to_Number_of_Objects, &QAction::triggered, this, &dlg_VisMainWindow::reorderHistogramTableDescending);
	connect(actionAs_Loaded, &QAction::triggered, this, &dlg_VisMainWindow::reorderHistogramTableAsLoaded);

	//activate switching between discretization methods
	connect(actionUniform, &QAction::triggered, this, &dlg_VisMainWindow::enableUniformTable);
	connect(actionBayesian_Blocks, &QAction::triggered, this, &dlg_VisMainWindow::enableBayesianBlocks);
	connect(actionNatual_Breaks, &QAction::triggered, this, &dlg_VisMainWindow::enableNaturalBreaks);

	connect(actionCurve_Representation, &QAction::triggered, this, &dlg_VisMainWindow::enableCurveTable);
}

void dlg_VisMainWindow::reorderHistogramTableAscending()
{
	m_main->orderHistogramTableAscending();
}

void dlg_VisMainWindow::reorderHistogramTableDescending()
{
	m_main->orderHistogramTableDescending();
}

void dlg_VisMainWindow::reorderHistogramTableAsLoaded()
{
	m_main->orderHistogramTableAsLoaded();
}

void dlg_VisMainWindow::enableUniformTable()
{
	m_main->enableUniformTable();
	m_main->activateOrderingButton();
}

void dlg_VisMainWindow::enableBayesianBlocks()
{
	m_main->enableBayesianBlocks();
	m_main->activateOrderingButton();
}

void dlg_VisMainWindow::enableNaturalBreaks()
{
	m_main->enableNaturalBreaks();
	m_main->activateOrderingButton();
}

void dlg_VisMainWindow::enableCurveTable()
{
	m_main->enableCurveTable();
	m_main->deactivateOrderingButton();
}

void dlg_VisMainWindow::deactivateOrdering()
{
	menuOrdering->setDisabled(true);
}

void dlg_VisMainWindow::activateOrdering()
{
	menuOrdering->setEnabled(true);
}