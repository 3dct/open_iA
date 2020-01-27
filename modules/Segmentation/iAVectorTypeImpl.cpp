/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iAVectorTypeImpl.h"

iAVectorDataType iAVectorType::operator[](size_t channelIdx) const
{
	return get(channelIdx);
}

QSharedPointer<iAVectorType const> iAVectorType::normalized() const
{
	QSharedPointer<iAStandaloneVector> result(new iAStandaloneVector(size()));
	iAVectorDataType sum = 0;
	for(iAVectorType::IndexType i = 0; i<size(); ++i)
	{
		sum += get(i);
	}
	for(iAVectorType::IndexType i = 0; i<size(); ++i)
	{
		result->set(i, get(i) / sum);
	}
	return result;
}




iAPixelVector::iAPixelVector(iAVectorArray const & data, size_t voxelIdx):
	m_data(data),
	m_voxelIdx(voxelIdx)
{}

iAVectorDataType iAPixelVector::get(size_t channelIdx) const
{
	iAVectorDataType value = m_data.get(m_voxelIdx, channelIdx);
	return value;
}

iAVectorType::IndexType iAPixelVector::size() const
{
	return m_data.channelCount();
}




iAStandaloneVector::iAStandaloneVector(IndexType size):
	m_data(size)
{}

iAVectorDataType iAStandaloneVector::get(size_t idx) const
{
	return m_data[idx];
}

iAVectorType::IndexType iAStandaloneVector::size() const
{
	return m_data.size();
}

void iAStandaloneVector::set(iAVectorType::IndexType idx, iAVectorDataType value)
{
	m_data[idx] = value;
}
