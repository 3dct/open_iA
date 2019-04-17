/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iARulerWidget.h"

#include "iARulerActor.h"
#include "iARulerRepresentation.h"

#include <vtkCallbackCommand.h>
#include <vtkCoordinate.h>
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


iARulerWidget::~iARulerWidget()
{
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


void iARulerWidget::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os,indent);

	os << indent << "Repositionable: " << this->Repositionable << endl;
}
