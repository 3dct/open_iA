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

#include "mainwindow.h"
#include "mdichild.h"
#include "iAToolsVTK.h"

#define ASTRA_CUDA
#include <astra/AstraObjectManager.h>
#include <astra/CudaForwardProjectionAlgorithm3D.h>
#include <astra/CudaProjector3D.h>

#include <vtkImageCast.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkSmartPointer.h>

#include <windows.h>

void iAAstraReconstructionModuleInterface::Initialize( )
{
	QMenu* toolsMenu = m_mainWnd->getToolsMenu( );
	QMenu * astraReconMenu = getMenuWithTitle( toolsMenu, QString( "Astra Reconstruction" ), false );
	
	QAction * actionForwardProject = new QAction( m_mainWnd );
	actionForwardProject->setText( QApplication::translate( "MainWindow", "Forward Projection", 0 ) );
	AddActionToMenuAlphabeticallySorted(astraReconMenu, actionForwardProject, true);
	connect(actionForwardProject, SIGNAL(triggered()), this, SLOT(ForwardProject()));
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

	const int DetectorRowCount = 200;
	const int DetectorColCount = 200;
	const int ProjectionAngles = 180;
	QString ProjectionAngleString("0");
	for (int i = 1; i < ProjectionAngles; ++i)
	{
		ProjectionAngleString += ","+QString::number(i);

	}
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
	projGeomNode.addAttribute("type", "cone");
	projGeomNode.addChildNode("DetectorSpacingX", 1);
	projGeomNode.addChildNode("DetectorSpacingY", 1);
	projGeomNode.addChildNode("DetectorRowCount", DetectorRowCount);
	projGeomNode.addChildNode("DetectorColCount", DetectorColCount);
	projGeomNode.addChildNode("ProjectionAngles", ProjectionAngleString.toStdString());
	projGeomNode.addChildNode("DistanceOriginDetector", 0);
	projGeomNode.addChildNode("DistanceOriginSource", 1);

	astra::XMLNode volGeomNode = projectorConfig.self.addChildNode("VolumeGeometry");
	volGeomNode.addChildNode("GridColCount", dim[0]);
	volGeomNode.addChildNode("GridRowCount", dim[1]);
	volGeomNode.addChildNode("GridSliceCount", dim[2]);
	/* optional:
	WindowMinX, WindowMaxX, WindowMinY
	WindowMaxY, WindowMinZ, WindowMaxZ
	*/


	astra::CCudaProjector3D* projector = new astra::CCudaProjector3D();
	projector->initialize(projectorConfig);

	astra::CFloat32ProjectionData3DMemory * projectionData = new astra::CFloat32ProjectionData3DMemory(projector->getProjectionGeometry());
	astra::CFloat32VolumeData3DMemory * volumeData = new astra::CFloat32VolumeData3DMemory(projector->getVolumeGeometry(), static_cast<astra::float32*>(float32Img->GetScalarPointer()));

	astra::CCudaForwardProjectionAlgorithm3D* algorithm = new astra::CCudaForwardProjectionAlgorithm3D();

	
	//int id = astra::CData3DManager::getSingleton().store();

	algorithm->initialize(projector, projectionData, volumeData);
	algorithm->run();
	
	// 
	int projDim[3] = { DetectorColCount, DetectorRowCount, ProjectionAngles };
	double projSpacing[3] = { 1.0, 1.0, 1.0 };
	auto projImg = AllocateImage(VTK_FLOAT, projDim, projSpacing);
	//astra::CAlgorithmManager::getSingleton().store(algorithm);

	FOR_VTKIMG_PIXELS(projImg, x, y, z)
	{
		//int index = x + y*DetectorColCount + z*DetectorColCount*DetectorRowCount;
		projImg->SetScalarComponentFromFloat(x, y, z, 0, projectionData->getData3D()[z][y][x]);
	}
	MdiChild* resultChild = m_mainWnd->GetResultChild("");
	resultChild->setImageData("Astra Forward Projection", projImg);
	resultChild->update();
}
