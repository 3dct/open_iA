// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAElementalDecomposition.h"

#include "iAEnergySpectrum.h"

#include <vtkMath.h>

#include <algorithm>

namespace
{
template<class TReal>
TReal **create_matrix ( size_t nrow, size_t ncol )
{
	typedef TReal* TRealPointer;
	TReal **m = new TRealPointer[nrow];
	for (size_t row = 0; row < nrow; ++row)
	{
		m[row] = new TReal[ncol];
	}
	return m;
}

template<class TReal>
void free_matrix ( TReal **m, size_t nrow)
{
	for (size_t row = 0; row < nrow; ++row)
	{
		delete [] m[row];
	}
	delete[] m;
}

bool anyElementAboveThreshold(std::shared_ptr<QVector<std::shared_ptr<iAEnergySpectrum> > > const & elements, int i, unsigned int threshold)
{
	for (
		QVector<std::shared_ptr<iAEnergySpectrum> >::const_iterator it = elements->begin();
		it != elements->end();
		++it)
	{
		if ((**it)[i] > threshold)
		{
			return true;
		}
	}
	return false;
}

} // namespace

bool fitSpectrum(
	iAEnergySpectrum const & unknownSpectrum,
	std::shared_ptr<QVector<std::shared_ptr<iAEnergySpectrum> > > elements,
	CountType threshold,
	QVector<double> & result)
{
	if (!elements || elements->size() == 0 ||
		unknownSpectrum.size() != (*elements)[0]->size())
	{
		// impossible to calculate decomposition if
		// no reference spectra or
		// reference spectra and unknown spectrum are of different size
		return false;
	}
	int noOfDataPoints = 0;
	int dataSampleRelevantPoints = 0;
	QVector<int> dataPointIdx;
	for (int i=0; i<unknownSpectrum.size(); ++i)
	{
		if (unknownSpectrum[i] > threshold || anyElementAboveThreshold(elements, i, threshold) )
		{
			if (unknownSpectrum[i] > threshold) {
				dataSampleRelevantPoints++;
			}
			dataPointIdx.push_back(i);
			noOfDataPoints++;
		}
	}

	if (dataSampleRelevantPoints == 0)
	{
		for (int i=0; i<elements->size(); ++i)
		{
			result.push_back(0.0);
		}
		return false;
	}

	double **X = create_matrix<double>(noOfDataPoints, elements->size());
	double **y = create_matrix<double>(noOfDataPoints, 1);

	double **c = create_matrix<double>(elements->size(), 1);

	for (int i = 0; i < noOfDataPoints; i++)
	{
		for (int j=0; j<elements->size(); ++j)
		{
			unsigned int elemCount = (*(*elements)[j])[dataPointIdx[i]];
			X[i][j] = elemCount;
		}
		y[i][0] = unknownSpectrum[dataPointIdx[i]];
	}

	vtkMath::SolveLeastSquares(noOfDataPoints, X, static_cast<int>(elements->size()), y, 1, c);

	result.clear();
	for (int i=0; i<elements->size(); ++i)
	{
		result.push_back(std::max(c[i][0], 0.0));
	}

	free_matrix(X, noOfDataPoints);
	free_matrix(y, noOfDataPoints);
	free_matrix(c, elements->size());

	return true;
}
