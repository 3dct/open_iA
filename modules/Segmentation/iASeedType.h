// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QVector>

#include <memory>

class iAImageCoordinate;

typedef std::pair<iAImageCoordinate, int> iASeedType;
typedef QVector<iASeedType> iASeedVector;
typedef std::shared_ptr<iASeedVector> iASeedsPointer;

iASeedsPointer ExtractSeedVector(QString const & seedString, int width, int height, int depth);
