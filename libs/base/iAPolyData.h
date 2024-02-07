// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "iADataSet.h"

#include <vtkSmartPointer.h>

class vtkPolyData;

//! a class for vtk polydata mesh datasets
class iAbase_API iAPolyData : public iADataSet
{
public:
	iAPolyData(vtkSmartPointer<vtkPolyData> mesh);
	vtkSmartPointer<vtkPolyData> poly() const;
	QString info() const override;
	std::array<double, 3> unitDistance() const override;

private:
	iAPolyData(iAPolyData const& other) = delete;
	iAPolyData& operator=(iAPolyData const& other) = delete;
	vtkSmartPointer<vtkPolyData> m_mesh;
};

//! a graph dataset
//! merge with iAPolyData ?
class iAbase_API iAGraphData : public iADataSet
{
public:
	iAGraphData(vtkSmartPointer<vtkPolyData> mesh,
		QStringList const& vertexValueNames,
		QStringList const& edgeValueNames);
	vtkSmartPointer<vtkPolyData> poly() const;
	QString info() const override;
	//std::array<double, 3> unitDistance() const override;
	QStringList const& vertexValueNames() const;
	QStringList const& edgeValueNames() const;

private:
	iAGraphData(iAGraphData const& other) = delete;
	iAGraphData& operator=(iAGraphData const& other) = delete;

	vtkSmartPointer<vtkPolyData> m_mesh;
	QStringList m_vertexValueNames, m_edgeValueNames;
};
