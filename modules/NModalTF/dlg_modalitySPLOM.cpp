/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "dlg_modalitySPLOM.h"

#include <iAQSplom.h>
#include <iAChannelData.h>
#include <iAModality.h>
#include <iAModalityList.h>
#include <iAPerformanceHelper.h>
#include <iASPLOMData.h>
#include <iAMdiChild.h>

#include "defines.h"    // for NotExistingChannel
#include <iALog.h>

#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>

#include <QHBoxLayout>

#include <functional>

dlg_modalitySPLOM::dlg_modalitySPLOM():
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

	connect(m_splom, &iAQSplom::selectionModified, this, &dlg_modalitySPLOM::SplomSelection);
}

void dlg_modalitySPLOM::SplomSelection(std::vector<size_t> const & selInds)
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

void dlg_modalitySPLOM::SetData(QSharedPointer<iAModalityList> modalities)
{
	iATimeGuard timer("Fill iASPLOMData", false);
	modalities->get(0)->image()->GetExtent(m_extent);
	modalities->get(0)->image()->GetSpacing(m_spacing);
	modalities->get(0)->image()->GetOrigin(m_origin);

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
	for (int imgIdx = 0; imgIdx < modalities->size(); ++imgIdx)
	{
		if (modalities->get(imgIdx)->image()->GetScalarType() != VTK_UNSIGNED_SHORT)
		{
			LOG(lvlError, QString("Modality %1 is not of type unsigned short (which is "
				"currently the only supported type for Modality SPLOM)!")
				.arg(modalities->get(imgIdx)->name()));
			return;
		}
		paramNames.push_back(modalities->get(imgIdx)->name());
		paramVisibility.push_back(true);
	}
	m_data->setParameterNames(paramNames);

	for (int imgIdx = 0; imgIdx < modalities->size(); ++imgIdx)
	{
		vtkSmartPointer<vtkImageData> img = modalities->get(imgIdx)->image();
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
		IteratePixels(img, step, pixelVisitor);
	}
	m_data->updateRanges();
	m_splom->setData(m_data, paramVisibility);
}