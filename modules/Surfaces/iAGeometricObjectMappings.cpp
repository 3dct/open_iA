// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAGeometricObjectMappings.h"

#include <iAValueTypeVectorHelpers.h>

#include "iAGeometricObject.h"

#include <vtkLineSource.h>
#include <vtkSphereSource.h>
#include <vtkCubeSource.h>

iALineObject::iALineObject() :
	m_lineSource(vtkSmartPointer<vtkLineSource>::New())
{
	vtkSource = m_lineSource.Get();
}

void iALineObject::applyAttributes(QVariantMap const& params)
{
	m_lineSource->SetPoint1(variantToVector<double>(params["Point 1"]).data()); // VTK problem: should take const data!
	m_lineSource->SetPoint2(variantToVector<double>(params["Point 2"]).data());
}

iAAttributes iALineObject::objectProperties()
{
	iAAttributes a;
	addAttr(a, "Point 1", iAValueType::Vector3, variantVector({ 0, 0, 0 }));
	addAttr(a, "Point 2", iAValueType::Vector3, variantVector({ 1, 1, 1 }));
	return a;
}



iASphereObject::iASphereObject() :
	m_sphereSource(vtkSmartPointer<vtkSphereSource>::New())
{
	vtkSource = m_sphereSource.Get();
}

void iASphereObject::applyAttributes(QVariantMap const& params)
{
	m_sphereSource->SetCenter(variantToVector<double>(params["Center"]).data());
	m_sphereSource->SetRadius(params["Radius"].toDouble());
	m_sphereSource->SetThetaResolution(params["Theta Resolution"].toDouble());
	m_sphereSource->SetStartTheta(params["Theta Start"].toDouble());
	m_sphereSource->SetEndTheta(params["Theta End"].toDouble());
	m_sphereSource->SetPhiResolution(params["Phi Resolution"].toDouble());
	m_sphereSource->SetStartPhi(params["Phi Start"].toDouble());
	m_sphereSource->SetEndPhi(params["Phi End"].toDouble());
	m_sphereSource->SetLatLongTessellation(params["LatLongTessellation"].toBool());
}

iAAttributes iASphereObject::objectProperties()
{
	iAAttributes a;
	addAttr(a, "Center", iAValueType::Vector3, variantVector({ 0, 0, 0 }));
	addAttr(a, "Radius", iAValueType::Continuous, 1.0);
	addAttr(a, "Theta Resolution", iAValueType::Discrete, 8, 3);
	addAttr(a, "Theta Start", iAValueType::Continuous, 0, 0, 360);
	addAttr(a, "Theta End", iAValueType::Continuous, 360, 0, 360);
	addAttr(a, "Phi Resolution", iAValueType::Continuous, 8, 3);
	addAttr(a, "Phi Start", iAValueType::Continuous, 0, 0, 180);
	addAttr(a, "Phi End", iAValueType::Continuous, 180, 0, 180);
	addAttr(a, "LatLongTessellation", iAValueType::Boolean, false);
	return a;
}



iACubeObject::iACubeObject() :
	m_cubeSource(vtkSmartPointer<vtkCubeSource>::New())
{
	vtkSource = m_cubeSource.Get();
}

void iACubeObject::applyAttributes(QVariantMap const& params)
{
	auto ptmin = variantToVector<double>(params["Point Min"]),
	     ptmax = variantToVector<double>(params["Point Max"]);
	m_cubeSource->SetBounds(ptmin[0], ptmax[0], ptmin[1], ptmax[1], ptmin[2], ptmax[2]);
}

iAAttributes iACubeObject::objectProperties()
{
	iAAttributes a;
	addAttr(a, "Point Min", iAValueType::Vector3, variantVector({ 0, 0, 0 }));
	addAttr(a, "Point Max", iAValueType::Vector3, variantVector({ 1, 1, 1 }));
	return a;
}



std::unique_ptr<iAGeometricObjectSource> createSource(QString const& name)
{
	if (name == "Cube")
	{
		return std::unique_ptr<iAGeometricObjectSource>(new iACubeObject());
	}
	else if (name == "Line")
	{
		return std::unique_ptr<iAGeometricObjectSource>(new iALineObject());
	}
	else/* if (name == "Sphere")*/
	{
		return std::unique_ptr<iAGeometricObjectSource>(new iASphereObject());
	}
}
