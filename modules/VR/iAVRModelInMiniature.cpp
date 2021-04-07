/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#include "iAVRModelInMiniature.h"

#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkIdFilter.h"
#include "vtkIdTypeArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkLookupTable.h"
#include "vtkColorTransferFunction.h"
#include "vtkCellPicker.h"
#include "vtkProp3DCollection.h"
#include "vtkVertexGlyphFilter.h"
#include <vtkAlgorithmOutput.h>
#include <vtkCubeSource.h>

#include <iALog.h>
#include <iAvec3.h>
#include <math.h>

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

	applyRelativeCubeOffset(2.9);

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
