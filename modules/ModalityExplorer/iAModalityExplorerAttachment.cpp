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
#include "iAModalityExplorerAttachment.h"

#include "dlg_modalities.h"
#include "dlg_planeSlicer.h"
#include "iAChannelVisualizationData.h"
#include "iAChildData.h"
#include "iAConsole.h"
#include "iALogger.h"
#include "iAModality.h"
#include "iARenderSettings.h"
#include "iASlicer.h"
#include "iASlicerWidget.h"
#include "iAWidgetAddHelper.h"
#include "mdichild.h"
#include "mainwindow.h"

#include <QFileDialog>

#include <fstream>
#include <sstream>
#include <string>


iAModalityExplorerAttachment::iAModalityExplorerAttachment(MainWindow * mainWnd, iAChildData childData):
	iAModuleAttachmentToChild(mainWnd, childData)
{
}

iAModalityExplorerAttachment* iAModalityExplorerAttachment::create(MainWindow * mainWnd, iAChildData childData)
{
	MdiChild * mdiChild = childData.child;
	iAModalityExplorerAttachment * newAttachment = new iAModalityExplorerAttachment(mainWnd, childData);

	/*
	dlg_planeSlicer* planeSlicer = new dlg_planeSlicer();
	mdiChild->splitDockWidget(renderWidget, planeSlicer, Qt::Horizontal);
	planeSlicer->hide();
	*/
		
	return newAttachment;
}
