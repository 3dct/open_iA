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

#include "iATransferFunction.h"

#include <vtkSmartPointer.h>

class iAHistogramWidget;

class vtkColorTransferFunction;
class vtkImageAccumulate;
class vtkImageData;
class vtkPiecewiseFunction;

class QColor;
class QDockWidget;
class QString;
class QWidget;

class ModalityTransfer: public TransferFunction
{
private:
	vtkSmartPointer<vtkImageAccumulate> accumulate;
	iAHistogramWidget* histogram;
	vtkSmartPointer<vtkColorTransferFunction> ctf;
	vtkSmartPointer<vtkPiecewiseFunction> otf;
public:
	ModalityTransfer(vtkSmartPointer<vtkImageData> imgData, QString const & name, QWidget * parent, int binCount);

	void SetHistogramBins(int binCount);

	// should return vtkSmartPointer, but can't at the moment because dlg_transfer doesn't have smart pointers:
	vtkPiecewiseFunction* GetOpacityFunction();
	vtkColorTransferFunction* GetColorFunction();

	vtkSmartPointer<vtkImageAccumulate> GetAccumulate();
	iAHistogramWidget* ShowHistogram(QDockWidget* histogramContainer, bool enableFunctions = false);
};
