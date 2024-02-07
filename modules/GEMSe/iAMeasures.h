// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImageTreeNode.h"

void CalculateMeasures(LabelImagePointer refImg, LabelImagePointer curImg, int labelCount,
	QVector<double> & measures, bool reportUndecided=false);
