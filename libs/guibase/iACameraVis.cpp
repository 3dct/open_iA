// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACameraVis.h"

#include <iAMathUtility.h>    // for dblApproxEqual

#include <vtkActor.h>
#include <vtkAssembly.h>
#include <vtkCylinderSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>

#include <QColor>

iACameraVis::iACameraVis(vtkRenderer* ren, double size, QColor bodyColor, QColor vecColor) :
	m_ren(ren),
	m_assembly(vtkSmartPointer<vtkAssembly>::New()),
	m_camPosSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_camDirSource(vtkSmartPointer<vtkCylinderSource>::New()),
	m_camUpSource(vtkSmartPointer<vtkCylinderSource>::New()),
	m_size(size),
	m_visible(false)
{
	const double ResolutionFactor = 2; // increase default resolution of meshes
	const int DefaultSphereResolution = 8;
	const int DefaultCylinderResolution = 6;
	m_camPosSource->SetRadius(size);
	m_camPosSource->SetPhiResolution(static_cast<int>(ResolutionFactor * DefaultSphereResolution));
	m_camPosSource->SetThetaResolution(static_cast<int>(ResolutionFactor * DefaultSphereResolution));
	vtkNew<vtkPolyDataMapper> camPosMapper;
	camPosMapper->SetInputConnection(m_camPosSource->GetOutputPort());
	vtkNew<vtkActor> camPosActor;
	camPosActor->SetMapper(camPosMapper);
	camPosActor->GetProperty()->SetOpacity(1.0);
	camPosActor->GetProperty()->SetColor(bodyColor.redF(), bodyColor.greenF(), bodyColor.blueF());

	m_camDirSource->SetHeight(0.8 * size);
	m_camDirSource->SetRadius(0.6 * size);
	m_camDirSource->SetResolution(static_cast<int>(ResolutionFactor * DefaultCylinderResolution));
	vtkNew<vtkPolyDataMapper> camDirMapper;
	camDirMapper->SetInputConnection(m_camDirSource->GetOutputPort());
	vtkNew<vtkActor> camDirActor;
	camDirActor->SetMapper(camDirMapper);
	camDirActor->GetProperty()->SetOpacity(1.0);
	camDirActor->GetProperty()->SetColor(vecColor.redF(), vecColor.greenF(), vecColor.blueF());
	camDirActor->RotateZ(90);
	camDirActor->SetPosition(1.2 * size, 0.0, 0.0);

	m_camUpSource->SetHeight(0.25 * size);
	m_camUpSource->SetRadius(0.4 * size);
	m_camUpSource->SetResolution(static_cast<int>(ResolutionFactor * DefaultCylinderResolution));
	vtkNew<vtkPolyDataMapper> camUpMapper;
	camUpMapper->SetInputConnection(m_camUpSource->GetOutputPort());
	vtkNew<vtkActor> camUpActor;
	camUpActor->SetMapper(camUpMapper);
	camUpActor->GetProperty()->SetOpacity(1.0);
	camUpActor->GetProperty()->SetColor(vecColor.redF(), vecColor.greenF(), vecColor.blueF());
	camUpActor->SetPosition(0.0, size, 0.0);

	m_assembly->AddPart(camPosActor);
	m_assembly->AddPart(camDirActor);
	m_assembly->AddPart(camUpActor);
}

iACameraVis::~iACameraVis()
{
	hide();
}

void iACameraVis::show()
{
	if (m_visible)
	{
		return;
	}
	m_ren->AddActor(m_assembly);
	m_visible = true;
}

void iACameraVis::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_ren->RemoveActor(m_assembly);
	m_visible = false;
}

bool iACameraVis::update(iAVec3d const& pos, iAVec3d const& dir, iAVec3d const& up)
{
	if ((pos == m_pos && dir == m_dir && up == m_up) ||  // no change
		!dblApproxEqual(dotProduct(m_dir, m_up), 0.0, 0.001))  // invalid data - dir and up need to be perpendicular
	{
		return false;
	}
	m_pos = pos;
	m_dir = dir.normalized();
	m_up = up.normalized();
	emit updateRequired();
	return true;
}

void iACameraVis::updateSource()
{	
	// compute rotations as proposed here: https://vtk.org/pipermail/vtkusers/2000-May/000942.html
	vtkNew<vtkTransform> tr;
	auto perpVec = crossProduct(m_dir, m_up);
	std::array<double, 16> matrix = {
		m_dir[0], m_up[0], perpVec[0], m_pos[0],
		m_dir[1], m_up[1], perpVec[1], m_pos[1],
		m_dir[2], m_up[2], perpVec[2], m_pos[2],
		0.0, 0.0, 0.0, 1.0
	};
	tr->SetMatrix(matrix.data());
	m_assembly->SetUserTransform(tr);
}

iAVec3d iACameraVis::pos() const
{
	return m_pos;
}

iAVec3d iACameraVis::dir() const
{
	return m_dir;
}

iAVec3d iACameraVis::up() const
{
	return m_up;
}
