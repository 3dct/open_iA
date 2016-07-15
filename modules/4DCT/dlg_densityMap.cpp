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
#include "dlg_densityMap.h"
// iA
#include "mainwindow.h"
#include "mdichild.h"
#include "iACalculateDensityMap.h"
#include "iAChannelID.h"
#include "iAChannelVisualizationData.h"
#include "iARenderer.h"
#include "iASlicerData.h"
// Qt
#include <QString>
#include <QStringList>
//#include <QPointer>
// vtk
#include <vtkImageData.h>
#include <vtkImageExtractComponents.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

dlg_densityMap::dlg_densityMap(MainWindow* mWindow, MdiChild* mdiChild) :
	QDockWidget(mWindow)
{
	setupUi(this);

	m_mainWindow = mWindow;
	m_mdiChild = mdiChild;

	QString defaultText =
		QString("C:\\data\\iAnalyse\\datasets\\roi0\\results\\pullouts.mhd\n") +
		QString("C:\\data\\iAnalyse\\datasets\\roi1\\results\\pullouts.mhd\n") +
		QString("C:\\data\\iAnalyse\\datasets\\roi2\\results\\pullouts.mhd\n") +
		QString("C:\\data\\iAnalyse\\datasets\\roi3\\results\\pullouts.mhd");

	this->labelImageTextEdit->setText(defaultText);

	connect(this->loadButton, SIGNAL(clicked()), this, SLOT(loadImages()));
	connect(this->calcButton, SIGNAL(clicked()), this, SLOT(calcDensityMap()));
}

dlg_densityMap::~dlg_densityMap()
{

}

void dlg_densityMap::loadImages()
{
	// load files
	QString text = this->labelImageTextEdit->toPlainText();
	QStringList sList = text.split('\n', QString::SplitBehavior::SkipEmptyParts);
	for (auto iFileName = begin(sList); iFileName != end(sList); iFileName++)
	{
		MdiChild* mdiChild = m_mainWindow->createMdiChild();
		mdiChild->loadFile(*iFileName, false);
		m_lMdiChilds.push_back(mdiChild);
	}
}

void dlg_densityMap::calcDensityMap()
{
	// calculate density maps
	int numOfFiles = m_lMdiChilds.size();
	typedef double TDensityMapPixel;

	typedef std::vector<std::vector<std::vector<double>>> TDensityValues;
	std::vector<TDensityValues> lDensity;
	for (int i = 0; i < numOfFiles; i++)
	{
		TDensityValues density;
		//density = CalculateDensityMap<double, unsigned short>::Calculate(m_lMdiChilds[0]->getImageData(), 25);
		lDensity.push_back(density);
	}

	TDensityValues density = lDensity[0];
	densityMap = vtkSmartPointer<vtkImageData>::New();
	densityMap->SetExtent(0, density.size() - 1, 0, density[0].size(), 0, density[0][0].size());
	densityMap->SetSpacing(50, 50, 50);
	densityMap->AllocateScalars(VTK_DOUBLE, numOfFiles);

	// fill the density map image
	for (int densityInd = 0; densityInd < numOfFiles; densityInd++)
	{
		for (int x = 0; x < density.size(); x++)
		{
			for (int y = 0; y < density[x].size(); y++)
			{
				for (int z = 0; z < density[x][y].size(); z++)
				{
					densityMap->SetScalarComponentFromDouble(x, y, z, densityInd, lDensity[densityInd][x][y][z]);
				}
			}
		}
	}

	InitChannel();

	//vtkSmartPointer<vtkImageExtractComponents> extractor =
	//	vtkSmartPointer<vtkImageExtractComponents>::New();
	//extractor->SetInputData(densityMap);
	//extractor->SetComponents(0);
	//extractor->Update();

	//vtkImageData* mdiImage = m_mdiChild->getImageData();
	//mdiImage->SetExtent(extractor->GetOutput()->GetExtent());
	//mdiImage->AllocateScalars(extractor->GetOutput()->GetScalarType(), 
	//	extractor->GetOutput()->GetNumberOfScalarComponents());
	//mdiImage->DeepCopy(extractor->GetOutput());
}

void dlg_densityMap::InitChannel()
{
	iAChannelID chId = iAChannelID::ch_DensityMap;
	iAChannelVisualizationData* chDensity = new iAChannelVisualizationData();
	m_mdiChild->InsertChannelData(chId, chDensity);

	vtkSmartPointer<vtkImageExtractComponents> extractor =
		vtkSmartPointer<vtkImageExtractComponents>::New();
	extractor->SetInputData(densityMap);
	extractor->SetComponents(0);
	extractor->Update();

	vtkImageData* image = extractor->GetOutput();
	double range[2];
	image->GetScalarRange(range);
	//image->SetSpacing(densityMap->GetSpacing());

	vtkSmartPointer<vtkColorTransferFunction> ctf =
		vtkSmartPointer<vtkColorTransferFunction>::New();
	ctf->AddRGBPoint(range[0], 0, 0, 0);
	ctf->AddRGBPoint(range[1], 1, 1, 1);

	vtkSmartPointer<vtkPiecewiseFunction> otf =
		vtkSmartPointer<vtkPiecewiseFunction>::New();
	otf->AddPoint(range[0], 0.5);
	otf->AddPoint(range[1], 0.5);


	chDensity->SetActiveImage(image);
	chDensity->SetColorTF(ctf);
	chDensity->SetOpacityTF(otf);

	m_mdiChild->InitChannelRenderer(chId, true);
	m_mdiChild->UpdateChannelSlicerOpacity(chId, 0.5);
	/*
	// rewrite using new volume
	m_mdiChild->getRenderer()->showMainVolumeWithChannels(false);
	m_mdiChild->getRenderer()->updateChannelImages();
	*/
	m_mdiChild->getSlicerDataXY()->updateChannelMappers();
	m_mdiChild->getSlicerDataXZ()->updateChannelMappers();
	m_mdiChild->getSlicerDataYZ()->updateChannelMappers();
	m_mdiChild->updateViews();
}
