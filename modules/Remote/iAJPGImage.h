// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QByteArray>

//! class holding byte data and size of a jpeg image.
class iAJPGImage
{
public:
	QByteArray data;
	int width, height;
};
