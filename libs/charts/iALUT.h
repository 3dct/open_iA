/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
