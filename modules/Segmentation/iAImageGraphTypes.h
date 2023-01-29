// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAImageCoordinate.h>	// for iAVoxelIndexType

#include <QVector>

#include <utility> // for std::pair



typedef unsigned int iAEdgeIndexType;
typedef unsigned int iAVertexIndexType;
typedef int iALabelType;
typedef double iAEdgeWeightType;
typedef std::pair<iAVoxelIndexType, iAVoxelIndexType> iAEdgeType;

typedef QVector<iALabelType> iALabelData;
