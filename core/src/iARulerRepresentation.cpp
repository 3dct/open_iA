/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
 
#include "pch.h"
#include "iARulerRepresentation.h"

#include "iARulerActor.h"

#include <vtkObjectFactory.h>
#include <vtkPropCollection.h>
#include <vtkSmartPointer.h>
#include <vtkTextProperty.h>
#include <vtkActor2D.h>
#include <vtkMapper2D.h>


vtkStandardNewMacro(iARulerRepresentation);


iARulerRepresentation::iARulerRepresentation()
{
	this->RulerActor = NULL;
	iARulerActor *actor = iARulerActor::New();
	this->SetScalarBarActor(actor);
	actor->Delete();

	this->SetShowBorder(vtkBorderRepresentation::BORDER_ACTIVE);
	this->BWActor->VisibilityOff();
}


iARulerRepresentation::~iARulerRepresentation()
{
	this->SetScalarBarActor(NULL);
}


void iARulerRepresentation::SetScalarBarActor(iARulerActor* actor)
{
	if (this->RulerActor != actor)
	{
		vtkSmartPointer<iARulerActor> oldActor = this->RulerActor;
		vtkSetObjectBodyMacro(RulerActor, iARulerActor, actor);
	}
}


void iARulerRepresentation::PrintSelf(ostream &os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);

	os << indent << "RulerActor: " << this->RulerActor << endl;
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
