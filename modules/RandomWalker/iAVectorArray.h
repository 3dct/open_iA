/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAVectorType.h"  // for iAVectorDataType

#include <cstddef> // for size_t

#include <QSharedPointer>

//! abstract base class for access to multi-channel/vector data, arranged as array
class iAVectorArray
{
private:
	mutable iAVectorDataType m_maxSum;
public:
	iAVectorArray();
	virtual ~iAVectorArray();
	virtual size_t size() const =0;
	virtual size_t channelCount() const =0;
	virtual QSharedPointer<iAVectorType const> get(size_t voxelIdx) const =0;
	virtual iAVectorDataType get(size_t voxelIdx, size_t channelIdx) const =0;
	iAVectorDataType getMaxSum() const;
};
