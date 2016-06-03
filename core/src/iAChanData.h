/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#pragma once

#include "iAChannelID.h"
#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>
#include <QScopedPointer>
#include <QColor>

class vtkImageData;
class vtkTransform;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class iAChannelVisualizationData;
class vtkScalarBarWidget;

struct open_iA_Core_API iAChanData
{
	iAChanData( QColor c1, QColor c2, iAChannelID chanId );
	iAChanData( const QList<QColor> & colors, iAChannelID chanId );
	void InitTFs();

	QScopedPointer<iAChannelVisualizationData> visData;
	vtkSmartPointer<vtkImageData> imgData;
	vtkSmartPointer<vtkColorTransferFunction> tf;
	vtkSmartPointer<vtkPiecewiseFunction> otf;
	vtkSmartPointer<vtkPiecewiseFunction> vol_otf;
	QList<QColor> cols;
	const iAChannelID id;
	vtkSmartPointer<vtkScalarBarWidget> scalarBarWgt;
};