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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/

#include "iABoneThicknessChart.h"

#include <vtkAxis.h>
#include <vtkBrush.h>
#include <vtkChartXY.h>
#include <vtkContextMouseEvent.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkIntArray.h>
#include <vtkPlot.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

iABoneThicknessChart::iABoneThicknessChart(QWidget* _pParent) : QVTKWidget2(_pParent)
{
	m_pChart = vtkSmartPointer<vtkChartXY>::New();
	m_pChart->SetInteractive(false);

	vtkAxis* pAxis1(m_pChart->GetAxis(vtkAxis::BOTTOM));
	pAxis1->SetLabelsVisible(false);
	pAxis1->SetTicksVisible(false);
	pAxis1->SetTitle("");

	vtkAxis* pAxis2(m_pChart->GetAxis(vtkAxis::LEFT));
	pAxis2->SetTitle("Thickness");

	m_pTable = vtkSmartPointer<vtkTable>::New();

	vtkSmartPointer<vtkIntArray> pIntArray1(vtkSmartPointer<vtkIntArray>::New());
	pIntArray1->SetName("Index");
	m_pTable->AddColumn(pIntArray1);

	vtkSmartPointer<vtkDoubleArray> pDoubleArray2(vtkSmartPointer<vtkDoubleArray>::New());
	pDoubleArray2->SetName("Thickness");
	m_pTable->AddColumn(pDoubleArray2);

	vtkSmartPointer<vtkDoubleArray> pDoubleArray3(vtkSmartPointer<vtkDoubleArray>::New());
	pDoubleArray3->SetName("Average");
	m_pTable->AddColumn(pDoubleArray3);

	m_pContextView = vtkSmartPointer<vtkContextView>::New();
	m_pContextView->GetScene()->AddItem(m_pChart);
	m_pContextView->SetRenderWindow((vtkRenderWindow*)GetRenderWindow());
}

void iABoneThicknessChart::setData(vtkDoubleArray* _daThickness)
{
	const vtkIdType n (_daThickness->GetNumberOfTuples());

	m_pTable->SetNumberOfRows(n);

	double dAverage (0.0);

	for (vtkIdType i(0); i < n; i++)
	{
		const double dValue(_daThickness->GetTuple1(i));

		m_pTable->SetValue(i, 0, i + 1);
		m_pTable->SetValue(i, 1, dValue);

		dAverage += dValue;
	}

	dAverage /= (double) n;

	double pRange[2];
	_daThickness->GetRange(pRange);

	vtkAxis* pAxis1(m_pChart->GetAxis(vtkAxis::BOTTOM));
	pAxis1->SetTitle("Minimum: " + std::to_string(pRange[0]) + "   Mean: " + std::to_string(dAverage) + "   Maximum: " + std::to_string(pRange[1]));

	for (vtkIdType i(0); i < n; i++)
	{
		m_pTable->SetValue(i, 2, dAverage);
	}

	while (m_pChart->GetNumberOfPlots())
	{
		m_pChart->RemovePlot(0);
	}

	vtkPlot* pPlot(m_pChart->AddPlot(vtkChart::LINE));
	pPlot->SetColor(0, 0, 255, 255);
	pPlot->SetInputData(m_pTable, 0, 2);

	m_pPlot = m_pChart->AddPlot(vtkChart::BAR);
	m_pPlot->SetColor(255, 0, 0, 255);
	m_pPlot->SetInputData(m_pTable, 0, 1);
	m_pPlot->SetSelectable(true);
}

void iABoneThicknessChart::setSelected(const vtkIdType& _idSelected)
{
	if (m_pPlot)
	{
		if (_idSelected < 0)
		{
			vtkSmartPointer<vtkBrush> pBrush(vtkSmartPointer<vtkBrush>::New());
			pBrush->SetColor(255, 0, 0);
			m_pPlot->SetSelectionBrush(pBrush);
		}
		else
		{
			vtkSmartPointer<vtkBrush> pBrush(vtkSmartPointer<vtkBrush>::New());
			pBrush->SetColor(0, 255, 0);
			m_pPlot->SetSelectionBrush(pBrush);

			vtkSmartPointer<vtkIdTypeArray> pIdTypeArray(vtkSmartPointer<vtkIdTypeArray>::New());
			pIdTypeArray->InsertNextTuple1(_idSelected);
			m_pPlot->SetSelection(pIdTypeArray);
		}

		update();
	}
}
