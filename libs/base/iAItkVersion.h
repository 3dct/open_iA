// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <itkConfigure.h>    // for ITK_VERSION...

#define ITK_VERSION_CHECK(major, minor, patch)                              \
  (10000000000ULL * major + 100000000ULL * minor + patch)
#define ITK_VERSION_NUMBER                                                  \
  ITK_VERSION_CHECK(ITK_VERSION_MAJOR, ITK_VERSION_MINOR, ITK_VERSION_PATCH)
