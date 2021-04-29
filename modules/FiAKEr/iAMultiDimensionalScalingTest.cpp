/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
	auto actualMDS = computeMDS(distMatrix, 1, 10);

	std::vector<double> expectedMDS = {-6.625, -5.625, -4.625, -2.625, -0.625, 2.375, 5.375, 12.375};
	for (int i = 0; i < expectedMDS.size(); ++i)
	{
		TestEqualFloatingPoint(expectedMDS[i], actualMDS[i][0]);
	}

	std::vector<double> values2 = { 1, 5, 9, 10, 11, 15, 19 };
	auto distMatrix2 = computeDistanceMatrix(values2, doubleDist);
	auto actualMDS2 = computeMDS(distMatrix2, 1, 10);
	std::vector<double> expectedMDS2 = {-9, -5, -1, 0, 1, 5, 9};
	for (int i = 0; i < expectedMDS2.size(); ++i)
	{
		TestEqualFloatingPoint(expectedMDS2[i], actualMDS2[i][0]);
	}

	auto actualMDS3 = computeMDS(distMatrix2, 2, 10);
	std::cout << matrixToString(actualMDS3).toStdString() << std::endl;
}
END_TEST
