// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

class vtkImageData;

class QString;

//! Encapsulates loading and storing (uncompressed) volumes from/to the Amira mesh format.
class iAAmiraMeshIO
{
public:
	static vtkSmartPointer<vtkImageData> Load(QString const & filename);
	static void Write(QString const & filename, vtkImageData* img);
};
