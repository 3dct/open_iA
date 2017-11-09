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

#include "iATransferFunction.h"
#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iAHistogramWidget;
class iAImageInfo;

class vtkColorTransferFunction;
class vtkImageAccumulate;
class vtkImageData;
class vtkPiecewiseFunction;

class QColor;
class QDockWidget;
class QString;
class QWidget;

//! class uniting a color transfer function, an opacity transfer function
//! and GUI classes used for viewing a histogram of the data and for editing the transfer functions
class open_iA_Core_API iAModalityTransfer : public iATransferFunction
{
private:
	QSharedPointer<iAImageInfo> m_imageInfo;
	vtkSmartPointer<vtkImageAccumulate> accumulate;
	iAHistogramWidget* histogram;
	vtkSmartPointer<vtkColorTransferFunction> ctf;
	vtkSmartPointer<vtkPiecewiseFunction> otf;
	double m_scalarRange[2];
	bool m_useAccumulate;
	void UpdateAccumulateImageData(vtkSmartPointer<vtkImageData> imgData, int binCount);
public:
	iAImageInfo const & Info() const;
	iAModalityTransfer(vtkSmartPointer<vtkImageData> imgData, QString const & name, QWidget * parent, int binCount);
	void SetName(QString name);
	iAHistogramWidget* GetHistogram();
	void SetHistogramBinCount(int binCount);
	void Update(vtkSmartPointer<vtkImageData> imgData, int binCount);

	// should return vtkSmartPointer, but can't at the moment because dlg_transfer doesn't have smart pointers:
	vtkPiecewiseFunction* GetOpacityFunction();
	vtkColorTransferFunction* GetColorFunction();

	vtkSmartPointer<vtkImageAccumulate> GetAccumulate();
	iAHistogramWidget* ShowHistogram(QDockWidget* histogramContainer, bool enableFunctions = false);

	static QWidget* NoHistogramAvailableWidget();
};
