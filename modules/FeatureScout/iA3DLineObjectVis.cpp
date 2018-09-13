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

#include "iALookupTable.h"
#include "iARenderer.h"
#include "mdichild.h"

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkIdFilter.h>
#include <vtkLine.h>
#include <vtkOutlineFilter.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkTable.h>

iA3DLineObjectVis::iA3DLineObjectVis( iAVtkWidgetClass* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & neutralColor ):
	iA3DColoredPolyObjectVis(widget, objectTable, columnMapping, neutralColor, 2),
	m_selectionActive(false),
	m_outlineFilter(vtkSmartPointer<vtkOutlineFilter>::New()),
	m_outlineMapper(vtkSmartPointer<vtkPolyDataMapper>::New()),
	m_outlineActor(vtkSmartPointer<vtkActor>::New())
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

	auto ids = vtkSmartPointer<vtkIdTypeArray>::New();
	ids->SetName("OriginalIds");
	vtkIdType numPoints = objectTable->GetNumberOfRows() * 2;
	ids->SetNumberOfTuples(numPoints);
	for (vtkIdType id = 0; id < numPoints; ++id)
		ids->SetTuple1(id, id);
	m_linePolyData->GetPointData()->AddArray(ids);

	m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_mapper->SetInputData(m_linePolyData);
	m_actor->SetMapper(m_mapper);

	m_outlineFilter->SetInputData(m_linePolyData);
	m_outlineMapper->SetInputConnection(m_outlineFilter->GetOutputPort());
	m_outlineActor->GetProperty()->SetColor(0, 0, 0);
	m_outlineActor->PickableOff();
	m_outlineActor->SetMapper(m_outlineMapper);
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

void iA3DLineObjectVis::showBoundingBox()
{
	m_outlineMapper->Update();
	m_widget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_outlineActor);
	updateRenderer();
}

void iA3DLineObjectVis::hideBoundingBox()
{
	m_widget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(m_outlineActor);
	updateRenderer();
}
vtkPolyData* iA3DLineObjectVis::getLinePolyData()
{
	return m_linePolyData;
}

void iA3DLineObjectVis::setColor(QColor const &color)
{
	m_baseColor = color;
	m_colorParamIdx = -1;
	m_lut.clear();
	updateColorSelectionRendering();
}

void iA3DLineObjectVis::setLookupTable(QSharedPointer<iALookupTable> lut, size_t paramIndex)
{
	m_lut = lut;
	m_colorParamIdx = paramIndex;
	updateColorSelectionRendering();
}

void iA3DLineObjectVis::setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive)
{
	m_selection = sortedSelInds;
	m_selectionActive = selectionActive;
	updateColorSelectionRendering();
}

void iA3DLineObjectVis::updateColorSelectionRendering()
{
	size_t curSelIdx = 0;
	for (size_t objID = 0; objID < m_objectTable->GetNumberOfRows(); ++objID)
	{
		QColor color = m_baseColor;
		if (m_lut)
		{
			double curValue = m_objectTable->GetValue(objID, m_colorParamIdx).ToDouble();
			color = m_lut->getQColor(curValue);
		}
		if (m_selectionActive)
		{
			if (curSelIdx < m_selection.size() && objID == m_selection[curSelIdx])
			{
				color.setAlpha(m_selectionAlpha);
				++curSelIdx;
			}
			else
				color.setAlpha(m_contextAlpha);
		}
		else
			color.setAlpha(m_selectionAlpha);
		setPolyPointColor(objID, color);
	}
	updatePolyMapper();
}
