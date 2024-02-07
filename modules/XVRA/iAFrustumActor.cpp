// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFrustumActor.h"


#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkLineSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSphereSource.h>

iAFrustumActor::iAFrustumActor(vtkRenderer* ren, vtkCamera* cam, double size) :
	m_ren(ren),
	m_cam(cam),
	m_size(size),
	m_camPosActor(vtkSmartPointer<vtkActor>::New()),
	m_camDirActor(vtkSmartPointer<vtkActor>::New()),
	m_camPosSource(vtkSmartPointer<vtkSphereSource>::New()),
	m_camDirSource(vtkSmartPointer<vtkLineSource>::New()),
	m_visible(false)
{
	vtkNew<vtkPolyDataMapper> camPosMapper;
	m_camPosSource->SetRadius(0.5 * size);
	camPosMapper->SetInputConnection(m_camPosSource->GetOutputPort());
	m_camPosActor->SetMapper(camPosMapper);
	m_camPosActor->GetProperty()->SetOpacity(0.2);
	m_camPosActor->GetProperty()->SetColor(0, 255, 0);

	vtkNew<vtkPolyDataMapper> camDirMapper;
	m_camDirSource->SetPoint1(100.0, 0.0, 0.0);
	m_camDirSource->SetPoint2(  0.0, 0.0, 0.0);
	camDirMapper->SetInputConnection(m_camDirSource->GetOutputPort());
	m_camDirActor->SetMapper(camDirMapper);
	m_camDirActor->GetProperty()->SetOpacity(0.2);
	m_camDirActor->GetProperty()->SetColor(0, 255, 0);
	m_camDirActor->GetProperty()->SetLineWidth(5);

	cam->AddObserver(vtkCommand::ModifiedEvent, this);
	m_lastUpdate.start();
}

void iAFrustumActor::Execute(vtkObject*, unsigned long, void*)
{
	const int UpdateIntervalMS = 250;
	if (m_lastUpdate.elapsed() < UpdateIntervalMS)
	{	// rate-limit the update to 25 fps
		return;
		// TODO: what if it's last update in a row?
	}
	iAVec3d pos;
	iAVec3d dir;
	{
		std::lock_guard l(m_mutex);
		pos = iAVec3d(m_cam->GetPosition());
		dir = iAVec3d(m_cam->GetDirectionOfProjection());
	}
	dir.normalize();
	if (pos == m_pos && dir == m_dir)
	{
		return;
	}
	m_pos = pos;
	m_dir = dir;
	m_lastUpdate.start();
	emit updateRequired();
}

void iAFrustumActor::updateSource()
{
	std::lock_guard l(m_mutex);
	iAVec3d dirVecPos(m_pos + m_dir * m_size * 5);
	m_camPosSource->SetCenter(m_pos.data());
	m_camDirSource->SetPoint1(m_pos.data());
	m_camDirSource->SetPoint2(dirVecPos.data());
}

void iAFrustumActor::show()
{
	if (m_visible)
	{
		return;
	}
	m_ren->AddActor(m_camPosActor);
	m_ren->AddActor(m_camDirActor);
	m_visible = true;
}

void iAFrustumActor::hide()
{
	if (!m_visible)
	{
		return;
	}
	m_ren->RemoveActor(m_camPosActor);
	m_ren->RemoveActor(m_camDirActor);
	m_visible = false;
}
