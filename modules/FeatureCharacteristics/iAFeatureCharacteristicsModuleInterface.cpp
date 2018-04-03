/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "pch.h"
#include "iAFeatureCharacteristicsModuleInterface.h"

#include "dlg_commoninput.h"
#include "iACalcFeatureCharacteristics.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QFileDialog>
#include <QMessageBox>

void iAFeatureCharacteristicsModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuPorosity = getMenuWithTitle( filtersMenu, QString( "Feature Characteristics" ) );
	QAction * actionCalcFeatureCharacteristics = new QAction( QApplication::translate( "MainWindow", "Calculate Feature Characteristics", 0 ), m_mainWnd );
	menuPorosity->addAction( actionCalcFeatureCharacteristics );
	connect( actionCalcFeatureCharacteristics, SIGNAL( triggered() ), this, SLOT( calcFeatureCharacteristics() ) );
}

void iAFeatureCharacteristicsModuleInterface::calcFeatureCharacteristics()
{
	//prepare
	QString filterName = tr( "Calculate Feature Characteristics" );
	PrepareActiveChild();
	m_mdiChild->addStatusMsg( filterName );
	QString filename = QFileDialog::getSaveFileName( 0, tr( "Save pore csv file" ),
		m_mainWnd->getPath(), tr( "csv Files (*.csv *.CSV)" ) );
	if ( filename.isEmpty() )
	{
		QMessageBox msgBox;
		msgBox.setText( "No destination file was specified!" );
		msgBox.setWindowTitle( "Calculate Feature Characteristics" );
		msgBox.exec();
		return;
	}

	QStringList inList = (QStringList() << tr("$Calculate Feret Diameter"));
	QList<QVariant> inPara = QList<QVariant>() << "false";
	dlg_commoninput dlg(m_mainWnd, tr("Calculate Feature Characteristics"), inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;

	bool feretDiameter = dlg.getCheckValue(0) != 0;

	//execute
	iACalcFeatureCharacteristics* thread = new iACalcFeatureCharacteristics( filterName,
		 m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild, filename, feretDiameter);
	m_mdiChild->connectAlgorithmSignalsToChildSlots( thread );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}
