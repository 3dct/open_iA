// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAImageCoordinate.h>	// for iAVoxelIndexType

#include <QVector>

#include <utility> // for std::pair



typedef qsizetype iAEdgeIndexType;
typedef qsizetype iAVertexIndexType;
typedef int iALabelType;
typedef double iAEdgeWeightType;
typedef std::pair<iAFlatIndexType, iAFlatIndexType> iAEdgeType;

typedef QVector<iALabelType> iALabelData;
