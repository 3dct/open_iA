#include "iANDimImagePointer.h"

#include <cassert>

iANDimImagePointer::iANDimImagePointer(unsigned int dimensionCount, unsigned int* dimSize, std::vector<std::vector<double>> &values) :
	_dimensionCount{dimensionCount},
	_dimSize{ dimSize },
	_values{values}
{
	// sum up all elements in their hypercube and return the value divided by the count of all elements

	for (int i = 0; i < _values.size(); i++) {
		std::vector<double> record = _values[i];

		bool allIn = true;

		itk::IndexValueType * index = new itk::IndexValueType[dimensionCount];
		for (int j = 0; j < record.size(); j++) {

			double value = record[j];
			index[j] = (int)value;
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
	for (int i = 0; i < _dimensionCount; i++) {
		if (_dimSize[i] > maxDim) {
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
	if (density != _densityMap.end()) {
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
	for (int i = 0; i < _dimensionCount; i++) {
		ret *= _dimSize[i];
	}
	return ret;
}