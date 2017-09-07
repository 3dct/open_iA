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
#include "iAChartView.h"

#include "iAColors.h"
#include "iAConsole.h"
#include "iAPerformanceHelper.h"
#include "iAToolsVTK.h"
#include "qcustomplot.h"

#include <vtkImageData.h>

QCPColorGradient GetGradientFromIdx(int index)
{
	const int PredefinedCount = QCPColorGradient::gpHues + 1;
	if (index < PredefinedCount)
	{
		return QCPColorGradient(static_cast<QCPColorGradient::GradientPreset>(index));
	}
	else
	{
		switch (index - PredefinedCount)
		{
			case 0:
			{
				QCPColorGradient myGrayScale;
				QMap<double, QColor> myGrayScaleMap;
				myGrayScaleMap.insert(0.0, QColor(255, 255, 255));
				myGrayScaleMap.insert(1.0, QColor(0, 0, 0));
				myGrayScale.setColorStops(myGrayScaleMap);
				return myGrayScale;
			}
			default:
			case 1:
			{
				QCPColorGradient whiteToBlue;
				QMap<double, QColor> whiteToBlueMap;
				whiteToBlueMap.insert(0.0, QColor(255, 255, 255));
				whiteToBlueMap.insert(1.0, QColor(0, 0, 255));
				whiteToBlue.setColorStops(whiteToBlueMap);
				return whiteToBlue;
			}
		}
	}
}

iAChartView::iAChartView():
	m_gradient(QCPColorGradient::gpGrayscale)
{
	m_plot = new QCustomPlot();
	//m_plot->setOpenGl(true, 1);
	if (!m_plot->openGl())
	{
		DEBUG_LOG("QCustomPlot is NOT using OpenGL!");
	}
	m_plot->setInteraction(QCP::iRangeDrag, true);
	m_plot->setInteraction(QCP::iRangeZoom, true);
	m_plot->setInteraction(QCP::iMultiSelect, true);
	m_plot->setMultiSelectModifier(Qt::ShiftModifier);
	m_plot->setInteraction(QCP::iSelectPlottables, true);
	connect(m_plot, SIGNAL(mousePress(QMouseEvent *)), this, SLOT(chartMousePress(QMouseEvent *)));
	setLayout(new QVBoxLayout());
	layout()->addWidget(m_plot);

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
	layout()->addWidget(datasetChoiceContainer);

	QComboBox * colorThemeChooser = new QComboBox();
	QStringList options;
	options
		<< "gpGrayscale"
		<< "gpHot"
		<< "gpCold"
		<< "gpNight"
		<< "gpCandy"
		<< "gpGeography"
		<< "gpIon"
		<< "gpThermal"
		<< "gpPolar"
		<< "gpSpectrum"
		<< "gpJet"
		<< "gpHues"
		<< "WhiteToBlack"
		<< "WhiteToBlue";
	colorThemeChooser->addItems(options);
	connect(colorThemeChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(colorThemeChanged(int)));
	QWidget* colorThemeContainer = new QWidget();
	colorThemeContainer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
	colorThemeContainer->setLayout(new QHBoxLayout());
	colorThemeContainer->layout()->addWidget(new QLabel("Color Theme:"));
	colorThemeContainer->layout()->addWidget(colorThemeChooser);
	layout()->addWidget(colorThemeContainer);
}


void iAChartView::AddPlot(vtkImagePointer imgX, vtkImagePointer imgY, QString const & captionX, QString const & captionY)
{
	m_plot->clearPlottables();
	const int BinCountX = 250;
	const int BinCountY = 250;
	iAPerformanceHelper heatmapCalcMeasure;
	heatmapCalcMeasure.start("Heatmap calculation");
	colorMap = new QCPColorMap(m_plot->xAxis, m_plot->yAxis);
	colorMap->data()->setSize(BinCountX, BinCountY);
	colorMap->data()->setRange(QCPRange(0, 1), QCPRange(0, 1));
	colorMap->data()->fill(0);

	double* bufX = static_cast<double*>(imgX->GetScalarPointer());
	double* bufY = static_cast<double*>(imgY->GetScalarPointer());
	FOR_VTKIMG_PIXELS_IDX(imgX, idx)
	{
		int x, y;
		colorMap->data()->coordToCell(bufX[idx], bufY[idx], &x, &y);
		colorMap->data()->setCell(x, y, colorMap->data()->cell(x, y)+1 );
	}

	colorScale = new QCPColorScale(m_plot);
	m_plot->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
	colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
	colorScale->setRangeDrag(false);
	colorScale->setRangeZoom(false);
	colorMap->setColorScale(colorScale); // associate the color map with the color scale
	colorScale->axis()->setLabel("Entropy");

	colorMap->setGradient(GetGradientFromIdx(m_gradient));
	colorMap->rescaleDataRange();
	QCPMarginGroup *marginGroup = new QCPMarginGroup(m_plot);
	m_plot->axisRect()->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);
	colorScale->setMarginGroup(QCP::msBottom | QCP::msTop, marginGroup);

	m_plot->rescaleAxes();
	heatmapCalcMeasure.stop();

	// create mapping from bins  to pixels that are contained 
	//     or still explicit uncertainty, to be able to select not only bins but sub-bins?

	// subdivide bins -> zoomable?

	m_plot->replot();
}


void iAChartView::SetDatasets(QSharedPointer<iAUncertaintyImages> imgs)
{
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
		connect(xButton, SIGNAL(clicked()), this, SLOT(xAxisChoice()));
		connect(yButton, SIGNAL(clicked()), this, SLOT(yAxisChoice()));
		m_xAxisChooser->layout()->addWidget(xButton);
		m_yAxisChooser->layout()->addWidget(yButton);
	}
	m_selectionImg = AllocateImage(imgs->GetEntropy(m_xAxisChoice));
	AddPlot(imgs->GetEntropy(m_xAxisChoice), imgs->GetEntropy(m_yAxisChoice),
		imgs->GetSourceName(m_xAxisChoice), imgs->GetSourceName(m_yAxisChoice));
}


void iAChartView::xAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_xAxisChoice)
		return;
	m_xAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}


void iAChartView::yAxisChoice()
{
	int imgId = qobject_cast<QToolButton*>(sender())->property("imgId").toInt();
	if (imgId == m_yAxisChoice)
		return;
	m_yAxisChoice = imgId;
	AddPlot(m_imgs->GetEntropy(m_xAxisChoice), m_imgs->GetEntropy(m_yAxisChoice),
		m_imgs->GetSourceName(m_xAxisChoice), m_imgs->GetSourceName(m_yAxisChoice));
}


void iAChartView::selectionChanged(QCPDataSelection const & selection)
{
	/*
	double* buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (int v=0; v<m_voxelCount; ++v)
	{
		*buf = 0;
		buf++;
	}
	buf = static_cast<double*>(m_selectionImg->GetScalarPointer());
	for (int r = 0; r < selection.dataRangeCount(); ++r)
	{
		std::fill(buf + selection.dataRange(r).begin(), buf + selection.dataRange(r).end(), 1);
	}
	m_selectionImg->Modified();
	*/

	//StoreImage(m_selectionImg, "C:/Users/p41143/selection.mhd", true);
	emit SelectionChanged();
}


vtkImagePointer iAChartView::GetSelectionImage()
{
	return m_selectionImg;
}


void iAChartView::chartMousePress(QMouseEvent *)
{
	if (QGuiApplication::keyboardModifiers().testFlag(Qt::ControlModifier))
	{	// allow selection with Ctrl key
		m_plot->setSelectionRectMode(QCP::srmSelect);
	}
	else
	{	// enable dragging otherwise
		m_plot->setSelectionRectMode(QCP::srmNone);
	}
}


void iAChartView::colorThemeChanged(int index)
{
	m_gradient = index;
	colorMap->setGradient(GetGradientFromIdx(m_gradient));
	colorScale->setGradient(GetGradientFromIdx(m_gradient));
	m_plot->replot();
}
