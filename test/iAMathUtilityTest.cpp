// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
