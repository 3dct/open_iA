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

#include "iAQMeasure.h"

#include "charts/iAChartWidget.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iASimpleHistogramData.h"
#include "charts/iAPlotTypes.h"
#include "iADockWidgetWrapper.h"
#include "defines.h"    // for DIM
#include "iAConnector.h"
#include "iAConsole.h"
#include "iAMathUtility.h"
#include "iAProgress.h"
#include "iATypedCallHelper.h"
#include "mdichild.h"

#include <itkImage.h>
#include <itkImageToHistogramFilter.h>

#include <vtkImageData.h>

namespace
{
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

	double calculateQ(double mean1, double mean2, double variance1, double variance2)
	{
		return std::abs(mean2 - mean1) / std::sqrt(variance1 + variance2);
	}
}

template <typename T> void computeHistogram(iAConnector* con, iAProgress* progress, size_t binCount,
	double minVal, double maxVal, std::vector<double> & vecHist)
{
	typedef itk::Image<T, DIM>  InputImageType;
	typedef itk::Statistics::ImageToHistogramFilter<InputImageType> ImageHistogramFilterType;
	const int ChannelCount = 1;
	auto histogramFilter = ImageHistogramFilterType::New();
	histogramFilter->ReleaseDataFlagOff();
	histogramFilter->SetInput(dynamic_cast<InputImageType*>(con->GetITKImage()));
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
	progress->Observe(histogramFilter);
	auto histogram = histogramFilter->GetOutput();
	vecHist.clear();
	for (auto it = histogram->Begin(); it != histogram->End(); ++it)
		vecHist.push_back(it.GetFrequency());
}

void computeQ(iAConnector* con, QMap<QString, QVariant> const & parameters, iAQMeasure* filter, iAProgress* progress)
{
	size_t numberOfPeaks = parameters["Number of peaks"].toULongLong();
	double histogramBinFactor = parameters["Histogram bin factor"].toDouble();
	double Kderiv = parameters["Derivative smoothing factor"].toDouble();
	double Kminima = parameters["Minima finding smoothing factor"].toDouble();

	double minVal = con->GetVTKImage()->GetScalarRange()[0];
	double maxVal = con->GetVTKImage()->GetScalarRange()[1];
	auto size =	con->GetITKImage()->GetLargestPossibleRegion().GetSize();
	size_t voxelCount = size[0] * size[1] * size[2];
	size_t binCount = static_cast<size_t>(histogramBinFactor * std::sqrt(voxelCount));
	std::vector<double> vecHist;

	ITK_TYPED_CALL(computeHistogram, con->GetITKScalarPixelType(), con, progress, binCount, minVal, maxVal, vecHist);
	
	if (filter->m_chart)
	{
		auto histoPlotData = iASimpleHistogramData::Create(minVal, maxVal, vecHist, Continuous);
		filter->m_chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(histoPlotData, QColor(180, 90, 90, 127))));
	}

	double derivSigma = static_cast<double>(binCount) / Kderiv;
	// 2. convolute with gaussian to smooth:
	auto smoothedHist = gaussianSmoothing(vecHist, derivSigma, 5);

	if (filter->m_chart)
	{
		auto smoothedHistoPlotData = iASimpleHistogramData::Create(minVal, maxVal, smoothedHist, Continuous);
		filter->m_chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(smoothedHistoPlotData, QColor(90, 180, 90, 127))));
	}

	// 3. find peaks: (derivative = 0, 2nd deriv. negative)
	auto firstDeriv = derivative(smoothedHist);
	auto smoothedDeriv = gaussianSmoothing(firstDeriv, derivSigma, 5);
	if (filter->m_chart)
	{
		auto firstDerivPlotData = iASimpleHistogramData::Create(minVal, maxVal, smoothedDeriv, Continuous);
		filter->m_chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(firstDerivPlotData, QColor(90, 90, 180, 127))));
	}

	// peak is at every 0-crossing, so where:
	//      - either deriv is 0, deriv is pos before and neg afterwards (pot. use 2nd deriv?)
	//      - or before deriv. is pos. and afterwards deriv. is neg.:
	std::vector<std::pair<size_t, double> > peaks;
	for (size_t i = 0; i < smoothedDeriv.size(); ++i)
	{
		if ((smoothedDeriv[i] == 0
			&& i > 0
			&& i < smoothedDeriv.size() - 1
			&& smoothedDeriv[i - 1] > 0
			&& smoothedDeriv[i + 1] < 0) ||
			(i < smoothedDeriv.size() - 1
				&& smoothedDeriv[i] > 0
				&& smoothedDeriv[i + 1] < 0))
			peaks.push_back(std::make_pair(i, smoothedHist[i]));
	}
	if (peaks.size() < numberOfPeaks)
	{
		DEBUG_LOG(QString("Only found %1 peaks in total!").arg(peaks.size()));
		if (peaks.size() < 2)
		{
			DEBUG_LOG(QString("Cannot continue with less than 2 peaks!"));
			return;
		}
		numberOfPeaks = peaks.size();
	}
	// order by peak height, descending:
	std::sort(peaks.begin(), peaks.end(), [](std::pair<size_t, double> const & a, std::pair<size_t, double> const & b) {
		return a.second > b.second;
	});
	peaks.resize(numberOfPeaks);		// only consider numberOfPeaks peaks
	if (filter->m_chart)
		for (size_t p = 0; p < numberOfPeaks; ++p)
			filter->m_chart->AddPlot(QSharedPointer<iAPlot>(new iASelectedBinDrawer(peaks[p].first, QColor(90, 180, 90, 182))));

										// order peaks by index
	std::sort(peaks.begin(), peaks.end(), [](std::pair<size_t, double> const & a, std::pair<size_t, double> const & b) {
		return a.first < b.first;
	});

	// find threshold=minimum between each pair

	std::vector<size_t> thresholdIndices(numberOfPeaks + 1);
	std::vector<double> mean(numberOfPeaks);
	std::vector<double> variance(numberOfPeaks);
	thresholdIndices[0] = 0;
	thresholdIndices[numberOfPeaks] = binCount;
	for (size_t m = 0; m < numberOfPeaks - 1; ++m)
	{
		int peakBinDist = peaks[m + 1].first - peaks[m].first;
		double minSigma = peakBinDist / Kminima;
		// find minimum between the two peaks:
		auto smoothedHistoMin = gaussianSmoothing(vecHist, minSigma, 10);
		if (filter->m_chart)
		{
			//auto smoothedHisto2PlotData = iASimpleHistogramData::Create(minVal, maxVal, smoothedHistoMin, Continuous);
			//filter->m_chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphDrawer(smoothedHisto2PlotData, QColor(90, 180, 180, 127))));
		}
		int minIdx = peaks[m].first;
		double curMinFreq = peaks[m].second;
		for (size_t i = minIdx + 1; i < peaks[m + 1].first; ++i)
		{
			if (smoothedHistoMin[i] < curMinFreq)
			{
				minIdx = i;
				curMinFreq = smoothedHistoMin[i];
			}
		}
		thresholdIndices[m + 1] = minIdx;
		DEBUG_LOG(QString("Threshold %1: %2 (bin %3)").arg(m).arg(minVal + (minIdx * (maxVal - minVal) / binCount)).arg(minIdx));
		// calculate mean/stddev:
		getMeanVariance(vecHist, minVal, maxVal, thresholdIndices[m], thresholdIndices[m + 1], mean[m], variance[m]);
		if (filter->m_chart)
			filter->m_chart->AddPlot(QSharedPointer<iAPlot>(new iASelectedBinDrawer(minVal, QColor(180, 90, 90, 182))));
	}
	// for last peak we still have to calculate mean and stddev
	getMeanVariance(vecHist, minVal, maxVal, thresholdIndices[numberOfPeaks - 1], thresholdIndices[numberOfPeaks], mean[numberOfPeaks - 1], variance[numberOfPeaks - 1]);
	for (int p = 0; p < numberOfPeaks; ++p)
		DEBUG_LOG(QString("Peak %1: mean=%2, variance=%3, stddev=%4").arg(p).arg(mean[p]).arg(variance[p]).arg(std::sqrt(variance[p])));
	if (filter->m_mdiChild)
		for (int p = 0; p < numberOfPeaks; ++p)
			filter->m_mdiChild->getHistogram()->addGaussianFunction(mean[p], std::sqrt(variance[p]), 15);

	// find out which of the peaks is closest to 0 (air)
	double minDistToZero = std::numeric_limits<double>::max();
	size_t minDistToZeroIdx = -1;
	for (int p = 0; p < numberOfPeaks; ++p)
	{
		double curDistToZero = std::abs(minVal + peaks[p].first * (maxVal - minVal) / binCount);
		if (curDistToZero < minDistToZero)
		{
			minDistToZero = curDistToZero;
			minDistToZeroIdx = p;
		}
	}

	// find out which of the non-air peaks is highest:
	double highestNonAirPeakValue = std::numeric_limits<double>::lowest();
	size_t highestNonAirPeakIdx = -1;
	for (int p = 0; p < numberOfPeaks; ++p)
	{
		if (p == minDistToZeroIdx)
			continue;
		if (peaks[p].second > highestNonAirPeakValue)
		{
			highestNonAirPeakValue = peaks[p].second;
			highestNonAirPeakIdx = p;
		}
	}
	if (parameters["Signal-to-noise ratio"].toBool())
	{
		filter->AddOutputValue("Signal-to-noise ratio", mean[highestNonAirPeakIdx] / std::sqrt(variance[minDistToZeroIdx]));
	}
	if (parameters["Contrast-to-noise ratio"].toBool())
	{
		filter->AddOutputValue("Conrast-to-noise ratio", (mean[highestNonAirPeakIdx]- mean[minDistToZeroIdx]) / std::sqrt(variance[minDistToZeroIdx]));
	}
	if (parameters["Q metric"].toBool())
	{
		std::vector<double> q;
		double Q;
		for (int p1 = 0; p1 < numberOfPeaks; ++p1)
		{
			for (int p2 = p1 + 1; p2 < numberOfPeaks; ++p2)
			{
				double curQ = calculateQ(mean[p1], mean[p2], variance[p1], variance[p2]);
				if ((p1 == minDistToZeroIdx || p2 == minDistToZeroIdx) &&
					(p2 == highestNonAirPeakIdx || p2 == highestNonAirPeakIdx))
				{
					Q = curQ;
				}
				DEBUG_LOG(QString("Q(peak %1, peak %2) = %3").arg(p1).arg(p2).arg(curQ));
			}
		}
		filter->AddOutputValue("Q", Q);
	}
}

void iAQMeasure::PerformWork(QMap<QString, QVariant> const & parameters)
{
	computeQ(m_con, parameters, this, m_progress);
}

IAFILTER_CREATE(iAQMeasure)

iAQMeasure::iAQMeasure() :
	iAFilter("Image Quality", "Metrics",
		"Computes the Signal-to-noise and Contrast-to-noise ratio as well as the Q metric.<br/>"
		"For more information on the Q metric, see "
		"<a href=\"http://www.ndt.net/article/ctc2014/papers/273.pdf\">M. Reiter, D. Weiss, C. Gusenbauer, "
		"J. Kastner, M. Erler, S. Kasperl: Evaluation of a histogram based image quality measure for X-ray "
		"computed tomography. Proceedings of Conference on Industrial Computed Tomography (iCT2014), Wels, "
		"Österreich, 2014.</a>", 1, 0),
	m_chart(nullptr),
	m_mdiChild(nullptr)
{
	AddParameter("Signal-to-noise ratio", Boolean, true);
	AddParameter("Contrast-to-noise ratio", Boolean, true);
	AddParameter("Q metric", Boolean, true);
	AddParameter("Number of peaks", Discrete, 2, 2);
	AddParameter("Histogram bin factor"       , Continuous, 0.125, 0.0000001);
	AddParameter("Derivative smoothing factor", Continuous,    64, 0.0000001);
	AddParameter("Minima finding smoothing factor", Continuous, 8, 0.0000001);
}

void iAQMeasure::SetupDebugGUI(iAChartWidget* chart, MdiChild* mdiChild)
{
	m_chart = chart;
	m_mdiChild = mdiChild;
}


IAFILTER_RUNNER_CREATE(iAQMeasureRunner);

void iAQMeasureRunner::FilterGUIPreparations(QSharedPointer<iAFilter> filter, MdiChild* mdiChild, MainWindow* mainWnd)
{
	iAChartWidget * chart = new iAChartWidget(mdiChild, "Intensity", "Frequency");
	iADockWidgetWrapper* wrapper = new iADockWidgetWrapper(chart, "TestHistogram", "TestHistogram");
	mdiChild->SplitDockWidget(mdiChild->logs, wrapper, Qt::Horizontal);
	iAQMeasure* qfilter = dynamic_cast<iAQMeasure*>(filter.data());
	qfilter->SetupDebugGUI(chart, mdiChild);
}
