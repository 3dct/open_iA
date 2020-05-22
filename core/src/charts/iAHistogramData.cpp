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
#include "iAHistogramData.h"

#include "iAConsole.h"
#include "iAImageInfo.h"
#include "iAMathUtility.h"
#include "iAVtkDataTypeMapper.h"
#include "iAToolsVTK.h"

#include <vtkImageAccumulate.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>


iAHistogramData::iAHistogramData()
	: m_binCount(0), m_rawData(nullptr), m_accSpacing(0), m_type(Continuous)
{
	m_xBounds[0] = m_xBounds[1] = 0;
	m_yBounds[0] = m_yBounds[1] = 0;
}

iAHistogramData::~iAHistogramData()
{
	delete[] m_rawData;
}

double iAHistogramData::spacing() const
{
	return m_accSpacing;
}

double const * iAHistogramData::xBounds() const
{
	return m_xBounds;
}

iAHistogramData::DataType const * iAHistogramData::rawData() const
{
	return m_rawData;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(vtkImageData* img, size_t binCount,
	iAImageInfo* info)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData);
	auto accumulate = vtkSmartPointer<vtkImageAccumulate>::New();
	accumulate->ReleaseDataFlagOn();
	accumulate->SetInputData(img);
	accumulate->SetComponentOrigin(img->GetScalarRange()[0], 0.0, 0.0);
	double * const scalarRange = img->GetScalarRange();
	if (binCount > std::numeric_limits<int>::max())
	{
		DEBUG_LOG(QString("iAHistogramData::create: Only up to %1 bins supported, but requested %2! Bin number will be set to %1!")
			.arg(std::numeric_limits<int>::max()).arg(binCount));
		binCount = std::numeric_limits<int>::max();
	}   // check above guarantees that binCount is smaller than int max, so cast below is safe!
	accumulate->SetComponentExtent(0, static_cast<int>(binCount - 1), 0, 0, 0, 0);
	const double RangeEnlargeFactor = 1 + 1e-10;  // to put max values in max bin (as vtkImageAccumulate otherwise would cut off with < max)
	accumulate->SetComponentSpacing(((scalarRange[1] - scalarRange[0]) * RangeEnlargeFactor) / binCount, 0.0, 0.0);
	accumulate->Update();

	int extent[6];
	accumulate->GetComponentExtent(extent);
	vtkSmartPointer<vtkImageCast> caster = vtkSmartPointer<vtkImageCast>::New();
	caster->SetInputData(accumulate->GetOutput());
	caster->SetOutputScalarType(iAVtkDataType<DataType>::value);
	caster->Update();
	auto rawImg = caster->GetOutput();

	result->m_binCount = extent[1] + 1;
	result->m_xBounds[0] = accumulate->GetMin()[0];
	result->m_xBounds[1] = accumulate->GetMax()[0];
	result->m_rawData = new double[result->m_binCount];
	auto vtkRawData = static_cast<DataType*>(rawImg->GetScalarPointer());
	std::copy(vtkRawData, vtkRawData + result->m_binCount, result->m_rawData);
	double null1, null2;
	if (isVtkIntegerType(static_cast<vtkImageData*>(accumulate->GetInput())->GetScalarType()))
	{	// for int types, the last value is inclusive:
		result->m_accSpacing = (result->m_xBounds[1] - result->m_xBounds[0] + 1) / result->m_binCount;
	}
	else
	{
		accumulate->GetComponentSpacing(result->m_accSpacing, null1, null2);
	}
	result->setMaxFreq();
	result->m_type = (img && (img->GetScalarType() != VTK_FLOAT) && (img->GetScalarType() != VTK_DOUBLE))
		? Discrete
		: Continuous;
	if (info)
	{
		*info = iAImageInfo(accumulate->GetVoxelCount(),
			*accumulate->GetMin(), *accumulate->GetMax(),
			*accumulate->GetMean(), *accumulate->GetStandardDeviation());
	}

	return result;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(
	iAPlotData::DataType* data, size_t bins, double space,
	iAPlotData::DataType min, iAPlotData::DataType max)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData);
	result->m_rawData = data;
	result->m_binCount = bins;
	result->m_accSpacing = space;
	result->m_xBounds[0] = min;
	result->m_xBounds[1] = max;
	result->setMaxFreq();
	return result;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(const std::vector<DataType>& histData, size_t binCount, iAValueType type,
	DataType minValue, DataType maxValue)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData);
	if (std::isinf(minValue))
	{
		minValue = std::numeric_limits<DataType>::max();
		for (DataType d : histData)
		{
			if (d < minValue)
			{
				minValue = d;
			}
		}
	}
	if (std::isinf(maxValue))
	{
		maxValue = std::numeric_limits<DataType>::lowest();
		for (DataType d : histData)
		{
			if (d > maxValue)
			{
				maxValue = d;
			}
		}
	}
	result->m_xBounds[0] = minValue;
	result->m_xBounds[1] = maxValue;
	result->m_type = type;
	if (dblApproxEqual(minValue, maxValue))
	{   // if min == max, there is only one bin - one in which all values are contained!
		result->m_binCount = 1;
		result->m_rawData = new DataType[result->m_binCount];
		result->m_rawData[0] = histData.size();
	}
	else
	{
		result->m_binCount = binCount;
		result->m_rawData = new DataType[binCount];
		result->m_accSpacing = (maxValue - minValue) / binCount;
		std::fill(result->m_rawData, result->m_rawData + binCount, 0.0);
		for (DataType d : histData)
		{
			size_t bin = clamp(static_cast<size_t>(0), binCount - 1, mapValue(minValue, maxValue, static_cast<size_t>(0), binCount, d));
			++result->m_rawData[bin];
		}
	}
	result->setMaxFreq();
	return result;
}

void iAHistogramData::setMaxFreq()
{
	if (!m_rawData)
	{
		return;
	}
	m_yBounds[1] = 1;
	for (size_t i = 0; i < numBin(); i++)
	{
		if (m_rawData[i] > m_yBounds[1])
		{
			m_yBounds[1] = m_rawData[i];
		}
	}
}

size_t iAHistogramData::numBin() const
{
	return m_binCount;
}

iAPlotData::DataType const * iAHistogramData::yBounds() const
{
	return m_yBounds;
}

iAValueType iAHistogramData::valueType() const
{
	return m_type;
}
