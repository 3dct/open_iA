// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGeometricObject.h"

#include <vtkPolyDataAlgorithm.h>

#include <array>

iAGeometricObjectSource::~iAGeometricObjectSource() = default;

iAGeometricObject::iAGeometricObject(QString const& name, std::unique_ptr<iAGeometricObjectSource> source) :
	iADataSet(iADataSetType::GeometricObject),
	m_source(std::move(source))
{
	setMetaData(NameKey, name);
}

vtkPolyDataAlgorithm* iAGeometricObject::source() const
{
	return m_source->vtkSource;
}

QString iAGeometricObject::info() const
{
	return QString("Geometric object: %1; %2")
		.arg(name())
		.arg(boundsStr(bounds()));
}

std::array<double, 3> iAGeometricObject::unitDistance() const
{
	m_source->vtkSource->Update();
	const double Divisor = 100;
	return {
		(bounds()[1] - bounds()[0]) / Divisor,
		(bounds()[3] - bounds()[2]) / Divisor,
		(bounds()[5] - bounds()[4]) / Divisor
	};
}

double const* iAGeometricObject::bounds() const
{
	auto out = m_source->vtkSource->GetOutput();
	static double bds[6];
	out->GetBounds(bds);
	return bds;
}

void iAGeometricObject::applyAttributes(QVariantMap const& values)
{
	m_source->applyAttributes(values);
}

iAAttributes iAGeometricObject::objectProperties()
{
	return m_source->objectProperties();
}
