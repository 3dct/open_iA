/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "iAThresholdingModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAGeneralThresholding.h"
#include "iAThresholding.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QSettings>

void iAThresholdingModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuSegmentation = getMenuWithTitle(filtersMenu, QString("Segmentation"));
	QMenu * menuThresholding = getMenuWithTitle(menuSegmentation, QString( "Thresholding" ) );

	QAction * actionBinary_threshold_filter = new QAction(QApplication::translate( "MainWindow", "Binary threshold filter", 0 ), m_mainWnd);
	menuThresholding->addAction( actionBinary_threshold_filter );
	connect( actionBinary_threshold_filter, SIGNAL( triggered() ), this, SLOT( binary_threshold() ) );

	QAction * actionGeneral_threshold_filter = new QAction(QApplication::translate( "MainWindow", "General threshold filter", 0 ), m_mainWnd);
	menuThresholding->addAction( actionGeneral_threshold_filter );
	connect( actionGeneral_threshold_filter, SIGNAL( triggered() ), this, SLOT( general_threshold() ) );
}

void iAThresholdingModuleInterface::binary_threshold()
{
	//set parameters
	QSettings settings;
	btlower = settings.value( "Filters/Segmentation/BinaryThresholding/btlower" ).toInt();
	btupper = settings.value( "Filters/Segmentation/BinaryThresholding/btupper" ).toInt();
	btoutside = settings.value( "Filters/Segmentation/BinaryThresholding/btoutside" ).toInt();
	btinside = settings.value( "Filters/Segmentation/BinaryThresholding/btinside" ).toInt();
	
	QStringList inList = (QStringList() << tr( "#Lower Threshold" ) << tr( "#Upper Threshold" ) << tr( "#Outside Value" ) << tr( "#Inside Value" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( btlower ) << tr( "%1" ).arg( btupper ) << tr( "%1" ).arg( btoutside ) << tr( "%1" ).arg( btinside );
	dlg_commoninput dlg( m_mainWnd, "Binary Threshold", 4, inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	btlower = dlg.getValues()[0];
	btupper = dlg.getValues()[1];
	btoutside = dlg.getValues()[2];
	btinside = dlg.getValues()[3];
											  
	settings.setValue( "Filters/Segmentation/BinaryThresholding/btlower", btlower );
	settings.setValue( "Filters/Segmentation/BinaryThresholding/btupper", btupper );
	settings.setValue( "Filters/Segmentation/BinaryThresholding/btoutside", btoutside );
	settings.setValue( "Filters/Segmentation/BinaryThresholding/btinside", btinside );
	
	//prepare
	QString filterName = tr( "Binary threshold filter" );
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAThresholding * thread = new iAThresholding( filterName, BINARY_THRESHOLD,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setBTParameters( btlower, btupper, btoutside, btinside );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

void iAThresholdingModuleInterface::general_threshold()
{
	//set parameters
	QSettings settings;
	gtlower = settings.value( "Filters/Segmentation/GeneralThresholding/gtlower" ).toDouble();
	gtupper = settings.value( "Filters/Segmentation/GeneralThresholding/gtupper" ).toDouble();
	gtoutside = settings.value( "Filters/Segmentation/GeneralThresholding/gtoutside" ).toDouble();

	QStringList inList = (QStringList() << tr( "#Lower Threshold" ) << tr( "#Upper Threshold" ) << tr( "#Outside Value" ));
	QList<QVariant> inPara; 	inPara << tr( "%1" ).arg( gtlower ) << tr( "%1" ).arg( gtupper ) << tr( "%1" ).arg( gtoutside );
	dlg_commoninput dlg( m_mainWnd, "General Threshold", 3, inList, inPara, NULL );
	if( dlg.exec() != QDialog::Accepted )
		return;
	gtlower = dlg.getValues()[0];
	gtupper = dlg.getValues()[1];
	gtoutside = dlg.getValues()[2];

	settings.setValue( "Filters/Segmentation/GeneralThresholding/gtlower", gtlower );
	settings.setValue( "Filters/Segmentation/GeneralThresholding/gtupper", gtupper );
	settings.setValue( "Filters/Segmentation/GeneralThresholding/gtoutside", gtoutside );

	//prepare
	QString filterName = tr("General threshold filter");
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iAGeneralThresholding * thread = new iAGeneralThresholding( filterName, GENERAL_THRESHOLD,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setGTParameters( gtlower, gtupper, gtoutside );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}