// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACylinderObjectVis.h"
#include "iAvtkTubeFilter.h"

#include "iACsvConfig.h"
#include "iAObjectsData.h"

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkTable.h>


iACylinderObjectVis::iACylinderObjectVis(iAObjectsData const* data,
	QColor const & color, int numberOfCylinderSides, size_t segmentSkip):
	iALineObjectVis(data, color, segmentSkip),
	m_tubeFilter(vtkSmartPointer<iAvtkTubeFilter>::New()),
	m_contextFactors(nullptr),
	m_objectCount(data->m_table->GetNumberOfRows()),
	m_contextDiameterFactor(1.0)
{
	auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
	tubeRadius->SetName("TubeRadius");
	tubeRadius->SetNumberOfTuples(m_points->GetNumberOfPoints());
	for (vtkIdType row = 0; row < data->m_table->GetNumberOfRows(); ++row)
	{
		double diameter = data->m_table->GetValue(row, m_data->m_colMapping->value(iACsvConfig::Diameter)).ToDouble();
		for (int p = 0; p < objectPointCount(row); ++p)
			tubeRadius->SetTuple1(objectStartPointIdx(row)+p, diameter/2);
	}
	m_linePolyData->GetPointData()->AddArray(tubeRadius);
	m_linePolyData->GetPointData()->SetActiveScalars("TubeRadius");
	m_tubeFilter->SetRadiusFactor(1.0);
	m_tubeFilter->SetInputData(m_linePolyData);
	m_tubeFilter->CappingOn();
	m_tubeFilter->SidesShareVerticesOff();
	m_tubeFilter->SetNumberOfSides(numberOfCylinderSides);
	m_tubeFilter->SetVaryRadiusToVaryRadiusByAbsoluteScalar();
	m_tubeFilter->Update();
	// add final point number
	m_finalObjectPointMap = m_tubeFilter->GetFinalObjectPointMap();
}

iACylinderObjectVis::~iACylinderObjectVis()
{
	delete [] m_contextFactors;
}

void iACylinderObjectVis::setDiameterFactor(double diameterFactor)
{
	m_tubeFilter->SetRadiusFactor(diameterFactor);
	m_tubeFilter->Modified();
	m_tubeFilter->Update();
	emit renderRequired();
}

void iACylinderObjectVis::setContextDiameterFactor(double contextDiameterFactor)
{
	if (contextDiameterFactor == 1.0)
	{
		if (m_contextFactors)
		{
			delete m_contextFactors;
			m_contextFactors = nullptr;
		}
		else
		{
			return;
		}
	}
	else
	{
		if (!m_contextFactors)
		{
			m_contextFactors = new float[m_points->GetNumberOfPoints()];
		}
		m_contextDiameterFactor = contextDiameterFactor;
		size_t selIdx = 0;
		for (vtkIdType row = 0; row < m_objectCount; ++row)
		{
			bool isSelected = selIdx < m_selection.size() && (m_selection[selIdx] == static_cast<size_t>(row));
			if (isSelected)
			{
				++selIdx;
			}
			float diameter = (!isSelected) ? m_contextDiameterFactor : 1.0;
			for (int p = 0; p < objectPointCount(row); ++p)
			{
				m_contextFactors[objectStartPointIdx(row) + p] = diameter;
			}
		}
	}
	m_tubeFilter->SetIndividualFactors(m_contextFactors);
	m_tubeFilter->Modified();
	m_tubeFilter->Update();
	emit renderRequired();
}

void iACylinderObjectVis::setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive)
{
	iAColoredPolyObjectVis::setSelection(sortedSelInds, selectionActive);
	setContextDiameterFactor(m_contextDiameterFactor);
}

QString iACylinderObjectVis::visualizationStatistics() const
{
	return iALineObjectVis::visualizationStatistics() + "; # cylinder sides: " +
		QString::number(m_tubeFilter->GetNumberOfSides());
}

vtkPolyData* iACylinderObjectVis::finalPolyData()
{
	//m_tubeFilter->Update();
	return m_tubeFilter->GetOutput();
}

iAColoredPolyObjectVis::IndexType iACylinderObjectVis::finalObjectStartPointIdx(IndexType objIdx) const
{
	return m_finalObjectPointMap[objIdx].first;
}

iAColoredPolyObjectVis::IndexType iACylinderObjectVis::finalObjectPointCount(IndexType objIdx) const
{
	return m_finalObjectPointMap[objIdx].second;
}

/*
vtkAlgorithmOutput* iACylinderObjectVis::output()
{
	return m_tubeFilter->GetOutputPort();
}
*/

std::vector<vtkSmartPointer<vtkPolyData>> iACylinderObjectVis::extractSelectedObjects(QColor color) const
{
	std::vector<vtkSmartPointer<vtkPolyData>> result;
	for (auto selIdx: m_selection)
	{
		vtkNew<vtkTable> tmpTbl;
		tmpTbl->Initialize();
		for (int c = 0; c<m_data->m_table->GetNumberOfColumns(); ++c)
		{
			vtkSmartPointer<vtkFloatArray> arrC = vtkSmartPointer<vtkFloatArray>::New();
			arrC->SetName(m_data->m_table->GetColumnName(c));
			tmpTbl->AddColumn(arrC);
		}
		tmpTbl->SetNumberOfRows(1);
		// TODO: use labelID everywhere to identify object!
		//int labelID = m_data->m_table->GetValue(selIdx, 0).ToInt() - 1;
		//for the moment: assert(labelID == selIdx);
		size_t labelID = selIdx;
		for (int c = 1; c < m_data->m_table->GetNumberOfColumns(); ++c)
		{
			tmpTbl->SetValue(0, c, m_data->m_table->GetValue(selIdx, c));
		}
		const int ExtractedID = 1;
		tmpTbl->SetValue(0, 0, ExtractedID);
		std::map<size_t, std::vector<iAVec3f>> tmpCurvedFiberData;
		auto tmpData = std::make_shared<iAObjectsData>(iAObjectVisType::Cylinder, tmpTbl.GetPointer(), m_data->m_colMapping);
		auto it = m_data->m_curvedFiberData.find(labelID);
		if (it != m_data->m_curvedFiberData.end())
		{	// Note: curved fiber data currently does not use LabelID, but starts from 0!
			tmpData->m_curvedFiberData.insert(std::make_pair(ExtractedID-1, it->second));
		}
		iACylinderObjectVis tmpVis(tmpData.get(), color.isValid() ? color : QColor(0, 0, 0));
		auto pd = tmpVis.finalPolyData();
		result.push_back(pd);
	}
	return result;
}
