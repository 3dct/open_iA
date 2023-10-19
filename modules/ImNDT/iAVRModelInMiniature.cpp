// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAVRModelInMiniature.h"

#include <vtkMapper.h>

iAVRModelInMiniature::iAVRModelInMiniature(vtkRenderer* ren):iAVRCubicVis{ren}
{
	defaultActorSize[0] = 0.18;
	defaultActorSize[1] = 0.18;
	defaultActorSize[2] = 0.18;
}

//! Creates for every region of the octree a cube glyph. The cubes are stored in one actor.
void iAVRModelInMiniature::createCubeModel()
{
	iAVRCubicVis::createCubeModel();

	applySPDisplacement(2.9);

	m_actor->GetMapper()->ScalarVisibilityOn();
	m_actor->GetMapper()->SetScalarModeToUsePointFieldData();
	m_actor->GetMapper()->SelectColorArray("colors");

	m_actor->SetScale(defaultActorSize);
	m_actor->Modified();
}

void iAVRModelInMiniature::setScale(double x, double y, double z)
{
	m_actor->SetScale(x * defaultActorSize[0], y * defaultActorSize[1], z * defaultActorSize[2]);
	m_activeActor->SetScale(x * defaultActorSize[0], y * defaultActorSize[1], z * defaultActorSize[2]);
}

void iAVRModelInMiniature::setPos(double x, double y, double z)
{
	m_actor->SetPosition(x, y, z);
	m_activeActor->SetPosition(x, y, z);
}

void iAVRModelInMiniature::addPos(double x, double y, double z)
{
	m_actor->AddPosition(x, y, z);
	m_activeActor->AddPosition(x, y, z);
}

void iAVRModelInMiniature::setOrientation(double x, double y, double z)
{
	m_actor->SetOrientation(x, y, z);
	m_activeActor->SetOrientation(x, y, z);
}
