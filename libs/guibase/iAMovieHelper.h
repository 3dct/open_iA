// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <vtkSmartPointer.h>

class iASimpleAbortListener;

class vtkGenericMovieWriter;

class QString;

iAguibase_API QString GetAvailableMovieFormats();
iAguibase_API vtkSmartPointer<vtkGenericMovieWriter> GetMovieWriter(QString const & fileName, int quality);
iAguibase_API void printFinalLogMessage(vtkGenericMovieWriter* movieWriter, iASimpleAbortListener const& abortListener);
