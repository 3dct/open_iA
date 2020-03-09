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
#include "iAScatterPlotView.h"

#include "iAUncertaintyColors.h"

#include <charts/iAScatterPlot.h>
#include <charts/iAScatterPlotWidget.h>
#include <charts/iASPLOMData.h>
#include <iAConsole.h>
#include <iAToolsVTK.h>
#include <iAVtkDraw.h>
#include <qthelper/iAQFlowLayout.h>

#include <vtkAxis.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkRenderer.h>
#include <vtkTable.h>
#include <vtkTextProperty.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>

iAScatterPlotView::iAScatterPlotView():
	m_voxelCount(0),
	m_xAxisChooser(new QWidget()),
	m_yAxisChooser(new QWidget()),
	m_settings(new QWidget()),
	m_scatterPlotContainer(new QWidget()),
	m_scatterPlotWidget(nullptr)
{
	setLayout(new QVBoxLayout());
	layout()->setSpacing(0);
	layout()->setContentsMargins(4, 4, 4, 4);
	m_scatterPlotContainer->setLayout(new QHBoxLayout());
	m_scatterPlotContainer->layout()->setSpacing(0);
	m_scatterPlotContainer->layout()->setContentsMargins(0, 0, 0, 0);
	m_scatterPlotContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Expanding);
	layout()->addWidget(m_scatterPlotContainer);

	m_settings->setLayout(new QVBoxLayout);
	m_settings->layout()->setSpacing(0);
	m_settings->layout()->setContentsMargins(0, 4, 0, 0);
	m_settings->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

	auto datasetChoiceContainer = new QWidget();
	datasetChoiceContainer->setLayout(new QVBoxLayout());

	m_xAxisChooser->setLayout(new iAQFlowLayout(0, 0, 0));
	m_xAxisChooser->layout()->addWidget(new QLabel("X Axis:"));
	m_xAxisChooser->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_yAxisChooser->setLayout(new iAQFlowLayout(0, 0, 0));
	m_yAxisChooser->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	m_yAxisChooser->layout()->addWidget(new QLabel("Y Axis:"));
	datasetChoiceContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	datasetChoiceContainer->layout()->addWidget(m_xAxisChooser);
	datasetChoiceContainer->layout()->addWidget(m_yAxisChooser);
	m_settings->layout()->addWidget(datasetChoiceContainer);
	layout()->addWidget(m_settings);
}


void iAScatterPlotView::AddPlot(vtkImagePointer imgX, vtkImagePointer imgY, QString const & captionX, QString const & captionY)
{
	std::vector<size_t> selection;
	if (m_scatterPlotWidget)
	{
		selection = m_scatterPlotWidget->GetSelection();
		delete m_scatterPlotWidget;
	}
	// setup data object:
	int * dim = imgX->GetDimensions();
	m_voxelCount = static_cast<size_t>(dim[0]) * dim[1] * dim[2];
	double* bufX = static_cast<double*>(imgX->GetScalarPointer());
	double* bufY = static_cast<double*>(imgY->GetScalarPointer());
	auto splomData = QSharedPointer<iASPLOMData>(new iASPLOMData());
	splomData->paramNames().push_back(captionX);
	splomData->paramNames().push_back(captionY);
	std::vector<double> values0;
	splomData->data().push_back(values0);
	std::vector<double> values1;
	splomData->data().push_back(values1);
	for (size_t i = 0; i < m_voxelCount; ++i)
	{
		splomData->data()[0].push_back(bufX[i]);
		splomData->data()[1].push_back(bufY[i]);
	}

	// setup scatterplot:
	m_scatterPlotWidget = new iAScatterPlotWidget(splomData);
	QColor c(iAUncertaintyColors::ScatterPlotDots);
	c.setAlpha(128);
	m_scatterPlotWidget->SetSelectionMode(iAScatterPlot::Rectangle);
	m_scatterPlotWidget->SetPlotColor(c, 0, 1);
	m_scatterPlotWidget->SetSelectionColor(iAUncertaintyColors::SelectedPixel);
	m_scatterPlotWidget->SetSelection(selection);
	m_scatterPlotWidget->setMinimumWidth(width() / 2);
	m_scatterPlotContainer->layout()->addWidget(m_scatterPlotWidget);
	connect(m_scatterPlotWidget->m_scatterplot, SIGNAL(selectionModified()), this, SLOT(SelectionUpdated()));
}


void iAScatterPlotView::SetDatasets(QSharedPointer<iAUncertaintyImages> imgs)
{
	if (m_scatterPlotWidget)
	{
		m_scatterPlotWidget->GetSelection().clear();
	}
	for (auto widget : m_xAxisChooser->findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
	for (auto widget : m_yAxisChooser->findChildren<QToolButton*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
	m_imgs = imgs;
	m_xAxisChoice = iAUncertaintyImages::LabelDistributionEntropy;
	m_yAxisChoice = iAUncertaintyImages::AvgAlgorithmEntropyEntrSum;
	for (int i = 0; i < iAUncertaintyImages::SourceCount; ++i)
	{
		QToolButton* xButton = new QToolButton(); xButton->setText(imgs->GetSourceName(i));
		QToolButton* yButton = new QToolButton(); yButton->setText(imgs->GetSourceName(i));
		xButton->setProperty("imgId", i);
		yButton->setProperty("imgId", i);
		xButton->setAutoExclusive(true);
		xButton->setCheckable(true);
		yButton->setAutoExclusive(true);
		yButton->setCheckable(true);
		if (i == m_xAxisChoice)
		{
			xButton->setChecked(true);
		}
		if (i == m_yAxisChoice)
		{
			yButton->setChecked(true);
		}
		connect(xButton, SIGNAL(clicked()), this, SLOT(XAxisChoice()));
		connect(yButton, SIGNAL(clicked()), this, SLOT(YAxisChoice()));
		m_xAxisChooser->layout()->addWidget(xButton);
		m_yAxisChooser->layout()->addWidget(yButton);
	}
	if (!m_selectionImg)
	{
		vtkImagePointer i = imgs->GetEntropy(m_xAxisChoice);
		m_selectionImg = vtkSmartPointer<iAvtkImageData>::New();
		m_selectionImg->SetDimensions(i->GetDimensions());
		m_selectionImg->AllocateScalars(i->GetScalarType(), 1);
		m_selectionImg->SetSpacing(i->GetSpacing());
		m_voxelCount = static_cast<size_t>(i->GetDimensions()[0]) * i->GetDimensions()[1] * i->GetDimensions()[2];
		int* imgbuf = static_cast<int*>(m_selectionImg->GetScalarPointer());
		std::fill(imgbuf, imgbuf + m_voxelCount, 0);
		m_selectionImg->SetScalarRange(0, 1);
	}
	AddPlot(imgs->GetEntropy(m_xAxisChoice), imgs->GetEntropy(m_yAxisChoice),
		imgs->GetSourceName(m_xAxisChoice), imgs->GetSourceName(m_yAxisChoice));
}


void iAScatterPlotView::XAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_xAxisChoice)
	{
		return;
	}
	m_xAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}


void iAScatterPlotView::YAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_yAxisChoice)
	{
		return;
	}
	m_yAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}

void iAScatterPlotView::SelectionUpdated()
{
	auto selectedPoints = m_scatterPlotWidget->GetSelection();
	std::set<size_t> selectedSet(selectedPoints.begin(), selectedPoints.end());
	double* buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (unsigned int v = 0; v<m_voxelCount; ++v)
	{
		*buf = selectedSet.find(v) != selectedSet.end() ? 1 : 0;
		buf++;
	}
	m_selectionImg->Modified();
	//storeImage(m_selectionImg, "C:/Users/p41143/selection.mhd", true);
	emit SelectionChanged();
}

vtkImagePointer iAScatterPlotView::GetSelectionImage()
{
	return m_selectionImg;
}

void iAScatterPlotView::ToggleSettings()
{
	m_settings->setVisible(!m_settings->isVisible());
}
