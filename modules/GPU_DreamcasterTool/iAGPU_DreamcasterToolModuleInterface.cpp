/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "iAGPU_DreamcasterToolModuleInterface.h"

#include "mainwindow.h"
#include "mdichild.h"
#include "dlg_commoninput.h"
#include "dreamcaster.h"

#include <QFileDialog>

void iAGPU_DreamcasterToolModuleInterface::Initialize()
{
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QAction * actionDreamcaster_Open_file = new QAction( m_mainWnd );
	actionDreamcaster_Open_file->setText( QApplication::translate( "MainWindow", "Dreamcaster", 0 ) );
	AddActionToMenuAlphabeticallySorted( toolsMenu,  actionDreamcaster_Open_file, false );
	connect( actionDreamcaster_Open_file, SIGNAL( triggered() ), this, SLOT( dreamcasterOpenFile() ) );
}

void iAGPU_DreamcasterToolModuleInterface::dreamcasterOpenFile()
{
	QString fileName = QFileDialog::getOpenFileName( m_mainWnd, tr( "Open File" ), m_mainWnd->getPath(), tr( "STL files (*.stl)" ) );
	if( (QFileInfo( fileName ).suffix() == "stl") || (QFileInfo( fileName ).suffix() == "STL") )
	{
		DreamCaster *child = new DreamCaster( m_mainWnd );
		m_mainWnd->addSubWindow( child );
		child->loadFile( fileName );
		m_mainWnd->statusBar()->showMessage( tr( "File loaded" ), 5000 );
		child->show();
	}
}
