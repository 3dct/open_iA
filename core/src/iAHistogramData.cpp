/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
 
#include "pch.h"
#include "iAHistogramData.h"

#include "iAVtkDataTypeMapper.h"

#include <vtkImageAccumulate.h>
#include <vtkImageCast.h>
#include <vtkImageData.h>

iAHistogramData::iAHistogramData()
	: accumulate(0),
	numBin(0), rawData(0), rawImg(0), maxFreq(0)
{
	accSpacing = 0;
	dataRange[0]  = dataRange[1]  = 0;
}

double iAHistogramData::GetSpacing() const
{
	return accSpacing;
}

double * iAHistogramData::GetDataRange()
{
	return dataRange;
}

double iAHistogramData::GetDataRange(int idx) const
{
	return dataRange[idx];
}

iAHistogramData::DataType const * iAHistogramData::GetData() const
{
	return rawData;
}

void iAHistogramData::initialize(vtkImageAccumulate* imgAccumulate,
	double * scalarRange)
{
	accumulate = imgAccumulate;
	dataRange[0] = scalarRange[0];
	dataRange[1] = scalarRange[1];
	Update();
}

void iAHistogramData::initialize(vtkImageAccumulate* imgAccumulate,
	iAAbstractDiagramData::DataType* data, size_t bins, double space,
	iAAbstractDiagramData::DataType min,
	iAAbstractDiagramData::DataType max)
{
	accumulate = imgAccumulate;
	rawData = data;
	numBin = bins;
	accSpacing = space;
	dataRange[0] = min;
	dataRange[1] = max;
	SetMaxFreq();
}


void iAHistogramData::Update()
{
	int extent[6];
	accumulate->GetComponentExtent(extent);
	numBin = extent[1] + 1;
	vtkSmartPointer<vtkImageCast> caster = vtkSmartPointer<vtkImageCast>::New();
	caster->SetInputData(accumulate->GetOutput());
	caster->SetOutputScalarType(VtkDataType<DataType>::value);
	caster->Update();
	rawImg = caster->GetOutput();
	rawData = static_cast<DataType* >(rawImg->GetScalarPointer());
	double null1, null2;
	accumulate->GetComponentSpacing(accSpacing, null1, null2);
	SetMaxFreq();
}

size_t iAHistogramData::GetNumBin() const
{
	return numBin;
}

iAAbstractDiagramData::DataType iAHistogramData::GetMaxValue() const
{
	return maxFreq;
}

void iAHistogramData::SetMaxFreq()
{
	if(rawData)
	{
		maxFreq = 1;
		for ( int i = 0; i < GetNumBin(); i++ ) {
			if (rawData[i] > maxFreq)
				maxFreq = rawData[i];
		}
	}
}
