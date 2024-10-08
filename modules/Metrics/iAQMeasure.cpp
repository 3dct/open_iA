// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAQMeasure.h"

// base
#include <defines.h>    // for DIM
#include <iAConnector.h>
#include <iAImageData.h>
#include <iALog.h>
#include <iAMathUtility.h>
#include <iAProgress.h>
#include <iAToolsITK.h>
#include <iATypedCallHelper.h>
#include <iAValueTypeVectorHelpers.h>

// charts
#include <iAChartWithFunctionsWidget.h>
#include <iAHistogramData.h>
#include <iAPlotTypes.h>

// guibase
#include <iADockWidgetWrapper.h>
#include <iAMdiChild.h>

#include <QMeasureCalculation.h>

#include <itkImage.h>
#ifdef __clang__
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic push
#if __clang_major__ > 10
#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#else
#pragma clang diagnostic ignored "-Wimplicit-int-float-conversion"
#endif
#endif
#include <itkImageToHistogramFilter.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <vtkImageData.h>
#include <vtkMath.h>

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
		if (histSum == 0)
		{
			mean = 0;
			variance = 0;
			return;
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
	histogramFilter->SetInput(dynamic_cast<InputImageType*>(filter->imageInput(0)->itkImage()));
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
	filter->progress()->observe(histogramFilter);
	auto histogram = histogramFilter->GetOutput();
	vecHist.clear();
	for (auto it = histogram->Begin(); it != histogram->End(); ++it)
	{
		vecHist.push_back(it.GetFrequency());
	}
}

void computeQ(iAQMeasure* filter, vtkSmartPointer<vtkImageData> img, QVariantMap const & parameters)
{
	size_t numberOfPeaks = parameters["Number of peaks"].toULongLong();
	double histogramBinFactor = parameters["Histogram bin factor"].toDouble();
	double Kderiv = parameters["Derivative smoothing factor"].toDouble();
	double Kminima = parameters["Minima finding smoothing factor"].toDouble();

	double minVal = img->GetScalarRange()[0];
	double maxVal = img->GetScalarRange()[1];
	auto size =	filter->imageInput(0)->itkImage()->GetLargestPossibleRegion().GetSize();
	size_t voxelCount = size[0] * size[1] * size[2];
	size_t binCount = std::max(static_cast<size_t>(2), static_cast<size_t>(histogramBinFactor * std::sqrt(voxelCount)));
	std::vector<double> vecHist;

	ITK_TYPED_CALL(computeHistogram, filter->inputScalarType(), filter, binCount, minVal, maxVal, vecHist);

	if (filter->m_chart)
	{
		auto histoPlotData = iAHistogramData::create("Frequency", iAValueType::Continuous, minVal, maxVal, vecHist);
		filter->m_chart->addPlot(std::make_shared<iABarGraphPlot>(histoPlotData, QColor(180, 90, 90, 127)));
	}

	double derivSigma = static_cast<double>(binCount) / Kderiv;
	// 2. convolute with gaussian to smooth:
	auto smoothedHist = gaussianSmoothing(vecHist, derivSigma, 5);

	if (filter->m_chart)
	{
		auto smoothedHistoPlotData = iAHistogramData::create("Smoothed Frequency", iAValueType::Continuous, minVal, maxVal, smoothedHist);
		filter->m_chart->addPlot(std::make_shared<iABarGraphPlot>(smoothedHistoPlotData, QColor(90, 180, 90, 127)));
	}

	// 3. find peaks: (derivative = 0, 2nd deriv. negative)
	auto firstDeriv = derivative(smoothedHist);
	auto smoothedDeriv = gaussianSmoothing(firstDeriv, derivSigma, 5);
	if (filter->m_chart)
	{
		auto firstDerivPlotData = iAHistogramData::create("Smoothed Derivative", iAValueType::Continuous, minVal, maxVal, smoothedDeriv);
		filter->m_chart->addPlot(std::make_shared<iABarGraphPlot>(firstDerivPlotData, QColor(90, 90, 180, 127)));
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
		{
			peaks.push_back(std::make_pair(i, smoothedHist[i]));
		}
	}
	if (peaks.size() < numberOfPeaks)
	{
		//LOG(lvlWarn, QString("Only found %1 peaks in total!").arg(peaks.size()));
		if (peaks.size() < 2)
		{
			//LOG(lvlWarn, QString("Cannot continue with less than 2 peaks!"));
			if (parameters["Histogram-based SNR (highest non-air-peak)"].toBool())
			{
				filter->addOutputValue("Histogram-based SNR (highest non-air-peak)", 0);
			}
			filter->addOutputValue("Q", 0);
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
	{
		for (size_t p = 0; p < numberOfPeaks; ++p)
		{
			filter->m_chart->addPlot(std::make_shared<iASelectedBinPlot>(filter->m_chart->plots()[0]->data(), peaks[p].first, QColor(90, 180, 90, 182)));
		}
	}
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
			//auto smoothedHisto2PlotData = iAHistogramData::create(minVal, maxVal, smoothedHistoMin, iAValueType::Continuous);
			//filter->m_chart->AddPlot(std::make_shared<iABarGraphPlot>(smoothedHisto2PlotData, QColor(90, 180, 180, 127)));
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
		//LOG(lvlInfo, QString("Threshold %1: %2 (bin %3)").arg(m).arg(minVal + (minIdx * (maxVal - minVal) / binCount)).arg(minIdx));
		// calculate mean/stddev:
		getMeanVariance(vecHist, minVal, maxVal, thresholdIndices[m], thresholdIndices[m + 1], mean[m], variance[m]);
		if (filter->m_chart)
			filter->m_chart->addPlot(std::make_shared<iASelectedBinPlot>(filter->m_chart->plots()[0]->data(), minIdx, QColor(180, 90, 90, 182)));
	}
	// for last peak we still have to calculate mean and stddev
	getMeanVariance(vecHist, minVal, maxVal, thresholdIndices[numberOfPeaks - 1], thresholdIndices[numberOfPeaks], mean[numberOfPeaks - 1], variance[numberOfPeaks - 1]);
	//for (int p = 0; p < numberOfPeaks; ++p)
	//	LOG(lvlInfo, QString("Peak %1: mean=%2, variance=%3, stddev=%4").arg(p).arg(mean[p]).arg(variance[p]).arg(std::sqrt(variance[p])));
	if (filter->m_chart)
	{
		for (size_t p = 0; p < numberOfPeaks; ++p)
		{
			double sigma = std::sqrt(variance[p]);
			// TODO NEWIO: test
			double binDifferenceFactor = 1;// static_cast<double>(binCount) / filter->m_mdiChild->preferences().HistogramBins;
			double multiplier = binDifferenceFactor * vecHist[peaks[p].first] * sigma * std::sqrt(2 * vtkMath::Pi());
			filter->m_chart->addGaussianFunction(mean[p], sigma, multiplier);
		}
	}

	// find out which of the peaks is closest to 0 (air)
	double minDistToZero = std::numeric_limits<double>::max();
	const size_t NoIdx = std::numeric_limits<size_t>::max();
	size_t minDistToZeroIdx = NoIdx;
	for (size_t p = 0; p < numberOfPeaks; ++p)
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
	size_t highestNonAirPeakIdx = NoIdx;
	for (size_t p = 0; p < numberOfPeaks; ++p)
	{
		if (p == minDistToZeroIdx)
		{
			continue;
		}
		if (peaks[p].second > highestNonAirPeakValue)
		{
			highestNonAirPeakValue = peaks[p].second;
			highestNonAirPeakIdx = p;
		}
	}
	if (minDistToZeroIdx == NoIdx || highestNonAirPeakIdx == NoIdx)
	{
		LOG(lvlError, "No index for peak close to zero or highest non-air peak found!");
		return;
	}
	if (parameters["Histogram-based SNR (highest non-air-peak)"].toBool())
	{
		filter->addOutputValue("Histogram-based SNR (highest non-air-peak)", mean[highestNonAirPeakIdx] / std::sqrt(variance[highestNonAirPeakIdx]));
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
					LOG(lvlInfo, QString("Q(peak %1, peak %2) = %3").arg(p1).arg(p2).arg(curQ));
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
				filter->addOutputValue(QString("Mean (%1)").arg(peakName), mean[i]);
				filter->addOutputValue(QString("Sigma (%1)").arg(peakName), std::sqrt(variance[minDistToZeroIdx]));
				filter->addOutputValue(QString("Min (%1)").arg(peakName), mapValue(static_cast<size_t>(0), binCount, minVal, maxVal, thresholdIndices[i]));
				filter->addOutputValue(QString("Max (%1)").arg(peakName), mapValue(static_cast<size_t>(0), binCount, minVal, maxVal, thresholdIndices[i+1]));
			}
		}
		*/
		double Q = calculateQ(mean[highestNonAirPeakIdx], mean[minDistToZeroIdx], variance[highestNonAirPeakIdx], variance[minDistToZeroIdx]);
		filter->addOutputValue("Q", Q);
	}
}

#include "ImageHistogram.h"

void computeOrigQ(iAFilter* filter, iAConnector & con, QVariantMap const & params)
{
	iAConnector floatImage;
	if (filter->inputScalarType() == iAITKIO::ScalarType::FLOAT)
	{
		floatImage = con;
	}
	else
	{
		floatImage.setImage(castImageTo<float>(con.itkImage()));
	}
	vtkSmartPointer<vtkImageData> img = floatImage.vtkImage();
	int const * dim = img->GetDimensions();
	double const * range = img->GetScalarRange();
	if (range[0] == range[1])
	{
		filter->addOutputValue("Q (orig, equ 0)", 0);
		filter->addOutputValue("Q (orig, equ 1)", 0);
		return;
	}
	float* fImage = static_cast<float*>(img->GetScalarPointer());
	auto result = QMeasureCalculation::computeOrigQ(fImage, dim, range, params["OrigQ Histogram bins"].toInt(),
		params["Number of peaks"].toInt(), params["Analyze Peaks"].toBool());
	for (auto c : result)
	{
		filter->addOutputValue(QString::fromStdString(c.first), c.second);
	}
}

void iAQMeasure::performWork(QVariantMap const & parameters)
{
	size_t size[3], index[3];
	setFromVectorVariant<int>(size, parameters["Size"]);
	setFromVectorVariant<int>(index, parameters["Index"]);

	auto extractImg = extractImage(imageInput(0)->itkImage(), index, size);
	iAConnector extractCon;
	extractCon.setImage(extractImg);
	computeQ(this, extractCon.vtkImage(), parameters);
	computeOrigQ(this, extractCon, parameters);
}

iAQMeasure::iAQMeasure() :
	iAFilter("Image Quality", "Metrics",
		"Computes the Q metric, as well as optionally a histogram-based Signal-to-noise ratio.<br/>"
		"If <em>Analyze Peaks</em> is enabled, a new chart will be shown with the computed smoothed histograms and their derivatives, "
		"and the output will contain information on the determined peaks."
		"The 'Usage' will show whether a peak was used as air peak (=1), as highest non-air peak (=2) or not at all (other value)."
		"For more information on the Q metric, see "
		"<a href=\"http://www.ndt.net/article/ctc2014/papers/273.pdf\">M. Reiter, D. Weiss, C. Gusenbauer, "
		"J. Kastner, M. Erler, S. Kasperl: Evaluation of a histogram based image quality measure for X-ray "
		"computed tomography. Proceedings of Conference on Industrial Computed Tomography (iCT2014), Wels, "
		"Österreich, 2014.</a>", 1, 0),
	m_chart(nullptr)
{
	addParameter("Index", iAValueType::Vector3i, variantVector<int>({0, 0, 0}));
	addParameter("Size", iAValueType::Vector3i, variantVector<int>({1, 1, 1}));
	addParameter("Histogram-based SNR (highest non-air-peak)", iAValueType::Boolean, true);
	addParameter("Q metric", iAValueType::Boolean, true);
	addParameter("Number of peaks", iAValueType::Discrete, 2, 2);
	addParameter("Histogram bin factor"       , iAValueType::Continuous, 0.125, 0.0000001);
	addParameter("Derivative smoothing factor", iAValueType::Continuous,    64, 0.0000001);
	addParameter("Minima finding smoothing factor", iAValueType::Continuous, 8, 0.0000001);
	addParameter("OrigQ Histogram bins", iAValueType::Discrete, 512, 2);
	addParameter("Analyze Peaks", iAValueType::Boolean, false);

	addOutputValue("Histogram-based SNR (highest non-air-peak)");
	addOutputValue("Q");
	addOutputValue("Q (orig, equ 0)");
	addOutputValue("Q (orig, equ 1)");
}

void iAQMeasure::setupDebugGUI(iAChartWithFunctionsWidget* chart)
{
	m_chart = chart;
}


IAFILTER_RUNNER_CREATE(iAQMeasureRunner);

void iAQMeasureRunner::filterGUIPreparations(std::shared_ptr<iAFilter> filter,
	iAMdiChild* mdiChild, iAMainWindow* /*mainWnd*/, QVariantMap const& params)
{
	if (params["Analyze Peaks"].toBool())
	{
		auto chart = new iAChartWithFunctionsWidget(mdiChild, "Intensity", "Frequency");
		// TODO: check if exists before recreating?
		iADockWidgetWrapper* wrapper = new iADockWidgetWrapper(chart, "Q Measure Peak Analysis", "QPeakAnalysis", "https://github.com/3dct/open_iA/wiki/Filters");
		mdiChild->splitDockWidget(mdiChild->renderDockWidget(), wrapper, Qt::Horizontal);
		iAQMeasure* qfilter = dynamic_cast<iAQMeasure*>(filter.get());
		qfilter->setupDebugGUI(chart);
	}
}

iASNR::iASNR() :
	iAFilter("Signal-to-Noise Ratio", "Metrics",
		"Computes the Signal-to-noise ratio as (mean / stddev) of the given image region.<br/>", 1, 0)
{
	addParameter("Index", iAValueType::Vector3i, variantVector<int>({ 0, 0, 0 }));
	addParameter("Size", iAValueType::Vector3i, variantVector<int>({ 1, 1, 1 }));
	addOutputValue("Signal-to-Noise Ratio");
}

void iASNR::performWork(QVariantMap const & parameters)
{
	size_t size[3], index[3];
	setFromVectorVariant<int>(size, parameters["Size"]);
	setFromVectorVariant<int>(index, parameters["Index"]);
	auto extractImg = extractImage(imageInput(0)->itkImage(), index, size);
	double mean, stddev;
	getStatistics(extractImg, nullptr, nullptr, &mean, &stddev);
	addOutputValue("Signal-to-Noise Ratio", mean / stddev);
}

iACNR::iACNR() :
	iAFilter("Contrast-to-Noise Ratio", "Metrics",
		"Computes the Contrast-to-noise ratio as (mean(region2) - mean(region1)) / stddev(region1).<br/>"
		"Region 1 should typically contain a homogeneous area of surroundings (air), "
		"while region 2 should typically contain a homogeneous region of material", 1, 0)
{
	addParameter("Region 1 Index", iAValueType::Vector3i, variantVector<int>({ 0, 0, 0 }));
	addParameter("Region 1 Size", iAValueType::Vector3i, variantVector<int>({ 1, 1, 1 }));
	addParameter("Region 2 Index", iAValueType::Vector3i, variantVector<int>({ 0, 0, 0 }));
	addParameter("Region 2 Size", iAValueType::Vector3i, variantVector<int>({ 1, 1, 1 }));
	addOutputValue("Contrast-to-Noise Ratio");
}

void iACNR::performWork(QVariantMap const & parameters)
{
	size_t size[3], index[3];
	setFromVectorVariant<int>(size,  parameters["Region 1 Size"]);
	setFromVectorVariant<int>(index, parameters["Region 1 Index"]);
	auto extractImg1 = extractImage(imageInput(0)->itkImage(), index, size);
	setFromVectorVariant<int>(size,  parameters["Region 2 Size"]);
	setFromVectorVariant<int>(index, parameters["Region 2 Index"]);
	auto extractImg2 = extractImage(imageInput(0)->itkImage(), index, size);
	double mean1, mean2, stddev1;
	getStatistics(extractImg1, nullptr, nullptr, &mean1, &stddev1);
	getStatistics(extractImg2, nullptr, nullptr, &mean2, nullptr);
	addOutputValue("Contrast-to-Noise Ratio", (mean2 - mean1) / stddev1);
}
