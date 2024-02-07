// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAModalitySPLOM.h"

#include <iAChannelID.h>    // for NotExistingChannel
#include <iAChannelData.h>
#include <iAPerformanceHelper.h>
#include <iAMdiChild.h>

#include <iAQSplom.h>
#include <iASPLOMData.h>

#include <iAImageData.h>
#include <iALog.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QHBoxLayout>

#include <omp.h>

#include <functional>

iAModalitySPLOM::iAModalitySPLOM():
	m_splom(new iAQSplom(this)),
	m_data(new iASPLOMData()),
	m_selection_ctf(vtkSmartPointer<vtkColorTransferFunction>::New()),
	m_selection_otf(vtkSmartPointer<vtkPiecewiseFunction>::New()),
	m_SPLOMSelectionChannelID(NotExistingChannel)
{
	m_selection_ctf->AddRGBPoint(0, 0, 0, 0);
	m_selection_ctf->AddRGBPoint(1, 1.0, 0.0, 0.0);

	m_selection_otf->AddPoint(0, 0);
	m_selection_otf->AddPoint(1, 1);
	setWindowTitle("Modality Correlation");
	setFeatures(DockWidgetVerticalTitleBar | DockWidgetClosable | DockWidgetMovable | DockWidgetFloatable);
	setObjectName("ModalityCorrelationWidget");

	QHBoxLayout* lay = new QHBoxLayout();
	lay->addWidget(m_splom);

	QWidget* content = new QWidget();
	content->setLayout(lay);
	setWidget(content);

	connect(m_splom, &iAQSplom::selectionModified, this, &iAModalitySPLOM::splomSelection);
}

void iAModalitySPLOM::splomSelection(std::vector<size_t> const & selInds)
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
	for (auto idx: selInds)
	{
		int x = static_cast<double>(m_data->data()[0][idx]);
		int y = static_cast<double>(m_data->data()[1][idx]);
		int z = static_cast<double>(m_data->data()[2][idx]);
		result->SetScalarComponentFromFloat(x, y, z, 0, 1);
	}
	iAMdiChild* mdiChild = dynamic_cast<iAMdiChild*>(parent());
	if (!m_selected)
	{
		mdiChild->setChannelRenderingEnabled(m_SPLOMSelectionChannelID, false);
		m_selected = true;
		return;
	}

	if (m_SPLOMSelectionChannelID == NotExistingChannel)
	{
		m_SPLOMSelectionChannelID = mdiChild->createChannel();
	}
	auto chData = mdiChild->channelData(m_SPLOMSelectionChannelID);
	chData->setData(result, m_selection_ctf, m_selection_otf);
	// TODO: initialize channel?
	mdiChild->initChannelRenderer(m_SPLOMSelectionChannelID, false);
	mdiChild->updateChannelOpacity(m_SPLOMSelectionChannelID, 0.5);
	mdiChild->updateViews();
}

typedef unsigned short VoxelValueType;

// TODO: extract to iAToolsVTK?
void iteratePixels(vtkSmartPointer<vtkImageData> img, const int step[3], std::function<void (int[3], VoxelValueType)> pixelVisitor)
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

void iAModalitySPLOM::setData(std::vector<iAImageData*> dataSets)
{
	iATimeGuard timer("Fill iASPLOMData", false);
	dataSets[0]->vtkImage()->GetExtent(m_extent);
	dataSets[0]->vtkImage()->GetSpacing(m_spacing);
	dataSets[0]->vtkImage()->GetOrigin(m_origin);

	// TODO: improve this very crude regular sampling. maybe random sampling is better?
	const int maxNumSteps = 50;
	const int step[3] = {
		(m_extent[1]-m_extent[0]+1) / std::min(m_extent[1]-m_extent[0]+1, maxNumSteps),
		(m_extent[3]-m_extent[2]+1) / std::min(m_extent[3]-m_extent[2]+1, maxNumSteps),
		(m_extent[5]-m_extent[4]+1) / std::min(m_extent[5]-m_extent[4]+1, maxNumSteps)
	};
	std::vector<QString> paramNames;
	std::vector<char> paramVisibility;
	paramNames.push_back("x"); paramVisibility.push_back(false);
	paramNames.push_back("y"); paramVisibility.push_back(false);
	paramNames.push_back("z"); paramVisibility.push_back(false);
	for (size_t imgIdx = 0; imgIdx < dataSets.size(); ++imgIdx)
	{
		if (dataSets[imgIdx]->vtkImage()->GetScalarType() != VTK_UNSIGNED_SHORT)
		{
			LOG(lvlError, QString("Modality %1 is not of type unsigned short (which is "
				"currently the only supported type for Modality SPLOM)!")
				.arg(dataSets[imgIdx]->name()));
			return;
		}
		paramNames.push_back(dataSets[imgIdx]->name());
		paramVisibility.push_back(true);
	}
	m_data->setParameterNames(paramNames);

	for (size_t imgIdx = 0; imgIdx < dataSets.size(); ++imgIdx)
	{
		vtkSmartPointer<vtkImageData> img = dataSets[imgIdx]->vtkImage();
		std::function<void (int[3], VoxelValueType)> pixelVisitor = [this, &imgIdx](int coord[3], VoxelValueType modalityValue)
		{
			if (imgIdx == 0)
			{
				m_data->data()[0].push_back(coord[0]);
				m_data->data()[1].push_back(coord[1]);
				m_data->data()[2].push_back(coord[2]);
			}
			m_data->data()[3+imgIdx].push_back(modalityValue);
		};
		iteratePixels(img, step, pixelVisitor);
	}
	m_data->updateRanges();
	m_splom->setData(m_data, paramVisibility);
}
