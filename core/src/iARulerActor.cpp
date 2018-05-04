/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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
#include "iARulerActor.h"

#include <vtkActor2D.h>
#include <vtkAxisActor2D.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCommand.h>
#include <vtkCoordinate.h>
#include <vtkEvent.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper2D.h>
#include <vtkRenderer.h>
#include <vtkTextMapper.h>
#include <vtkTextProperty.h>
#include <vtkUnsignedCharArray.h>
#include <vtkWindow.h>

#include <algorithm>

vtkStandardNewMacro(iARulerActor);


iARulerActor::iARulerActor()
{
	this->ReferenceCoordinate = vtkCoordinate::New();
	this->ReferenceCoordinate->SetCoordinateSystem(VTK_VIEWPORT);
	this->PositionCoordinate = vtkCoordinate::New();
	this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
	this->PositionCoordinate->SetReferenceCoordinate(this->ReferenceCoordinate);
	this->SizeCoordinate = vtkCoordinate::New();
	this->SizeCoordinate->SetCoordinateSystemToNormalizedViewport();
	this->SizeCoordinate->SetReferenceCoordinate(this->ReferenceCoordinate);

	this->LabelMode = DISTANCE;

	this->AxisOffset = 17;
	this->RightBorderOffset = 0;
	this->TopBorderOffset = 0;
	this->LeftBorderOffset = 0;
	this->BottomBorderOffset = 0;
	this->CornerOffsetFactor = 2.0;

	this->LeftAxis = vtkAxisActor2D::New();
	this->LeftAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
	this->LeftAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
	this->LeftAxis->GetPositionCoordinate()->SetReferenceCoordinate(NULL);
	this->LeftAxis->SetFontFactor(0.7);
	this->LeftAxis->SetNumberOfLabels(5);
	this->LeftAxis->AdjustLabelsOff();

	this->BottomAxis = vtkAxisActor2D::New();
	this->BottomAxis->GetPositionCoordinate()->SetCoordinateSystemToViewport();
	this->BottomAxis->GetPosition2Coordinate()->SetCoordinateSystemToViewport();
	this->BottomAxis->GetPositionCoordinate()->SetReferenceCoordinate(NULL);
	this->BottomAxis->SetFontFactor(0.7);
	//this->BottomAxis->SetSizeFontRelativeToAxis(1);
	this->BottomAxis->SetNumberOfLabels(5);
	this->BottomAxis->AdjustLabelsOff();

	this->LeftAxisVisibility = 1;
	this->BottomAxisVisibility = 1;
}


iARulerActor::~iARulerActor()
{
	this->ReferenceCoordinate->Delete();
	this->PositionCoordinate->Delete();
	this->SizeCoordinate->Delete();

	this->LeftAxis->Delete();
	this->BottomAxis->Delete();
}


void iARulerActor::GetActors2D(vtkPropCollection *pc)
{
	pc->AddItem(this->LeftAxis);
	pc->AddItem(this->BottomAxis);
}


void iARulerActor::ReleaseGraphicsResources(vtkWindow *w)
{
	this->LeftAxis->ReleaseGraphicsResources(w);
	this->BottomAxis->ReleaseGraphicsResources(w);
}


int iARulerActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
	this->BuildRepresentation(viewport);

	int renderedSomething=0;
	if ( this->LeftAxisVisibility )
	{
		renderedSomething += this->LeftAxis->RenderOpaqueGeometry(viewport);
	}
	if ( this->BottomAxisVisibility )
	{
		renderedSomething += this->BottomAxis->RenderOpaqueGeometry(viewport);
	}
	return renderedSomething;
}


int iARulerActor::RenderOverlay(vtkViewport *viewport)
{
	int renderedSomething=0;
	if ( this->LeftAxisVisibility )
	{
		renderedSomething += this->LeftAxis->RenderOverlay(viewport);
	}
	if ( this->BottomAxisVisibility )
	{
		renderedSomething += this->BottomAxis->RenderOverlay(viewport);
	}
	return renderedSomething;
}


void iARulerActor::BuildRepresentation(vtkViewport *viewport)
{
	if ( 1 ) //it's probably best just to rerender every time
		//   if ( this->GetMTime() > this->BuildTime ||
		//        (this->Renderer && this->Renderer->GetVTKWindow() &&
		//         this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
	{
		// Specify the locations of the axes.
		int barOrigin[2];
		int size[2];
		barOrigin[0] =	this->PositionCoordinate->GetComputedViewportValue(viewport)[0];
		barOrigin[1] =	this->PositionCoordinate->GetComputedViewportValue(viewport)[1];
		size[0] =		this->SizeCoordinate->GetComputedViewportValue(viewport)[0];
		size[1] =		this->SizeCoordinate->GetComputedViewportValue(viewport)[1];
		//check orientation
		bool orientation = (size[0] < size[1]);
		this->SetLeftAxisVisibility(orientation);
		this->SetBottomAxisVisibility(!orientation);

		double fontFactor = std::min(1.5, 0.7+ static_cast<double>(size[orientation?0:1]) / 50);

		this->LeftAxis->SetFontFactor( fontFactor );
		this->BottomAxis->SetFontFactor( fontFactor );

		this->LeftAxis->GetPositionCoordinate()->SetValue(
			barOrigin[0] + AxisOffset,
			barOrigin[1] + size[1] - CornerOffsetFactor*TopBorderOffset, 0.0);
		this->LeftAxis->GetPosition2Coordinate()->SetValue(
			barOrigin[0] + AxisOffset,
			barOrigin[1] + CornerOffsetFactor*BottomBorderOffset,0.0);

		this->BottomAxis->GetPositionCoordinate()->SetValue(
			barOrigin[0]+CornerOffsetFactor*LeftBorderOffset,
			barOrigin[1]+AxisOffset,0.0);
		this->BottomAxis->GetPosition2Coordinate()->SetValue(
			barOrigin[0]+size[0]-CornerOffsetFactor*RightBorderOffset,
			barOrigin[1]+AxisOffset,0.0);

		// Now specify the axis values
		double *xL, *xR;
		if ( this->LabelMode == XY_COORDINATES )
		{
			xL = LeftAxis->GetPositionCoordinate()->GetComputedWorldValue(viewport);
			xR = LeftAxis->GetPosition2Coordinate()->GetComputedWorldValue(viewport);
			LeftAxis->SetRange(xL[1],xR[1]);

			xL = BottomAxis->GetPositionCoordinate()->GetComputedWorldValue(viewport);
			xR = BottomAxis->GetPosition2Coordinate()->GetComputedWorldValue(viewport);
			BottomAxis->SetRange(xL[0],xR[0]);
		}
		else //distance between points
		{
			double d;
			xL = LeftAxis->GetPositionCoordinate()->GetComputedWorldValue(viewport);
			xR = LeftAxis->GetPosition2Coordinate()->GetComputedWorldValue(viewport);
			d = sqrt (vtkMath::Distance2BetweenPoints(xL,xR));
			LeftAxis->SetRange(d/2.0,-d/2.0);

			xL = BottomAxis->GetPositionCoordinate()->GetComputedWorldValue(viewport);
			xR = BottomAxis->GetPosition2Coordinate()->GetComputedWorldValue(viewport);
			d = sqrt (vtkMath::Distance2BetweenPoints(xL,xR));
			BottomAxis->SetRange(-d/2.0,d/2.0);
		}
		this->BuildTime.Modified();
	}
}


void iARulerActor::AllAnnotationsOn()
{
	if ( LeftAxisVisibility && BottomAxisVisibility )
	{
		return;
	}
	// If here, we are modified and something gets turned on
	this->LeftAxisVisibility = 1;
	this->BottomAxisVisibility = 1;
	this->Modified();
}


void iARulerActor::AllAnnotationsOff()
{
	if ( !LeftAxisVisibility && !BottomAxisVisibility )
	{
		return;
	}
	// If here, we are modified and something gets turned off
	this->LeftAxisVisibility = 0;
	this->BottomAxisVisibility = 0;
	this->Modified();
}


void iARulerActor::AllAxesOn()
{
	if ( LeftAxisVisibility && BottomAxisVisibility )
	{
		return;
	}
	// If here, we are modified and something gets turned on
	this->LeftAxisVisibility = 1;
	this->BottomAxisVisibility = 1;
	this->Modified();
}


void iARulerActor::AllAxesOff()
{
	if ( !LeftAxisVisibility && !BottomAxisVisibility )
	{
		return;
	}

	// If here, we are modified and something gets turned off
	this->LeftAxisVisibility = 0;
	this->BottomAxisVisibility = 0;
	this->Modified();
}


void iARulerActor::PrintSelf(ostream& os, vtkIndent indent)
{
	//Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
	this->Superclass::PrintSelf(os,indent);

	os << indent << "Label Mode: ";
	if ( this->LabelMode == DISTANCE )
	{
		os << "Distance\n";
	}
	else //if ( this->LabelMode == DISTANCE )
	{
		os << "XY_Coordinates\n";
	}

	os << indent << "Left Axis Visibility: "
		<< (this->LeftAxisVisibility ? "On\n" : "Off\n");
	os << indent << "Bottom Axis Visibility: "
		<< (this->BottomAxisVisibility ? "On\n" : "Off\n");
	os << indent << "Corner Offset Factor: " << this->CornerOffsetFactor << "\n";

	os << indent << "Right Border Offset: " << this->RightBorderOffset << "\n";
	os << indent << "Top Border Offset: " << this->TopBorderOffset << "\n";
	os << indent << "Left Border Offset: " << this->LeftBorderOffset << "\n";
	os << indent << "Bottom Border Offset: " << this->BottomBorderOffset << "\n";

	os << indent << "Left Axis: ";
	if ( this->LeftAxis )
	{
		os << this->LeftAxis << "\n";
	}
	else
	{
		os << "(none)\n";
	}
	os << indent << "Bottom Axis: ";
	if ( this->BottomAxis )
	{
		os << this->BottomAxis << "\n";
	}
	else
	{
		os << "(none)\n";
	}
}


void iARulerActor::SetPosition( double* coord)
{
	PositionCoordinate->SetValue(coord[0], coord[1]);
}


void iARulerActor::SetSize( double* coord)
{
	SizeCoordinate->SetValue(coord[0], coord[1]);
}