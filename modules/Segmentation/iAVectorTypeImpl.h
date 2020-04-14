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
#pragma once

#include "iAVectorType.h"	// for iAVectorDataType
#include "iAVectorArray.h"

#include <QSharedPointer>

#include <vector>
#include <cstddef> // for size_t

// implementation of a standalone vector of values
class iAStandaloneVector: public iAVectorType
{
private:
	std::vector<iAVectorDataType> m_data;
public:
	iAStandaloneVector(IndexType size);
	~iAStandaloneVector() override;
	virtual iAVectorDataType get(size_t channelIdx) const override;
	virtual IndexType size() const override;
	void set(IndexType, iAVectorDataType);
};

//! a single vector accessing its data via the iAVectorArray it is contained in
//! (mainly to directly access the vector for a single pixel from a iAVectorArray
//! drawing its data from a collection of images)
class iAPixelVector: public iAVectorType
{
private:
	iAVectorArray const & m_data;
	size_t m_voxelIdx;
public:
	~iAPixelVector() override;
	iAPixelVector(iAVectorArray const & data, size_t voxelIdx);
	iAVectorDataType get(size_t channelIdx) const override;
	IndexType size() const override;
};
