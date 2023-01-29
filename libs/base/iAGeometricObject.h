// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iADataSet.h"

#include <vtkSmartPointer.h>

class vtkPolyDataAlgorithm;

//! a geometric object produced by some VTK algorithm such as the various sources
class iAbase_API iAGeometricObject : public iADataSet
{
public:
	iAGeometricObject(QString const& name, vtkSmartPointer<vtkPolyDataAlgorithm> source);
	vtkSmartPointer<vtkPolyDataAlgorithm> source() const;
	QString info() const override;
	std::array<double, 3> unitDistance() const override;
	double const* bounds() const;

private:
	iAGeometricObject(iAGeometricObject const& other) = delete;
	iAGeometricObject& operator=(iAGeometricObject const& other) = delete;
	vtkSmartPointer<vtkPolyDataAlgorithm> m_polySource;
};
