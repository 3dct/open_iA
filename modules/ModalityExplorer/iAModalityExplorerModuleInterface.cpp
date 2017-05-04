/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAModalityExplorerModuleInterface.h"

#include "dlg_commoninput.h"
#include "iAConsole.h"
#include "iAIO.h"
#include "mainwindow.h"
#include "mdichild.h"

#include "dlg_modalities.h"
#include "dlg_modalitySPLOM.h"
#include "iAModalityExplorerAttachment.h"
#include "iAModality.h"

#include <vtkImageReader2.h>
#include <vtkTIFFReader.h>
#include <vtkBMPReader.h>
#include <vtkPNGReader.h>
#include <vtkJPEGReader.h>
#include <vtkStringArray.h>

#include <QFileDialog>
#include <QMessageBox>

#include <cassert>


iAModalityExplorerModuleInterface::iAModalityExplorerModuleInterface()
{}


void iAModalityExplorerModuleInterface::Initialize()
{
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuMultiModalChannel = getMenuWithTitle( toolsMenu, QString( "Multi-Modal/-Channel Images" ), false );
	
	QAction * actionModalitySlicer = new QAction(QApplication::translate("MainWindow", "Modality Slicer", 0), m_mainWnd );
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, actionModalitySlicer, true);
	connect(actionModalitySlicer, SIGNAL(triggered()), this, SLOT(ModalitySlicer()));

	QAction * actionModalitySPLOM = new QAction(QApplication::translate("MainWindow", "Modality SPLOM", 0), m_mainWnd);
	AddActionToMenuAlphabeticallySorted(menuMultiModalChannel, actionModalitySPLOM, true);
	connect(actionModalitySPLOM, SIGNAL(triggered()), this, SLOT(ModalitySPLOM()));
}


iAModuleAttachmentToChild* iAModalityExplorerModuleInterface::CreateAttachment(MainWindow* mainWnd, iAChildData childData)
{
	iAModalityExplorerAttachment* result = iAModalityExplorerAttachment::create( mainWnd, childData);
	return result;
}

void iAModalityExplorerModuleInterface::ModalitySlicer()
{

}


void iAModalityExplorerModuleInterface::ModalitySPLOM()
{
	PrepareActiveChild();
	m_dlgModalitySPLOM = new dlg_modalitySPLOM();
	m_dlgModalitySPLOM->SetData(m_mdiChild->GetModalities());
	m_mdiChild->tabifyDockWidget(m_childData.logs, m_dlgModalitySPLOM);
}