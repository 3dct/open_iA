/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iA3DLineObjectVis.h"

#include "iACsvConfig.h"

#include "mdichild.h"
#include "iARenderer.h"

#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkTable.h>

iA3DLineObjectVis::iA3DLineObjectVis( iAVtkWidgetClass* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & neutralColor ):
	iA3DColoredPolyObjectVis(widget, objectTable, columnMapping, neutralColor, 2)
{
	auto pts = vtkSmartPointer<vtkPoints>::New();
	m_linePolyData = vtkSmartPointer<vtkPolyData>::New();
	auto lines = vtkSmartPointer<vtkCellArray>::New();
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		float first[3], end[3];
		for (int i = 0; i < 3; ++i)
		{
			first[i] = m_objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::StartX + i)).ToFloat();
			end[i] = m_objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::EndX + i)).ToFloat();
		}
		pts->InsertNextPoint(first);
		pts->InsertNextPoint(end);
		auto line = vtkSmartPointer<vtkLine>::New();
		line->GetPointIds()->SetId(0, 2 * row);     // the index of line start point in pts
		line->GetPointIds()->SetId(1, 2 * row + 1); // the index of line end point in pts
		lines->InsertNextCell(line);
	}
	m_linePolyData->SetPoints(pts);
	m_linePolyData->SetLines(lines);
	m_linePolyData->GetPointData()->AddArray(m_colors);
	m_mapper->SetInputData(m_linePolyData);
}

double const * iA3DLineObjectVis::bounds()
{
	return m_linePolyData->GetBounds();
}
