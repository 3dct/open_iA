// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAThresholdDefinitions.h"

#include <QVector>

class QLineF;
class QPointF;

//! intersects a line with some segments
QVector<QPointF> intersectLineWithRange(QLineF const & line, const threshold_defs::iAParametersRanges& aRange);
