/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iALog.h"
#include "iAImageInfo.h"
#include "iAMathUtility.h"
#include "iAVtkDataTypeMapper.h"
#include "iAToolsVTK.h"

#include <vtkImageAccumulate.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>


iAHistogramData::iAHistogramData() :
	m_numBin(0), m_histoData(nullptr), m_spacing(0), m_valueType(iAValueType::Continuous), m_dataOwner(true)
{
	m_xBounds[0] = m_xBounds[1] = 0;
	m_yBounds[0] = m_yBounds[1] = 0;
}


iAHistogramData::iAHistogramData(DataType minX, DataType maxX, size_t numBin, iAValueType type) :
	m_numBin(numBin), m_valueType(type), m_dataOwner(true)
{
	m_histoData = new DataType[numBin];
	std::fill(m_histoData, m_histoData + m_numBin, 0);
	m_xBounds[0] = minX;
	m_xBounds[1] = maxX;
	m_spacing = (m_xBounds[1] - m_xBounds[0] + ((valueType() == iAValueType::Discrete) ? 1 : 0)) / m_numBin;
	m_yBounds[0] = 0;
	m_yBounds[1] = 0;
}

iAHistogramData::iAHistogramData(DataType minX, DataType maxX, size_t numBin, iAValueType type, double* histoData) :
	m_numBin(numBin), m_valueType(type), m_dataOwner(false)
{
	m_histoData = histoData;
	m_xBounds[0] = minX;
	m_xBounds[1] = maxX;
	m_spacing = (m_xBounds[1] - m_xBounds[0] + ((valueType() == iAValueType::Discrete) ? 1 : 0)) / m_numBin;
	m_yBounds[0] = *std::min_element(m_histoData, m_histoData + numBin);
	m_yBounds[1] = *std::max_element(m_histoData, m_histoData + numBin);
}

iAHistogramData::~iAHistogramData()
{
	if (m_dataOwner)
	{
		delete[] m_histoData;
	}
}

void iAHistogramData::setBin(size_t binIdx, DataType value)
{
	m_histoData[binIdx] = value;
	if (value > m_yBounds[1])
	{
		m_yBounds[1] = value;
	}
}

double iAHistogramData::spacing() const
{
	return m_spacing;
}

double const * iAHistogramData::xBounds() const
{
	return m_xBounds;
}

iAHistogramData::DataType const * iAHistogramData::rawData() const
{
	return m_histoData;
}

void iAHistogramData::setMaxFreq()
{
	if (!m_histoData)
	{
		return;
	}
	m_yBounds[1] = 1;
	for (size_t i = 0; i < m_numBin; i++)
	{
		if (m_histoData[i] > m_yBounds[1])
		{
			m_yBounds[1] = m_histoData[i];
		}
	}
}

size_t iAHistogramData::numBin() const
{
	return m_numBin;
}

iAPlotData::DataType const* iAHistogramData::yBounds() const
{
	return m_yBounds;
}

iAValueType iAHistogramData::valueType() const
{
	return m_valueType;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(vtkImageData* img, size_t numBin,
	iAImageInfo* info)
{
	auto accumulate = vtkSmartPointer<vtkImageAccumulate>::New();
	accumulate->ReleaseDataFlagOn();
	accumulate->SetInputData(img);
	accumulate->SetComponentOrigin(img->GetScalarRange()[0], 0.0, 0.0);
	double * const scalarRange = img->GetScalarRange();
	double valueRange = scalarRange[1] - scalarRange[0];
	double histRange = valueRange;
	if (numBin > std::numeric_limits<int>::max())
	{
		LOG(lvlWarn, QString("iAHistogramData::create: Only up to %1 bins supported, but requested %2! Bin number will be set to %1!")
				.arg(std::numeric_limits<int>::max()).arg(numBin));
		numBin = std::numeric_limits<int>::max();
	}
	if (isVtkIntegerType(static_cast<vtkImageData*>(accumulate->GetInput())->GetScalarType()))
	{   // make sure we have bins of integer step size:
		double stepSize = std::ceil(valueRange / numBin);
		double newMax = scalarRange[0] + static_cast<int>(stepSize * numBin);
		histRange = newMax - scalarRange[0];
	}   // check above guarantees that numBin is smaller than int max, so cast below is safe!
	accumulate->SetComponentExtent(0, static_cast<int>(numBin - 1), 0, 0, 0, 0);
	if (dblApproxEqual(valueRange, histRange))
	{  // to put max values in max bin (as vtkImageAccumulate otherwise would cut off with < max)
		const double RangeEnlargeFactor = 1 + 1e-10;
		histRange = valueRange * RangeEnlargeFactor;
	}
	accumulate->SetComponentSpacing(histRange / numBin, 0.0, 0.0);
	accumulate->Update();

	int extent[6];
	accumulate->GetComponentExtent(extent);
	vtkSmartPointer<vtkImageCast> caster = vtkSmartPointer<vtkImageCast>::New();
	caster->SetInputData(accumulate->GetOutput());
	caster->SetOutputScalarType(iAVtkDataType<DataType>::value);
	caster->Update();
	auto rawImg = caster->GetOutput();

	auto valueType = (img && (img->GetScalarType() != VTK_FLOAT) && (img->GetScalarType() != VTK_DOUBLE))
		? iAValueType::Discrete
		: iAValueType::Continuous;
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData(scalarRange[0], scalarRange[0]+histRange, numBin, valueType));

	auto vtkRawData = static_cast<DataType*>(rawImg->GetScalarPointer());
	std::copy(vtkRawData, vtkRawData + result->m_numBin, result->m_histoData);
	result->m_spacing = histRange / result->m_numBin;
	result->setMaxFreq();
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
	iAPlotData::DataType minX, iAPlotData::DataType maxX)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData);
	result->m_histoData = data;
	result->m_numBin = bins;
	result->m_spacing = space;
	result->m_xBounds[0] = minX;
	result->m_xBounds[1] = maxX;
	result->setMaxFreq();
	return result;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(const std::vector<DataType>& histData, size_t numBin, iAValueType type,
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
	result->m_valueType = type;
	if (dblApproxEqual(minValue, maxValue))
	{   // if min == max, there is only one bin - one in which all values are contained!
		result->m_numBin = 1;
		result->m_histoData = new DataType[result->m_numBin];
		result->m_histoData[0] = histData.size();
	}
	else
	{
		result->m_numBin = numBin;
		result->m_histoData = new DataType[numBin];
		result->m_spacing = (maxValue - minValue) / numBin;
		std::fill(result->m_histoData, result->m_histoData + numBin, 0.0);
		for (DataType d : histData)
		{
			size_t bin = clamp(
				static_cast<size_t>(0), numBin - 1, mapValue(minValue, maxValue, static_cast<size_t>(0), numBin, d));
			++result->m_histoData[bin];
		}
	}
	result->setMaxFreq();
	return result;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(DataType minX, DataType maxX, size_t numBin, iAValueType type)
{
	return QSharedPointer<iAHistogramData>(new iAHistogramData(minX, maxX, numBin, type));
}

QSharedPointer<iAHistogramData> iAHistogramData::create(
	DataType minX, DataType maxX, size_t numBin, iAValueType type, double* histoData)
{
	return QSharedPointer<iAHistogramData>(new iAHistogramData(minX, maxX, numBin, type, histoData));
}

template <typename ContT> double* createArrayFromContainer(ContT const & cont)
{
	double* dataArr = new double[cont.size()];
	for (size_t i = 0; i < cont.size(); ++i)
	{
		dataArr[i] = cont[i];
	}
	return dataArr;
}

QSharedPointer<iAHistogramData> iAHistogramData::create(
	DataType minX, DataType maxX, iAValueType type, std::vector<double> const& histoData)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData(minX, maxX, histoData.size(), type, createArrayFromContainer(histoData)));
	result->m_dataOwner = true;
	return result;
}


QSharedPointer<iAHistogramData> iAHistogramData::create(
	DataType minX, DataType maxX, iAValueType type, QVector<double> const& histoData)
{
	auto result = QSharedPointer<iAHistogramData>(new iAHistogramData(minX, maxX, histoData.size(), type, createArrayFromContainer(histoData)));
	result->m_dataOwner = true;
	return result;
}
