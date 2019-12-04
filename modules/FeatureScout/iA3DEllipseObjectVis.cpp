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
#include "iA3DEllipseObjectVis.h"

#include "iACsvConfig.h"

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkEllipsoidSource.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPointData.h>
#include <vtkTable.h>

iA3DEllipseObjectVis::iA3DEllipseObjectVis(vtkRenderer* ren, vtkTable* objectTable, QSharedPointer<QMap<uint, uint> > columnMapping,
	QColor const & color, int phiRes, int thetaRes) :
	iA3DColoredPolyObjectVis(ren, objectTable, columnMapping, color),
	m_pointsPerEllipse((phiRes - 2) * thetaRes + 2)
{
	auto fullPolySource = vtkSmartPointer<vtkAppendPolyData>::New();
	// maybe use vtkParametricFunctionSource with vtkParametricEllipsoid?
	for (vtkIdType row = 0; row < objectTable->GetNumberOfRows(); ++row)
	{
		double cx = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::CenterX)).ToDouble();
		double cy = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::CenterY)).ToDouble();
		double cz = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::CenterZ)).ToDouble();
		double dx = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::DimensionX)).ToDouble()/2;
		double dy = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::DimensionY)).ToDouble()/2;
		double dz = objectTable->GetValue(row, m_columnMapping->value(iACsvConfig::DimensionZ)).ToDouble()/2;
		auto ellipsoidSrc = vtkSmartPointer<vtkEllipsoidSource>::New();
		ellipsoidSrc->SetThetaResolution(thetaRes);
		ellipsoidSrc->SetPhiResolution(phiRes);
		ellipsoidSrc->SetCenter(cx, cy, cz);
		ellipsoidSrc->SetXRadius(dx);
		ellipsoidSrc->SetYRadius(dy);
		ellipsoidSrc->SetZRadius(dz);
		ellipsoidSrc->Update();
		fullPolySource->AddInputData(ellipsoidSrc->GetOutput());
	}
	fullPolySource->Update();
	m_fullPoly = fullPolySource->GetOutput();
	setupColors();
	m_fullPoly->GetPointData()->AddArray(m_colors);
	assert ( objectPointCount(0)*objectTable->GetNumberOfRows() == fullPolySource->GetOutput()->GetNumberOfPoints() );
	m_mapper->SetInputData(m_fullPoly);
	setupBoundingBox();
	setupOriginalIds();
}

double const * iA3DEllipseObjectVis::bounds()
{
	return m_fullPoly->GetBounds();
}

vtkPolyData* iA3DEllipseObjectVis::getPolyData()
{
	return m_fullPoly;
}

QString iA3DEllipseObjectVis::visualizationStatistics() const
{
	return QString("Number of cells: %1; Number of points: %3")
		.arg(m_fullPoly->GetNumberOfCells())
		.arg(m_fullPoly->GetNumberOfPoints());
}

int iA3DEllipseObjectVis::objectStartPointIdx(int objIdx) const
{
	return objIdx * m_pointsPerEllipse;
}

int iA3DEllipseObjectVis::objectPointCount(int objIdx) const
{
	return m_pointsPerEllipse;
}
