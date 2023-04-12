// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

//! Checks whether CUDA is available; two aspects need to be fulfilled:
//!      - open_iA has to be built with CUDA support,
//!      - and CUDA has to be available on the system currently running open_iA
iAbase_API bool isCUDAAvailable();
