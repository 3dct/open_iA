// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAGeometricObject.h>

#include <vtkSmartPointer.h>

class vtkCubeSource;
class vtkLineSource;
class vtkSphereSource;

class iACubeObject : public iAGeometricObjectSource
{
public:
	iACubeObject();
	void applyAttributes(QVariantMap const& values) override;
	iAAttributes objectProperties();
private:
	vtkSmartPointer<vtkCubeSource> m_cubeSource;
};

class iALineObject : public iAGeometricObjectSource
{
public:
	iALineObject();
	void applyAttributes(QVariantMap const& values) override;
	iAAttributes objectProperties();
private:
	vtkSmartPointer<vtkLineSource> m_lineSource;
};

class iASphereObject: public iAGeometricObjectSource
{
public:
	iASphereObject();
	void applyAttributes(QVariantMap const& values) override;
	iAAttributes objectProperties();
private:
	vtkSmartPointer<vtkSphereSource> m_sphereSource;
};

std::unique_ptr<iAGeometricObjectSource> createSource(QString const& name);
