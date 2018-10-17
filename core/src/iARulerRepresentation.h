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
#pragma once

#include <vtkBorderRepresentation.h>

class iARulerActor;

class iARulerRepresentation : public vtkBorderRepresentation
{
public:
  vtkTypeMacro(iARulerRepresentation, vtkBorderRepresentation);
  void PrintSelf(ostream &os, vtkIndent indent) override;
  static iARulerRepresentation *New();

  // Description:
  // The prop that is placed in the renderer.
  vtkGetObjectMacro(RulerActor, iARulerActor);
  virtual void SetScalarBarActor(iARulerActor *);

  // Description:
  // Satisfy the superclass' API.
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
    {size[0]=2.0; size[1]=2.0;}

  // Description:
  // These methods are necessary to make this representation behave as
  // a vtkProp.
  void GetActors2D(vtkPropCollection *collection) override;
  void ReleaseGraphicsResources(vtkWindow *window) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  int HasTranslucentPolygonalGeometry() override;

protected:
  iARulerRepresentation();
  ~iARulerRepresentation();

  iARulerActor *RulerActor;
private:
  iARulerRepresentation(const iARulerRepresentation &); // Not implemented
  void operator=(const iARulerRepresentation &);   // Not implemented
};
