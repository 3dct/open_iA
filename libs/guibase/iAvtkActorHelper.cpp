// Copyright(c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAvtkActorHelper.h"

#include <vtkRenderer.h>

void showActor(vtkRenderer* renderer, vtkProp* actor, bool show);
{
	if (show)
	{
		renderer->AddActor(actor);
	}
	else
	{
		renderer->RemoveActor(actor);
	}
}
