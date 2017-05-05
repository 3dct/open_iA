/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
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
