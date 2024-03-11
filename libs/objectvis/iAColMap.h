// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <memory>

//! Type used for addressing a single column (16 bit unsigned, so we assume there aren't more than 65535 columns...)
using iAColIdxT = uint16_t;
//! Type used for mapping specific characteristics to column indices (see iACsvConfig::MappedColumn for characteristics constants)
using iAColMapT = QMap<iAColIdxT, iAColIdxT>;
//! pointer to a column mapping
using iAColMapP = std::shared_ptr<iAColMapT>;
