// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iALookupTable.h"

#include "iAMathUtility.h"

#include <vtkLookupTable.h>

#include <QColor>

namespace
{
	const int NumberOfColorComponents = 4;
}

iALookupTable::iALookupTable() :
	m_rangeLen( 1.0 ),
	m_numColors( 0 )
{
	m_range[0] = m_range[1] = 0.0;
}

iALookupTable::iALookupTable(QColor color):
	m_rangeLen(1.0),
	m_numColors(1)
{
	allocate(1);
	m_data[0] = color.redF();
	m_data[1] = color.greenF();
	m_data[2] = color.blueF();
	m_data[3] = color.alphaF();
}

iALookupTable::iALookupTable(vtkLookupTable * vtk_lut)
{
	copyFromVTK(vtk_lut);
}

void iALookupTable::copyFromVTK(vtkLookupTable * vtk_lut)
{
	allocate( vtk_lut->GetNumberOfTableValues() );
	setRange( vtk_lut->GetRange() );
	for (size_t i = 0; i < m_numColors; ++i)
	{
		double rgba[NumberOfColorComponents];
		vtk_lut->GetTableValue(i, rgba);
		setColor(i, rgba);
	}
}

void iALookupTable::getColor(double val, double * rgba_out) const
{
	assert(m_isInitialized);
	if (m_data.size() < NumberOfColorComponents)
	{
		for (int i = 0; i < 3; ++i)
		{
			rgba_out[i] = 0;
		}
		rgba_out[3] = 1;
		return;
	}
	double t = (val - m_range[0]) / m_rangeLen;
	// clamp needs signed type so that value falls out on right side of table!
	size_t index = static_cast<size_t>(clamp(static_cast<long long>(0), static_cast<long long>(m_numColors)-1, static_cast<long long>(t * m_numColors)));
	getTableValue(index, rgba_out);
}

QColor iALookupTable::getQColor(double val) const
{
	double rgba[4];
	getColor(val, rgba);
	return QColor(rgba[0] * 255, rgba[1] * 255, rgba[2] * 255, rgba[3] * 255);
}

void iALookupTable::getTableValue(size_t index, double * rgba_out) const
{
	index *= NumberOfColorComponents;
	for (int i = 0; i < NumberOfColorComponents; ++i)
	{
		rgba_out[i] = m_data[index++];
	}
}

void iALookupTable::allocate(size_t numberOfColors)
{
	m_numColors = numberOfColors;
	m_data.clear();
	size_t size = numberOfColors * NumberOfColorComponents;
	m_data.resize(size, 0.0);
	m_isInitialized = true;
}

size_t iALookupTable::numberOfValues() const
{
	return m_numColors;
}

void iALookupTable::setColor(size_t colInd, double * rgba)
{
	assert(m_isInitialized);
	size_t offset = colInd * NumberOfColorComponents;
	if (m_data.size() < (offset + NumberOfColorComponents))
	{
		return;
	}
	for (int i = 0; i < NumberOfColorComponents; ++i)
	{
		m_data[offset++] = rgba[i];
	}
}

void iALookupTable::setColor(size_t colInd, QColor const & col)
{
	assert(m_isInitialized);
	double rgba[NumberOfColorComponents] = { col.redF(), col.greenF(), col.blueF(), col.alphaF() };
	setColor(colInd, rgba);
}

void iALookupTable::setData(size_t numberOfColors, double * rgba_data)
{
	assert(m_isInitialized);
	allocate(numberOfColors);
	size_t offset = 0;
	for (size_t i = 0; i < numberOfColors; ++i)
	{
		setColor(i, &rgba_data[offset]);
		offset += NumberOfColorComponents;
	}
}

void iALookupTable::setOpacity(double alpha)
{
	assert(m_isInitialized);
	size_t offset = 0;
	for (size_t i = 0; i < m_numColors; ++i)
	{
		m_data[offset + 3] = alpha;
		offset += NumberOfColorComponents;
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

void iALookupTable::setRange(double const * range)
{
	setRange(range[0], range[1]);
}

bool iALookupTable::initialized() const
{
	return m_isInitialized;
}
