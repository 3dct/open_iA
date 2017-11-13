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
#include "dlg_modalitySPLOM.h"

#include "iAChannelVisualizationData.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "charts/iAQSplom.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QHBoxLayout>
#include <QTableWidget>
#include <mdichild.h>

#include <functional>

dlg_modalitySPLOM::dlg_modalitySPLOM():
	m_splom(new iAQSplom(this)),
	m_voxelData(new QTableWidget()),
	m_selection_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_selection_otf(vtkSmartPointer<vtkPiecewiseFunction>::New())
{
	m_selection_ctf->AddRGBPoint(0, 0, 0, 0);
	m_selection_ctf->AddRGBPoint(1, 1.0, 0.0, 0.0);

	m_selection_otf->AddPoint(0, 0);
	m_selection_otf->AddPoint(1, 1);
	setWindowTitle("Modality Correlation");
	setFeatures(DockWidgetVerticalTitleBar | DockWidgetClosable | DockWidgetMovable | DockWidgetFloatable);
	setObjectName("ModalityCorrelationWidget");

	QHBoxLayout* lay = new QHBoxLayout();
	//lay->addWidget(m_voxelData);
	lay->addWidget(m_splom);
	
	QWidget* content = new QWidget();
	content->setLayout(lay);
	setWidget(content);

	connect(m_splom, SIGNAL(selectionModified(QVector<unsigned int> *)), this, SLOT(SplomSelection(QVector<unsigned int> *)));
}

void dlg_modalitySPLOM::SplomSelection(QVector<unsigned int> * selInds)
{
	vtkSmartPointer<vtkImageData> result = vtkSmartPointer<vtkImageData>::New();
	result->SetExtent(m_extent);
	result->SetOrigin(m_origin);
	result->SetSpacing(m_spacing);
	result->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
	
	// fill with 0:
	for (int x=m_extent[0]; x<=m_extent[1]; ++x)
	{
		for (int y=m_extent[2]; y<=m_extent[3]; ++y)
		{
			for (int z=m_extent[4]; z<=m_extent[5]; ++z)
			{
				result->SetScalarComponentFromFloat(x, y, z, 0, 0);
			}
		}
	}

	// set 1 for selection:
	for (int i=0; i<selInds->size(); ++i)
	{
		int idx = selInds->at(i);
		int x = m_voxelData->item(idx, 0)->text().toInt();
		int y = m_voxelData->item(idx, 1)->text().toInt();
		int z = m_voxelData->item(idx, 2)->text().toInt();
		result->SetScalarComponentFromFloat(x, y, z, 0, 1);
	}


	MdiChild* mdiChild = dynamic_cast<MdiChild*>(parent());

	if (!m_selected)
	{
		mdiChild->SetChannelRenderingEnabled(ch_ModSPLOMSelection, false);
		m_selected = true;
		return;
	}
	
	iAChannelVisualizationData* chData = mdiChild->GetChannelData(ch_ModSPLOMSelection);
	if (!chData)
	{
		chData = new iAChannelVisualizationData();
		mdiChild->InsertChannelData(ch_ModSPLOMSelection, chData);
	}
	ResetChannel(chData, result, m_selection_ctf, m_selection_otf);

	mdiChild->InitChannelRenderer(ch_ModSPLOMSelection, true);
	mdiChild->UpdateChannelSlicerOpacity(ch_ModSPLOMSelection, 0.5);
	mdiChild->updateViews();
}


dlg_modalitySPLOM::~dlg_modalitySPLOM()
{
	delete m_voxelData;
}

typedef unsigned short VoxelValueType;

void IteratePixels(vtkSmartPointer<vtkImageData> img, const int step[3], std::function<void (int[3], VoxelValueType)> pixelVisitor)
{
	int extent[6];
	img->GetExtent(extent);
	VoxelValueType* imgData = static_cast<VoxelValueType*>(img->GetScalarPointer());
	int coord[3];

	for (coord[0]=extent[0]; coord[0]<=extent[1]; coord[0] += step[0])
	{
		for (coord[1]=extent[2]; coord[1]<=extent[3]; coord[1] += step[1])
		{
			for (coord[2]=extent[4]; coord[2]<=extent[5]; coord[2] += step[2])
			{
				int voxelIdx = img->ComputePointId(coord);
				pixelVisitor(coord, imgData[voxelIdx]);
			}
		}
	}
}

#include "iAPerformanceHelper.h"


void dlg_modalitySPLOM::SetData(QSharedPointer<iAModalityList> modalities)
{
	m_lut = vtkSmartPointer<vtkLookupTable>::New();
	double lutRange[2] = { 0, 1 };
	m_lut->SetRange( lutRange );
	m_lut->Build();
	vtkIdType lutColCnt = m_lut->GetNumberOfTableValues();
	double alpha = 0.005;
	for( vtkIdType i = 0; i < lutColCnt; i++ )
	{
		double rgba[4]; m_lut->GetTableValue( i, rgba );
		rgba[3] = alpha;
		m_lut->SetTableValue( i, rgba );
	}
	m_lut->Build();


	iATimeGuard timer("QTableWidget Conversion", false);
	// fill QTableWidget
	// TODO: implement sampling - full calculation takes too long for larger datasets!
	m_voxelData->clear();
	m_voxelData->setColumnCount(3 + modalities->size()); // x,y,z coordinate + one value per modality

	modalities->Get(0)->GetImage()->GetExtent(m_extent);
	modalities->Get(0)->GetImage()->GetSpacing(m_spacing);
	modalities->Get(0)->GetImage()->GetOrigin(m_origin);

	
	// TODO: improve this very crude regular sampling. maybe random sampling is better?
	const int maxNumSteps = 50;
	const int step[3] = {
		(m_extent[1]-m_extent[0]+1) / std::min(m_extent[1]-m_extent[0]+1, maxNumSteps),
		(m_extent[3]-m_extent[2]+1) / std::min(m_extent[3]-m_extent[2]+1, maxNumSteps),
		(m_extent[5]-m_extent[4]+1) / std::min(m_extent[5]-m_extent[4]+1, maxNumSteps)
	};
	int tableSize = (int)(((m_extent[1]-m_extent[0]+1) / step[0]) + 1) *
	                (int)(((m_extent[3]-m_extent[2]+1) / step[1]) + 1) *
	                (int)(((m_extent[5]-m_extent[4]+1) / step[2]) + 1);
	
	//std::cout << "Steps: " << step[0] << ", " << step[1] << ", " << step[2] << "; tableSize: " << tableSize;
	
	iATimeAdder adder;
	adder.resume();
	m_voxelData->setRowCount(1 + tableSize);
	adder.pause();
	m_voxelData->setItem(0, 0, new QTableWidgetItem("x"));
	m_voxelData->setItem(0, 1, new QTableWidgetItem("y"));
	m_voxelData->setItem(0, 2, new QTableWidgetItem("z"));

	int voxelIdx;
	for (int imgIdx=0; imgIdx<modalities->size(); ++imgIdx)
	{
		m_voxelData->setItem(0, 3+imgIdx, new QTableWidgetItem(QString("Mod")+QString::number(imgIdx)));
		voxelIdx = 1;
		vtkSmartPointer<vtkImageData> img = modalities->Get(imgIdx)->GetImage();
		std::function<void (int[3], VoxelValueType)> pixelVisitor = [this, &imgIdx, &voxelIdx, &adder, &tableSize](int coord[3], VoxelValueType modalityValue)
		{
			assert (voxelIdx < tableSize);
			adder.resume();
			if (imgIdx == 0)
			{
				m_voxelData->setItem(voxelIdx, 0, new QTableWidgetItem(QString::number(coord[0])));
				m_voxelData->setItem(voxelIdx, 1, new QTableWidgetItem(QString::number(coord[1])));
				m_voxelData->setItem(voxelIdx, 2, new QTableWidgetItem(QString::number(coord[2])));
			}
			m_voxelData->setItem(voxelIdx, 3+imgIdx, new QTableWidgetItem(QString::number(modalityValue)));
			adder.pause();
			voxelIdx++;
		};
		IteratePixels(img, step, pixelVisitor);
	}
	m_voxelData->setRowCount(voxelIdx);
	// pass to scatter plot matrix:
	m_splom->setData(m_voxelData);
	m_splom->setLookupTable(m_lut, QString("Mod0"));
	m_splom->setParameterVisibility("x", false);
	m_splom->setParameterVisibility("y", false);
	m_splom->setParameterVisibility("z", false);

	//std::cout << "Elapsed for adding to QTableWidget: " << adder.elapsed() << " seconds" << std::endl;
}