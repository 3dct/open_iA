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

#include "iAAbstractDiagramData.h"

#include <vtkSmartPointer.h>

class vtkImageData;
class vtkImageAccumulate;

class iAHistogramData: public iAAbstractDiagramRangedData
{
public:
	iAHistogramData();
	virtual double GetSpacing() const;
	virtual double const * GetDataRange() const;
	virtual DataType const * GetData() const;
	virtual size_t GetNumBin() const;
	virtual DataType GetMaxValue() const;
	virtual iAValueType GetRangeType() const;

	void initialize(vtkImageAccumulate* imgAccumulate);
	void initialize(vtkImageAccumulate* imgAccumulate, DataType* data, size_t numBin, double space, DataType min, DataType max);
	void Update();

private:
	vtkImageAccumulate* accumulate;
	size_t				numBin;
	vtkSmartPointer<vtkImageData> rawImg;
	iAAbstractDiagramData::DataType*	rawData;
	iAAbstractDiagramData::DataType		maxFreq;
	double				accSpacing;
	double				dataRange[2];
	iAValueType			m_type;
	void SetMaxFreq();
};
