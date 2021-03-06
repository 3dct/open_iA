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
	double spacing() const override;
	double const * xBounds() const override;
	DataType const * rawData() const override;
	size_t numBin() const override;
	DataType const * yBounds() const override;
	iAValueType valueType() const override;

	static QSharedPointer<iAHistogramData> create(vtkImageData* img, size_t binCount, iAImageInfo* imageInfo = nullptr);
	static QSharedPointer<iAHistogramData> create(DataType* data, size_t binCount, double space, DataType min, DataType max);
	static QSharedPointer<iAHistogramData> create(const std::vector<DataType> &histData, size_t binCount,
		iAValueType type = iAValueType::Continuous,
		DataType minValue=std::numeric_limits<double>::infinity(),
		DataType maxValue=std::numeric_limits<double>::infinity());
private:
	iAHistogramData();
	void setMaxFreq();

	size_t m_binCount;
	iAPlotData::DataType* m_rawData;
	iAPlotData::DataType m_yBounds[2];
	double m_accSpacing;
	double m_xBounds[2];
	iAValueType m_type;
};
