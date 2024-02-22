// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAITKIO.h>

#include <QVector>

iAITKIO::ImagePointer CalculateDifferenceMarkers(QVector<iAITKIO::ImagePointer> imgs, double differenceMarkerValue);
