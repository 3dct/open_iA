// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAAttributes.h"
#include "iADataSet.h"

class vtkPolyDataAlgorithm;

//! Base class for different geometric objects
//! see deriving classes in Surfaces module
class iAbase_API iAGeometricObjectSource
{
public:
	virtual ~iAGeometricObjectSource();
	vtkPolyDataAlgorithm* vtkSource;
	virtual void applyAttributes(QVariantMap const& values) = 0;
	virtual iAAttributes objectProperties() = 0;
};

//! A dataset containing a geometric object produced by some VTK algorithm such as the various sources.
class iAbase_API iAGeometricObject : public iADataSet
{
public:
	iAGeometricObject(QString const& name, std::unique_ptr<iAGeometricObjectSource> source);
	vtkPolyDataAlgorithm* source() const;
	QString info() const override;
	std::array<double, 3> unitDistance() const override;
	double const* bounds() const;
	void applyAttributes(QVariantMap const& values);
	iAAttributes objectProperties();

private:
	iAGeometricObject(iAGeometricObject const& other) = delete;
	iAGeometricObject& operator=(iAGeometricObject const& other) = delete;
	std::unique_ptr<iAGeometricObjectSource> m_source;
};
