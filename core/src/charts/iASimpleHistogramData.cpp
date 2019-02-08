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
#include "iASimpleHistogramData.h"

#include "iAMathUtility.h"

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

iASimpleHistogramData::DataType const * iASimpleHistogramData::GetRawData() const
{
	return m_data;
}

size_t iASimpleHistogramData::GetNumBin() const
{
	return m_numBin;
}

double iASimpleHistogramData::GetSpacing() const
{						// check int type (see also iAHistogramData)
	return (XBounds()[1] - XBounds()[0] + ((GetRangeType() == Discrete) ? 1 : 0)) / m_numBin;
}

double const * iASimpleHistogramData::XBounds() const
{
	return m_xBounds;
}

iASimpleHistogramData::DataType const * iASimpleHistogramData::YBounds() const
{
	return m_yBounds;
}

iAValueType iASimpleHistogramData::GetRangeType() const
{
	return m_xValueType;
}


/*
void iASimpleHistogramData::AddValue(DataType value)
{
	size_t bin = mapValue(GetDataRange()[0], GetDataRange()[1], 0.0, static_cast<DataType>(m_numBin), value);
	bin = clamp(static_cast<size_t>(0), m_numBin, bin);
	m_data[bin]++;
	if (value > m_rangeY[1])
	m_rangeY[1] = value;
}
*/

void iASimpleHistogramData::SetBin(size_t binIdx, DataType value)
{
	m_data[binIdx] = value;
	if (value > m_yBounds[1])
		m_yBounds[1] = value;
}

iASimpleHistogramData::iASimpleHistogramData(DataType minX, DataType maxX, size_t numBin, iAValueType xValueType) :
	m_numBin(numBin),
	m_xValueType(xValueType),
	m_dataOwner(true)
{
	m_data = new DataType[numBin];
	std::fill(m_data, m_data + m_numBin, 0);
	m_xBounds[0] = minX;
	m_xBounds[1] = maxX;
	m_yBounds[0] = 0;
	m_yBounds[1] = 0;
}

iASimpleHistogramData::iASimpleHistogramData(DataType minX, DataType maxX, size_t numBin, double* data, iAValueType xValueType) :
	m_numBin(numBin),
	m_xValueType(xValueType),
	m_dataOwner(false)
{
	m_data = data;
	m_xBounds[0] = minX;
	m_xBounds[1] = maxX;
	m_yBounds[0] = std::numeric_limits<double>::max();
	m_yBounds[1] = std::numeric_limits<double>::lowest();
	for (int i = 0; i < numBin; ++i)
	{
		if (data[i] < m_yBounds[0])
		{
			m_yBounds[0] = data[i];
		}
		if (data[i] > m_yBounds[1])
		{
			m_yBounds[1] = data[i];
		}
	}
}

iASimpleHistogramData::~iASimpleHistogramData()
{
	if (m_dataOwner)
		delete[] m_data;
}

QSharedPointer<iASimpleHistogramData> iASimpleHistogramData::Create(DataType minX, DataType maxX, size_t numBin, iAValueType xValueType)
{
	return QSharedPointer<iASimpleHistogramData>(new iASimpleHistogramData(minX, maxX, numBin, xValueType));
}

QSharedPointer<iASimpleHistogramData> iASimpleHistogramData::Create(DataType minX, DataType maxX, size_t numBin, double * data, iAValueType xValueType)
{
	return QSharedPointer<iASimpleHistogramData>(new iASimpleHistogramData(minX, maxX, numBin, data, xValueType));
}

QSharedPointer<iASimpleHistogramData> iASimpleHistogramData::Create(DataType minX, DataType maxX, std::vector<double> const & data, iAValueType xValueType)
{
	double* dataArr = new double[data.size()];
	for (size_t i=0; i<data.size(); ++i)
	{
		dataArr[i] = data[i];
	}
	auto result = QSharedPointer<iASimpleHistogramData>(new iASimpleHistogramData(minX, maxX, data.size(), dataArr, xValueType));
	result->m_dataOwner = true;
	return result;
}
