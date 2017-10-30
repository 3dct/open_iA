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
#include "iAAstraReconstructionModuleInterface.h"

#include "iAFilterRegistry.h"
#include "iAConsole.h"
#include "mainwindow.h"
#include "mdichild.h"

#include "iAAstraAlgorithm.h"
#include "iAFreeBeamCalculation.h"

#include <vtkImageData.h>

#include <QSettings>

#include <cassert>


void iAAstraReconstructionModuleInterface::Initialize( )
{
	REGISTER_FILTER_WITH_RUNNER(iAASTRAForwardProject, iAASTRAFilterRunner);
	REGISTER_FILTER_WITH_RUNNER(iAASTRAReconstruct, iAASTRAFilterRunner);
	if (!m_mainWnd)
		return;
	QMenu* filterMenu = m_mainWnd->getFiltersMenu( );
	QMenu * astraReconMenu = getMenuWithTitle(filterMenu, QString( "ASTRA Toolbox" ), false );

	QAction * actionFreeBeamIntensity = new QAction(m_mainWnd);
	actionFreeBeamIntensity->setText(QApplication::translate("MainWindow", "Free Beam Intensity", 0));
	AddActionToMenuAlphabeticallySorted(astraReconMenu, actionFreeBeamIntensity, true);
	connect(actionFreeBeamIntensity, SIGNAL(triggered()), this, SLOT(FreeBeamIntensity()));
}

void iAAstraReconstructionModuleInterface::FreeBeamIntensity()
{
	PrepareActiveChild();
	if (!m_mdiChild)
		return;
	MdiChild* origChild = m_mdiChild;
	m_childClosed = false;
	connect(origChild, SIGNAL(closed()), this, SLOT(childClosed()));

	//set parameters
	QSettings settings;
	manualMeanFreeBeamIntensity = settings.value("Tools/AstraReconstruction/manualMeanFreeBeamIntensity").toBool();
	manualMeanFreeBeamIntensityValue = settings.value("Tools/AstraReconstruction/manualMeanFreeBeamIntensityValue").toInt();

	QStringList inList = (QStringList() << tr("*IndexX") << tr("*IndexY") 
		<< tr("*SizeX") << tr("*SizeY") << tr("$Set intensity manually") << tr("#"));
	QList<QVariant> inPara; inPara << tr("%1").arg(eiIndexX) << tr("%1").arg(eiIndexY)
		<< tr("%1").arg(m_childData.imgData->GetExtent()[1] + 1) << tr("%1").arg(m_childData.imgData->GetExtent()[3] + 1)
		<< tr("%1").arg(manualMeanFreeBeamIntensity) << tr("%1").arg(manualMeanFreeBeamIntensityValue);
		
	dlg_commoninput dlg(m_mainWnd, "Free Beam Intensity", inList, inPara, NULL);
	dlg.connectMdiChild(origChild);
	dlg.setModal(false);
	dlg.show();
	origChild->activate(MdiChild::cs_ROI);
	origChild->setROI(eiIndexX, eiIndexY, 0.0,
		m_childData.imgData->GetExtent()[1] + 1,
		m_childData.imgData->GetExtent()[3] + 1,
		m_childData.imgData->GetExtent()[5] + 1);
	origChild->showROI();
	int result = dlg.exec();
	if (!m_mainWnd->isVisible() || m_childClosed)	// main window  or mdi child was closed in the meantime
		return;
	origChild->hideROI();
	origChild->deactivate();
	if (result != QDialog::Accepted)
		return;
	
	eiIndexX = dlg.getIntValue(0);
	eiIndexY = dlg.getIntValue(1);
	eiIndexZ = 0.0;
	eiSizeX = dlg.getIntValue(2);
	eiSizeY = dlg.getIntValue(3);
	eiSizeZ = m_childData.imgData->GetExtent()[5] + 1;
	manualMeanFreeBeamIntensity = dlg.getCheckValue(4);
	manualMeanFreeBeamIntensityValue = dlg.getDblValue(5);

	settings.setValue("Tools/AstraReconstruction/manualMeanFreeBeamIntensity", manualMeanFreeBeamIntensity);
	settings.setValue("Tools/AstraReconstruction/manualMeanFreeBeamIntensityValue", manualMeanFreeBeamIntensityValue);

	//prepare
	QString filterName = "Free Beam Intensity";
	// at the moment, PrepareResultChild always takes the active child, but that might have changed
	m_mdiChild = m_mainWnd->GetResultChild(origChild, filterName + " " + origChild->windowTitle());
	if (!m_mdiChild)
	{
		m_mainWnd->statusBar()->showMessage("Cannot get result child from main window!", 5000);
		return;
	}
	m_mdiChild->addStatusMsg(filterName);
	UpdateChildData();
	//execute
	m_mdiChild->setUpdateSliceIndicator(true);
	iAFreeBeamCalculation* thread = new iAFreeBeamCalculation(filterName, FREEBEAMCALCULATION,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(thread);
	thread->setEParameters(eiIndexX, eiIndexY, eiIndexZ, eiSizeX, eiSizeY, eiSizeZ, manualMeanFreeBeamIntensity, manualMeanFreeBeamIntensityValue);
	thread->start();
	m_mainWnd->statusBar()->showMessage(filterName, 5000);
}
