// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <itkConfigure.h>    // for ITK_VERSION...

//! Assemble a single number from a major,minor,patch tuple against which the ITK version number can be checked
//! To conditionally add code depending on a specific ITK version, use code following this pattern:
//!     #if ITK_VERSION_NUMBER < ITK_VERSION_CHECK(5,3,0)
//!         // code that applies for ITK < 5.3.0
//!     # else
//!         // code that applies for ITK >= 5.3.0
//!     #endif
#define ITK_VERSION_CHECK(major, minor, patch)                              \
  (10000000000ULL * major + 100000000ULL * minor + patch)
//! Assemble a single number from the version of the ITK library currently in use
#define ITK_VERSION_NUMBER                                                  \
  ITK_VERSION_CHECK(ITK_VERSION_MAJOR, ITK_VERSION_MINOR, ITK_VERSION_PATCH)
