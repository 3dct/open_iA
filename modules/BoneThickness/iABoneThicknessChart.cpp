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
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkDoubleArray.h>
#include <vtkIntArray.h>
#include <vtkPlot.h>
#include <vtkSmartPointer.h>
#include <vtkTable.h>

iABoneThicknessChart::iABoneThicknessChart(QWidget* _pParent) : QVTKWidget2(_pParent)
{
	m_pChart = vtkSmartPointer<vtkChartXY>::New();
	m_pChart->SetForceAxesToBounds(true);
	m_pChart->SetSelectionMode(vtkContextScene::SELECTION_NONE);
	m_pChart->SetZoomWithMouseWheel(false);

	vtkAxis* pAxis1(m_pChart->GetAxis(vtkAxis::BOTTOM));
	pAxis1->SetTitle("");

	vtkAxis* pAxis2(m_pChart->GetAxis(vtkAxis::LEFT));
	pAxis2->SetTitle("Thickness");

	m_pTable = vtkSmartPointer<vtkTable>::New();

	vtkSmartPointer<vtkIntArray> pIntArray(vtkSmartPointer<vtkIntArray>::New());
	pIntArray->SetName("Index");
	m_pTable->AddColumn(pIntArray);

	vtkSmartPointer<vtkDoubleArray> pDoubleArray(vtkSmartPointer<vtkDoubleArray>::New());
	pDoubleArray->SetName("Thickness");
	m_pTable->AddColumn(pDoubleArray);

	vtkSmartPointer<vtkContextView> pContextView (vtkSmartPointer<vtkContextView>::New());
	pContextView->SetRenderWindow((vtkRenderWindow*)GetRenderWindow());
	pContextView->GetScene()->AddItem(m_pChart);
}

void iABoneThicknessChart::setData(vtkDoubleArray* _daThickness)
{
	const vtkIdType n (_daThickness->GetNumberOfTuples());

	m_pTable->SetNumberOfRows(n);

	for (vtkIdType i(0); i < n; i++)
	{
		m_pTable->SetValue(i, 0, i + 1);
		m_pTable->SetValue(i, 1, _daThickness->GetTuple1(i));
	}

	while (m_pChart->GetNumberOfPlots())
	{
		m_pChart->RemovePlot(0);
	}

	vtkPlot* pPlot (m_pChart->AddPlot(vtkChart::BAR));
	pPlot->SetColor(255, 0, 0, 255);
	pPlot->SetInputData(m_pTable, 0, 1);
}
