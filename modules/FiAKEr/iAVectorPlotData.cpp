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
#include "iAVectorPlotData.h"

iAVectorPlotData::iAVectorPlotData(std::vector<double> const & data):
	m_data(data)
{
	m_xBounds[0] = 0;
	m_xBounds[1] = m_data.size()-1;

	updateBounds();
}

iAVectorPlotData::DataType const * iAVectorPlotData::GetRawData() const
{
	return m_data.data();
}

size_t iAVectorPlotData::GetNumBin() const
{
	return m_data.size();
}

double iAVectorPlotData::GetSpacing() const
{
	return 1.0;
}

double const * iAVectorPlotData::XBounds() const
{
	return m_xBounds;
}

iAVectorPlotData::DataType const * iAVectorPlotData::YBounds() const
{
	return m_yBounds;
}

iAValueType iAVectorPlotData::GetRangeType() const
{
	return m_xDataType;
}

void iAVectorPlotData::setXDataType(iAValueType xDataType)
{	m_xDataType = xDataType;
}

std::vector<double> & iAVectorPlotData::data()
{
	return m_data;
}

void iAVectorPlotData::updateBounds()
{
	m_yBounds[0] = std::numeric_limits<double>::max();
	m_yBounds[1] = std::numeric_limits<double>::lowest();
	for (size_t i = 0; i < m_data.size(); ++i)
	{
		if (m_data[i] < m_yBounds[0])
			m_yBounds[0] = m_data[i];
		if (m_data[i] > m_yBounds[1])
			m_yBounds[1] = m_data[i];
	}
}
