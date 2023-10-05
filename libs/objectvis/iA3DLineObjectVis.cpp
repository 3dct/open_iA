// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA3DLineObjectVis.h"

#include "iACsvConfig.h"

#include <iALog.h>

#include <vtkLine.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyLine.h>
#include <vtkTable.h>

iA3DLineObjectVis::iA3DLineObjectVis(std::shared_ptr<iA3DObjectsData> data, QColor const& color,
	std::map<size_t, std::vector<iAVec3f> > const & curvedFiberData, size_t segmentSkip):
	iA3DColoredPolyObjectVis(data, color),
	m_linePolyData(vtkSmartPointer<vtkPolyData>::New()),
	m_points(vtkSmartPointer<vtkPoints>::New()),
	m_curvedFiberData(curvedFiberData),
	m_totalNumOfSegments(0)
{
	auto lines = vtkSmartPointer<vtkCellArray>::New();
	for (vtkIdType row = 0; row < m_data->m_table->GetNumberOfRows(); ++row)
	{
		//int labelID = m_data->m_table->GetValue(row, 0).ToInt();
		auto it = curvedFiberData.find(row);
		IndexType numberOfPts;
		IndexType totalNumOfPtsBefore = m_points->GetNumberOfPoints();
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
				first[i] = m_data->m_table->GetValue(row, m_data->m_colMapping->value(iACsvConfig::StartX + i)).ToFloat();
				end[i] = m_data->m_table->GetValue(row, m_data->m_colMapping->value(iACsvConfig::EndX + i)).ToFloat();
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
	setupOriginalIds();
}

void iA3DLineObjectVis::updateValues(std::vector<std::vector<double> > const & values, int straightOrCurved)
{
	if (2*values.size()+1 >= static_cast<size_t>(std::numeric_limits<vtkIdType>::max()))
	{
		LOG(lvlError, QString("More values (current number: %1) than VTK can handle (limit: %2)")
			.arg(2 * values.size() + 1)
			.arg(std::numeric_limits<vtkIdType>::max()));
	}
	for (size_t f = 0; f < values.size(); ++f)
	{
		// "magic numbers" 1 and 2 need to match values in FIAKER - iAFiberResult::StepDataType:
		if (straightOrCurved == 1) // SimpleStepData
		{
			m_points->SetPoint(static_cast<vtkIdType>(2 * f), values[f].data());
			m_points->SetPoint(static_cast<vtkIdType>(2 * f + 1), values[f].data() + 3);
		}
		else if (straightOrCurved == 2) // CurvedStepData
		{
			if (f == 0 && static_cast<IndexType>(values[f].size()) / 3 != m_objectPointMap[f].second)
			{
				LOG(lvlWarn, QString("For fiber %1, number of points given "
					"doesn't match number of existing points; expected %2, got %3. "
					"The visualization will probably contain errors as some old points "
					"might be continued to show, or some new points might not be shown!")
					.arg(f)
					.arg(m_objectPointMap[f].second)
					.arg(values[f].size()/3));
			}
			IndexType pointCount = std::min( static_cast<IndexType>(values[f].size() / 3), m_objectPointMap[f].second);
			for (int p = 0; p < pointCount; ++p)
			{
				m_points->SetPoint(m_objectPointMap[f].first + p, values[f].data() + p * 3);
			}
		}
		else
		{
			LOG(lvlError, QString("Invalid straightOrCurved value (%1) in updateValues, expected 1 or 2").arg(straightOrCurved));
		}
	}
	m_points->Modified();
	emit dataChanged();
}

vtkPolyData* iA3DLineObjectVis::polyData()
{
	return m_linePolyData;
}

vtkPolyData* iA3DLineObjectVis::finalPolyData()
{
	return m_linePolyData;
}

QString iA3DLineObjectVis::visualizationStatistics() const
{
	return QString("# lines: %1; # line segments: %2; # points: %3")
		.arg(m_linePolyData->GetNumberOfCells()).arg(m_totalNumOfSegments).arg(m_points->GetNumberOfPoints());
}

iA3DColoredPolyObjectVis::IndexType iA3DLineObjectVis::objectStartPointIdx(IndexType objIdx) const
{
	return m_objectPointMap[objIdx].first;
}

iA3DColoredPolyObjectVis::IndexType iA3DLineObjectVis::objectPointCount(IndexType objIdx) const
{
	return m_objectPointMap[objIdx].second;
}

std::vector<vtkSmartPointer<vtkPolyData>> iA3DLineObjectVis::extractSelectedObjects(QColor c) const
{
	Q_UNUSED(c);
	std::vector<vtkSmartPointer<vtkPolyData>> result;
	return result;
}
