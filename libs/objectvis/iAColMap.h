// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QMap>
#include <memory>

//! Type used for mapping specific characteristics to column indices (see iACsvConfig::MappedColumn for characteristics constants)
using iAColMapT = QMap<uint16_t, uint16_t>;
//! pointer to a column mapping
using iAColMapP = std::shared_ptr<iAColMapT>;
