// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>

class iAJPGImage;

class vtkRenderWindow;

class QString;

namespace iAImagegenerator
{
	std::shared_ptr<iAJPGImage> createImage(QString const& viewID, vtkRenderWindow* window, int quality);
}
