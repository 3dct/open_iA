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

#include "iAConsole.h"
#include "mainwindow.h"
#include "mdichild.h"
#include "iAToolsVTK.h"
#include "dlg_commoninput.h"

#define ASTRA_CUDA
#include <astra/AstraObjectManager.h>
#include <astra/CudaForwardProjectionAlgorithm3D.h>
#include <astra/CudaProjector3D.h>
#include <astra/CudaBackProjectionAlgorithm3D.h>

#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <QSettings>
#include <QtMath>

#include <qdebug.h>

#include <windows.h>

void iAAstraReconstructionModuleInterface::Initialize( )
{
	QMenu* toolsMenu = m_mainWnd->getToolsMenu( );
	QMenu * astraReconMenu = getMenuWithTitle( toolsMenu, QString( "Astra Reconstruction" ), false );
	
	QAction * actionForwardProject = new QAction( m_mainWnd );
	QAction * actionBackProject = new QAction(m_mainWnd);
	actionForwardProject->setText( QApplication::translate( "MainWindow", "Forward Projection", 0 ) );
	actionBackProject->setText(QApplication::translate("MainWindow", "Back Projection", 0));
	AddActionToMenuAlphabeticallySorted(astraReconMenu, actionForwardProject, true);
	AddActionToMenuAlphabeticallySorted(astraReconMenu, actionBackProject, true);
	
	connect(actionForwardProject, SIGNAL(triggered()), this, SLOT(ForwardProject()));
	connect(actionBackProject, SIGNAL(triggered()), this, SLOT(BackProject()));
}

void iAAstraReconstructionModuleInterface::ForwardProject( )
{
	AllocConsole();
	freopen("CON", "w", stdout);

	MdiChild* child = m_mainWnd->activeMdiChild();

	vtkSmartPointer<vtkImageData> img = child->getImageData();
	int const * const dim = img->GetDimensions();

	vtkNew<vtkImageCast> cast;
	cast->SetInputData(img);
	cast->SetOutputScalarTypeToFloat();
	cast->Update();
	vtkSmartPointer<vtkImageData> float32Img = cast->GetOutput();

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
	
	//QString ProjectionAngleString("0");
	//for (int i = 1; i < projAngles; ++i)
	//{
	//	ProjectionAngleString += ","+QString::number(i);
	//}
	astra::Config projectorConfig;
	projectorConfig.initialize("Projector3D");
	astra::XMLNode gpuIndexOption = projectorConfig.self.addChildNode("Option");
	gpuIndexOption.addAttribute("key", "GPUIndex");
	gpuIndexOption.addAttribute("value", "0");
	/*
	// further (optional) "Option"s (as GPUIndex):
	"ProjectionKernel"
	"VoxelSuperSampling"
	"DetectorSuperSampling"
	"DensityWeighting"
	*/

	astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
	projGeomNode.addAttribute("type", projGeomType.toStdString());
	projGeomNode.addChildNode("DetectorSpacingX", detSpacingX);
	projGeomNode.addChildNode("DetectorSpacingY", detSpacingY);
	projGeomNode.addChildNode("DetectorRowCount", detRowCnt);
	projGeomNode.addChildNode("DetectorColCount", detColCnt);
	projGeomNode.addChildNode("ProjectionAngles", linspace(qDegreesToRadians(projAngleStart),
		qDegreesToRadians(projAngleEnd), projAnglesCount).toStdString());
	projGeomNode.addChildNode("DistanceOriginDetector", distOrigDet);
	projGeomNode.addChildNode("DistanceOriginSource", distOrigSource);

	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	volGeomNode.addChildNode("GridColCount", dim[0]);
	volGeomNode.addChildNode("GridRowCount", dim[1]);
	volGeomNode.addChildNode("GridSliceCount", dim[2]);

	astra::XMLNode winMinXOption = volGeomNode.addChildNode("Option");
	winMinXOption.addAttribute("key", "WindowMinX");
	winMinXOption.addAttribute("value", -dim[0] * img->GetSpacing()[0]/2.0);
	astra::XMLNode winMaxXOption = volGeomNode.addChildNode("Option");
	winMaxXOption.addAttribute("key", "WindowMaxX");
	winMaxXOption.addAttribute("value", dim[0] * img->GetSpacing()[0] / 2.0);

	astra::XMLNode winMinYOption = volGeomNode.addChildNode("Option");
	winMinYOption.addAttribute("key", "WindowMinY");
	winMinYOption.addAttribute("value", -dim[1] * img->GetSpacing()[1] / 2.0);
	astra::XMLNode winMaxYOption = volGeomNode.addChildNode("Option");
	winMaxYOption.addAttribute("key", "WindowMaxY");
	winMaxYOption.addAttribute("value", dim[1] * img->GetSpacing()[1] / 2.0);

	astra::XMLNode winMinZOption = volGeomNode.addChildNode("Option");
	winMinZOption.addAttribute("key", "WindowMinZ");
	winMinZOption.addAttribute("value", -dim[2] * img->GetSpacing()[2] / 2.0);
	astra::XMLNode winMaxZOption = volGeomNode.addChildNode("Option");
	winMaxZOption.addAttribute("key", "WindowMaxZ");
	winMaxZOption.addAttribute("value", dim[2] * img->GetSpacing()[2] / 2.0);

	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry());
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), static_cast<astra::float32*>(float32Img->GetScalarPointer()));
	astra::CCudaForwardProjectionAlgorithm3D* algorithm = new astra::CCudaForwardProjectionAlgorithm3D();

	DEBUG_LOG(QString("Dimensions: %1x%2x%3").arg(projectionData->getWidth()).arg(projectionData->getHeight()).arg(projectionData->getDepth()));

	algorithm->initialize(projector, projectionData, volumeData);
	algorithm->run();
	int projDim[3] = { detRowCnt, detColCnt, projAnglesCount };
	double projSpacing[3] = { detSpacingX, detSpacingY, detSpacingX };
	auto projImg = AllocateImage(VTK_FLOAT, projDim, projSpacing);

	FOR_VTKIMG_PIXELS(projImg, x, y, z)
	{
		projImg->SetScalarComponentFromFloat(x, y, z, 0, projectionData->getData3D()[y][z][x]);
	}
	MdiChild* resultChild = m_mainWnd->GetResultChild("");
	resultChild->setImageData("Astra Forward Projection", projImg);
	resultChild->update();

	delete algorithm;
	delete volumeData;
	delete projectionData;
	delete projector;
}

namespace
{
	QStringList GetDimStringList(int selectedDim)
	{
		assert(selectedDim >= 0 && selectedDim <= 2);
		return QStringList()
			<< ((selectedDim == 0) ? "!x" : "x")
			<< ((selectedDim == 1) ? "!y" : "y")
			<< ((selectedDim == 2) ? "!z" : "z");
	}
}

void iAAstraReconstructionModuleInterface::BackProject()
{
	AllocConsole();
	freopen("CON", "w", stdout);

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
		<< GetDimStringList(detRowDim) << GetDimStringList(detColDim) << GetDimStringList(projAngleDim)
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
		DEBUG_LOG("One of the axes (x, y, z) has been specified for more than one usage out of (detector row / detector column / projection angle) dimensions. "
			"Make sure each axis is used exactly for one dimension!");
		return;
	}

	vtkNew<vtkImageCast> cast;
	cast->SetInputData(img);
	cast->SetOutputScalarTypeToFloat();
	cast->Update();
	vtkSmartPointer<vtkImageData> float32Img = cast->GetOutput();

	float* buf = new float[ dim[0] * dim[1] * dim[2] ];
	detRowCnt = dim[detRowDim];
	detColCnt = dim[detColDim];
	projAnglesCount = dim[projAngleDim];
	FOR_VTKIMG_PIXELS(img, x, y, z)
	{
		int detCol = (detColDim == 0) ? x : (detColDim == 1) ? y : z;
		int detRow = (detRowDim == 0) ? x : (detRowDim == 1) ? y : z;
		int projAngle = (projAngleDim == 0) ? x : (projAngleDim == 1) ? y : z;
		int index = detCol + projAngle*detColCnt + detRow*detColCnt*projAnglesCount;
		buf[index] = img->GetScalarComponentAsFloat(x, y, z, 0);
	}

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
	
	astra::Config projectorConfig;
	projectorConfig.initialize("Projector3D");
	astra::XMLNode gpuIndexOption = projectorConfig.self.addChildNode("Option");
	gpuIndexOption.addAttribute("key", "GPUIndex");
	gpuIndexOption.addAttribute("value", "0");

	astra::XMLNode projGeomNode = projectorConfig.self.addChildNode("ProjectionGeometry");
	projGeomNode.addAttribute("type", projGeomType.toStdString());
	projGeomNode.addChildNode("DetectorSpacingX", detSpacingX);
	projGeomNode.addChildNode("DetectorSpacingY", detSpacingY);
	projGeomNode.addChildNode("DetectorRowCount", detRowCnt);
	projGeomNode.addChildNode("DetectorColCount", detColCnt);
	projGeomNode.addChildNode("ProjectionAngles", linspace(qDegreesToRadians(projAngleStart),
		qDegreesToRadians(projAngleEnd), projAnglesCount).toStdString());
	projGeomNode.addChildNode("DistanceOriginDetector", distOrigDet);
	projGeomNode.addChildNode("DistanceOriginSource", distOrigSource);

	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	volGeomNode.addChildNode("GridColCount", 128);
	volGeomNode.addChildNode("GridRowCount", 128);
	volGeomNode.addChildNode("GridSliceCount", 128);

	astra::XMLNode winMinXOption = volGeomNode.addChildNode("Option");
	winMinXOption.addAttribute("key", "WindowMinX");
	winMinXOption.addAttribute("value", -volDim[0] * volSpacing[0]/2.0);
	astra::XMLNode winMaxXOption = volGeomNode.addChildNode("Option");
	winMaxXOption.addAttribute("key", "WindowMaxX");
	winMaxXOption.addAttribute("value", volDim[0] * volSpacing[0] / 2.0);

	astra::XMLNode winMinYOption = volGeomNode.addChildNode("Option");
	winMinYOption.addAttribute("key", "WindowMinY");
	winMinYOption.addAttribute("value", -volDim[1] * volSpacing[1] / 2.0);
	astra::XMLNode winMaxYOption = volGeomNode.addChildNode("Option");
	winMaxYOption.addAttribute("key", "WindowMaxY");
	winMaxYOption.addAttribute("value", volDim[1] * volSpacing[1] / 2.0);

	astra::XMLNode winMinZOption = volGeomNode.addChildNode("Option");
	winMinZOption.addAttribute("key", "WindowMinZ");
	winMinZOption.addAttribute("value", -volDim[2] * volSpacing[2] / 2.0);
	astra::XMLNode winMaxZOption = volGeomNode.addChildNode("Option");
	winMaxZOption.addAttribute("key", "WindowMaxZ");
	winMaxZOption.addAttribute("value", volDim[2] * volSpacing[2] / 2.0);

	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);
	
	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry(), static_cast<astra::float32*>(buf));
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry() );

	DEBUG_LOG(QString("Dimensions: %1x%2x%3").arg(volumeData->getWidth()).arg(volumeData->getHeight()).arg(volumeData->getDepth()));

	astra::CCudaBackProjectionAlgorithm3D* algorithm = new astra::CCudaBackProjectionAlgorithm3D();
	algorithm->initialize(projector, projectionData, volumeData);
	algorithm->run();

	auto volImg = AllocateImage(VTK_FLOAT, volDim, volSpacing);

	FOR_VTKIMG_PIXELS(volImg, x, y, z)
	{
		volImg->SetScalarComponentFromFloat(x, y, z, 0, volumeData->getData3D()[z][y][x]);
	}
	MdiChild* resultChild = m_mainWnd->GetResultChild("");
	resultChild->setImageData("Astra Back Projection", volImg);
	resultChild->update();

	delete[] buf;
	delete algorithm;
	delete volumeData;
	delete projectionData;
	delete projector;
}

QString iAAstraReconstructionModuleInterface::linspace(double projAngleStart, double projAngleEnd, int projAnglesCount)
{
	QString result;
	for (int i = 0; i <= projAnglesCount - 2; i++)
	{
		double temp = projAngleStart + i*(projAngleEnd - projAngleStart) / (floor((double)projAnglesCount) - 1);
		result.append(QString::number(temp) + ",");
	}
	result.append( QString::number(projAngleEnd));
	return result;
}