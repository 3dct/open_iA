// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

class vtkProp;
class vtkRenderer;

iAguibase_API void showActor(vtkRenderer* renderer, vtkProp* actor, bool show);
