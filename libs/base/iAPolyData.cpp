// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAPolyData.h"

#include <vtkPolyData.h>

namespace
{
	QString meshInfo(vtkPolyData* mesh)
	{
		return QString("Points: %1; Lines: %2; Cells: %3\n")
			.arg(mesh->GetNumberOfPoints()).arg(mesh->GetNumberOfLines()).arg(mesh->GetNumberOfCells()) +
			QString("Polygons: %1; Strips: %2; Pieces: %3\n%4")
			.arg(mesh->GetNumberOfPolys()).arg(mesh->GetNumberOfStrips()).arg(mesh->GetNumberOfPieces()).arg(boundsStr(mesh->GetBounds()));
	}
}

// ---------- iAPolyData ----------

iAPolyData::iAPolyData(vtkSmartPointer<vtkPolyData> mesh) :
	iADataSet(iADataSetType::Mesh),
	m_mesh(mesh)
{
}

vtkSmartPointer<vtkPolyData> iAPolyData::poly() const
{
	return m_mesh;
}

QString iAPolyData::info() const
{
	return meshInfo(m_mesh);
}

std::array<double, 3> iAPolyData::unitDistance() const
{
	auto bounds = m_mesh->GetBounds();
	const double Divisor = 100;
	return {
		(bounds[1] - bounds[0]) / Divisor,
		(bounds[3] - bounds[2]) / Divisor,
		(bounds[5] - bounds[4]) / Divisor
	};
}

// ---------- iAGraphData ----------

iAGraphData::iAGraphData(vtkSmartPointer<vtkPolyData> mesh,
	QStringList const& vertexValueNames,
	QStringList const& edgeValueNames) :
	iADataSet(iADataSetType::Graph),
	m_mesh(mesh),
	m_vertexValueNames(vertexValueNames),
	m_edgeValueNames(edgeValueNames)
{
}

vtkSmartPointer<vtkPolyData> iAGraphData::poly() const
{
	return m_mesh;
}

QString iAGraphData::info() const
{
	return meshInfo(m_mesh);
}

QStringList const& iAGraphData::vertexValueNames() const
{
	return m_vertexValueNames;
}

QStringList const& iAGraphData::edgeValueNames() const
{
	return m_edgeValueNames;
}
/*
std::array<double, 3> iAGraphData::unitDistance() const
{
	auto bounds = m_mesh->GetBounds();
	const double Divisor = 100;
	return {
		(bounds[1] - bounds[0]) / Divisor,
		(bounds[3] - bounds[2]) / Divisor,
		(bounds[5] - bounds[4]) / Divisor
	};
}
*/