// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAHistogramPlotData.h"

#include <QMultiMap>

struct iAHMData
{
	QList< QList< iAHistogramPlotData > > histogramPlots;
	QStringList datasets;
	QStringList filters;
	QMultiMap<QString, double> gtPorosityMap;
	void clear()
	{
		histogramPlots.clear();
		datasets.clear();
		filters.clear();
		gtPorosityMap.clear();
	}
};
