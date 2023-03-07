// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAHistogramData.h"

#include "iALog.h"
#include "iAMathUtility.h"
#include "iAVtkDataTypeMapper.h"
#include "iAToolsVTK.h"
#include "iATypedCallHelper.h"

#include <vtkImageAccumulate.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>

//#include <QElapsedTimer>

#include <algorithm>
#include <cassert>
#include <cmath>

iAHistogramData::iAHistogramData(QString const& name, iAValueType type,
	DataType minX, DataType maxX, size_t numBin) :
	iAPlotData(name, type), m_histoData(new DataType[numBin]), m_dataOwner(true), m_numBin(numBin)
{
	clear();
	setXBounds(minX, maxX);
	m_yBounds[0] = 0;
	m_yBounds[1] = 0;
}

iAHistogramData::iAHistogramData(QString const& name, iAValueType type,
	DataType minX, DataType maxX, size_t numBin, DataType* histoData) :
	iAPlotData(name, type), m_histoData(histoData), m_dataOwner(false), m_numBin(numBin)
{
	setXBounds(minX, maxX);
	updateYBounds();
}

iAHistogramData::~iAHistogramData()
{
	if (m_dataOwner)
	{
		delete[] m_histoData;
	}
}

void iAHistogramData::setBin(size_t binIdx, DataType value)
{
	m_histoData[binIdx] = value;
	adaptBounds(m_yBounds, value);
}

void iAHistogramData::clear()
{
	std::fill(m_histoData, m_histoData + m_numBin, 0);
}

void iAHistogramData::setSpacing(DataType spacing)
{
	m_spacing = spacing;
}

void iAHistogramData::setYBounds(DataType yMin, DataType yMax)
{
	m_yBounds[0] = yMin;
	m_yBounds[1] = yMax;
}

iAPlotData::DataType iAHistogramData::spacing() const
{
	return m_spacing;
}

iAPlotData::DataType const* iAHistogramData::xBounds() const
{
	return m_xBounds;
}

iAPlotData::DataType const* iAHistogramData::yBounds() const
{
	return m_yBounds;
}

iAPlotData::DataType iAHistogramData::yValue(size_t idx) const
{
	assert(m_histoData && idx < valueCount());
	if (idx >= valueCount())
	{
		return 0;
	}
	return m_histoData[idx];
}

iAPlotData::DataType iAHistogramData::xValue(size_t idx) const
{
	return xBounds()[0] + spacing() * idx;
}

void iAHistogramData::updateYBounds()
{
	assert(m_histoData);
	// histogram probably should always have min y 0
	m_yBounds[0] = 0 /**std::min_element(m_histoData, m_histoData + m_numBin)*/;
	m_yBounds[1] = *std::max_element(m_histoData, m_histoData + m_numBin);
}

void iAHistogramData::setXBounds(DataType minX, DataType maxX)
{
	m_xBounds[0] = minX;
	m_xBounds[1] = maxX;
	m_spacing = (m_xBounds[1] - m_xBounds[0] + ((valueType() == iAValueType::Discrete) ? 1 : 0)) / m_numBin;
}

size_t iAHistogramData::valueCount() const
{
	return m_numBin;
}

size_t iAHistogramData::nearestIdx(DataType dataX) const
{
	DataType binRng[2] = {0, static_cast<DataType>(valueCount())};
	return clamp(static_cast<size_t>(0), valueCount() - 1, static_cast<size_t>(mapValue(xBounds(), binRng, dataX)));
}

QString iAHistogramData::toolTipText(DataType dataX) const
{
	size_t idx = nearestIdx(dataX);
	auto bStart = xValue(idx);
	auto bEnd = xValue(idx + 1);
	if (valueType() == iAValueType::Discrete || valueType() == iAValueType::Categorical)
	{
		bStart = static_cast<int>(bStart);
		bEnd = static_cast<int>(bEnd - 1);
	}
	auto freq = yValue(idx);
	auto range = (bStart == bEnd) ? QString::number(bStart) : QString("%1-%2").arg(bStart).arg(bEnd);
	return QString("%1 (%2): %3").arg(name()).arg(range).arg(freq);
}

size_t iAHistogramData::finalNumBin(vtkImageData* img, size_t desiredBins)
{
	auto newBinCount = desiredBins;
	if (desiredBins > std::numeric_limits<int>::max())
	{
		LOG(lvlWarn,
			QString("iAHistogramData::create: Only up to %1 bins supported, but requested %2! Bin number will be set "
					"to %1!")
				.arg(std::numeric_limits<int>::max())
				.arg(desiredBins));
		newBinCount = std::numeric_limits<int>::max();
	}
	if (isVtkIntegerImage(img))	// for images with discrete pixel data types...
	{
		// ...the maximum number of bins that makes sense is the number of different values
		auto const scalarRange = img->GetScalarRange();
		double rangeSize = (scalarRange[1] - scalarRange[0]) + 1;
		newBinCount = std::min(newBinCount, static_cast<size_t>(rangeSize));
		// ...and make sure we have bins of integer step size; round to closest integral number, so that actual numBin is closest to desired numBin
		double stepSize = std::round(rangeSize / newBinCount);
		// adapt numBin so that the maximum is as close as possible to the last number in actual data:
		newBinCount = std::ceil(rangeSize / stepSize);
	}
	return newBinCount;
}

template <typename T>
void computeHistogram(QSharedPointer<iAHistogramData> histData, vtkImageData* img, iAImageStatistics* imgStatistics, int component)
{
	auto dim = img->GetDimensions();
	long long numOfVoxels = static_cast<long long>(dim[0]) * dim[1] * dim[2];
	long long numOfValues = numOfVoxels * img->GetNumberOfScalarComponents();
	auto stride = img->GetNumberOfScalarComponents();
	auto imgData = static_cast<T*>(img->GetScalarPointer());
	auto plotRng = histData->xBounds();
	auto numBin = histData->valueCount();
	size_t binRng[2] = {0, numBin};
	double sum = 0;
#pragma omp parallel
	{
		std::vector private_hist(numBin, 0);
		double private_sum = 0;
#pragma omp for
		for (long long v = 0; v < numOfValues; v += stride)
		{
			auto value = static_cast<double>(imgData[v + component]);
			size_t bin = clamp(static_cast<size_t>(0), numBin - 1, mapValue(plotRng, binRng, value));
			private_sum += value ;
			private_hist[bin] += 1;
		}
#pragma omp critical
		{
			for (size_t i = 0; i < numBin; ++i)
			{
				histData->setBin(i, histData->yValue(i) + private_hist.at(i));
			}
			sum += private_sum;
		}
	}
	if (imgStatistics)
	{
		double mean = sum / numOfVoxels;
		double sum_udev = 0;
#pragma omp parallel
		{
			double private_sum_udev = 0;
#pragma omp for
			for (long long v = 0; v < numOfValues; v += stride)
			{
				auto value = static_cast<double>(imgData[v + component]);
				auto diff = value - mean;
				private_sum_udev += diff * diff;
			}
#pragma omp critical
			{
				sum_udev += private_sum_udev;
			}
		}
		double stddev = std::sqrt(sum_udev / numOfVoxels);
		auto imgRng = img->GetScalarRange();
		*imgStatistics = iAImageStatistics{ imgRng[0], imgRng[1], mean, stddev};
	}
}


#define MODE_VTK 0    // use self-written code above
//#define MODE_VTK 1    // use vtkImageAccumulate

QSharedPointer<iAHistogramData> iAHistogramData::create(QString const& name,
	vtkImageData* img, size_t desiredNumBin, iAImageStatistics* imgStatistics, int component)
{
	if (!img)
	{
		LOG(lvlWarn, "iAHistogram::create: No image given!");
		return QSharedPointer<iAHistogramData>(); // return "dummy": histogram with range 0..1, 1 bin with value 0?
	}

	double* const scalarRange = img->GetScalarRange();
	double valueRange = scalarRange[1] - scalarRange[0];
	double histRange = valueRange;
	auto valueType = isVtkIntegerImage(img) ? iAValueType::Discrete : iAValueType::Continuous;
	auto numBin = finalNumBin(img, desiredNumBin);
	if (valueType == iAValueType::Discrete)
	{
		double stepSize = std::max(1.0, std::round((valueRange + 1) / numBin));
		double newMax = scalarRange[0] + static_cast<int>(stepSize * numBin);
		histRange = newMax - scalarRange[0];
	}
	if (dblApproxEqual(valueRange, histRange))
	{  // to put max values in max bin (as vtkImageAccumulate otherwise would cut off with < max)
		const double RangeEnlargeFactor = 1 + 1e-10;
		histRange = valueRange * RangeEnlargeFactor;
	}
	auto result = iAHistogramData::create(name, valueType, scalarRange[0], scalarRange[0] + histRange, numBin);

	//QElapsedTimer timer;
	//timer.start();
#if MODE_VTK
	if (img->GetNumberOfScalarComponents() != 1)
	{
		LOG(lvlDebug,
			QString("Image has %2 components, only computing histogram of first one!")
				.arg(img->GetNumberOfScalarComponents()));
	}
	auto accumulate = vtkSmartPointer<vtkImageAccumulate>::New();
	accumulate->ReleaseDataFlagOn();
	accumulate->SetInputData(img);
	accumulate->SetComponentOrigin(img->GetScalarRange()[0], 0.0, 0.0);
	// The check in finalNumBin guarantees that numBin is smaller than int max, so the cast to int below is safe!
	accumulate->SetComponentExtent(0, static_cast<int>(numBin - 1), 0, 0, 0, 0);
	accumulate->SetComponentSpacing(histRange / numBin, 0.0, 0.0);
	accumulate->Update();
	int extent[6];
	accumulate->GetComponentExtent(extent);
	vtkSmartPointer<vtkImageCast> caster = vtkSmartPointer<vtkImageCast>::New();
	caster->SetInputData(accumulate->GetOutput());
	caster->SetOutputScalarType(iAVtkDataType<DataType>::value);
	caster->Update();
	auto rawImg = caster->GetOutput();

	auto vtkRawData = static_cast<DataType*>(rawImg->GetScalarPointer());
	std::copy(vtkRawData, vtkRawData + result->m_numBin, result->m_histoData);
	if (imgStatistics)
	{
		*imgStatistics = iAImageStatistics{ *accumulate->GetMin(), *accumulate->GetMax(), *accumulate->GetMean(), *accumulate->GetStandardDeviation() };
	}
	//LOG(lvlDebug, QString("VTK: %1 seconds").arg(timer.elapsed() / 1000.0, 5, 'f', 3));
#else
	VTK_TYPED_CALL(computeHistogram, img->GetScalarType(), result, img, imgStatistics, component);
	//LOG(lvlDebug, QString("OpenMP: %1 seconds").arg(timer.elapsed() / 1000.0, 5, 'f', 3));
#endif
	result->m_spacing = histRange / result->m_numBin;
	result->updateYBounds();

	return result;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(QString const& name, iAValueType type,
	const std::vector<DataType>& data, size_t numBin, DataType minX, DataType maxX)
{
	if (std::isinf(minX))
	{
		minX = *std::min_element(data.begin(), data.end());
	}
	if (std::isinf(maxX))
	{
		maxX = *std::max_element(data.begin(), data.end());
	}
	DataType* histoData;
	if (dblApproxEqual(minX, maxX))
	{   // if min == max, there is only one bin - one in which all values are contained!
		numBin = 1;
		histoData = new DataType[numBin];
		histoData[0] = data.size();
	}
	else
	{
		histoData = new DataType[numBin];
		std::fill(histoData, histoData + numBin, 0.0);
		for (DataType d : data)
		{
			size_t bin = clamp(static_cast<size_t>(0), numBin - 1, mapValue(minX, maxX, static_cast<size_t>(0), numBin, d));
			++histoData[bin];
		}
	}
	return QSharedPointer<iAHistogramData>(new iAHistogramData(name, type, minX, maxX, numBin, histoData));
}

QSharedPointer<iAHistogramData> iAHistogramData::create(QString const& name, iAValueType type,
	DataType minX, DataType maxX, size_t numBin)
{
	return QSharedPointer<iAHistogramData>(new iAHistogramData(name, type, minX, maxX, numBin));
}

QSharedPointer<iAHistogramData> iAHistogramData::create(QString const& name, iAValueType type,
	DataType minX, DataType maxX, size_t numBin, DataType* histoData)
{
	return QSharedPointer<iAHistogramData>(new iAHistogramData(name, type, minX, maxX, numBin, histoData));
}

template <typename ContT> double* createArrayFromContainer(ContT const & cont)
{
	double* dataArr = new double[cont.size()];
	for (typename ContT::size_type i = 0; i < cont.size(); ++i)
	{
		dataArr[i] = cont[i];
	}
	return dataArr;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(QString const& name, iAValueType type,
	DataType minX, DataType maxX, std::vector<double> const& histoData)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData(name, type, minX, maxX, histoData.size(), createArrayFromContainer(histoData)));
	result->m_dataOwner = true;
	return result;
}


QSharedPointer<iAHistogramData> iAHistogramData::create(QString const& name, iAValueType type,
	DataType minX, DataType maxX, QVector<double> const& histoData)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData(name, type, minX, maxX, histoData.size(), createArrayFromContainer(histoData)));
	result->m_dataOwner = true;
	return result;
}


// TODO: Check if this can be replaced with vtkMath::Round
#if (defined(_MSC_VER) && _MSC_VER < 1800)
static inline double Round(double val)
{
	return std::floor(val + 0.5);
}
#else
#define Round std::round
#endif

QSharedPointer<iAHistogramData> createMappedHistogramData(QString const& name,
	iAPlotData::DataType const* data, size_t srcNumBin,
	double srcMinX, double srcMaxX, size_t targetNumBin, double targetMinX, double targetMaxX,
	iAPlotData::DataType const maxValue)
{
	auto result = iAHistogramData::create(name, iAValueType::Continuous, targetMinX, targetMaxX, targetNumBin);
	assert(srcNumBin > 1 && targetNumBin > 1);
	double srcSpacing = (srcMaxX - srcMinX) / (srcNumBin - 1);
	double targetSpacing = (targetMaxX - targetMinX) / (targetNumBin - 1);
	result->setSpacing(targetSpacing); // TODO: check whether we need this or whether automatic computation in iAHistogram constructor is the same!
	// get scale factor from all source data
	iAPlotData::DataType myMax = *std::max_element(data, data + srcNumBin);
	double scaleFactor = static_cast<double>(maxValue) / myMax;

	// map source data to target indices:
	for (size_t i = 0; i < targetNumBin; ++i)
	{
		double sourceIdxDbl = ((i * targetSpacing) + targetMinX - srcMinX) / srcSpacing;
		int sourceIdx = static_cast<int>(Round(sourceIdxDbl));

		result->setBin(i,
			(sourceIdx >= 0 && static_cast<size_t>(sourceIdx) < srcNumBin)
				? static_cast<iAPlotData::DataType>(data[sourceIdx] * scaleFactor)
				: 0);
	}
	result->setYBounds(0, maxValue);  // TODO: use updateBounds here?
	return result;
}
