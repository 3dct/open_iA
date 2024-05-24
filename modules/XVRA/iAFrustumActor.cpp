// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAFrustumActor.h"

#include "iACameraVis.h"

#include <vtkCamera.h>

iAFrustumActor::iAFrustumActor(vtkRenderer* ren, vtkCamera* cam, double size) :
	m_cam(cam),
	m_cameraVis(std::make_unique<iACameraVis>(ren, size))
{
	cam->AddObserver(vtkCommand::ModifiedEvent, this);
	m_lastUpdate.start();
	connect(m_cameraVis.get(), &iACameraVis::updateRequired, this, &iAFrustumActor::updateRequired);
}

iAFrustumActor::~iAFrustumActor() = default;

void iAFrustumActor::Execute(vtkObject*, unsigned long, void*)
{
	// rate-limit the update to 10 fps:
	const int UpdateIntervalMS = 100;
	if (m_lastUpdate.elapsed() < UpdateIntervalMS)
	{
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
	m_cameraVis->update(pos, dir);
}

void iAFrustumActor::updateSource()
{
	std::lock_guard l(m_mutex);
	m_cameraVis->updateSource();
}

void iAFrustumActor::show()
{
	m_cameraVis->show();
}

void iAFrustumActor::hide()
{
	m_cameraVis->hide();
}
