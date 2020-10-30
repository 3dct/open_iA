#pragma once

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

#include <QString>

QString distanceMetricToString(int i);
iADistanceMetricID stringToDistanceMetric(QString const & string);
*/

// Multidimensional scaling (MDS) with SMACOF
// This code re-implements Michael Bronstein's SMACOF in his Matlab Toolbox for Surface Comparison and Analysis
// The Matlab SMACOF can be downloaded at http://tosca.cs.technion.ac.il/
//[1] A. M. Bronstein, M. M. Bronstein, R. Kimmel,"Numerical geometry of nonrigid shapes", Springer, 2008.
iAMatrixType calculateMDS(iAMatrixType const& distanceMatrix,
	int outputDimensions, int iterations/*, iADistanceMetricID distanceMetric*/);