// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <memory>

class QImage;

template <typename ArgType, typename ValType>
class iAFunctionalBoxplot;
typedef iAFunctionalBoxplot<size_t, unsigned int> FunctionalBoxPlot;

std::shared_ptr<QImage> drawFunctionalBoxplot(FunctionalBoxPlot const * fbp, int width, int height);
