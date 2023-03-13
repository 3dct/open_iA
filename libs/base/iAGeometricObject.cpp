// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGeometricObject.h"

#include <vtkPolyDataAlgorithm.h>

#include <array>

iAGeometricObject::iAGeometricObject(QString const& name, vtkSmartPointer<vtkPolyDataAlgorithm> source) :
	iADataSet(iADataSetType::GeometricObject),
	m_polySource(source)
{
	setMetaData(NameKey, name);
}

vtkSmartPointer<vtkPolyDataAlgorithm> iAGeometricObject::source() const
{
	return m_polySource;
}

QString iAGeometricObject::info() const
{
	return QString("Geometric object: %1; %2")
		.arg(name())
		.arg(boundsStr(bounds()));
}

double const* iAGeometricObject::bounds() const
{
	auto out = m_polySource->GetOutput();
	static double bds[6];
	out->GetBounds(bds);
	return bds;
}

std::array<double, 3> iAGeometricObject::unitDistance() const
{
	m_polySource->Update();
	const double Divisor = 100;
	return {
		(bounds()[1] - bounds()[0]) / Divisor,
		(bounds()[3] - bounds()[2]) / Divisor,
		(bounds()[5] - bounds()[4]) / Divisor
	};

}
