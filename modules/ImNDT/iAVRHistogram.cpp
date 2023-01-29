// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later

#include "iAVRHistogram.h"

iAVRHistogram::iAVRHistogram()
{
	initializeDataStructure();
}

//! Returns a pointer to the boost::histogram structure
regularStatic1DHistogram* iAVRHistogram::getHistogram()
{
	return &m_histogram;
}


void iAVRHistogram::initializeDataStructure()
{
	m_histogramParameters = HistogramParameters();
	m_histogram = regularStatic1DHistogram();
}
