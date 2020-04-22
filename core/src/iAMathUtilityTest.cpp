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
#include "iASimpleTester.h"
#include "iAMathUtility.h"

BEGIN_TEST
	FuncType func_a = { 2.0, 3.0, 3.0, 5.0, 5.5, 8.0, 10.0, 10.0 };
	FuncType func_b = { 1.5, 1.5, 4.0, 3.0, 1.0, 5.0, 5.0, 9.5 };

	auto normedRanks_a = getNormedRanks(func_a);
	TestEqual(normedRanks_a[0], 1.0);
	TestEqual(normedRanks_a[1], 2.5);
	TestEqual(normedRanks_a[2], 2.5);
	TestEqual(normedRanks_a[3], 4.0);
	TestEqual(normedRanks_a[4], 5.0);
	TestEqual(normedRanks_a[5], 6.0);
	TestEqual(normedRanks_a[6], 7.5);
	TestEqual(normedRanks_a[7], 7.5);

	auto normedRanks_b = getNormedRanks(func_b);
	TestEqual(normedRanks_b[0], 2.5);
	TestEqual(normedRanks_b[1], 2.5);
	TestEqual(normedRanks_b[2], 5.0);
	TestEqual(normedRanks_b[3], 4.0);
	TestEqual(normedRanks_b[4], 1.0);
	TestEqual(normedRanks_b[5], 6.5);
	TestEqual(normedRanks_b[6], 6.5);
	TestEqual(normedRanks_b[7], 8.0);

	double sCC = spearmansCorrelationCoefficient(func_a, func_b);
	TestEqualFloatingPoint(sCC, 0.682927);
END_TEST