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
 
#include "pch.h"
#include "iAModalityTransfer.h"

#include "iAHistogramWidget.h"
#include "iAToolsVTK.h"

#include <vtkColorTransferFunction.h>
#include <vtkImageAccumulate.h>
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>

#include <QLabel>
#include <QLayout>
#include <QLayoutItem>
#include <QDockWidget>

iAModalityTransfer::iAModalityTransfer(vtkSmartPointer<vtkImageData> imgData, QString const & name, QWidget * parent, int binCount):
	histogram(0)
{
	ctf = GetDefaultColorTransferFunction(imgData);
	otf = GetDefaultPiecewiseFunction(imgData);
	accumulate = vtkSmartPointer<vtkImageAccumulate>::New();
	m_useAccumulate = imgData->GetNumberOfScalarComponents() == 1;
	if (m_useAccumulate)
	{
		accumulate->ReleaseDataFlagOn();
		UpdateAccumulateImageData(imgData, binCount);
		histogram = new iAHistogramWidget(parent,
			/* MdiChild */ 0, // todo: remove!
			accumulate,
			otf,
			ctf,
			name + QString(" Histogram"),
			false);
		histogram->hide();
	}
}

void iAModalityTransfer::UpdateAccumulateImageData(vtkSmartPointer<vtkImageData> imgData, int binCount)
{
	if (!m_useAccumulate)
		return;
	m_useAccumulate = imgData->GetNumberOfScalarComponents() == 1;
	m_scalarRange[0] = imgData->GetScalarRange()[0];
	m_scalarRange[1] = imgData->GetScalarRange()[1];
	accumulate->SetInputData(imgData);
	accumulate->SetComponentOrigin(imgData->GetScalarRange()[0], 0.0, 0.0);
	SetHistogramBinCount(binCount);
}

void iAModalityTransfer::SetHistogramBinCount(int binCount)
{
	if (!m_useAccumulate)
		return;
	if (isVtkIntegerType(static_cast<vtkImageData*>(accumulate->GetInput())->GetScalarType()))
	{
		binCount = std::min(binCount, static_cast<int>(m_scalarRange[1] - m_scalarRange[0] + 1));
	}
	accumulate->SetComponentExtent(0, binCount - 1, 0, 0, 0, 0);
	const double RangeEnlargeFactor = 1 + 1e-10;  // to put max values in max bin (as vtkImageAccumulate otherwise would cut off with < max)
	accumulate->SetComponentSpacing(((m_scalarRange[1] - m_scalarRange[0]) * RangeEnlargeFactor) / binCount, 0.0, 0.0);
	accumulate->Update();
	if (histogram)
	{
		histogram->UpdateData();
	}
}

iAHistogramWidget* iAModalityTransfer::ShowHistogram(QDockWidget* histogramContainer, bool enableFunctions)
{
	if (histogram)
	{
		histogram->SetEnableAdditionalFunctions(enableFunctions);
		histogram->show();
		histogramContainer->setWidget(histogram);
	}
	else
	{
		histogramContainer->setWidget(NoHistogramAvailableWidget());
	}
	return histogram;
}


iAHistogramWidget* iAModalityTransfer::GetHistogram()
{
	return histogram;
}

vtkPiecewiseFunction* iAModalityTransfer::GetOpacityFunction()
{
	return otf.Get();
}

vtkColorTransferFunction* iAModalityTransfer::GetColorFunction()
{
	return ctf;
}

vtkSmartPointer<vtkImageAccumulate> iAModalityTransfer::GetAccumulate()
{
	return accumulate;
}

void iAModalityTransfer::Update(vtkSmartPointer<vtkImageData> imgData, int binCount)
{
	if (!m_useAccumulate)
		return;
	UpdateAccumulateImageData(imgData, binCount);
	histogram->initialize(accumulate, true);
	histogram->redraw();
}

QWidget* iAModalityTransfer::NoHistogramAvailableWidget()
{
	static QLabel * NoHistogramLabel(new QLabel("Histogram not available for this dataset!"));
	NoHistogramLabel->setAlignment(Qt::AlignCenter);
	return NoHistogramLabel;
}
