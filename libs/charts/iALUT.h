// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAcharts_export.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QStringList>


class iAColorTheme;
class iALookupTable;

class vtkColorTransferFunction;
class vtkLookupTable;
class vtkPiecewiseFunction;

class iAcharts_API iALUT
{
public:
	static QStringList colorMapNames();
	static void loadMaps(QString const& folder);
	static int BuildLUT(vtkSmartPointer<vtkLookupTable> pLUT, double const* lutRange, QString colorMap, int numCols = 256, bool reverse = false);
	static int BuildLUT(vtkSmartPointer<vtkLookupTable> pLUT, double rangeFrom, double rangeTo, QString colorMap, int numCols = 256, bool reverse = false);
	static iALookupTable Build(double const* lutRange, QString colorMap, int numCols, double alpha, bool reverse = false);

	static vtkSmartPointer<vtkPiecewiseFunction> BuildLabelOpacityTF(int labelCount);
	static vtkSmartPointer<vtkLookupTable> BuildLabelColorTF(int labelCount, iAColorTheme const* colorTheme);

private:
	static QMap<QString, vtkSmartPointer<vtkColorTransferFunction>> m_colorMaps;
};
