/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAPlotTypes.h"
#include "open_iA_Core_export.h"

#include <QSharedPointer>

#include <itkImage.h>
#include <itkImageRegionConstIterator.h>

class open_iA_Core_API iASimpleHistogramData : public iAPlotData
{
public:
	virtual ~iASimpleHistogramData();
	static QSharedPointer<iASimpleHistogramData> Create(DataType minX, DataType maxX, size_t numBin, iAValueType xValueType);
	static QSharedPointer<iASimpleHistogramData> Create(DataType minX, DataType maxX, size_t numBin, double * data, iAValueType xValueType);
	static QSharedPointer<iASimpleHistogramData> Create(DataType minX, DataType maxX, std::vector<double> const & data, iAValueType xValueType);

	// Inherited via iAAbstractDiagramRangedData
	DataType const * GetRawData() const override;
	size_t GetNumBin() const override;
	double GetSpacing() const override;
	double const * XBounds() const override;
	DataType const * YBounds() const override;
	iAValueType GetRangeType() const override;

	void SetBin(size_t binIdx, DataType value);
	//void AddValue(DataType value);
	//void AdjustYBounds()
private:
	iASimpleHistogramData(DataType minX, DataType maxX, size_t numBin, iAValueType xValueType);
	iASimpleHistogramData(DataType minX, DataType maxX, size_t numBin, double* data, iAValueType xValueType);
	double * m_data;
	double m_xBounds[2];
	double m_yBounds[2];
	size_t m_numBin;
	iAValueType m_xValueType;
	bool m_dataOwner;
};


template <typename PixelT>
QSharedPointer<iASimpleHistogramData> CreateHistogram(QVector<typename itk::Image<PixelT, 3>::Pointer> const & imgs, size_t numBin, PixelT min, PixelT max, iAValueType xValueType)
{
	/*
	img->ReleaseDataFlagOff();
	itk::MinimumMaximumImageCalculator<TImage> minMaxCalc;
	minMaxCalc->SetInput(img);
	minMaxCalc->Compute();
	auto result = iASimpleHistogramData::Create(minMaxCalc->GetMinimum(), minMaxCalc->GetMaximum(), numBin);
	*/
	auto result = iASimpleHistogramData::Create(min, max, numBin, xValueType);
	for (int i = 0; i < imgs.size(); ++i)
	{
		double sum = 0;
		itk::ImageRegionConstIterator<itk::Image<PixelT, 3> > it(imgs[i], imgs[i]->GetLargestPossibleRegion());
		it.GoToBegin();
		while (!it.IsAtEnd())
		{
			sum += it.Get();
			++it;
		}
		result->SetBin(i, sum);
	}
	return result;
}

/*
// for pixel probing:
template <typename PixelT>
QSharedPointer<iASimpleHistogramData> CreateHistogram(QVector<typename itk::Image<PixelT, 3>::Pointer> const & imgs, size_t numBin, int index[3], PixelT min, PixelT max, iAValueType xValueType)
{
auto result = iASimpleHistogramData::Create(min, max, numBin, xValueType);
itk::Index<3> idx;
for (int i = 0; i < 3; ++i) { idx[i] = index[i]; }
for (int m=0; m<imgs.size(); ++m)
{
result->SetBin(m, imgs[m]->GetPixel(idx));
}
return result;
}
*/
