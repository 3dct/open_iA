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

#include "iAAstraAlgorithm.h"
#include "mainwindow.h"
#include "mdichild.h"
#include "dlg_commoninput.h"

#include <vtkImageData.h>

#include <QSettings>

//#include <windows.h>

namespace
{
	QStringList GetDimStringList(int selectedDim, int const imgDims[3])
	{
		assert(selectedDim >= 0 && selectedDim <= 2);
		return QStringList()
			<< QString("%1x (%2)").arg((selectedDim == 0) ? "!" : "").arg(imgDims[0])
			<< QString("%1y (%2)").arg((selectedDim == 1) ? "!" : "").arg(imgDims[1])
			<< QString("%1z (%2)").arg((selectedDim == 2) ? "!" : "").arg(imgDims[2]);
	}
}


void iAAstraReconstructionModuleInterface::Initialize( )
{
	QMenu* toolsMenu = m_mainWnd->getToolsMenu( );
	QMenu * astraReconMenu = getMenuWithTitle( toolsMenu, QString( "Astra Reconstruction" ), false );
	
	QAction * actionForwardProject = new QAction( m_mainWnd );
	actionForwardProject->setText( QApplication::translate( "MainWindow", "Forward Projection", 0 ) );
	AddActionToMenuAlphabeticallySorted(astraReconMenu, actionForwardProject, true);
	connect(actionForwardProject, SIGNAL(triggered()), this, SLOT(ForwardProject()));

	QAction * actionBackProject = new QAction(m_mainWnd);
	actionBackProject->setText(QApplication::translate("MainWindow", "Back Projection", 0));
	AddActionToMenuAlphabeticallySorted(astraReconMenu, actionBackProject, true);
	connect(actionBackProject, SIGNAL(triggered()), this, SLOT(BackProject()));
}


void iAAstraReconstructionModuleInterface::ForwardProject()
{
	// ask for and store settings:
	QSettings settings;
	projGeomType = settings.value("Tools/AstraReconstruction/ForwardProjection/projGeomType").toString();
	detSpacingX = settings.value("Tools/AstraReconstruction/ForwardProjection/detSpacingX").toDouble();
	detSpacingY = settings.value("Tools/AstraReconstruction/ForwardProjection/detSpacingY").toDouble();
	detRowCnt = settings.value("Tools/AstraReconstruction/ForwardProjection/detRowCnt").toInt();
	detColCnt = settings.value("Tools/AstraReconstruction/ForwardProjection/detColCnt").toInt();
	projAngleStart = settings.value("Tools/AstraReconstruction/ForwardProjection/projAngleStart").toDouble();
	projAngleEnd = settings.value("Tools/AstraReconstruction/ForwardProjection/projAngleEnd").toDouble();
	projAnglesCount = settings.value("Tools/AstraReconstruction/ForwardProjection/projAnglesCount").toInt();
	distOrigDet = settings.value("Tools/AstraReconstruction/ForwardProjection/distOrigDet").toDouble();
	distOrigSource = settings.value("Tools/AstraReconstruction/ForwardProjection/distOrigSource").toDouble();
	QStringList inList = (QStringList() << tr("+Projection Geometry Type")
		<< tr("^Detector Spacing X") << tr("^Detector Spacing Y")
		<< tr("*Detector Row Count") << tr("*Detector Column Count")
		<< tr("^Projection Angle Start [°]") << tr("^Projection Angle End [°]")
		<< tr("*Number of Projections")
		<< tr("^Distance Origin Detector") << tr("^Distance Origin Source"));
	const QStringList projectionGeometryTypes = QStringList() << QString("!") + "cone";
	QList<QVariant> inPara; 	inPara << projectionGeometryTypes << tr("%1").arg(detSpacingX) << tr("%1").arg(detSpacingY) << tr("%1").arg(detRowCnt)
		<< tr("%1").arg(detColCnt) << tr("%1").arg(projAngleStart) << tr("%1").arg(projAngleEnd) << tr("%1").arg(projAnglesCount)
		<< tr("%1").arg(distOrigDet) << tr("%1").arg(distOrigSource);
	dlg_commoninput dlg(m_mainWnd, "Forward Projection", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;
	projGeomType = dlg.getComboBoxValues()[0];
	detSpacingX = dlg.getDoubleSpinBoxValues()[1];
	detSpacingY = dlg.getDoubleSpinBoxValues()[2];
	detRowCnt = dlg.getSpinBoxValues()[3];
	detColCnt = dlg.getSpinBoxValues()[4];
	projAngleStart = dlg.getDoubleSpinBoxValues()[5];
	projAngleEnd = dlg.getDoubleSpinBoxValues()[6];
	projAnglesCount = dlg.getSpinBoxValues()[7];
	distOrigDet = dlg.getDoubleSpinBoxValues()[8];
	distOrigSource = dlg.getDoubleSpinBoxValues()[9];
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/projGeomType", projGeomType);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/detSpacingX", detSpacingX);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/detSpacingY", detSpacingY);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/detRowCnt", detRowCnt);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/detColCnt", detColCnt);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/projAngleStart", projAngleStart);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/projAngleEnd", projAngleEnd);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/projAnglesCount", projAnglesCount);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/distOrigDet", distOrigDet);
	settings.setValue("Tools/AstraReconstruction/ForwardProjection/distOrigSource", distOrigSource);
	
	// start forward projection filter:
	QString filterName = "Forward Projection";
	PrepareResultChild(filterName);
	iAAstraAlgorithm* fwdProjection = new iAAstraAlgorithm(iAAstraAlgorithm::ForwardProjection, filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(fwdProjection);
	fwdProjection->SetFwdProjectionParams(projGeomType, detSpacingX, detSpacingY, detRowCnt, detColCnt, projAngleStart, projAngleEnd, projAnglesCount, distOrigDet, distOrigSource);
	fwdProjection->start();
	m_mdiChild->addStatusMsg(filterName);
	m_mainWnd->statusBar()->showMessage(filterName, 10000);
}


void iAAstraReconstructionModuleInterface::BackProject()
{
	// ask for and store settings:
	MdiChild* child = m_mainWnd->activeMdiChild();
	vtkSmartPointer<vtkImageData> img = child->getImageData();
	int const * const dim = img->GetDimensions();
	QSettings settings;
	projGeomType = settings.value("Tools/AstraReconstruction/BackProjection/projGeomType").toString();
	detSpacingX = settings.value("Tools/AstraReconstruction/BackProjection/detSpacingX").toDouble();
	detSpacingY = settings.value("Tools/AstraReconstruction/BackProjection/detSpacingY").toDouble();
	detRowDim = settings.value("Tools/AstraReconstruction/BackProjection/detRowDim", 1).toInt();
	detColDim = settings.value("Tools/AstraReconstruction/BackProjection/detColDim", 0).toInt();
	projAngleDim = settings.value("Tools/AstraReconstruction/BackProjection/projAngleDim", 2).toInt();
	projAngleStart = settings.value("Tools/AstraReconstruction/BackProjection/projAngleStart").toDouble();
	projAngleEnd = settings.value("Tools/AstraReconstruction/BackProjection/projAngleEnd").toDouble();
	distOrigDet = settings.value("Tools/AstraReconstruction/BackProjection/distOrigDet").toDouble();
	distOrigSource = settings.value("Tools/AstraReconstruction/BackProjection/distOrigSource").toDouble();
	volDim[0] = settings.value("Tools/AstraReconstruction/BackProjection/volumeDimX").toInt();
	volDim[1] = settings.value("Tools/AstraReconstruction/BackProjection/volumeDimY").toInt();
	volDim[2] = settings.value("Tools/AstraReconstruction/BackProjection/volumeDimZ").toInt();
	volSpacing[0] = settings.value("Tools/AstraReconstruction/BackProjection/volumeSpacingX", 1).toDouble();
	volSpacing[1] = settings.value("Tools/AstraReconstruction/BackProjection/volumeSpacingY", 1).toDouble();
	volSpacing[2] = settings.value("Tools/AstraReconstruction/BackProjection/volumeSpacingZ", 1).toDouble();
	QStringList inList = (QStringList() << tr("+Projection Geometry Type")
		<< tr("^Detector Spacing X") << tr("^Detector Spacing Y")
		<< tr("+Detector Row Dimension") << tr("+Detector Column Dimension")
		<< tr("+Projection Dimension")
		<< tr("^Projection Angle Start [°]") << tr("^Projection Angle End [°]")
		<< tr("^Distance Origin Detector") << tr("^Distance Origin Source")
		<< tr("*Volume Width") << tr("*Volume Height") << tr("*Volume Depth")
		<< tr("^Volume Spacing X") << tr("^Volume Spacing Y") << tr("^Volume Spacing Z"));
	const QStringList projectionGeometryTypes = QStringList() << QString("!") + "cone";
	QList<QVariant> inPara; 	inPara << projectionGeometryTypes
		<< tr("%1").arg(detSpacingX) << tr("%1").arg(detSpacingY)
		<< GetDimStringList(detRowDim, dim) << GetDimStringList(detColDim, dim) << GetDimStringList(projAngleDim, dim)
		<< tr("%1").arg(projAngleStart) << tr("%1").arg(projAngleEnd)
		<< tr("%1").arg(distOrigDet) << tr("%1").arg(distOrigSource)
		<< tr("%1").arg(volDim[0]) << tr("%1").arg(volDim[1]) << tr("%1").arg(volDim[2])
		<< tr("%1").arg(volSpacing[0]) << tr("%1").arg(volSpacing[1]) << tr("%1").arg(volSpacing[2]);
	dlg_commoninput dlg(m_mainWnd, "Back Projection", inList, inPara, NULL);
	if (dlg.exec() != QDialog::Accepted)
		return;
	projGeomType = dlg.getComboBoxValues()[0];
	detSpacingX = dlg.getDoubleSpinBoxValues()[1];
	detSpacingY = dlg.getDoubleSpinBoxValues()[2];
	detRowDim = dlg.getComboBoxIndices()[3];
	detColDim = dlg.getComboBoxIndices()[4];
	projAngleDim = dlg.getComboBoxIndices()[5];
	projAngleStart = dlg.getDoubleSpinBoxValues()[6];
	projAngleEnd = dlg.getDoubleSpinBoxValues()[7];
	distOrigDet = dlg.getDoubleSpinBoxValues()[8];
	distOrigSource = dlg.getDoubleSpinBoxValues()[9];
	volDim[0] = dlg.getSpinBoxValues()[10];
	volDim[1] = dlg.getSpinBoxValues()[11];
	volDim[2] = dlg.getSpinBoxValues()[12];
	volSpacing[0] = dlg.getDoubleSpinBoxValues()[13];
	volSpacing[1] = dlg.getDoubleSpinBoxValues()[14];
	volSpacing[2] = dlg.getDoubleSpinBoxValues()[15];
	if (detColDim == detRowDim || detColDim == projAngleDim || detRowDim == projAngleDim)
	{
		child->addMsg("One of the axes (x, y, z) has been specified for more than one usage out of (detector row / detector column / projection angle) dimensions. "
			"Make sure each axis is used exactly for one dimension!");
		return;
	}
	detRowCnt = dim[detRowDim];
	detColCnt = dim[detColDim];
	projAnglesCount = dim[projAngleDim];
	settings.setValue("Tools/AstraReconstruction/BackProjection/projGeomType", projGeomType);
	settings.setValue("Tools/AstraReconstruction/BackProjection/detSpacingX", detSpacingX);
	settings.setValue("Tools/AstraReconstruction/BackProjection/detSpacingY", detSpacingY);
	settings.setValue("Tools/AstraReconstruction/BackProjection/detRowDim", detRowDim);
	settings.setValue("Tools/AstraReconstruction/BackProjection/detColDim", detColDim);
	settings.setValue("Tools/AstraReconstruction/BackProjection/projAngleDim", projAngleDim);
	settings.setValue("Tools/AstraReconstruction/BackProjection/projAngleStart", projAngleStart);
	settings.setValue("Tools/AstraReconstruction/BackProjection/projAngleEnd", projAngleEnd);
	settings.setValue("Tools/AstraReconstruction/BackProjection/distOrigDet", distOrigDet);
	settings.setValue("Tools/AstraReconstruction/BackProjection/distOrigSource", distOrigSource);
	settings.setValue("Tools/AstraReconstruction/BackProjection/volumeDimX", volDim[0]);
	settings.setValue("Tools/AstraReconstruction/BackProjection/volumeDimY", volDim[1]);
	settings.setValue("Tools/AstraReconstruction/BackProjection/volumeDimZ", volDim[2]);
	settings.setValue("Tools/AstraReconstruction/BackProjection/volumeSpacingX", volSpacing[0]);
	settings.setValue("Tools/AstraReconstruction/BackProjection/volumeSpacingY", volSpacing[1]);
	settings.setValue("Tools/AstraReconstruction/BackProjection/volumeSpacingZ", volSpacing[2]);
	
	// start back projection filter:
	QString filterName = "Filtered Back-Projection";
	PrepareResultChild(filterName);
	iAAstraAlgorithm* backProjection = new iAAstraAlgorithm(iAAstraAlgorithm::FilteredBackProjection, filterName,
		m_childData.imgData, m_childData.polyData, m_mdiChild->getLogger(), m_mdiChild);
	m_mdiChild->connectThreadSignalsToChildSlots(backProjection);
	backProjection->SetFBPParams(projGeomType, detSpacingX, detSpacingY, detRowCnt, detColCnt, projAngleStart, projAngleEnd, projAnglesCount, distOrigDet, distOrigSource,
		detRowDim, detColDim, projAngleDim, volDim, volSpacing);
	backProjection->start();
	m_mdiChild->addStatusMsg(filterName);
	m_mainWnd->statusBar()->showMessage(filterName, 10000);
}