// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iABoxPlotData.h>

struct iABPMData
{
	QList< QList< iABoxPlotData > > boxPlots;
	QStringList datasets;
	QStringList filters;
	void clear()
	{
		datasets.clear();
		filters.clear();
		boxPlots.clear();
	}
};
