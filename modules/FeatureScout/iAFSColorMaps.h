// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class QColor;

typedef void (*ColormapFuncPtr)(const double normal[3], double color_out[3]);

ColormapFuncPtr getColorMap(int index);

//! returns the color for a given class id
//! TODO: use iAColorTheme?
QColor getClassColor(int cid);
