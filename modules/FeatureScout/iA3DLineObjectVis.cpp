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

#include <vtkActor.h>
#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkTable.h>

iA3DLineObjectVis::iA3DLineObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping, QColor const & color,
	std::map<size_t, std::vector<iAVec3f> > const & curvedFiberData, size_t segmentSkip):
	iA3DColoredPolyObjectVis(ren, objectTable, columnMapping, color),
	m_points(vtkSmartPointer<vtkPoints>::New()),
	m_linePolyData(vtkSmartPointer<vtkPolyData>::New()),
	m_totalNumOfSegments(0)
{
	auto lines = vtkSmartPointer<vtkCellArray>::New();
	for (vtkIdType row = 0; row < m_objectTable->GetNumberOfRows(); ++row)
	{
		auto it = curvedFiberData.find(row);
		size_t numberOfPts;
		size_t totalNumOfPtsBefore = static_cast<size_t>(m_points->GetNumberOfPoints());
		if (it != curvedFiberData.end())
		{
			auto line = vtkSmartPointer<vtkPolyLine>::New();
			size_t availNumOfSegs = it->second.size();
			numberOfPts = (availNumOfSegs-1)/segmentSkip + 1 +
					(( (availNumOfSegs-1) % segmentSkip != 0)?1:0);
			line->GetPointIds()->SetNumberOfIds(numberOfPts);
			size_t i;
			vtkIdType curLineSeg = 0;
			for (i = 0; i < availNumOfSegs - 1; i += segmentSkip)
			{
				m_points->InsertNextPoint(it->second[i].data());
				line->GetPointIds()->SetId(curLineSeg++, m_points->GetNumberOfPoints() - 1);
				++m_totalNumOfSegments;
			}
			// make sure last point of fiber is inserted:
			i = it->second.size() - 1;
			assert(numberOfPts == curLineSeg + 1);
			m_points->InsertNextPoint(it->second[i].data());
			line->GetPointIds()->SetId(curLineSeg, m_points->GetNumberOfPoints() - 1);
			lines->InsertNextCell(line);
		}
		else
		{
			numberOfPts = 2;
			float first[3], end[3];
			for (int i = 0; i < 3; ++i)
			{
				first[i] = m_objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::StartX + i)).ToFloat();
				end[i] = m_objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::EndX + i)).ToFloat();
			}
			m_points->InsertNextPoint(first);
			m_points->InsertNextPoint(end);
			auto line = vtkSmartPointer<vtkLine>::New();
			line->GetPointIds()->SetId(0, m_points->GetNumberOfPoints()-2);
			line->GetPointIds()->SetId(1, m_points->GetNumberOfPoints()-1);
			lines->InsertNextCell(line);
			++m_totalNumOfSegments;
		}
		m_objectPointMap.push_back(std::make_pair(totalNumOfPtsBefore, numberOfPts));
	}
	m_linePolyData->SetPoints(m_points);
	m_linePolyData->SetLines(lines);
	setupColors();
	m_linePolyData->GetPointData()->AddArray(m_colors);
	setupBoundingBox();
	setupOriginalIds();

	m_mapper->SetInputData(m_linePolyData);
	m_actor->SetMapper(m_mapper);
}

void iA3DLineObjectVis::updateValues(std::vector<std::vector<double> > const & values, int straightOrCurved)
{
	for (int f = 0; f < values.size(); ++f)
	{
		// "magic numbers" 1 and 2 need to match values in FIAKER - iAFiberCharData::StepDataType:
		if (straightOrCurved == 1) // SimpleStepData
		{
			m_points->SetPoint(2 * f, values[f].data());
			m_points->SetPoint(2 * f + 1, values[f].data() + 3);
		}
		else if (straightOrCurved == 2) // CurvedStepData
		{
			if (f == 0 && (values[f].size()) / 3 != m_objectPointMap[f].second)
			{
				DEBUG_LOG(QString("For fiber %1, number of points given "
					"doesn't match number of existing points; expected %2, got %3. "
					"The visualization will probably contain errors as some old points "
					"might be continued to show, or some new points might not be shown!")
					.arg(f)
					.arg(m_objectPointMap[f].second)
					.arg(values[f].size()/3));
			}
			int pointCount = std::min( (values[f].size()) / 3, m_objectPointMap[f].second);
			for (int p = 0; p < pointCount; ++p)
			{
				m_points->SetPoint(m_objectPointMap[f].first + p, values[f].data() + p * 3);
			}
		}
		else
		{
			DEBUG_LOG(QString("Invalid straightOrCurved value (%1) in updateValues, expected 1 or 2").arg(straightOrCurved));
		}
	}
	m_points->Modified();
	updatePolyMapper();
}

vtkPolyData* iA3DLineObjectVis::getPolyData()
{
	return m_linePolyData;
}


QString iA3DLineObjectVis::visualizationStatistics() const
{
	return QString("# lines: %1; # line segments: %2; # points: %3")
		.arg(m_linePolyData->GetNumberOfCells()).arg(m_totalNumOfSegments).arg(m_points->GetNumberOfPoints());
}

int iA3DLineObjectVis::objectStartPointIdx(int objIdx) const
{
	return m_objectPointMap[objIdx].first;
}

int iA3DLineObjectVis::objectPointCount(int objIdx) const
{
	return m_objectPointMap[objIdx].second;
}
