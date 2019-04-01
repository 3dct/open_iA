/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>
#include <QScopedPointer>
#include <QColor>

class vtkImageData;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class iAChannelData;
class vtkScalarBarWidget;

struct open_iA_Core_API iAChanData
{
	iAChanData( QColor c1, QColor c2, uint chanId );
	iAChanData( const QList<QColor> & colors, uint chanId );
	void InitTFs();

	QScopedPointer<iAChannelData> visData;
	vtkSmartPointer<vtkImageData> imgData;
	vtkSmartPointer<vtkColorTransferFunction> tf;
	vtkSmartPointer<vtkPiecewiseFunction> otf;
	vtkSmartPointer<vtkPiecewiseFunction> vol_otf;
	QList<QColor> cols;
	const uint id;
	vtkSmartPointer<vtkScalarBarWidget> scalarBarWgt;
};
