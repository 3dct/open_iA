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
