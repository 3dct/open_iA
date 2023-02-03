// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARulerWidget.h"

#include "iARulerActor.h"
#include "iARulerRepresentation.h"

#include <vtkCallbackCommand.h>
#include <vtkObjectFactory.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>  // for VTK_CURSOR_DEFAULT
#include <vtkWidgetCallbackMapper.h>
#include <vtkWidgetEvent.h>

vtkStandardNewMacro(iARulerWidget);


iARulerWidget::iARulerWidget()
{
	this->Selectable = 0;
	this->Repositionable = 1;

	// Override the subclasses callback to handle the Repositionable flag.
	this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
		vtkWidgetEvent::Move,
		this, iARulerWidget::MoveAction);
}


void iARulerWidget::SetRepresentation(iARulerRepresentation *rep)
{
	this->SetWidgetRepresentation(rep);
}


void iARulerWidget::SetRulerActor(iARulerActor *actor)
{
	iARulerRepresentation *rep = this->GetScalarBarRepresentation();
	if (!rep)
	{
		this->CreateDefaultRepresentation();
		rep = this->GetScalarBarRepresentation();
	}

	if (rep->GetRulerActor() != actor)
	{
		rep->SetScalarBarActor(actor);
		this->Modified();
	}
}


iARulerActor *iARulerWidget::GetRulerActor()
{
	iARulerRepresentation *rep = this->GetScalarBarRepresentation();
	if (!rep)
	{
		this->CreateDefaultRepresentation();
		rep = this->GetScalarBarRepresentation();
	}

	return rep->GetRulerActor();
}


void iARulerWidget::CreateDefaultRepresentation()
{
	if (!this->WidgetRep)
	{
		iARulerRepresentation *rep = iARulerRepresentation::New();
		this->SetRepresentation(rep);
		rep->Delete();
	}
}


void iARulerWidget::SetCursor(int cState)
{
	if (   !this->Repositionable && !this->Selectable
		&& cState == vtkBorderRepresentation::Inside)
	{
		// Don't have a special cursor for the inside if we cannot reposition.
		this->RequestCursorShape(VTK_CURSOR_DEFAULT);
	}
	else
	{
		this->Superclass::SetCursor(cState);
	}
}


void iARulerWidget::MoveAction(vtkAbstractWidget *w)
{
	// The the superclass handle most stuff.
	iARulerWidget::Superclass::MoveAction(w);

	iARulerWidget *self = reinterpret_cast<iARulerWidget*>(w);
	iARulerRepresentation *representation=self->GetScalarBarRepresentation();

	// Handle the case where we suppress widget translation.
	if (   !self->Repositionable
		&& (   representation->GetInteractionState()
		== vtkBorderRepresentation::Inside ) )
	{
		representation->MovingOff();
	}
}


void iARulerWidget::PrintSelf(std::ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os,indent);

	os << indent << "Repositionable: " << this->Repositionable << "\n";
}
