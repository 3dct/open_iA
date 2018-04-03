/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
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
#include "iAXRFModuleInterface.h"

#include "iAXRFAttachment.h"
#include "mainwindow.h"
#include "mdichild.h"


void iAXRFModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QAction * actionXRF = new QAction( m_mainWnd );
	actionXRF->setText( QApplication::translate( "MainWindow", "InSpectr", 0 ) );
	AddActionToMenuAlphabeticallySorted( toolsMenu,  actionXRF );
	connect(actionXRF, SIGNAL(triggered()), this, SLOT(XRF_Visualization()));
}

bool iAXRFModuleInterface::XRF_Visualization()
{
	PrepareActiveChild();
	return AttachToMdiChild( m_mdiChild );
}

iAModuleAttachmentToChild * iAXRFModuleInterface::CreateAttachment( MainWindow* mainWnd, iAChildData childData )
{
	return new iAXRFAttachment( mainWnd, childData );
}
