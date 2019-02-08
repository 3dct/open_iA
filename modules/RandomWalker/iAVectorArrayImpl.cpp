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
#include "iAVectorArrayImpl.h"

iAVectorArray::~iAVectorArray()
{}


iAvtkPixelVectorArray::iAvtkPixelVectorArray(int const * dim):
	m_coordConv(dim[0], dim[1], dim[2])
{
}

iAvtkPixelVectorArray::iAvtkPixelVectorArray(size_t width, size_t height, size_t depth):
	m_coordConv(width, height, depth)
{
}

	
void iAvtkPixelVectorArray::AddImage(vtkSmartPointer<vtkImageData> img)
{
	int extent[6];
	img->GetExtent(extent);
	assert((extent[1]-extent[0]+1) == m_coordConv.GetWidth() &&
		(extent[3]-extent[2]+1) == m_coordConv.GetHeight() &&
		(extent[5]-extent[4]+1) == m_coordConv.GetDepth());
	m_images.push_back(img);
}

size_t iAvtkPixelVectorArray::size() const
{
	return m_coordConv.GetVertexCount();
}

size_t iAvtkPixelVectorArray::channelCount() const
{
	return m_images.size();
}

QSharedPointer<iAVectorType const> iAvtkPixelVectorArray::get(size_t voxelIdx) const
{
	return QSharedPointer<iAVectorType const>(new iAPixelVector(*this, voxelIdx));
}

iAVectorDataType iAvtkPixelVectorArray::get(size_t voxelIdx, size_t channelIdx) const
{
	iAImageCoordinate coords = m_coordConv.GetCoordinatesFromIndex(voxelIdx);
	iAVectorDataType value = m_images[channelIdx]->GetScalarComponentAsDouble(coords.x, coords.y, coords.z, 0);
	return value;
}
