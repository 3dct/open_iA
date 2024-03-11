// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iASimpleTester.h"

#include "iAMultidimensionalScaling.h"



std::ostream& operator<<(std::ostream& o, const iAMatrixType& m)
{
	o << matrixToString(m).toStdString();
	return o;
}

BEGIN_TEST
{
	auto doubleDist = [](double a, double b)
	{
		return std::abs(a - b);
	};
	std::vector<double> values = { 1, 2, 3, 5, 7, 10, 13, 20 };
	auto distMatrix = computeDistanceMatrix(values, doubleDist);
	auto actualMDS = computeMDS(distMatrix, 1, 10, 0.0, false);

	std::vector<double> expectedMDS = {-6.625, -5.625, -4.625, -2.625, -0.625, 2.375, 5.375, 12.375};
	for (size_t i = 0; i < expectedMDS.size(); ++i)
	{
		TestEqualFloatingPoint(expectedMDS[i], actualMDS[i][0]);
	}

	std::vector<double> values2 = { 1, 5, 9, 10, 11, 15, 19 };
	auto distMatrix2 = computeDistanceMatrix(values2, doubleDist);
	auto actualMDS2 = computeMDS(distMatrix2, 1, 10, 0.0, false);
	std::vector<double> expectedMDS2 = {-9, -5, -1, 0, 1, 5, 9};
	for (size_t i = 0; i < expectedMDS2.size(); ++i)
	{
		TestEqualFloatingPoint(expectedMDS2[i], actualMDS2[i][0]);
	}

	auto actualMDS3 = computeMDS(distMatrix2, 2, 10, 0.0, false);
	std::cout << matrixToString(actualMDS3).toStdString() << std::endl;
}
END_TEST
