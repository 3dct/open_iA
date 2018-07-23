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
#include "iAHistogramData.h"

#include "iAImageInfo.h"
#include "iAMathUtility.h"
#include "iAVtkDataTypeMapper.h"
#include "iAToolsVTK.h"

#include <vtkImageAccumulate.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>


iAHistogramData::iAHistogramData()
	: m_binCount(0), rawData(nullptr), accSpacing(0)
{
	xBounds[0] = xBounds[1] = 0;
	yBounds[0] = yBounds[1] = 0;
}

iAHistogramData::~iAHistogramData()
{
	delete[] rawData;
}

double iAHistogramData::GetSpacing() const
{
	return accSpacing;
}

double const * iAHistogramData::XBounds() const
{
	return xBounds;
}

iAHistogramData::DataType const * iAHistogramData::GetRawData() const
{
	return rawData;
}


QSharedPointer<iAHistogramData> iAHistogramData::Create(vtkImageData* img, size_t binCount,
	iAImageInfo* info)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData);
	auto accumulate = vtkSmartPointer<vtkImageAccumulate>::New();
	accumulate->ReleaseDataFlagOn();
	accumulate->SetInputData(img);
	accumulate->SetComponentOrigin(img->GetScalarRange()[0], 0.0, 0.0);
	double * const scalarRange = img->GetScalarRange();
	accumulate->SetComponentExtent(0, binCount - 1, 0, 0, 0, 0);
	const double RangeEnlargeFactor = 1 + 1e-10;  // to put max values in max bin (as vtkImageAccumulate otherwise would cut off with < max)
	accumulate->SetComponentSpacing(((scalarRange[1] - scalarRange[0]) * RangeEnlargeFactor) / binCount, 0.0, 0.0);
	accumulate->Update();

	int extent[6];
	accumulate->GetComponentExtent(extent);
	vtkSmartPointer<vtkImageCast> caster = vtkSmartPointer<vtkImageCast>::New();
	caster->SetInputData(accumulate->GetOutput());
	caster->SetOutputScalarType(VtkDataType<DataType>::value);
	caster->Update();
	auto rawImg = caster->GetOutput();

	result->m_binCount = extent[1] + 1;
	result->xBounds[0] = accumulate->GetMin()[0];
	result->xBounds[1] = accumulate->GetMax()[0];
	result->rawData = new double[result->m_binCount];
	auto vtkRawData = static_cast<DataType*>(rawImg->GetScalarPointer());
	std::copy(vtkRawData, vtkRawData + result->m_binCount, result->rawData);
	double null1, null2;
	if (isVtkIntegerType(static_cast<vtkImageData*>(accumulate->GetInput())->GetScalarType()))
	{	// for int types, the last value is inclusive:
		result->accSpacing = (result->xBounds[1] - result->xBounds[0] + 1) / result->m_binCount;
	}
	else
	{
		accumulate->GetComponentSpacing(result->accSpacing, null1, null2);
	}
	result->SetMaxFreq();
	result->m_type = (img && (img->GetScalarType() != VTK_FLOAT) && (img->GetScalarType() != VTK_DOUBLE))
		? Discrete
		: Continuous;
	if (info)
		*info = iAImageInfo(accumulate->GetVoxelCount(),
			*accumulate->GetMin(), *accumulate->GetMax(),
			*accumulate->GetMean(), *accumulate->GetStandardDeviation());

	return result;
}

QSharedPointer<iAHistogramData> iAHistogramData::Create(
	iAPlotData::DataType* data, size_t bins, double space,
	iAPlotData::DataType min, iAPlotData::DataType max)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData);
	result->rawData = data;
	result->m_binCount = bins;
	result->accSpacing = space;
	result->xBounds[0] = min;
	result->xBounds[1] = max;
	result->SetMaxFreq();
	return result;
}

QSharedPointer<iAHistogramData> iAHistogramData::Create(const QList<DataType>& histData, size_t binCount)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData);
	DataType minValue = std::numeric_limits<DataType>::max();
	DataType maxValue = std::numeric_limits<DataType>::lowest();
	for (DataType d : histData)
	{
		if (d < minValue)
			minValue = d;
		if (d > maxValue)
			maxValue = d;
	}
	result->rawData = new DataType[binCount];
	result->m_binCount = binCount;
	result->accSpacing = (maxValue - minValue) / binCount;
	result->xBounds[0] = minValue;
	result->xBounds[1] = maxValue;
	std::fill(result->rawData, result->rawData + binCount, 0.0);
	//double factor = 1 / result->accSpacing;
	for (DataType d : histData)
	{
		//int bin = clamp(static_cast<size_t>(0), binCount-1, static_cast<int>(((d - minValue) * factor) ));	 // potentially faster but less readable
		int bin = clamp(static_cast<size_t>(0), binCount-1, mapValue(minValue, maxValue, static_cast<size_t>(0), binCount, d));
		++result->rawData[bin];
	}
	result->SetMaxFreq();
	return result;
}

void iAHistogramData::SetMaxFreq()
{
	if (!rawData)
		return;
	yBounds[1] = 1;
	for ( int i = 0; i < GetNumBin(); i++ )
		if (rawData[i] > yBounds[1])
			yBounds[1] = rawData[i];
}

size_t iAHistogramData::GetNumBin() const
{
	return m_binCount;
}

iAPlotData::DataType const * iAHistogramData::YBounds() const
{
	return yBounds;
}

iAValueType iAHistogramData::GetRangeType() const
{
	return m_type;
}
