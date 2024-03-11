// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>

#include <vector>

using iAMatrixType = std::vector<std::vector<double>>;

/*

enum class iADistanceMetricID
{
	Unknown = -1,
	EuclideanDistance,
	MinkowskiDistance,
	NumberOfDistanceMetrics
};

QString distanceMetricToString(int i);
iADistanceMetricID stringToDistanceMetric(QString const & string);
*/
QString matrixToString(iAMatrixType const& input);

//! Multidimensional scaling (MDS) with SMACOF
//! This code re-implements Michael Bronstein's SMACOF in his Matlab Toolbox for Surface Comparison and Analysis
//! The Matlab SMACOF can be downloaded at http://tosca.cs.technion.ac.il/
//! [1] A. M. Bronstein, M. M. Bronstein, R. Kimmel,"Numerical geometry of nonrigid shapes", Springer, 2008.
//! TODO: Move to core!
iAMatrixType computeMDS(iAMatrixType const& distanceMatrix, int outputDimensions, int iterations,
	double maxError = 0.0 /*, iADistanceMetricID distanceMetric*/, bool initRandom = true);

template <typename InT, typename DistT>
iAMatrixType computeDistanceMatrix(InT const& data, DistT distance)
{
	iAMatrixType result(data.size(), std::vector<double>(data.size(), 0.0));
	for (size_t r = 0; r < data.size() - 1; r++)
	{
		for (size_t c = r + 1; c < data.size(); c++)
		{
			result[r][c] = distance(data[r], data[c]);
		}
	}
	for (size_t r = 1; r < data.size(); r++)
	{
		for (size_t c = 0; c < r; c++)
		{
			result[r][c] = result[c][r];
		}
	}
	return result;
}
