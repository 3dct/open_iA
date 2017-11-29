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

#include "charts/iADiagramFctWidget.h"
#include "dlg_gaussian.h"

void getMeanVariance(std::vector<double> hist, double minVal, double maxVal, size_t minIdx, size_t maxIdx, double & mean, double & variance)
{
	mean = 0;
	double step = (maxVal - minVal) / hist.size();
	double histSum = 0;
	for (size_t i = minIdx; i < maxIdx; ++i)
	{
		double grayvalue = minVal + (i + 0.5)*step;
		mean += hist[i] * grayvalue;
		histSum += hist[i];
	}
	mean /= histSum;

	variance = 0;
	for (size_t i = minIdx; i < maxIdx; ++i)
	{
		double grayvalue = minVal + (i + 0.5)*step;
		variance += hist[i] * (grayvalue - mean)*(grayvalue - mean);
	}
	variance /= histSum;
}

template <typename T>
void calculateQ_template(iAConnector& con, double histogramBinFactor, double minVal, double maxVal,
	unsigned int numberOfPeaks, double Kderiv, double Kminima, double & Q, iAChartWidget* chart, MdiChild* mdichild)
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
	std::vector<double> vecHist;
	for (auto it = histogram->Begin(); it != histogram->End(); ++it)
		vecHist.push_back(it.GetFrequency());

	if (chart)
	{
		auto histoPlotData = iASimpleHistogramData::Create(minVal, maxVal, vecHist, Continuous);
		chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(histoPlotData, QColor(180, 90, 90, 127))));
	}

	double derivSigma = static_cast<double>(binCount) / Kderiv;
	// 2. convolute with gaussian to smooth:
	auto smoothedHist = gaussianSmoothing(vecHist, derivSigma, 5);

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
			&& firstDeriv[i - 1] > 0
			&& firstDeriv[i + 1] < 0) ||
			(i < firstDeriv.size() - 1
				&& firstDeriv[i] > 0
				&& firstDeriv[i + 1] < 0))
			peaks.push_back(std::make_pair(i, smoothedHist[i]));
	}

	std::sort(peaks.begin(), peaks.end(), [](std::pair<size_t, double> const & a, std::pair<size_t, double> const & b) {
		return a.second > b.second;
	});
	if (chart)
		for (size_t p = 0; p < numberOfPeaks; ++p)
			chart->AddPlot(QSharedPointer<iAPlot>(new iASelectedBinDrawer(peaks[p].first, QColor(90, 180, 90, 182))));
	// TODO:
	//   - take first numberOfPeaks peaks
	//   - order by index
	//   - find threshold between each pair
	//   - only then, calculate mean/stddev/Q

	// for now, only consider the first two peaks:
	size_t peak1 = std::min(peaks[0].first, peaks[1].first);
	size_t peak2 = std::max(peaks[0].first, peaks[1].first);
	int peakBinDist = peak2 - peak1;
	double minSigma = peakBinDist / Kminima;

	// find minima between the two peaks:
	auto smoothedHisto2 = gaussianSmoothing(vecHist, minSigma, 10);
	auto deriv2 = derivative(smoothedHisto2);
	if (chart)
	{
		auto smoothedHisto2PlotData = iASimpleHistogramData::Create(minVal, maxVal, smoothedHisto2, Continuous);
		chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(smoothedHisto2PlotData, QColor(90, 180, 180, 127))));
		//auto firstDeriv2PlotData = iASimpleHistogramData::Create(minVal, maxVal, deriv2, Continuous);
		//chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(firstDeriv2PlotData, QColor(90, 180, 180, 127))));
	}
	int minimum = 0;
	for (size_t i = peak1; i < peak2; ++i)
	{
		if ((deriv2[i] == 0
			&& i > 0
			&& i < deriv2.size() - 1
			&& deriv2[i - 1] < 0
			&& deriv2[i + 1] > 0) ||
			(i < deriv2.size() - 1
				&& deriv2[i] < 0
				&& deriv2[i + 1] > 0))
		{
			minimum = i;
			break;
		}
	}
	if (chart)
		chart->AddPlot(QSharedPointer<iAPlot>(new iASelectedBinDrawer(minimum, QColor(180, 90, 90, 182))));

	double * mean = new double[numberOfPeaks];
	double * variance = new double[numberOfPeaks];
	getMeanVariance(vecHist, minVal, maxVal, 0, minimum, mean[0], variance[0]);
	getMeanVariance(vecHist, minVal, maxVal, minimum+1, vecHist.size()-1, mean[1], variance[1]);

	DEBUG_LOG(QString("minimum=%1").arg(minimum));
	for (int p = 0; p < numberOfPeaks; ++p)
	{
		DEBUG_LOG(QString("%1: mean=%2, variance=%3, stddev=%4").arg(p).arg(mean[p]).arg(variance[p]).arg(std::sqrt(variance[p])));
	}
	if (mdichild)
	{
		mdichild->getHistogram()->addGaussianFunction(mean[0], std::sqrt(variance[0]), 1);
		mdichild->getHistogram()->addGaussianFunction(mean[1], std::sqrt(variance[1]), 1);
	}
	Q = std::abs(mean[1] - mean[0]) / std::sqrt(variance[0] + variance[1]);
	DEBUG_LOG(QString("Q = %1").arg(Q));
}


void iAQualityMeasureModuleInterface::CalculateQ()
{
	PrepareActiveChild();
	iAImageInfo info = m_mdiChild->GetModality(0)->Info();
	if (info.Min() == info.Max())
	{
		DEBUG_LOG("Image min and max value not available yet, aborting...")
	}
	iAChartWidget * chart = new iAChartWidget(m_mdiChild, "Intensity", "Frequency");
	iADockWidgetWrapper* wrapper = new iADockWidgetWrapper(chart, "TestHistogram", "TestHistogram");
	m_mdiChild->SplitDockWidget(m_mdiChild->logs, wrapper, Qt::Horizontal);
	unsigned int NumberOfPeaks = 2;
	double HistogramBinFactor = 0.125;	// should be between 1/4 and 1/8, according to [1]
	double Kderiv = 64; // should be between 64 and 512, according to [1]
	double Kminima = 8; // should be between 8 and 64, according to [1]
	iAConnector con;
	double Q;
	con.SetImage(m_mdiChild->GetModality(0)->GetImage());
	ITK_TYPED_CALL(calculateQ_template, con.GetITKScalarPixelType(), con, HistogramBinFactor,
		info.Min(), info.Max(),
		NumberOfPeaks, Kderiv, Kminima, Q, chart, m_mdiChild);

	// [1] M. Reiter, D. Weiß, C. Gusenbauer, M. Erler, C. Kuhn, S. Kasperl, J. Kastner:
	//     Evaluation of a histogram-based image quality measure for X-ray computed tomography
}
