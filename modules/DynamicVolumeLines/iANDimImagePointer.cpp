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
#include "iANDimImagePointer.h"

#include <cassert>

iANDimImagePointer::iANDimImagePointer(unsigned int dimensionCount, unsigned int* dimSize, std::vector<std::vector<double>> &values) :
	_dimensionCount{dimensionCount},
	_dimSize{ dimSize },
	_values{values}
{
	// sum up all elements in their hypercube and return the value divided by the count of all elements

	for (size_t i = 0; i < _values.size(); i++)
	{
		std::vector<double> record = _values[i];
		itk::IndexValueType * index = new itk::IndexValueType[dimensionCount];
		for (size_t j = 0; j < record.size(); j++)
		{
			double value = record[j];
			index[j] = static_cast<int>(value);
		}

		Key indexKey;
		indexKey._size = _dimensionCount;
		indexKey.indizes = index;

		_densityMap[indexKey]++;
	}
}


iANDimImagePointer::~iANDimImagePointer()
{
}

unsigned int iANDimImagePointer::getMaxDim()
{
	unsigned int maxDim = 0;
	for (unsigned int i = 0; i < _dimensionCount; i++)
	{
		if (_dimSize[i] > maxDim)
		{
			maxDim = _dimSize[i];
		}
	}
	return maxDim;
}

double iANDimImagePointer::getDensityAt(const itk::IndexValueType * index, unsigned int indexDim)
{
	// sum up all elements at the given index hypercube and return the value divided by the count of all elements

	assert(indexDim == _dimensionCount);

	Key indexKey;
	indexKey._size = _dimensionCount;
	indexKey.indizes = index;

	auto density = _densityMap.find(indexKey);
	if (density != _densityMap.end())
	{
		return density->second;
	}
	return 0.0;

	//unsigned int count = 0;

	//for (int i = 0; i < _values.size(); i++) {
	//	std::vector<double> record = _values[i];

	//	bool allIn = true;

	//	for (int j = 0; j < record.size(); j++) {
	//		double value = record[j];
	//		if (value < index[j] || value >= index[j] + 1) {
	//			allIn = false;
	//		}
	//	}

	//	if (allIn) {
	//		count++;
	//	}
	//}

	//if (count > 0) {
	//	count++;
	//}

	//return count; // / double(_values.size());
}

unsigned int iANDimImagePointer::getLinearizedDimSize()
{
	unsigned int ret = 1;
	for (unsigned int i = 0; i < _dimensionCount; i++)
	{
		ret *= _dimSize[i];
	}
	return ret;
}