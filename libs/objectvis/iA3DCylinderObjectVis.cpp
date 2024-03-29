// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA3DCylinderObjectVis.h"
#include "iAvtkTubeFilter.h"

#include "iACsvConfig.h"

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkNew.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkTable.h>


iA3DCylinderObjectVis::iA3DCylinderObjectVis(vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color, std::map<size_t, std::vector<iAVec3f> > const & curvedFiberData, int numberOfCylinderSides, size_t segmentSkip):
	iA3DLineObjectVis(objectTable, columnMapping, color, curvedFiberData, segmentSkip),
	m_tubeFilter(vtkSmartPointer<iAvtkTubeFilter>::New()),
	m_contextFactors(nullptr),
	m_objectCount(objectTable->GetNumberOfRows()),
	m_contextDiameterFactor(1.0)
{
	auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
	tubeRadius->SetName("TubeRadius");
	tubeRadius->SetNumberOfTuples(m_points->GetNumberOfPoints());
	for (vtkIdType row = 0; row < objectTable->GetNumberOfRows(); ++row)
	{
		double diameter = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::Diameter)).ToDouble();
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

iA3DCylinderObjectVis::~iA3DCylinderObjectVis()
{
	delete [] m_contextFactors;
}

void iA3DCylinderObjectVis::setDiameterFactor(double diameterFactor)
{
	m_tubeFilter->SetRadiusFactor(diameterFactor);
	m_tubeFilter->Modified();
	m_tubeFilter->Update();
	emit renderRequired();
}

void iA3DCylinderObjectVis::setContextDiameterFactor(double contextDiameterFactor)
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

void iA3DCylinderObjectVis::setSelection(std::vector<size_t> const & sortedSelInds, bool selectionActive)
{
	iA3DColoredPolyObjectVis::setSelection(sortedSelInds, selectionActive);
	setContextDiameterFactor(m_contextDiameterFactor);
}

QString iA3DCylinderObjectVis::visualizationStatistics() const
{
	return iA3DLineObjectVis::visualizationStatistics() + "; # cylinder sides: " +
		QString::number(m_tubeFilter->GetNumberOfSides());
}

vtkPolyData* iA3DCylinderObjectVis::finalPolyData()
{
	//m_tubeFilter->Update();
	return m_tubeFilter->GetOutput();
}

iA3DColoredPolyObjectVis::IndexType iA3DCylinderObjectVis::finalObjectStartPointIdx(IndexType objIdx) const
{
	return m_finalObjectPointMap[objIdx].first;
}

iA3DColoredPolyObjectVis::IndexType iA3DCylinderObjectVis::finalObjectPointCount(IndexType objIdx) const
{
	return m_finalObjectPointMap[objIdx].second;
}

/*
vtkAlgorithmOutput* iA3DCylinderObjectVis::output()
{
	return m_tubeFilter->GetOutputPort();
}
*/

std::vector<vtkSmartPointer<vtkPolyData>> iA3DCylinderObjectVis::extractSelectedObjects(QColor color) const
{
	std::vector<vtkSmartPointer<vtkPolyData>> result;
	for (auto selIdx: m_selection)
	{
		vtkNew<vtkTable> tmpTbl;
		tmpTbl->Initialize();
		for (int c = 0; c<m_objectTable->GetNumberOfColumns(); ++c)
		{
			vtkSmartPointer<vtkFloatArray> arrC = vtkSmartPointer<vtkFloatArray>::New();
			arrC->SetName(m_objectTable->GetColumnName(c));
			tmpTbl->AddColumn(arrC);
		}
		tmpTbl->SetNumberOfRows(1);
		// TODO: use labelID everywhere to identify object!
		//int labelID = m_objectTable->GetValue(selIdx, 0).ToInt() - 1;
		//for the moment: assert(labelID == selIdx);
		size_t labelID = selIdx;
		for (int c = 1; c < m_objectTable->GetNumberOfColumns(); ++c)
		{
			tmpTbl->SetValue(0, c, m_objectTable->GetValue(selIdx, c));
		}
		const int ExtractedID = 1;
		tmpTbl->SetValue(0, 0, ExtractedID);
		std::map<size_t, std::vector<iAVec3f>> tmpCurvedFiberData;
		auto it = m_curvedFiberData.find(labelID);
		if (it != m_curvedFiberData.end())
		{	// Note: curved fiber data currently does not use LabelID, but starts from 0!
			tmpCurvedFiberData.insert(std::make_pair(ExtractedID-1, it->second));
		}
		iA3DCylinderObjectVis tmpVis(
			tmpTbl.GetPointer(), m_columnMapping, color.isValid() ? color : QColor(0, 0, 0), tmpCurvedFiberData);
		auto pd = tmpVis.finalPolyData();
		result.push_back(pd);
	}
	return result;
}
