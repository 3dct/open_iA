// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACameraVis.h"

#include <iALog.h>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCylinderSource.h>
#include <vtkConeSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

iACameraVis::iACameraVis(vtkRenderer* ren, double size) :
	m_ren(ren),
	m_camPosActor(vtkSmartPointer<vtkActor>::New()),
	m_camDirActor(vtkSmartPointer<vtkActor>::New()),
	m_camUpActor(vtkSmartPointer<vtkActor>::New()),
	m_camPosSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_camDirSource(vtkSmartPointer<vtkConeSource>::New()),
	m_camUpSource(vtkSmartPointer<vtkCylinderSource>::New()),
	m_size(size),
	m_visible(false)
{
	vtkNew<vtkPolyDataMapper> camPosMapper;
	m_camPosSource->SetRadius(size);
	camPosMapper->SetInputConnection(m_camPosSource->GetOutputPort());
	m_camPosActor->SetMapper(camPosMapper);
	m_camPosActor->GetProperty()->SetOpacity(1.0);
	m_camPosActor->GetProperty()->SetColor(0, 128, 0);

	vtkNew<vtkPolyDataMapper> camDirMapper;
	m_camDirSource->SetHeight(1 * size);
	m_camDirSource->SetRadius(0.5 * size);
	m_camDirSource->SetResolution(12);
	camDirMapper->SetInputConnection(m_camDirSource->GetOutputPort());
	m_camDirActor->SetMapper(camDirMapper);
	m_camDirActor->GetProperty()->SetOpacity(1.0);
	m_camDirActor->GetProperty()->SetColor(32, 96, 0);
	m_camDirActor->GetProperty()->SetLineWidth(5);

	vtkNew<vtkPolyDataMapper> camUpMapper;
	m_camUpSource->SetHeight(1 * size);
	m_camUpSource->SetRadius(0.5 * size);
	m_camUpSource->SetResolution(12);
	camUpMapper->SetInputConnection(m_camUpSource->GetOutputPort());
	m_camUpActor->SetMapper(camUpMapper);
	m_camUpActor->GetProperty()->SetOpacity(1.0);
	m_camUpActor->GetProperty()->SetColor(32, 96, 0);
	m_camUpActor->GetProperty()->SetLineWidth(5);
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
	m_ren->AddActor(m_camPosActor);
	m_ren->AddActor(m_camDirActor);
	m_ren->AddActor(m_camUpActor);
	m_visible = true;
}

void iACameraVis::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_ren->RemoveActor(m_camPosActor);
	m_ren->RemoveActor(m_camDirActor);
	m_ren->RemoveActor(m_camUpActor);
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
	m_camPosSource->SetCenter(m_pos.data());

	iAVec3d defaultDir(0, 1, 0);
	
	// compute rotations as proposed here: https://vtk.org/pipermail/vtkusers/2000-May/000942.html
	auto dirTheta = vtkMath::DegreesFromRadians(angleBetween(defaultDir, m_dir));
	auto dirAxis = crossProduct(defaultDir, m_dir);
	m_camDirActor->RotateWXYZ(dirTheta, dirAxis[0], dirAxis[1], dirAxis[2]);
	iAVec3d dirVecPos(m_pos + m_dir * m_size * 1.5); // 1.35 because pos specifies center of cone, and cone is 0.8 size high
	LOG(lvlInfo, QString("Dir pos: %1; rot (axis=%2, theta=%3").arg(dirVecPos.toString()).arg(dirAxis.toString()).arg(dirTheta));
	m_camDirActor->SetPosition(dirVecPos.data());     // probably we could use m_camDirSource->SetCenter instead

	auto upTheta = vtkMath::DegreesFromRadians(angleBetween(defaultDir, m_up));
	auto upAxis = crossProduct(defaultDir, m_up);
	m_camDirActor->RotateWXYZ(upTheta, upAxis[0], upAxis[1], upAxis[2]);
	iAVec3d upVecPos(m_pos + m_up * m_size * 1.5); // 1.35 because pos specifies center of cone, and cone is 0.8 size high
	LOG(lvlInfo, QString("Up pos: %1; rot (axis=%2, theta=%3").arg(upVecPos.toString()).arg(upAxis.toString()).arg(upTheta));
	m_camUpActor->SetPosition(upVecPos.data());     // probably we could use m_camDirSource->SetCenter instead
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
