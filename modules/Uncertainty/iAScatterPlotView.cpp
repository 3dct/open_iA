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
#include "iAScatterPlotView.h"

#include "iAConsole.h"
#include "iAPerformanceHelper.h"
#include "iAToolsVTK.h"
#include "iAScatterPlot.h"
#include "iAScatterPlotWidget.h"
#include "iASPLOMData.h"
#include "iAUncertaintyColors.h"
#include "iAVtkDraw.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QToolButton>
#include <QVariant>
#include <QVBoxLayout>

#include <QVTKWidget.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkPen.h>
#include <vtkPlot.h>
#include <vtkTable.h>

#define VTK_CREATE(type, name) \
    vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

iAScatterPlotView::iAScatterPlotView():
	m_scatterPlotWidget(nullptr),
	m_scatterPlotContainer(new QWidget()),
	m_vtkChartWidget(nullptr)
{
	setLayout(new QVBoxLayout());
	layout()->setSpacing(0);
	m_scatterPlotContainer->setLayout(new QHBoxLayout());
	m_scatterPlotContainer->layout()->setSpacing(0);
	layout()->addWidget(m_scatterPlotContainer);

	m_settings = new QWidget();
	m_settings->setLayout(new QVBoxLayout);
	m_settings->layout()->setSpacing(0);
	m_settings->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

	auto datasetChoiceContainer = new QWidget();
	datasetChoiceContainer->setLayout(new QVBoxLayout());
	m_xAxisChooser = new QWidget();
	m_xAxisChooser->setLayout(new QHBoxLayout());
	m_xAxisChooser->layout()->setSpacing(0);
	m_xAxisChooser->layout()->addWidget(new QLabel("X Axis"));
	m_yAxisChooser = new QWidget();
	m_yAxisChooser->setLayout(new QHBoxLayout());
	m_yAxisChooser->layout()->setSpacing(0);
	m_yAxisChooser->layout()->addWidget(new QLabel("Y Axis"));
	datasetChoiceContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	datasetChoiceContainer->layout()->addWidget(m_xAxisChooser);
	datasetChoiceContainer->layout()->addWidget(m_yAxisChooser);
	m_settings->layout()->addWidget(datasetChoiceContainer);
	layout()->addWidget(m_settings);
}


void iAScatterPlotView::AddPlot(vtkImagePointer imgX, vtkImagePointer imgY, QString const & captionX, QString const & captionY)
{
	QVector<unsigned int> selection;
	if (m_scatterPlotWidget)
	{
		selection = m_scatterPlotWidget->getSelection();
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
	QList<double> values0;
	splomData->data().push_back(values0);
	QList<double> values1;
	splomData->data().push_back(values1);
	for (size_t i = 0; i < m_voxelCount; ++i)
	{
		splomData->data()[0].push_back(bufX[i]);
		splomData->data()[1].push_back(bufY[i]);
	}

	// setup scatterplot:
	m_scatterPlotWidget = new iAScatterPlotWidget(splomData);
	QColor c(iAUncertaintyColors::Chart);
	c.setAlpha(128);
	m_scatterPlotWidget->setPlotColor(c, 0, 1);
	m_scatterPlotWidget->setSelectionColor(iAUncertaintyColors::Selection);
	m_scatterPlotWidget->setSelection(selection);
	m_scatterPlotWidget->setMinimumWidth(width() / 2);
	m_scatterPlotContainer->layout()->addWidget(m_scatterPlotWidget);
	connect(m_scatterPlotWidget->m_scatterplot, SIGNAL(selectionModified()), this, SLOT(SelectionUpdated()));

	if (m_vtkChartWidget)
	{
		delete m_vtkChartWidget;
	}
	m_vtkChartWidget = new QVTKWidget();
	auto chart = vtkSmartPointer<vtkChartXY>::New();
	// Test charting with a few more points...
	VTK_CREATE(vtkTable, table);
	//VTK_CREATE(vtkFloatArray, arrIdx);
	//arrIdx->SetName("X Axis");
	//table->AddColumn(arrIdx);
	VTK_CREATE(vtkFloatArray, arX);
	arX->SetName(captionX.toStdString().c_str());
	table->AddColumn(arX);
	VTK_CREATE(vtkFloatArray, arY);
	arY->SetName(captionY.toStdString().c_str());
	table->AddColumn(arY);
	table->SetNumberOfRows(m_voxelCount);
	for (size_t i = 0; i < m_voxelCount; ++i)
	{
		table->SetValue(i, 0, bufX[i]);
		table->SetValue(i, 1, bufY[i]);
	}
	VTK_CREATE(vtkContextView, view); // This contains a chart object
	view->GetScene()->AddItem(chart);
	view->SetInteractor(m_vtkChartWidget->GetInteractor());
	m_vtkChartWidget->SetRenderWindow(view->GetRenderWindow());
	vtkPlot *points = chart->AddPlot(vtkChart::POINTS);
	points->SetInputData(table, 0, 1);
	points->SetColor(iAUncertaintyColors::Chart.red(), iAUncertaintyColors::Chart.green(), iAUncertaintyColors::Chart.blue(), iAUncertaintyColors::Chart.alpha());
	points->GetSelectionPen()->SetColor(iAUncertaintyColors::Selection.red(), iAUncertaintyColors::Selection.green(), iAUncertaintyColors::Selection.blue(), iAUncertaintyColors::Selection.alpha());
	m_scatterPlotContainer->layout()->addWidget(m_vtkChartWidget);
	*/
}


void iAScatterPlotView::SetDatasets(QSharedPointer<iAUncertaintyImages> imgs)
{
	if (m_scatterPlotWidget)
	{
		m_scatterPlotWidget->getSelection().clear();
	}
	for (auto widget : m_xAxisChooser->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
	for (auto widget : m_yAxisChooser->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
	{
		delete widget;
	}
	m_imgs = imgs;
	m_xAxisChoice = iAUncertaintyImages::LabelDistributionEntropy;
	m_yAxisChoice = iAUncertaintyImages::AvgAlgorithmEntropyProbSum;
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
		return;
	m_xAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}


void iAScatterPlotView::YAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_yAxisChoice)
		return;
	m_yAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}

void iAScatterPlotView::SelectionUpdated()
{
	QVector<unsigned int> selectedPoints = m_scatterPlotWidget->getSelection();
	std::set<unsigned int> selectedSet(selectedPoints.begin(), selectedPoints.end());
	double* buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (unsigned int v = 0; v<m_voxelCount; ++v)
	{
		*buf = selectedSet.find(v) != selectedSet.end() ? 1 : 0;
		buf++;
	}
	m_selectionImg->Modified();
	//StoreImage(m_selectionImg, "C:/Users/p41143/selection.mhd", true);
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
