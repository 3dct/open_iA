// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

class vtkRenderWindow;

class QByteArray;

class iAImagegenerator: QObject
{
	Q_OBJECT
public:
	static QByteArray createImage(vtkRenderWindow* window, int quality);
};
