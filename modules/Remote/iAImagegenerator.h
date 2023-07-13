// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

class vtkRenderWindow;

class QByteArray;
class QString;

namespace iAImagegenerator
{
	QByteArray createImage(QString const& viewID, vtkRenderWindow* window, int quality);
}
