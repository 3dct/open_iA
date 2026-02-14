// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaio_export.h"

#include <vtkSmartPointer.h>

class iAProgress;

class vtkImageData;

class QString;

//! Read an image from disk into a VTK image.
//! @param filename the name of the file to read.
iAio_API vtkSmartPointer<vtkImageData> readImage(QString const& filename);

//! Stores an image on disk (typically in .mhd format).
//! @param img the image to store
//! @param filename the name of the file to write to.
//! @param useCompression whether the file should be compressed (.zraw) or not (.raw) in case we are storing .mhd files
//! @param progress an optional progress link; if != null, the file writer will trigger its progress signal
iAio_API void storeImage(vtkSmartPointer<vtkImageData> img, QString const & filename, bool useCompression = true, iAProgress const* progress = nullptr);

