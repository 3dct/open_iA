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
#pragma once

#include <itkIndex.h>

#include <unordered_map>
#include <vector>

struct Key
{
	const itk::IndexValueType* indizes;
	unsigned int _size;

	bool operator==(const Key &other) const
	{
		if (other._size != _size)
		{
			return false;
		}

		bool _equal = true;
		for (int j = 0; j < _size; j++)
		{
			if (indizes[j] != other.indizes[j])
			{
				return false;
			}
		}
		return _equal;
	}
};

namespace std {

	template <>
	struct hash<Key>
	{
		std::size_t operator()(const Key& k) const
		{
			using std::size_t;
			using std::hash;

			// Compute individual hash values for first,
			// second and third and combine them using XOR
			// and bit shifting:

			size_t retVal = 0;
			for (int j = 0; j < k._size; j++) {
				retVal = retVal * 31 + (hash<int>()(k.indizes[j]));
			}

			return retVal;
		}
	};

}

class iANDimImagePointer
{
public:

	iANDimImagePointer(unsigned int dimensionCount,unsigned int* dimSize, std::vector<std::vector<double>> &values);
	virtual ~iANDimImagePointer();

	unsigned int getMaxDim();
	double getDensityAt(const itk::IndexValueType* index, unsigned int indexDim);

	unsigned int getDimensionCount() { return _dimensionCount; }

	unsigned int* getDimSizes() { return _dimSize.get(); }

	unsigned int getLinearizedDimSize();
private:
	unsigned int _dimensionCount;
	std::unique_ptr<unsigned int[]> _dimSize;

	std::vector<std::vector<double>> _values; // list of data records of n-dimensional vectors

	std::unordered_map<Key, double> _densityMap;


};
