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
 
#include "pch.h"
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
	for ( int row = 0; row < nrow; ++row )
	{
		m[row] = new TReal[ncol];
	}
	return m;
}

template<class TReal>
void free_matrix ( TReal **m, size_t nrow)
{
	for ( int row = 0; row < nrow; ++row )
	{
		delete [] m[row];
	}
	delete[] m;
}

bool anyElementAboveThreshold(QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > const & elements, int i, unsigned int threshold)
{
	for (
		QVector<QSharedPointer<iAEnergySpectrum> >::const_iterator it = elements->begin();
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
	QSharedPointer<QVector<QSharedPointer<iAEnergySpectrum> > > elements,
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