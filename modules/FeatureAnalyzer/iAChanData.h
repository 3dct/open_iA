// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkSmartPointer.h>

#include <QScopedPointer>
#include <QColor>

class vtkImageData;
class vtkColorTransferFunction;
class vtkPiecewiseFunction;
class iAChannelData;
class vtkScalarBarWidget;

struct iAChanData
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
