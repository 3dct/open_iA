// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <limits>

#include <QtGlobal>    // for uint

#define DIM 3

// for both 2D and 3D magic lenses
const int DefaultMagicLensSize = 120;
const int MinimumMagicLensSize = 40;
const int MaximumMagicLensSize = 8192;

const uint NotExistingChannel = std::numeric_limits<uint>::max();
