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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAFiberScoutModuleInterface.h"

#include "iAConsole.h"
#include "iAFiberScoutAttachment.h"
#include "iAFiberScoutToolbar.h"
#include "mainwindow.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>

void iAFiberScoutModuleInterface::Initialize()
{
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QAction * actionFibreScout = new QAction( m_mainWnd );
	actionFibreScout->setText( QApplication::translate( "MainWindow", "FiberScout", 0 ) );
	AddActionToMenuAlphabeticallySorted( toolsMenu, actionFibreScout );
	tlbFiberScout = 0;
	connect( actionFibreScout, SIGNAL( triggered() ), this, SLOT( FiberScout() ) );
}

void iAFiberScoutModuleInterface::FiberScout()
{
	QMap<QString, FilterID> objectMap;
	objectMap["Fibers"] = INDIVIDUAL_FIBRE_VISUALIZATION;
	objectMap["Voids"] = INDIVIDUAL_PORE_VISUALIZATION;

	QStringList items;
	items << tr( "Fibers" ) << tr( "Voids" );
	QString fileName, filterName = tr( "FiberScout" ), item;

	PrepareActiveChild();

	fileName = QFileDialog::getOpenFileName( m_mdiChild, tr( "Select CSV File" ), m_mdiChild->getFilePath(), tr( "CSV Files (*.csv)" ) );

	if ( !fileName.isEmpty() )
	{
		QFile file( fileName );
		if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
		{
			// TODO: create convention, 2nd line of csv file for fibers (pore csv file have this line)
			// Automatic csv file detection
			QTextStream in( &file );
			in.readLine();
			item = in.readLine();
			if ( item != "Voids" )
				item = "Fibers";
			file.close();

			if ( item == items[0] || item == items[1] )
			{
				if ( m_mdiChild && filter_FiberScout( m_mdiChild, fileName, objectMap[item] ) )
				{
					SetupToolbar();
					m_mdiChild->addStatusMsg( filterName );
				}
			}
			else
				m_mdiChild->addMsg( "CSV-file header error." );
		}
		else
			m_mdiChild->addMsg( "CSV-file could not be opened." );
	}
	else
		m_mdiChild->addMsg( "CSV-file name error." );
}

void iAFiberScoutModuleInterface::SetupToolbar()
{
	if ( tlbFiberScout )
	{
		return;
	}
	tlbFiberScout = new iAFiberScoutToolbar( m_mainWnd );
	m_mainWnd->addToolBar( Qt::BottomToolBarArea, tlbFiberScout );
	connect( tlbFiberScout->actionLength_Distribution, SIGNAL( triggered() ), this, SLOT( FiberScout_Options() ) );
	connect( tlbFiberScout->actionMeanObject, SIGNAL( triggered() ), this, SLOT( FiberScout_Options() ) );
	connect( tlbFiberScout->actionMultiRendering, SIGNAL( triggered() ), this, SLOT( FiberScout_Options() ) );
	connect( tlbFiberScout->actionOrientation_Rendering, SIGNAL( triggered() ), this, SLOT( FiberScout_Options() ) );
	connect( tlbFiberScout->actionActivate_SPM, SIGNAL( triggered() ), this, SLOT( FiberScout_Options() ) );
	tlbFiberScout->setVisible( true );
}

bool iAFiberScoutModuleInterface::filter_FiberScout( MdiChild* mdiChild, QString fileName, FilterID filterID )
{
	if ( !m_mdiChild->LoadCsvFile( filterID, fileName ) )
		return false;

	QString filtername = tr( "FiberScout started" );
	m_mdiChild->addStatusMsg( filtername );
	m_mdiChild->addMsg( filtername );
	AttachToMdiChild( m_mdiChild );
	connect( m_mdiChild, SIGNAL( closed() ), this, SLOT( onChildClose() ) );
	iAFiberScoutAttachment* attach = GetAttachment<iAFiberScoutAttachment>();
	if ( !attach )
	{
		m_mdiChild->addMsg( "Error while creating FiberScout module!" );
		return false;
	}
	attach->init( filterID );
	return true;
}

void iAFiberScoutModuleInterface::FiberScout_Options()
{
	QAction *action = (QAction *) sender();
	QString actionText = action->text();

	int idx = 0;

	if ( actionText.toStdString() == "Length Distribution" ) idx = 7;
	if ( actionText.toStdString() == "Mean Object" ) idx = 4;
	if ( actionText.toStdString() == "Multi Rendering" ) idx = 3;
	if ( actionText.toStdString() == "Orientation Rendering" ) idx = 5;
	if ( actionText.toStdString() == "Activate SPM" ) idx = 6;

	m_mdiChild = m_mainWnd->activeMdiChild();
	iAFiberScoutAttachment* attach = GetAttachment<iAFiberScoutAttachment>();
	if ( !attach )
	{
		DEBUG_LOG( "No FiberScout attachment in current MdiChild!" );
		return;
	}
	attach->FiberScout_Options( idx );

	m_mainWnd->statusBar()->showMessage( tr( "FiberScout options changed to: " ).append( actionText ), 5000 );
}

void iAFiberScoutModuleInterface::onChildClose()
{
	m_mainWnd->removeToolBar( tlbFiberScout );
	delete tlbFiberScout;
	tlbFiberScout = 0;
}


iAModuleAttachmentToChild * iAFiberScoutModuleInterface::CreateAttachment( MainWindow* mainWnd, iAChildData childData )
{
	return new iAFiberScoutAttachment( mainWnd, childData );
}
