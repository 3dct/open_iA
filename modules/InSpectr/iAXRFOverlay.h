// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>
#include <vtkColorTransferFunction.h>

#include <QColor>
#include <QImage>
#include <QSharedPointer>

class iAAccumulatedXRFData;

void initSpectraColormap(
	vtkSmartPointer<vtkColorTransferFunction> colormapLUT,
	QSharedPointer<iAAccumulatedXRFData> accData,
	double val, double max,
	QImage const & spectraHistogramColormap);

QSharedPointer<QImage> CalculateSpectraHistogramImage(
	vtkSmartPointer<vtkColorTransferFunction> colormapLUT,
	QSharedPointer<iAAccumulatedXRFData> accData,
	QImage const & spectraHistogramColormap,
	long numBin,
	double sensVal, double sensMax, double threshVal, double threshMax,
	bool smoothFade);
