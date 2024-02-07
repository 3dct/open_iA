// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAEllipsoidObjectVis.h"

#include "iACsvConfig.h"
#include "iAObjectsData.h"
#include "vtkEllipsoidSource.h"

#include <vtkAppendPolyData.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkTable.h>

iAEllipsoidObjectVis::iAEllipsoidObjectVis(iAObjectsData const* data,
	QColor const & color, int phiRes, int thetaRes) :
	iAColoredPolyObjectVis(data, color),
	m_pointsPerEllipse((phiRes - 2) * thetaRes + 2)
{
	auto fullPolySource = vtkSmartPointer<vtkAppendPolyData>::New();
	// maybe use vtkParametricFunctionSource with vtkParametricEllipsoid?
	for (vtkIdType row = 0; row < data->m_table->GetNumberOfRows(); ++row)
	{
		double cx = data->m_table->GetValue(row, data->m_colMapping->value(iACsvConfig::CenterX)).ToDouble();
		double cy = data->m_table->GetValue(row, data->m_colMapping->value(iACsvConfig::CenterY)).ToDouble();
		double cz = data->m_table->GetValue(row, data->m_colMapping->value(iACsvConfig::CenterZ)).ToDouble();
		double dx = data->m_table->GetValue(row, data->m_colMapping->value(iACsvConfig::DimensionX)).ToDouble()/2;
		double dy = data->m_table->GetValue(row, data->m_colMapping->value(iACsvConfig::DimensionY)).ToDouble()/2;
		double dz = data->m_table->GetValue(row, data->m_colMapping->value(iACsvConfig::DimensionZ)).ToDouble()/2;
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
	// TODO: color updates etc. don't work because of this "static" mapping!
	m_fullPoly = fullPolySource->GetOutput();
	setupColors();
	m_fullPoly->GetPointData()->AddArray(m_colors);
	assert ( objectPointCount(0)*m_data->m_table->GetNumberOfRows() == fullPolySource->GetOutput()->GetNumberOfPoints() );
	setupOriginalIds();
}

double const * iAEllipsoidObjectVis::bounds()
{
	return m_fullPoly->GetBounds();
}

vtkPolyData* iAEllipsoidObjectVis::polyData()
{
	return m_fullPoly;
}

vtkPolyData* iAEllipsoidObjectVis::finalPolyData()
{
	return m_fullPoly;
}

QString iAEllipsoidObjectVis::visualizationStatistics() const
{
	return QString("Number of cells: %1; Number of points: %3")
		.arg(m_fullPoly->GetNumberOfCells())
		.arg(m_fullPoly->GetNumberOfPoints());
}

iAColoredPolyObjectVis::IndexType iAEllipsoidObjectVis::objectStartPointIdx(IndexType objIdx) const
{
	return objIdx * m_pointsPerEllipse;
}

iAColoredPolyObjectVis::IndexType iAEllipsoidObjectVis::objectPointCount(IndexType /*objIdx*/) const
{
	return m_pointsPerEllipse;
}

std::vector<vtkSmartPointer<vtkPolyData>> iAEllipsoidObjectVis::extractSelectedObjects(QColor c) const
{
	Q_UNUSED(c);
	std::vector<vtkSmartPointer<vtkPolyData>> result;
	return result;
}
