// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACameraVis.h"

#include <iAMathUtility.h>    // for dblApproxEqual

#include <vtkActor.h>
#include <vtkAssembly.h>
#include <vtkCylinderSource.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>
#include <vtkTransform.h>

namespace
{
	iAVec3d DefaultDirection(0, 1, 0);
}

iACameraVis::iACameraVis(vtkRenderer* ren, double size) :
	m_ren(ren),
	m_assembly(vtkSmartPointer<vtkAssembly>::New()),
	m_camPosSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_camDirSource(vtkSmartPointer<vtkConeSource>::New()),
	m_camUpSource(vtkSmartPointer<vtkCylinderSource>::New()),
	m_size(size),
	m_visible(false)
{
	m_camPosSource->SetRadius(size);
	vtkNew<vtkPolyDataMapper> camPosMapper;
	camPosMapper->SetInputConnection(m_camPosSource->GetOutputPort());
	vtkNew<vtkActor> camPosActor;
	camPosActor->SetMapper(camPosMapper);
	camPosActor->GetProperty()->SetOpacity(1.0);
	camPosActor->GetProperty()->SetColor(0, 128, 0);

	m_camDirSource->SetHeight(1 * size);
	m_camDirSource->SetRadius(0.75 * size);
	m_camDirSource->SetResolution(12);
	m_camDirSource->SetDirection(DefaultDirection.data());
	vtkNew<vtkPolyDataMapper> camDirMapper;
	camDirMapper->SetInputConnection(m_camDirSource->GetOutputPort());
	vtkNew<vtkActor> camDirActor;
	camDirActor->SetMapper(camDirMapper);
	camDirActor->GetProperty()->SetOpacity(1.0);
	camDirActor->GetProperty()->SetColor(32, 96, 0);
	camDirActor->SetPosition(0, 1.25 * size, 0);

	m_camUpSource->SetHeight(size);
	m_camUpSource->SetRadius(0.75 * size);
	m_camUpSource->SetResolution(12);
	vtkNew<vtkPolyDataMapper> camUpMapper;
	camUpMapper->SetInputConnection(m_camUpSource->GetOutputPort());
	vtkNew<vtkActor> camUpActor;
	camUpActor->SetMapper(camUpMapper);
	camUpActor->GetProperty()->SetOpacity(1.0);
	camUpActor->GetProperty()->SetColor(32, 96, 0);
	camUpActor->RotateX(90);
	camUpActor->SetPosition(0, 0, 1.25*size);

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
	if (pos == m_pos && dir == m_dir && up == m_up)
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
	if (!dblApproxEqual(dotProduct(m_dir, m_up), 0.0, 0.001))
	{
		return;
	}
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
