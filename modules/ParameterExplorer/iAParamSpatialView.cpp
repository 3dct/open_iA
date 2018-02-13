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
#include "iAParamSpatialView.h"

#include "iAParamTableView.h"
#include "iAImageWidget.h"

#include "iAConnector.h"
#include "iAConsole.h"
#include "iASlicerMode.h"
#include "io/iAFileUtils.h"

#include <vtkImageData.h>

#include <QHBoxLayout>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

iAParamSpatialView::iAParamSpatialView(iAParamTableView* table, QString const & basePath) :
	m_table(table),
	m_basePath(basePath),
	m_imageWidget(nullptr),
	m_curMode(iASlicerMode::XY),
	m_settings(new QWidget),
	m_imageContainer(new QWidget),
	m_sliceControl(new QSpinBox()),
	m_sliceNrInitialized(false)
{
	m_sliceControl->setMaximum(0);
	connect(m_sliceControl, SIGNAL(valueChanged(int)), this, SLOT(SliceChanged(int)));

	auto sliceButtonBar = new QToolBar();			// same order as in iASlicerMode!
	static const char* const slicerModeButtonLabels[] = { "YZ", "XY", "XZ" };
	for (int i = 0; i < 3; ++i)
	{
		slicerModeButton.push_back(new QToolButton());
		slicerModeButton[i]->setText(slicerModeButtonLabels[i]);
		slicerModeButton[i]->setAutoExclusive(true);
		slicerModeButton[i]->setCheckable(true);
		connect(slicerModeButton[i], SIGNAL(clicked(bool)), this, SLOT(SlicerModeButtonClicked(bool)));
		sliceButtonBar->addWidget(slicerModeButton[i]);
	}
	slicerModeButton[m_curMode]->setChecked(true);

	auto sliceBar = new QWidget();
	sliceBar->setLayout(new QHBoxLayout());
	sliceBar->layout()->setSpacing(0);
	sliceBar->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	sliceBar->layout()->addWidget(sliceButtonBar);
	sliceBar->layout()->addWidget(m_sliceControl);

	m_settings->setLayout(new QHBoxLayout);
	m_settings->layout()->setMargin(0);
	m_settings->layout()->setSpacing(2);
	m_settings->setFixedHeight(24);
	m_settings->layout()->addWidget(m_sliceControl);
	m_settings->layout()->addWidget(sliceButtonBar);

	m_imageContainer->setLayout(new QHBoxLayout);

	setLayout(new QVBoxLayout);
	layout()->addWidget(m_imageContainer);
	layout()->addWidget(m_settings);

	std::fill(m_sliceNr, m_sliceNr + 3, 0);
}

void iAParamSpatialView::SetImage(int id)
{
	if (!m_imageCache.contains(id))
	{
		if (id < 0 || id >= m_table->Table()->rowCount())
		{
			DEBUG_LOG("Invalid column index!");
			return;
		}
		QString fileName = m_table->Table()->item(id, 0)->text();	// assumes filename is in column 0!
		try
		{
			fileName = MakeAbsolute(m_basePath, fileName);
			iAITKIO::ScalarPixelType pixelType;
			auto itkImg = iAITKIO::readFile(fileName, pixelType, false);
			m_loadedImgs.push_back(itkImg);
			iAConnector con;
			con.SetImage(itkImg);
			con.Modified();
			m_imageCache.insert(id, con.GetVTKImage());
		}
		catch (std::exception & e)
		{
			DEBUG_LOG(QString("Could not load image %1: %2").arg(fileName).arg(e.what()));
			return;
		}
	}
	auto img = m_imageCache[id];

	if (!m_sliceNrInitialized)
	{
		const int sliceAxis[3] = { 0, 2, 1 };
		for (int i = 0; i < 3; ++i)
			m_sliceNr[i] = img->GetDimensions()[sliceAxis[i]] / 2;
	}
	if (!m_imageWidget)
	{
		m_imageWidget = new iAImageWidget(img);
		m_imageContainer->layout()->addWidget(m_imageWidget);
		m_imageWidget->SetMode(m_curMode);
		m_imageWidget->SetSlice(m_sliceNr[m_curMode]);
	}
	else
		m_imageWidget->SetImage(img);
	m_sliceControl->setMaximum(m_imageWidget->GetSliceCount() - 1);
	if (!m_sliceNrInitialized)
	{
		m_sliceNrInitialized = true;
		m_sliceControl->setValue(m_sliceNr[m_curMode]);
	}
}

void iAParamSpatialView::SlicerModeButtonClicked(bool checked)
{
	int modeIdx = slicerModeButton.indexOf(qobject_cast<QToolButton*>(sender()));
	if (!m_imageWidget || m_curMode == modeIdx || modeIdx == -1)
		return;
	m_imageWidget->SetMode(modeIdx);
	m_sliceControl->setValue(m_sliceNr[m_curMode]);
	m_sliceControl->setMaximum(m_imageWidget->GetSliceCount() - 1);
	m_curMode = modeIdx;
}

void iAParamSpatialView::SliceChanged(int slice)
{
	m_sliceNr[m_curMode] = slice;
	m_imageWidget->SetSlice(slice);
}