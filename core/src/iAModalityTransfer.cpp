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
 
#include "pch.h"
#include "iAModalityTransfer.h"

#include "iAHistogramWidget.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageAccumulate.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <QLayout>
#include <QLayoutItem>
#include <QDockWidget>

ModalityTransfer::ModalityTransfer(vtkSmartPointer<vtkImageData> imgData, QString const & name, QWidget * parent, int binCount)
{
	double rangeMin = imgData->GetScalarRange()[0];
	double rangeMax = imgData->GetScalarRange()[1];
	
	ctf = GetDefaultColorTransferFunction(imgData);
	otf = GetDefaultPiecewiseFunction(imgData);

	accumulate = vtkSmartPointer<vtkImageAccumulate>::New();
	accumulate->ReleaseDataFlagOn();
	accumulate->SetComponentExtent(0, binCount - 1, 0, 0, 0, 0); // number of bars
	accumulate->SetComponentSpacing((rangeMax - rangeMin) / binCount, 0.0, 0.0);
	accumulate->SetComponentOrigin(rangeMin, 0.0, 0.0);
	accumulate->SetInputData(imgData);
	accumulate->Update();

	histogram = new iAHistogramWidget(parent,
		/* MdiChild */ 0, // todo: remove!
		imgData->GetScalarRange(),
		accumulate,
		otf,
		ctf,
		name + QString(" Histogram"),
		false);
}

void ModalityTransfer::SetHistogramBins(int binCount)
{
	accumulate->SetComponentExtent(0, binCount - 1, 0, 0, 0, 0); // number of bars
	accumulate->Update();
}

iAHistogramWidget* ModalityTransfer::ShowHistogram(QDockWidget* histogramContainer, bool enableFunctions)
{
	QLayoutItem * child;
	while ((child = histogramContainer->layout()->takeAt(0)) != 0)
	{
		child->widget()->hide();
	}
	histogram->SetEnableAdditionalFunctions(enableFunctions);
	histogram->show();

	histogramContainer->setWidget(histogram);

	return histogram;
}

vtkPiecewiseFunction* ModalityTransfer::GetOpacityFunction()
{
	return otf.Get();
}

vtkColorTransferFunction* ModalityTransfer::GetColorFunction()
{
	return ctf;
}

vtkSmartPointer<vtkImageAccumulate> ModalityTransfer::GetAccumulate()
{
	return accumulate;
}
