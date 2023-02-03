// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

class vtkImageData;

class QString;


class iAAmiraMeshIO
{
public:
	static vtkSmartPointer<vtkImageData> Load(QString const & filename);
	static void Write(QString const & filename, vtkImageData* img);
};
