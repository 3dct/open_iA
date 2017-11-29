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
#include "iAQualityMeasureModuleInterface.h"

#include "iAMathUtility.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAImageInfo.h"
#include "iAModality.h"
#include "iATypedCallHelper.h"
#include "mainwindow.h"
#include "mdichild.h"

void iAQualityMeasureModuleInterface::Initialize()
{
	if (!m_mainWnd)
		return;
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuEnsembles = getMenuWithTitle(toolsMenu, QString("Image Ensembles"), false);

	QAction * actionQ = new QAction(m_mainWnd);
	actionQ->setText(QApplication::translate("MainWindow", "Q Metric", 0));
	AddActionToMenuAlphabeticallySorted(menuEnsembles, actionQ, true);
	connect(actionQ, SIGNAL(triggered()), this, SLOT(CalculateQ()));
}


#include <itkImage.h>
#include <itkImageToHistogramFilter.h>


#include "charts/iAChartWidget.h"
#include "charts/iASimpleHistogramData.h"
#include "charts/iAPlotTypes.h"
#include "iADockWidgetWrapper.h"

template <typename T>
void calculateQ_template(iAConnector& con, double histogramBinFactor, double minVal, double maxVal,
	unsigned int numberOfPeaks, double Kderiv, double Kminima, iAChartWidget* chart)
{
	// 1. calculate histogram:
	const int ChannelCount = 1;
	typedef itk::Image<T, DIM>  InputImageType;
	typedef itk::Statistics::ImageToHistogramFilter<InputImageType> ImageHistogramFilterType;
	auto itkImg = con.GetITKImage();
	auto size = itkImg->GetLargestPossibleRegion().GetSize();
	size_t voxelCount = size[0] * size[1] * size[2];
	size_t binCount = static_cast<size_t>(histogramBinFactor * std::sqrt(voxelCount));
	auto histogramFilter = ImageHistogramFilterType::New();
	histogramFilter->ReleaseDataFlagOff();
	histogramFilter->SetInput(dynamic_cast<InputImageType*>(itkImg));
	typename ImageHistogramFilterType::HistogramType::MeasurementVectorType	binMin(ChannelCount);
	binMin.Fill(minVal);
	typename ImageHistogramFilterType::HistogramType::MeasurementVectorType	binMax(ChannelCount);
	binMax.Fill(maxVal);
	typename ImageHistogramFilterType::HistogramType::SizeType binCountMulti(ChannelCount);
	binCountMulti.Fill(binCount);
	histogramFilter->SetHistogramBinMinimum(binMin);
	histogramFilter->SetHistogramBinMaximum(binMax);
	histogramFilter->SetAutoMinimumMaximum(false);
	histogramFilter->SetHistogramSize(binCountMulti);
	histogramFilter->Update();
	auto histogram = histogramFilter->GetOutput();
	size_t b = 0;
	std::vector<double> vecHist;
	for (auto it = histogram->Begin(); it != histogram->End(); ++it)
	{
		DEBUG_LOG(QString(" Bin %1: %2").arg(b).arg(it.GetFrequency()));
		vecHist.push_back(it.GetFrequency());
		++b;
	}
	if (chart)
	{
		auto histoPlotData = iASimpleHistogramData::Create(minVal, maxVal, vecHist, Continuous);
		chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(histoPlotData, QColor(180, 90, 90, 127))));
	}

	double derivSigma = static_cast<double>(binCount) / Kderiv;
	// 2. convolute with gaussian to smooth:
	auto smoothedHist = gaussianSmoothing(vecHist, derivSigma, 5, (maxVal-minVal)/binCount);

	b = 0;
	for (auto it = smoothedHist.begin(); it != smoothedHist.end(); ++it)
	{
		DEBUG_LOG(QString(" Smoothed Bin %1: %2").arg(b).arg(*it));
		++b;
	}
	if (chart)
	{
		auto smoothedHistoPlotData = iASimpleHistogramData::Create(minVal, maxVal, smoothedHist, Continuous);
		chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(smoothedHistoPlotData, QColor(90, 180, 90, 127))));
	}

	// 3. find peaks: (derivative = 0, 2nd deriv. negative)
	auto firstDeriv = derivative(smoothedHist);
	if (chart)
	{
		auto firstDerivPlotData = iASimpleHistogramData::Create(minVal, maxVal, firstDeriv, Continuous);
		chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(firstDerivPlotData, QColor(90, 90, 180, 127))));
	}
	// peak is at every 0-crossing, so where:
	//      - either deriv is 0, deriv is pos before and neg afterwards (pot. use 2nd deriv?)
	//      - or before deriv. is pos. and afterwards deriv. is neg.:
	std::vector<std::pair<size_t, double> > peaks;
	for (size_t i = 0; i < firstDeriv.size(); ++i)
	{
		if ((firstDeriv[i] == 0
				&& i > 0
				&& i < firstDeriv.size() - 1
				&& firstDeriv[i-1] > 0
				&& firstDeriv[i+1] < 0) ||
			(i < firstDeriv.size()-1
				&& firstDeriv[i] > 0
				&& firstDeriv[i+1] < 0))
			peaks.push_back(std::make_pair(i, smoothedHist[i]));
	}
		
	std::sort(peaks.begin(), peaks.end(), [](std::pair<size_t, double> const & a, std::pair<size_t, double> const & b) {
		return a.second > b.second;
	});
	if (chart)
		for (size_t p = 0; p<numberOfPeaks; ++p)
			chart->AddPlot(QSharedPointer<iAPlot>(new iASelectedBinDrawer(peaks[p].first, QColor(90, 90, 90, 182))));
}


void iAQualityMeasureModuleInterface::CalculateQ()
{
	PrepareActiveChild();
	iAChartWidget * chart = new iAChartWidget(m_mdiChild, "Intensity", "Frequency");
	iADockWidgetWrapper* wrapper= new iADockWidgetWrapper(chart, "TestHistogram", "TestHistogram");
	m_mdiChild->SplitDockWidget(m_mdiChild->logs, wrapper, Qt::Horizontal);
	unsigned int NumberOfPeaks = 2;
	double HistogramBinFactor = 0.125;	// should be between 1/4 and 1/8, according to [1]
	double Kderiv = 64; // should be between 64 and 512, according to [1]
	double Kminima = 8; // should be between 8 and 64, according to [1]
	iAConnector con;
	con.SetImage(m_mdiChild->GetModality(0)->GetImage());
	ITK_TYPED_CALL(calculateQ_template, con.GetITKScalarPixelType(), con, HistogramBinFactor,
		m_mdiChild->GetModality(0)->Info().Min(), m_mdiChild->GetModality(0)->Info().Max(),
		NumberOfPeaks, Kderiv, Kminima, chart);

	// [1] M. Reiter, D. Weiß, C. Gusenbauer, M. Erler, C. Kuhn, S. Kasperl, J. Kastner:
	//     Evaluation of a histogram-based image quality measure for X-ray computed tomography
}
