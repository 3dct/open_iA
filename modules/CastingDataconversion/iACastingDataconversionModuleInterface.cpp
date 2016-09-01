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
#include "iACastingDataconversionModuleInterface.h"

#include "iACastImageFilter.h"

#include "mainwindow.h"
#include "mdichild.h"
#include "dlg_commoninput.h"

void iACastingDataconversionModuleInterface::Initialize()
{
	QMenu * filtersMenu = m_mainWnd->getFiltersMenu();
	QAction * actionDataType_Conversion_Filter = new QAction( m_mainWnd );
	actionDataType_Conversion_Filter->setText( QApplication::translate( "MainWindow", "DataType Conversion Filter", 0 ) );
	AddActionToMenuAlphabeticallySorted(filtersMenu,  actionDataType_Conversion_Filter );
	connect( actionDataType_Conversion_Filter, SIGNAL( triggered() ), this, SLOT( castimage_Filter() ) );
}

void iACastingDataconversionModuleInterface::castimage_Filter()
{
	//set parameters
	QStringList datatype = (QStringList() << tr( "VTK_SIGNED_CHAR" ) << tr( "VTK_UNSIGNED_CHAR" ) << tr( "VTK_SHORT" )
		<< tr( "VTK_UNSIGNED_SHORT" ) << tr( "VTK_INT" ) << tr( "VTK_UNSIGNED_INT" ) << tr( "VTK_LONG" ) << tr( "VTK_UNSIGNED_LONG" )
		<< tr( "VTK_FLOAT" ) << tr( "VTK_DOUBLE" ) << tr( "VTK_LONG_LONG" ) << tr( "VTK_UNSIGNED_LONG_LONG" ) << tr( "VTK__INT64" )
		<< tr( "VTK_UNSIGNED__INT64" ) << ( "Label image to color-coded RGBA image" ) );
	QStringList inList = (QStringList() << tr( "+Output Data Type" ));
	QList<QVariant> inPara; 	inPara << datatype;
	dlg_commoninput dlg( m_mainWnd, "FHW CastImage Filter", 1, inList, inPara, NULL );
	QString fhwcifdt;
	if( dlg.exec() != QDialog::Accepted )
		return;
	fhwcifdt = dlg.getComboBoxValues()[0];
	//prepare
	QString filterName = tr( "FHW CastImage Filter" );
	PrepareResultChild( filterName );
	m_mdiChild->addStatusMsg( filterName );
	//execute
	iACastImageFilter* thread = new iACastImageFilter( filterName, FHW_CAST_IMAGE,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild );
	m_mdiChild->connectThreadSignalsToChildSlots( thread );
	thread->setODTParameters( fhwcifdt.toStdString() );
	thread->start();
	m_mainWnd->statusBar()->showMessage( filterName, 5000 );
}

