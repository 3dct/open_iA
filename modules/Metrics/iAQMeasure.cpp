/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAQMeasure.h"

#include <charts/iAChartWidget.h>
#include <charts/iADiagramFctWidget.h>
#include <charts/iASimpleHistogramData.h>
#include <charts/iAPlotTypes.h>
#include <defines.h>    // for DIM
#include <iAConnector.h>
#include <iAConsole.h>
#include <iAMathUtility.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iAToolsVTK.h>
#include <iATypedCallHelper.h>
#include <mdichild.h>
#include <qthelper/iADockWidgetWrapper.h>

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

template <typename T> void computeHistogram(iAFilter* filter, size_t binCount,
	double minVal, double maxVal, std::vector<double> & vecHist)
{
	typedef itk::Image<T, DIM>  InputImageType;
	typedef itk::Statistics::ImageToHistogramFilter<InputImageType> ImageHistogramFilterType;
	const int ChannelCount = 1;
	auto histogramFilter = ImageHistogramFilterType::New();
	histogramFilter->ReleaseDataFlagOff();
	histogramFilter->SetInput(dynamic_cast<InputImageType*>(filter->Input()[0]->GetITKImage()));
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
	filter->Progress()->Observe(histogramFilter);
	auto histogram = histogramFilter->GetOutput();
	vecHist.clear();
	for (auto it = histogram->Begin(); it != histogram->End(); ++it)
		vecHist.push_back(it.GetFrequency());
}

void computeQ(iAQMeasure* filter, vtkSmartPointer<vtkImageData> img, QMap<QString, QVariant> const & parameters)
{
	size_t numberOfPeaks = parameters["Number of peaks"].toULongLong();
	double histogramBinFactor = parameters["Histogram bin factor"].toDouble();
	double Kderiv = parameters["Derivative smoothing factor"].toDouble();
	double Kminima = parameters["Minima finding smoothing factor"].toDouble();

	double minVal = img->GetScalarRange()[0];
	double maxVal = img->GetScalarRange()[1];
	auto size =	filter->Input()[0]->GetITKImage()->GetLargestPossibleRegion().GetSize();
	size_t voxelCount = size[0] * size[1] * size[2];
	size_t binCount = std::max(static_cast<size_t>(2), static_cast<size_t>(histogramBinFactor * std::sqrt(voxelCount)));
	std::vector<double> vecHist;

	ITK_TYPED_CALL(computeHistogram, filter->InputPixelType(), filter, binCount, minVal, maxVal, vecHist);
	
	if (filter->m_chart)
	{
		auto histoPlotData = iASimpleHistogramData::Create(minVal, maxVal, vecHist, Continuous);
		filter->m_chart->addPlot(QSharedPointer<iAPlot>(new iABarGraphPlot(histoPlotData, QColor(180, 90, 90, 127))));
	}

	double derivSigma = static_cast<double>(binCount) / Kderiv;
	// 2. convolute with gaussian to smooth:
	auto smoothedHist = gaussianSmoothing(vecHist, derivSigma, 5);

	if (filter->m_chart)
	{
		auto smoothedHistoPlotData = iASimpleHistogramData::Create(minVal, maxVal, smoothedHist, Continuous);
		filter->m_chart->addPlot(QSharedPointer<iAPlot>(new iABarGraphPlot(smoothedHistoPlotData, QColor(90, 180, 90, 127))));
	}

	// 3. find peaks: (derivative = 0, 2nd deriv. negative)
	auto firstDeriv = derivative(smoothedHist);
	auto smoothedDeriv = gaussianSmoothing(firstDeriv, derivSigma, 5);
	if (filter->m_chart)
	{
		auto firstDerivPlotData = iASimpleHistogramData::Create(minVal, maxVal, smoothedDeriv, Continuous);
		filter->m_chart->addPlot(QSharedPointer<iAPlot>(new iABarGraphPlot(firstDerivPlotData, QColor(90, 90, 180, 127))));
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
		//DEBUG_LOG(QString("Only found %1 peaks in total!").arg(peaks.size()));
		if (peaks.size() < 2)
		{
			//DEBUG_LOG(QString("Cannot continue with less than 2 peaks!"));
			if (parameters["Histogram-based SNR (highest non-air-peak)"].toBool())
				filter->AddOutputValue("Histogram-based SNR (highest non-air-peak)", 0);
			filter->AddOutputValue("Q", 0);
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
			filter->m_chart->addPlot(QSharedPointer<iAPlot>(new iASelectedBinPlot(filter->m_chart->plots()[0]->data(), peaks[p].first, QColor(90, 180, 90, 182))));

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
			//filter->m_chart->AddPlot(QSharedPointer<iAPlot>(new iABarGraphPlot(smoothedHisto2PlotData, QColor(90, 180, 180, 127))));
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
		//DEBUG_LOG(QString("Threshold %1: %2 (bin %3)").arg(m).arg(minVal + (minIdx * (maxVal - minVal) / binCount)).arg(minIdx));
		// calculate mean/stddev:
		getMeanVariance(vecHist, minVal, maxVal, thresholdIndices[m], thresholdIndices[m + 1], mean[m], variance[m]);
		if (filter->m_chart)
			filter->m_chart->addPlot(QSharedPointer<iAPlot>(new iASelectedBinPlot(filter->m_chart->plots()[0]->data(), minIdx, QColor(180, 90, 90, 182))));
	}
	// for last peak we still have to calculate mean and stddev
	getMeanVariance(vecHist, minVal, maxVal, thresholdIndices[numberOfPeaks - 1], thresholdIndices[numberOfPeaks], mean[numberOfPeaks - 1], variance[numberOfPeaks - 1]);
	//for (int p = 0; p < numberOfPeaks; ++p)
	//	DEBUG_LOG(QString("Peak %1: mean=%2, variance=%3, stddev=%4").arg(p).arg(mean[p]).arg(variance[p]).arg(std::sqrt(variance[p])));
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
	if (parameters["Histogram-based SNR (highest non-air-peak)"].toBool())
	{
		filter->AddOutputValue("Histogram-based SNR (highest non-air-peak)", mean[highestNonAirPeakIdx] / std::sqrt(variance[highestNonAirPeakIdx]));
	}
	if (parameters["Q metric"].toBool())
	{
		/*		double Q;
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
				if (numberOfPeaks > 2)
				{
					DEBUG_LOG(QString("Q(peak %1, peak %2) = %3").arg(p1).arg(p2).arg(curQ));
				}
			}
		}
		*/
		/*
		for (int i = 0; i < mean.size(); ++i)
		{
			QString peakName(i == minDistToZeroIdx ? "air" : "highest non-air");
			if (i == minDistToZeroIdx || i == highestNonAirPeakIdx)
			{
				filter->AddOutputValue(QString("Mean (%1)").arg(peakName), mean[i]);
				filter->AddOutputValue(QString("Sigma (%1)").arg(peakName), std::sqrt(variance[minDistToZeroIdx]));
				filter->AddOutputValue(QString("Min (%1)").arg(peakName), mapValue(static_cast<size_t>(0), binCount, minVal, maxVal, thresholdIndices[i]));
				filter->AddOutputValue(QString("Max (%1)").arg(peakName), mapValue(static_cast<size_t>(0), binCount, minVal, maxVal, thresholdIndices[i+1]));
			}
		}
		*/
		double Q = calculateQ(mean[highestNonAirPeakIdx], mean[minDistToZeroIdx], variance[highestNonAirPeakIdx], variance[minDistToZeroIdx]);
		filter->AddOutputValue("Q", Q);
	}
}

#include "ImageHistogram.h"

void computeOrigQ(iAFilter* filter, vtkSmartPointer<vtkImageData> img, QMap<QString, QVariant> const & params)
{
	// some "magic numbers"
	unsigned int dgauss_size_BINscale = 24;
	unsigned int gauss_size_P2Pscale = 24;
	double threshold_x = -0.1;
	double threshold_y = 2;						// one single voxel is no valid class

	vtkSmartPointer<vtkImageData> floatImage;
	if (filter->InputPixelType() == itk::ImageIOBase::FLOAT)
		floatImage = img;
	else
		floatImage = CastVTKImage(img, VTK_FLOAT);

	int const * dim = floatImage->GetDimensions();
	double const * range = floatImage->GetScalarRange();
	if (range[0] == range[1])
	{
		filter->AddOutputValue("Q (orig, equ 0)", 0);
		filter->AddOutputValue("Q (orig, equ 1)", 0);
		return;
	}
	float* fImage = static_cast<float*>(floatImage->GetScalarPointer());
	cImageHistogram curHist;
	curHist.CreateHist(fImage, dim[0], dim[1], dim[2],
		params["OrigQ Histogram bins"].toInt(), range[0], range[1], false, 0, 0);
	unsigned int Peaks_fnd = curHist.DetectPeaksValleys(params["Number of peaks"].toInt(),
		dgauss_size_BINscale, gauss_size_P2Pscale, threshold_x, threshold_y, false);

	// Calculate histogram quality measures Q using the valley thresholds to seperate classes
	std::vector<int> thresholds_IDX = curHist.GetValleyThreshold_IDX();
	std::vector<float> thresholds = curHist.GetValleyThreshold();
	std::vector<ClassMeasure> classMeasures;
	double Q0 = (thresholds_IDX.size() == 0) ? 0.0 : curHist.CalcQ(thresholds_IDX, classMeasures, 0);
	double Q1 = (thresholds_IDX.size() == 0) ? 0.0 : curHist.CalcQ(thresholds_IDX, classMeasures, 1);
	filter->AddOutputValue("Q (orig, equ 0)", Q0);
	filter->AddOutputValue("Q (orig, equ 1)", Q1);

	/*
	int classNr = 0;
	for (auto c: classMeasures)
	{
		QString peakName(c.UsedForQ == 1 ? "air" : "highest non-air");
		if (c.UsedForQ == 1 || c.UsedForQ == 2)
		{
			filter->AddOutputValue(QString("Qorig Mean (%1)").arg(peakName), c.mean);
			filter->AddOutputValue(QString("Qorig Sigma (%1)").arg(peakName), c.sigma);
			filter->AddOutputValue(QString("Probability (%1)").arg(peakName), c.probability);
			filter->AddOutputValue(QString("Min (%1)").arg(peakName), c.LowerThreshold);
			filter->AddOutputValue(QString("Max (%1)").arg(peakName), c.UpperThreshold);
		}
		++classNr;
	}
	*/
}


void iAQMeasure::PerformWork(QMap<QString, QVariant> const & parameters)
{
	size_t size[3], index[3];
	size[0] = parameters["Size X"].toUInt(); size[1] = parameters["Size Y"].toUInt(); size[2] = parameters["Size Z"].toUInt();
	index[0] = parameters["Index X"].toUInt(); index[1] = parameters["Index Y"].toUInt(); index[2] = parameters["Index Z"].toUInt();
	auto extractImg = ExtractImage(Input()[0]->GetITKImage(), index, size);
	iAConnector extractCon;
	extractCon.SetImage(extractImg);
	computeQ(this, extractCon.GetVTKImage(), parameters);
	computeOrigQ(this, extractCon.GetVTKImage(), parameters);
}

IAFILTER_CREATE(iAQMeasure)

iAQMeasure::iAQMeasure() :
	iAFilter("Image Quality", "Metrics",
		"Computes the Q metric, as well as optionally a histogram-based Signal-to-noise ratio.<br/>"
		"For more information on the Q metric, see "
		"<a href=\"http://www.ndt.net/article/ctc2014/papers/273.pdf\">M. Reiter, D. Weiss, C. Gusenbauer, "
		"J. Kastner, M. Erler, S. Kasperl: Evaluation of a histogram based image quality measure for X-ray "
		"computed tomography. Proceedings of Conference on Industrial Computed Tomography (iCT2014), Wels, "
		"Österreich, 2014.</a>", 1, 0),
	m_chart(nullptr),
	m_mdiChild(nullptr)
{
	AddParameter("Index X", Discrete, 0);
	AddParameter("Index Y", Discrete, 0);
	AddParameter("Index Z", Discrete, 0);
	AddParameter("Size X", Discrete, 1);
	AddParameter("Size Y", Discrete, 1);
	AddParameter("Size Z", Discrete, 1);
	AddParameter("Histogram-based SNR (highest non-air-peak)", Boolean, true);
	AddParameter("Q metric", Boolean, true);
	AddParameter("Number of peaks", Discrete, 2, 2);
	AddParameter("Histogram bin factor"       , Continuous, 0.125, 0.0000001);
	AddParameter("Derivative smoothing factor", Continuous,    64, 0.0000001);
	AddParameter("Minima finding smoothing factor", Continuous, 8, 0.0000001);

	AddParameter("OrigQ Histogram bins", Discrete, 512, 2);

	AddOutputValue("Histogram-based SNR (highest non-air-peak)");
	AddOutputValue("Q");
	AddOutputValue("Q (orig, equ 0)");
	AddOutputValue("Q (orig, equ 1)");
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

IAFILTER_CREATE(iASNR)

iASNR::iASNR() :
	iAFilter("Signal-to-Noise Ratio", "Metrics",
		"Computes the Signal-to-noise ratio as (mean / stddev) of the given image region.<br/>", 1, 0)
{
	AddParameter("Index X", Discrete, 0);
	AddParameter("Index Y", Discrete, 0);
	AddParameter("Index Z", Discrete, 0);
	AddParameter("Size X", Discrete, 1);
	AddParameter("Size Y", Discrete, 1);
	AddParameter("Size Z", Discrete, 1);
	AddOutputValue("Signal-to-Noise Ratio");
}

void iASNR::PerformWork(QMap<QString, QVariant> const & parameters)
{
	size_t size[3], index[3];
	size[0] = parameters["Size X"].toUInt(); size[1] = parameters["Size Y"].toUInt(); size[2] = parameters["Size Z"].toUInt();
	index[0] = parameters["Index X"].toUInt(); index[1] = parameters["Index Y"].toUInt(); index[2] = parameters["Index Z"].toUInt();
	auto extractImg = ExtractImage(Input()[0]->GetITKImage(), index, size);
	double mean, stddev;
	getStatistics(extractImg, nullptr, nullptr, &mean, &stddev);
	AddOutputValue("Signal-to-Noise Ratio", mean / stddev);
}

IAFILTER_CREATE(iACNR)

iACNR::iACNR() :
	iAFilter("Contrast-to-Noise Ratio", "Metrics",
		"Computes the Contrast-to-noise ratio as (mean(region2) - mean(region1)) / stddev(region2).<br/>"
		"Region 1 should typically contain a homogeneous area of surroundings (air), "
		"while region 2 should typically contain a homogeneous region of material", 1, 0)
{
	AddParameter("Region 1 Index X", Discrete, 0);
	AddParameter("Region 1 Index Y", Discrete, 0);
	AddParameter("Region 1 Index Z", Discrete, 0);
	AddParameter("Region 1 Size X" , Discrete, 1);
	AddParameter("Region 1 Size Y" , Discrete, 1);
	AddParameter("Region 1 Size Z" , Discrete, 1);
	AddParameter("Region 2 Index X", Discrete, 0);
	AddParameter("Region 2 Index Y", Discrete, 0);
	AddParameter("Region 2 Index Z", Discrete, 0);
	AddParameter("Region 2 Size X" , Discrete, 1);
	AddParameter("Region 2 Size Y" , Discrete, 1);
	AddParameter("Region 2 Size Z" , Discrete, 1);
	AddOutputValue("Contrast-to-Noise Ratio");
}

void iACNR::PerformWork(QMap<QString, QVariant> const & parameters)
{
	size_t size[3], index[3];
	size[0] = parameters["Region 1 Size X"].toUInt(); size[1] = parameters["Region 1 Size Y"].toUInt(); size[2] = parameters["Region 1 Size Z"].toUInt();
	index[0] = parameters["Region 1 Index X"].toUInt(); index[1] = parameters["Region 1 Index Y"].toUInt(); index[2] = parameters["Region 1 Index Z"].toUInt();
	auto extractImg1 = ExtractImage(Input()[0]->GetITKImage(), index, size);
	size[0] = parameters["Region 2 Size X"].toUInt(); size[1] = parameters["Region 2 Size Y"].toUInt(); size[2] = parameters["Region 2 Size Z"].toUInt();
	index[0] = parameters["Region 2 Index X"].toUInt(); index[1] = parameters["Region 2 Index Y"].toUInt(); index[2] = parameters["Region 2 Index Z"].toUInt();
	auto extractImg2 = ExtractImage(Input()[0]->GetITKImage(), index, size);
	double mean1, mean2, stddev2;
	getStatistics(extractImg1, nullptr, nullptr, &mean1, nullptr);
	getStatistics(extractImg2, nullptr, nullptr, &mean2, &stddev2);
	AddOutputValue("Contrast-to-Noise Ratio", (mean2 - mean1) / stddev2);
}