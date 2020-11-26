/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#pragma once
#include "iAPlotData.h"

#include "open_iA_Core_export.h"

#include <QSharedPointer>

#include <vector>

class vtkImageData;

class iAImageInfo;

//! Computes and stores histogram data, which can be used in plots.
class open_iA_Core_API iAHistogramData: public iAPlotData
{
public:
	~iAHistogramData();
	DataType const* rawData() const override;
	double const* xBounds() const override;
	DataType const* yBounds() const override;

	double spacing() const override;
	size_t numBin() const override;
	iAValueType valueType() const override;

	void setBin(size_t binIdx, DataType value);

	//! create a histogram for a vtk image
	static QSharedPointer<iAHistogramData> create(vtkImageData* img, size_t numBin, iAImageInfo* imageInfo = nullptr);
	//! create a histogram for the given (raw) data array
	static QSharedPointer<iAHistogramData> create(DataType* data, size_t numBin, double space, DataType min, DataType max);
	//! create a histogram for the given (raw) data vector
	static QSharedPointer<iAHistogramData> create(const std::vector<DataType>& data, size_t numBin,
		iAValueType type = iAValueType::Continuous,
		DataType minValue=std::numeric_limits<double>::infinity(),
		DataType maxValue=std::numeric_limits<double>::infinity());
	//! create an empty histogram (with numBin bins, initialized to 0, and the given range)
	static QSharedPointer<iAHistogramData> create(DataType minX, DataType maxX, size_t numBin, iAValueType type);
	//! create a histogram from the given histogram data, range
	//! @param numBin the number of bins - the number of items contained in histoData
	static QSharedPointer<iAHistogramData> create(
		DataType minX, DataType maxX, size_t numBin, iAValueType type, double* histoData);
	static QSharedPointer<iAHistogramData> create(
		DataType minX, DataType maxX, iAValueType type, std::vector<double> const& histoData);
	static QSharedPointer<iAHistogramData> create(
		DataType minX, DataType maxX, iAValueType type, QVector<double> const& histoData);

private:
	iAHistogramData();
	//! create an empty histogram (with numBin bins, initialized to 0)
	iAHistogramData(DataType minX, DataType maxX, size_t numBin, iAValueType type);
	//! create a histogram with the given histogram data
	iAHistogramData(DataType minX, DataType maxX, size_t numBin, iAValueType type, double* histoData);
	void setMaxFreq();

	iAPlotData::DataType* m_histoData;
	double m_xBounds[2];
	iAPlotData::DataType m_yBounds[2];
	size_t m_numBin;
	iAValueType m_valueType;
	double m_spacing;
	bool m_dataOwner;
};

#include <itkImage.h>
#include <itkImageRegionConstIterator.h>

// TODO: separate file for that ?
// specific histogram for a collection of binary images (one for each label)
// the created histogram has one bin per label
template <typename PixelT>
QSharedPointer<iAHistogramData> createHistogram(QVector<typename itk::Image<PixelT, 3>::Pointer> const& imgs,
	size_t numBin, PixelT min, PixelT max, iAValueType xValueType)
{
	/*
	img->ReleaseDataFlagOff();
	itk::MinimumMaximumImageCalculator<TImage> minMaxCalc;
	minMaxCalc->SetInput(img);
	minMaxCalc->Compute();
	auto result = iAHistogramData::Create(minMaxCalc->GetMinimum(), minMaxCalc->GetMaximum(), numBin);
	*/
	auto result = iAHistogramData::create(min, max, numBin, xValueType);
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
QSharedPointer<iAHistogramData> CreateHistogram(QVector<typename itk::Image<PixelT, 3>::Pointer> const & imgs, size_t numBin, int index[3], PixelT min, PixelT max, iAValueType xValueType)
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
