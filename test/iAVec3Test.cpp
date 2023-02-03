// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iASimpleTester.h"
#include "iAVec3.h"

BEGIN_TEST
	iAVec3i vec1(0, 1, 2);
	iAVec3i vec2(2, 3, 4);
	iAVec3i resVec = vec1 + vec2;
	TestAssert(resVec[0] == 2 && resVec[1] == 4 && resVec[2] == 6);
END_TEST
