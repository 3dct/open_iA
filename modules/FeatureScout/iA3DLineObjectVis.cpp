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
#include "iA3DLineObjectVis.h"

#include "iACsvConfig.h"

#include <iAConsole.h>
#include <iARenderer.h>
#include <mdichild.h>

#include <vtkActor.h>
#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkTable.h>

iA3DLineObjectVis::iA3DLineObjectVis( iAVtkWidget* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & color ):
	iA3DColoredPolyObjectVis(widget, objectTable, columnMapping, color, 2)
{
	m_points = vtkSmartPointer<vtkPoints>::New();
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
		m_points->InsertNextPoint(first);
		m_points->InsertNextPoint(end);
		auto line = vtkSmartPointer<vtkLine>::New();
		line->GetPointIds()->SetId(0, 2 * row);     // the index of line start point in pts
		line->GetPointIds()->SetId(1, 2 * row + 1); // the index of line end point in pts
		lines->InsertNextCell(line);
	}
	m_linePolyData->SetPoints(m_points);
	m_linePolyData->SetLines(lines);
	m_linePolyData->GetPointData()->AddArray(m_colors);
	setupBoundingBox();
	setupOriginalIds();

	m_mapper->SetInputData(m_linePolyData);
	m_actor->SetMapper(m_mapper);
}

void iA3DLineObjectVis::updateValues(std::vector<std::vector<double> > const & values)
{
	for (int f = 0; f < values.size(); ++f)
	{
		m_points->SetPoint(2 * f, values[f].data());
		m_points->SetPoint(2 * f + 1, values[f].data() + 3);
	}
	m_points->Modified();
	updatePolyMapper();
}

vtkPolyData* iA3DLineObjectVis::getPolyData()
{
	return m_linePolyData;
}