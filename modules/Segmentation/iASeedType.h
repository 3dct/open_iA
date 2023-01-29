// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QSharedPointer>
#include <QString>
#include <QVector>

class iAImageCoordinate;

typedef std::pair<iAImageCoordinate, int> iASeedType;
typedef QVector<iASeedType> iASeedVector;
typedef QSharedPointer<iASeedVector> iASeedsPointer;

iASeedsPointer ExtractSeedVector(QString const & seedString, int width, int height, int depth);
