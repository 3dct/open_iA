/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iALookupTable.h"

#include "iAMathUtility.h"

#include <vtkLookupTable.h>

#include <QColor>

iALookupTable::iALookupTable() :
	m_isInitialized(false),
	m_rangeLen(1.0)
{
	m_range[0] = m_range[1] = 0.0;
}

iALookupTable::iALookupTable(vtkLookupTable * vtk_lut) :
	m_isInitialized(false)
{
	copyFromVTK(vtk_lut);
}

void iALookupTable::copyFromVTK(vtkLookupTable * vtk_lut)
{
	allocate(vtk_lut->GetNumberOfTableValues());
	setRange(vtk_lut->GetRange());
	for (size_t i = 0; i < m_data.size(); ++i)
	{
		double rgba[4];
		vtk_lut->GetTableValue(i, rgba);
		setColor(i, rgba);
	}
}

void iALookupTable::getColor(double val, double * rgba_out)
{
	assert(m_isInitialized);
	if (m_data.size() < 4)
	{
		for (int i = 0; i < 3; ++i)
			rgba_out[i] = 0;
		rgba_out[3] = 1;
		return;
	}
	double t = (val - m_range[0]) / m_rangeLen;
	int index = clamp(0, static_cast<int>(m_data.size() - 1), static_cast<int>(t * m_data.size()));
	index *= 4;
	for (int i = 0; i < 4; ++i)
		rgba_out[i] = m_data[index++];
}

void iALookupTable::allocate(size_t numberOfColors)
{
	m_data.clear();
	size_t size = numberOfColors * 4;
	for (size_t i = 0; i < size; ++i)
		m_data.push_back(0.0);
	m_isInitialized = true;
}

void iALookupTable::setColor(size_t colInd, double * rgba)
{
	assert(m_isInitialized);
	int maxInd = (colInd + 1) * 4;
	if (m_data.size() < maxInd)
		return;
	size_t offset = colInd * 4;
	for (int i = 0; i < 4; ++i)
		m_data[offset++] = rgba[i];
}

void iALookupTable::setColor(size_t colInd, QColor const & col)
{
	assert(m_isInitialized);
	double rgba[4] = { col.redF(), col.greenF(), col.blueF(), col.alphaF(), };
	setColor(colInd, rgba);
}

void iALookupTable::setData(size_t numberOfColors, double * rgba_data)
{
	assert(m_isInitialized);
	allocate(numberOfColors);
	size_t offset = 0;
	for (size_t i = 0; i < m_data.size(); ++i)
	{
		setColor(i, &rgba_data[offset]);
		offset += 4;
	}
}

void iALookupTable::setOpacity(double alpha)
{
	assert(m_isInitialized);
	size_t offset = 0;
	for (size_t i = 0; i < m_data.size(); ++i)
	{
		m_data[offset + 3] = alpha;
		offset += 4;
	}
}

const double * iALookupTable::getRange() const
{
	return m_range;
}

void iALookupTable::setRange(double from_val, double to_val)
{
	m_range[0] = from_val; m_range[1] = to_val;
	m_rangeLen = m_range[1] - m_range[0];
}

void iALookupTable::setRange(double * range)
{
	setRange(range[0], range[1]);
}

bool iALookupTable::initialized() const
{
	return m_isInitialized;
}
