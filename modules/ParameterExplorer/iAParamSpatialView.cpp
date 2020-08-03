/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "iAParamColors.h"
#include "iAParamTableView.h"
#include "iAHistogramCreator.h"
#include "iAImageWidget.h"

#include <charts/iAChartWithFunctionsWidget.h>
#include <charts/iAPlotTypes.h>
#include <iAConnector.h>
#include <iAConsole.h>
#include <iASlicerMode.h>
#include <io/iAFileUtils.h>

#include <vtkImageData.h>

#include <QHBoxLayout>
#include <QSpinBox>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include <cassert>

iAParamSpatialView::iAParamSpatialView(iAParamTableView* table, QString const & basePath, iAChartWithFunctionsWidget* chartWidget, int binCount) :
	m_table(table),
	m_basePath(basePath),
	m_curMode(iASlicerMode::XY),
	m_sliceControl(new QSpinBox()),
	m_imageWidget(nullptr),
	m_settings(new QWidget),
	m_imageContainer(new QWidget),
	m_sliceNrInitialized(false),
	m_chartWidget(chartWidget),
	m_binCount(binCount)
{
	m_sliceControl->setMaximum(0);
	connect(m_sliceControl, QOverload<int>::of(&QSpinBox::valueChanged), this, &iAParamSpatialView::SliceChanged);

	auto sliceButtonBar = new QToolBar();			// same order as in iASlicerMode!
	static const char* const slicerModeButtonLabels[] = { "YZ", "XY", "XZ" };
	for (int i = 0; i < 3; ++i)
	{
		slicerModeButton.push_back(new QToolButton());
		slicerModeButton[i]->setText(slicerModeButtonLabels[i]);
		slicerModeButton[i]->setAutoExclusive(true);
		slicerModeButton[i]->setCheckable(true);
		connect(slicerModeButton[i], &QToolButton::clicked, this, &iAParamSpatialView::SlicerModeButtonClicked);
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

void iAParamSpatialView::setImage(size_t id)
{
	if (!m_imageCache.contains(id))
	{
		assert(m_table->Table()->rowCount() >= 0);
		if (id >= static_cast<size_t>(m_table->Table()->rowCount()))
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
			con.setImage(itkImg);
			con.modified();
			m_imageCache.insert(id, con.vtkImage());
		}
		catch (std::exception & e)
		{
			DEBUG_LOG(QString("Could not load image %1: %2").arg(fileName).arg(e.what()));
			return;
		}
	}
	auto img = m_imageCache[id];
	if (m_histogramCache.contains(id))
	{
		SwitchToHistogram(id);
	}
	else
	{
		auto creator = QSharedPointer<iAHistogramCreator>(new iAHistogramCreator(img, m_binCount, id));
		connect(creator.data(), &iAHistogramCreator::finished, this, &iAParamSpatialView::HistogramReady);
		m_histogramCreaters.push_back(creator);
		creator->start();
	}

	if (!m_sliceNrInitialized)
	{
		for (int i = 0; i < 3; ++i)
			m_sliceNr[i] = img->GetDimensions()[mapSliceToGlobalAxis(i, iAAxisIndex::Z)] / 2;
	}
	if (!m_imageWidget)
	{
		m_imageWidget = new iAImageWidget(img);
		m_imageContainer->layout()->addWidget(m_imageWidget);
		m_imageWidget->SetMode(m_curMode);
		m_imageWidget->SetSlice(m_sliceNr[m_curMode]);
	}
	else
		m_imageWidget->setImage(img);
	m_sliceControl->setMaximum(m_imageWidget->GetSliceCount() - 1);
	if (!m_sliceNrInitialized)
	{
		m_sliceNrInitialized = true;
		m_sliceControl->setValue(m_sliceNr[m_curMode]);
	}
}

void iAParamSpatialView::SlicerModeButtonClicked(bool /*checked*/)
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

void iAParamSpatialView::HistogramReady()
{
	auto creator = qobject_cast<iAHistogramCreator*>(QObject::sender());
	int id = creator->GetID();
	m_histogramCache[id] = creator->GetData();
	SwitchToHistogram(id);
}

void iAParamSpatialView::SwitchToHistogram(int id)
{
	m_chartWidget->removePlot(m_curHistogramPlot);
	QColor histoChartColor(SPLOMDotQColor);
	histoChartColor.setAlpha(96);
	m_curHistogramPlot = QSharedPointer<iAPlot>(new iABarGraphPlot(m_histogramCache[id], histoChartColor, 2));
	m_chartWidget->addPlot(m_curHistogramPlot);
	m_chartWidget->update();
}

void iAParamSpatialView::ToggleSettings(bool visible)
{
	m_settings->setVisible(visible);
}
