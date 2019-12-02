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
#include "iA3DCylinderObjectVis.h"
#include "iAvtkTubeFilter.h"

#include "iACsvConfig.h"

#include <vtkDoubleArray.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkTable.h>


iA3DCylinderObjectVis::iA3DCylinderObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color, std::map<size_t, std::vector<iAVec3f> > const & curvedFiberData, int numberOfCylinderSides, size_t segmentSkip):
	iA3DLineObjectVis( ren, objectTable, columnMapping, color, curvedFiberData, segmentSkip),
	m_objectCount(objectTable->GetNumberOfRows()),
	m_contextFactors(nullptr),
	m_contextDiameterFactor(1.0),
	m_lines(false)
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
	m_tubeFilter = vtkSmartPointer<iAvtkTubeFilter>::New();
	m_tubeFilter->SetRadiusFactor(1.0);
	m_tubeFilter->SetInputData(m_linePolyData);
	m_tubeFilter->CappingOn();
	m_tubeFilter->SidesShareVerticesOff();
	m_tubeFilter->SetNumberOfSides(numberOfCylinderSides);
	m_tubeFilter->SetVaryRadiusToVaryRadiusByAbsoluteScalar();
	m_tubeFilter->Update();
	m_mapper->SetInputConnection(m_tubeFilter->GetOutputPort());

	m_outlineFilter->SetInputConnection(m_tubeFilter->GetOutputPort());
}

void iA3DCylinderObjectVis::setDiameterFactor(double diameterFactor)
{
	m_tubeFilter->SetRadiusFactor(diameterFactor);
	m_tubeFilter->Modified();
	m_tubeFilter->Update();
	updateRenderer();
}

void iA3DCylinderObjectVis::setContextDiameterFactor(double contextDiameterFactor)
{
	if (contextDiameterFactor == 1.0)
	{
		if (m_contextFactors)
			delete m_contextFactors;
		m_contextFactors = nullptr;
	}
	else
	{
		if (!m_contextFactors)
			m_contextFactors = new float[m_points->GetNumberOfPoints()];
		m_contextDiameterFactor = contextDiameterFactor;
		size_t selIdx = 0;
		for (vtkIdType row = 0; row < m_objectCount; ++row)
		{
			bool isSelected = selIdx < m_selection.size() && (m_selection[selIdx] == row);
			if (isSelected)
				++selIdx;
			float diameter = (!isSelected) ? m_contextDiameterFactor : 1.0;
			for (int p = 0; p < objectPointCount(row); ++p)
				m_contextFactors[objectStartPointIdx(row) + p] = diameter;
		}
	}
	m_tubeFilter->SetIndividualFactors(m_contextFactors);
	m_tubeFilter->Modified();
	m_tubeFilter->Update();
	updateRenderer();
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

void iA3DCylinderObjectVis::setShowLines(bool lines)
{
	m_lines = lines;
	if (m_lines)
	{
		m_mapper->SetInputData(m_linePolyData);
	}
	else
	{
		m_mapper->SetInputConnection(m_tubeFilter->GetOutputPort());
	}
}
