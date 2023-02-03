// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARulerRepresentation.h"

#include "iARulerActor.h"

#include <vtkActor2D.h>
#include <vtkObjectFactory.h>
#include <vtkPropCollection.h>
#include <vtkSmartPointer.h>
#include <vtkVersion.h>


vtkStandardNewMacro(iARulerRepresentation);


iARulerRepresentation::iARulerRepresentation()
{
	this->RulerActor = nullptr;
	iARulerActor *actor = iARulerActor::New();
	this->SetScalarBarActor(actor);
	actor->Delete();

	this->SetShowBorder(vtkBorderRepresentation::BORDER_ACTIVE);
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 4)
	this->BWActor->VisibilityOff();
#else
	this->BWActorEdges->VisibilityOff();
	this->BWActorPolygon->VisibilityOff();
#endif
}


iARulerRepresentation::~iARulerRepresentation()
{
	this->SetScalarBarActor(nullptr);
}


void iARulerRepresentation::SetScalarBarActor(iARulerActor* actor)
{
	if (this->RulerActor != actor)
	{
		vtkSmartPointer<iARulerActor> oldActor = this->RulerActor;
		vtkSetObjectBodyMacro(RulerActor, iARulerActor, actor);
	}
}


void iARulerRepresentation::PrintSelf(std::ostream &os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);

	os << indent << "RulerActor: " << this->RulerActor << "\n";
}


void iARulerRepresentation::BuildRepresentation()
{
	if (this->RulerActor)
	{
		this->RulerActor->SetPosition(this->GetPosition());
		this->RulerActor->SetSize(this->GetPosition2());
	}

	this->Superclass::BuildRepresentation();
}


void iARulerRepresentation::GetActors2D(vtkPropCollection *collection)
{
	if (this->RulerActor)
	{
		collection->AddItem(this->RulerActor);
	}
	this->Superclass::GetActors2D(collection);
}


void iARulerRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
	if (this->RulerActor)
	{
		this->RulerActor->ReleaseGraphicsResources(w);
	}
	this->Superclass::ReleaseGraphicsResources(w);
}


int iARulerRepresentation::RenderOverlay(vtkViewport *w)
{
	int count = this->Superclass::RenderOverlay(w);
	if (this->RulerActor)
	{
		count += this->RulerActor->RenderOverlay(w);
	}
	return count;
}


int iARulerRepresentation::RenderOpaqueGeometry(vtkViewport *w)
{
	int count = this->Superclass::RenderOpaqueGeometry(w);
	if (this->RulerActor)
	{
		count += this->RulerActor->RenderOpaqueGeometry(w);
	}
	return count;
}


int iARulerRepresentation::RenderTranslucentPolygonalGeometry(
	vtkViewport *w)
{
	int count = this->Superclass::RenderTranslucentPolygonalGeometry(w);
	if (this->RulerActor)
	{
		count += this->RulerActor->RenderTranslucentPolygonalGeometry(w);
	}
	return count;
}


int iARulerRepresentation::HasTranslucentPolygonalGeometry()
{
	int result = this->Superclass::HasTranslucentPolygonalGeometry();
	if (this->RulerActor)
	{
		result |= this->RulerActor->HasTranslucentPolygonalGeometry();
	}
	return result;
}
