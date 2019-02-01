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
#include "iA3DCylinderObjectVis.h"

#include "iACsvConfig.h"

#include <vtkDoubleArray.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkTable.h>
#include <vtkTubeFilter.h>


iA3DCylinderObjectVis::iA3DCylinderObjectVis( iAVtkWidget* widget, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color, int numberOfCylinderSides ):
	iA3DLineObjectVis( widget, objectTable, columnMapping, color )
{
	auto tubeRadius = vtkSmartPointer<vtkDoubleArray>::New();
	tubeRadius->SetName("TubeRadius");
	tubeRadius->SetNumberOfTuples(objectTable->GetNumberOfRows()*2);
	for (vtkIdType row = 0; row < objectTable->GetNumberOfRows(); ++row)
	{
		double diameter = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::Diameter)).ToDouble();
		tubeRadius->SetTuple1(row*2,   diameter/2);
		tubeRadius->SetTuple1(row*2+1, diameter/2);
	}
	m_linePolyData->GetPointData()->AddArray(tubeRadius);
	m_linePolyData->GetPointData()->SetActiveScalars("TubeRadius");
	m_tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
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
	// NOTE: this requires modifying the vtkTubeFilter.cxx:
	/*
		else if ( inScalars &&
			this->VaryRadius == VTK_VARY_RADIUS_BY_ABSOLUTE_SCALAR )
		{
			- sFactor = inScalars->GetComponent(pts[j],0);
			+ sFactor = inScalars->GetComponent(pts[j],0) * this->RadiusFactor;
	*/
	m_tubeFilter->SetRadiusFactor(diameterFactor);
	m_tubeFilter->Modified();
	m_tubeFilter->Update();
	updateRenderer();
}

void iA3DCylinderObjectVis::setContextDiameterFactor(double contextDiameterFactor)
{
}
