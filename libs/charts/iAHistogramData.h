// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "iAPlotData.h"

#include "iacharts_export.h"

#include <QVector>

#include <vector>

class vtkImageData;

//! simple data holder for image statistics
struct iAImageStatistics
{
	double minimum, maximum, mean, standardDeviation;
};

//! Computes and stores histogram data, which can be used in plots.
class iAcharts_API iAHistogramData : public iAPlotData
{
public:
	//! Create an empty histogram (with numBin bins, initialized to 0).
	iAHistogramData(QString const& name, iAValueType type, DataType minX, DataType maxX, size_t numBin);
	//! Create with the given, already computed histogram data.
	iAHistogramData(QString const& name, iAValueType type, DataType minX, DataType maxX, size_t numBin, DataType* histoData);
	~iAHistogramData();
	//! @{ overriden from iAPlotData, check description there!
	DataType yValue(size_t idx) const override;
	DataType xValue(size_t idx) const override;
	DataType const* xBounds() const override;
	DataType const* yBounds() const override;
	size_t valueCount() const override;
	size_t nearestIdx(DataType dataX) const override;
	QString toolTipText(DataType dataX) const override;
	//! @}
	//! Get the spacing (the witdh of a bin)
	iAPlotData::DataType spacing() const;

	//! Set the value for a given bin index.
	//! Also updates y bounds
	//! @param binIdx the index of the bin that should be changed
	//! @param value the new value for the bin
	void setBin(size_t binIdx, DataType value);
	//! Sets all histogram frequencies back to 0
	void clear();

	//! Sets custom spacing.
	//! @deprecated should be set automatically - if not it's a bug that needs to be fixed inside the class, not by setting it from externally
	void setSpacing(DataType spacing);
	//! Sets custom y bounds.
	//! @deprecated should be set automatically - if not it's a bug that needs to be fixed inside the class, not by setting it from externally
	void setYBounds(DataType yMin, DataType yMax);

	//! create a histogram for a vtk image.
	//! @param name the name of the plot
	//! @param img a pointer to the vtk image for which to create the histogram
	//! @param desiredNumBin the desired number of bins the data will be split into; can be adapted, depending on the actual number of different values in image
	//! @param imgStatistics optional iAImageStatistics struct that will be filled with the statistical information determined while computing the histogram
	//! @param component which component of the image the histogram should be created for (in case it has multiple components)
	static std::shared_ptr<iAHistogramData> create(QString const& name,
		vtkImageData* img, size_t desiredNumBin, iAImageStatistics* imgStatistics = nullptr, int component = 0);
	//! create a histogram for the given (raw) data vector.
	//! @param name the name of the plot
	//! @param type the type of the data values (continuous or discrete)
	//! @param data the vector containing the raw data values
	//! @param numBin the number of bins the data will be split into
	//! @param minValue the minimum value in the data values (performance improvement if both minValue and maxValue are given; if left out, it is determined automatically)
	//! @param maxValue the maximum value in the data values (performance improvement if both minValue and maxValue are given; if left out, it is determined automatically)
	static std::shared_ptr<iAHistogramData> create(QString const& name, iAValueType type,
		const std::vector<DataType>& data, size_t numBin,
		DataType minValue=std::numeric_limits<DataType>::infinity(),
		DataType maxValue=std::numeric_limits<DataType>::infinity());
	//! Create an empty histogram.
	//! Useful if you need a custom way of creating a histogram; use setBin to populate the values.
	//! @param name the name of the plot
	//! @param type the type of the data values (continuous or discrete)
	//! @param minX minimum value in the data values
	//! @param maxX minimum value in the data values
	//! @param numBin the number of bins in the new histogram data (all initialized to 0).
	static std::shared_ptr<iAHistogramData> create(QString const& name, iAValueType type,
		DataType minX, DataType maxX, size_t numBin);
	//! Create from already computed histogram data.
	//! @deprecated (because of data ownership issues, see notes for histoData parameter)
	//! @param name the name of the plot
	//! @param type the type of the data values (continuous or discrete)
	//! @param minX minimum value in the data values
	//! @param maxX minimum value in the data values
	//! @param numBin the number of bins - the number of items contained in histoData
	//! @param histoData the histogram frequencies. note that the class DOES NOT take ownership of the given array:
	//!        you have to delete the array manually!
	static std::shared_ptr<iAHistogramData> create(QString const& name, iAValueType type,
		DataType minX, DataType maxX, size_t numBin, DataType* histoData);
	//! Create from already computed histogram data in a std::vector.
	//! @param name the name of the plot
	//! @param type the type of the data values (continuous or discrete)
	//! @param minX minimum value in the data values
	//! @param maxX minimum value in the data values
	//! @param histoData the histogram frequencies
	static std::shared_ptr<iAHistogramData> create(QString const& name, iAValueType type,
		DataType minX, DataType maxX, std::vector<DataType> const& histoData);
	//! Create from already computed histogram data in a QVector.
	//! @param name the name of the plot
	//! @param type the type of the data values (continuous or discrete)
	//! @param minX minimum value in the data values
	//! @param maxX minimum value in the data values
	//! @param histoData the histogram frequencies
	static std::shared_ptr<iAHistogramData> create(QString const& name, iAValueType type,
		DataType minX, DataType maxX, QVector<DataType> const& histoData);

	//! compute the final bin count for a given image and desired bin count
	static size_t finalNumBin(vtkImageData* img, size_t desiredNumBin);

	//! compute the final bin count for given data type, value count, and desired bin count
	static size_t finalNumBin(size_t numValues, iAValueType type, double const valueRange[2], size_t desiredNumBin);

	//! compute the actual histogram range for the given range and number of bins
	static double histoRange(double const range[2], size_t numBins, iAValueType valueType);

private:
	//! Set y value range from current data.
	void updateYBounds();
	void setXBounds(DataType minX, DataType maxX);

	//! The frequency values for each histogram bin.
	DataType* m_histoData;
	//! Whether this object is owner of the data array (m_histoData).
	//! TODO: rethink - easy to cause mistakes; maybe always take ownership, but don't provide constructors passing in a raw pointer?
	bool m_dataOwner;
	//! Number of bins in the histogram (i.e. size of m_histoData).
	size_t m_numBin;
	//! The data range covered by the histogram.
	DataType m_xBounds[2];
	//! The range of frequency values contained in the histogram (i.e. min and max element in m_histoData)
	DataType m_yBounds[2];
	//! The width of a single bin in the histogram.
	DataType m_spacing;
};

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif
#include <itkImage.h>
#include <itkImageRegionConstIterator.h>
#ifdef __clang__
#pragma clang diagnostic pop
#endif

//! Specific histogram for a collection of binary images (one for each label)
//! the created histogram has one bin per label
//! @todo move to a separate file?
template <typename PixelT>
std::shared_ptr<iAHistogramData> createHistogramData(QString const& name, iAValueType xValueType,
	QVector<typename itk::Image<PixelT, 3>::Pointer> const& imgs, size_t numBin, PixelT min, PixelT max)
{
	/*
	img->ReleaseDataFlagOff();
	itk::MinimumMaximumImageCalculator<TImage> minMaxCalc;
	minMaxCalc->SetInput(img);
	minMaxCalc->Compute();
	auto result = iAHistogramData::Create(minMaxCalc->GetMinimum(), minMaxCalc->GetMaximum(), numBin);
	*/
	auto result = iAHistogramData::create(name, xValueType, min, max, numBin);
	for (int i = 0; i < imgs.size(); ++i)
	{
		double sum = 0;
		itk::ImageRegionConstIterator<itk::Image<PixelT, 3>> it(imgs[i], imgs[i]->GetLargestPossibleRegion());
		it.GoToBegin();
		while (!it.IsAtEnd())
		{
			sum += it.Get();
			++it;
		}
		result->setBin(i, sum);
	}
	return result;
}

/*
// for pixel probing:
template <typename PixelT>
std::shared_ptr<iAHistogramData> CreateHistogram(QVector<typename itk::Image<PixelT, 3>::Pointer> const & imgs, size_t numBin, int index[3], PixelT min, PixelT max, iAValueType xValueType)
{
	auto result = iAHistogramData::Create(min, max, numBin, xValueType);
	itk::Index<3> idx;
	for (int i = 0; i < 3; ++i) { idx[i] = index[i]; }
	for (int m=0; m<imgs.size(); ++m)
	{
		result->SetBin(m, imgs[m]->GetPixel(idx));
	}
	return result;
}
*/

//! Returns histogram with given data mapped from specified source to target range.
iAcharts_API std::shared_ptr<iAHistogramData> createMappedHistogramData(QString const& name,
	iAPlotData::DataType const* data, size_t srcNumBin, double srcMinX, double srcMaxX,
	size_t targetNumBin, double targetMinX, double targetMaxX, iAPlotData::DataType const maxValue);
