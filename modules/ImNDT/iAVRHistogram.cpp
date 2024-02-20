// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAVRHistogram.h"

iAVRHistogram::iAVRHistogram()
{
	initializeDataStructure();
}

regularStatic1DHistogram* iAVRHistogram::getHistogram()
{
	return &m_histogram;
}

void iAVRHistogram::initializeDataStructure()
{
	m_histogramParameters = HistogramParameters();
	m_histogram = regularStatic1DHistogram();
}
