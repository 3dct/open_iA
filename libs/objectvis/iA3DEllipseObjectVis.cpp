// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iA3DEllipseObjectVis.h"

#include "iACsvConfig.h"

#include "vtkEllipsoidSource.h"

#include <vtkAppendPolyData.h>
#include <vtkPolyData.h>
#include <vtkPointData.h>
#include <vtkTable.h>

iA3DEllipseObjectVis::iA3DEllipseObjectVis(std::shared_ptr<iA3DObjectsData> data,
	QColor const & color, int phiRes, int thetaRes) :
	iA3DColoredPolyObjectVis(data, color),
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
	assert ( objectPointCount(0)*objectTable->GetNumberOfRows() == fullPolySource->GetOutput()->GetNumberOfPoints() );
	setupOriginalIds();
}

double const * iA3DEllipseObjectVis::bounds()
{
	return m_fullPoly->GetBounds();
}

vtkPolyData* iA3DEllipseObjectVis::polyData()
{
	return m_fullPoly;
}

vtkPolyData* iA3DEllipseObjectVis::finalPolyData()
{
	return m_fullPoly;
}

QString iA3DEllipseObjectVis::visualizationStatistics() const
{
	return QString("Number of cells: %1; Number of points: %3")
		.arg(m_fullPoly->GetNumberOfCells())
		.arg(m_fullPoly->GetNumberOfPoints());
}

iA3DColoredPolyObjectVis::IndexType iA3DEllipseObjectVis::objectStartPointIdx(IndexType objIdx) const
{
	return objIdx * m_pointsPerEllipse;
}

iA3DColoredPolyObjectVis::IndexType iA3DEllipseObjectVis::objectPointCount(IndexType /*objIdx*/) const
{
	return m_pointsPerEllipse;
}

std::vector<vtkSmartPointer<vtkPolyData>> iA3DEllipseObjectVis::extractSelectedObjects(QColor c) const
{
	Q_UNUSED(c);
	std::vector<vtkSmartPointer<vtkPolyData>> result;
	return result;
}
