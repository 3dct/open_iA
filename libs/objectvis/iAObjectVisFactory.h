// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaobjectvis_export.h"

#include <QColor>

#include <memory>

class iAObjectsData;
class iAObjectVis;

//! Factory function for creating a fitting iAObjectVis-derived class for the given visualization type
//! @param data the data of the objects
//! @param color the default color of the objects
iAobjectvis_API std::shared_ptr<iAObjectVis> createObjectVis(iAObjectsData const* data, QColor const& color);
