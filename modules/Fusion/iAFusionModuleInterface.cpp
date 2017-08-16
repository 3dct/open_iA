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

#include "iAFusionModuleInterface.h"
#include "iASimpleFusion.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <QDialog>
#include <QMdiSubWindow>

void iAFusionModuleInterface::Initialize()
{
	QMenu * filterMenu = m_mainWnd->getFiltersMenu();
	QMenu * menuFusion = getMenuWithTitle(filterMenu, QString( "Fusion" ) );

	QAction * actionAddImages = new QAction(m_mainWnd);
	actionAddImages->setText(QApplication::translate("MainWindow", "Add Images", 0));
	menuFusion->addAction( actionAddImages);
	connect(actionAddImages, SIGNAL(triggered()), this, SLOT(addImages()));
}


void iAFusionModuleInterface::addImages()
{
	//set parameters
	QList<QMdiSubWindow *> mdiwindows = m_mainWnd->MdiChildList();
	QStringList inList = (QStringList() << tr("+Input 1") << tr("+Input 2"));
	int inputIndxs[2];
	if (QDialog::Accepted != m_mainWnd->SelectInputs("Threshold Fusion", inList, inputIndxs))
		return;
	vtkImageData * secondChildImgData = qobject_cast<MdiChild *>(mdiwindows.at(inputIndxs[1])->widget())->getImageData();
	//prepare
	QString filterName = "Add Images Fusion";
	PrepareResultChild(inputIndxs[0], filterName);
	m_mdiChild->addStatusMsg(filterName);
	//execute
	iASimpleFusion* thread = new iASimpleFusion(filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	thread->setInput2( secondChildImgData );
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}
