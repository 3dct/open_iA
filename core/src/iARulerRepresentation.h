/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef IARULERREPRESENTATION_H
#define IARULERREPRESENTATION_H

#include <vtkBorderRepresentation.h>

class iARulerActor;

class iARulerRepresentation : public vtkBorderRepresentation
{
public:
  vtkTypeMacro(iARulerRepresentation, vtkBorderRepresentation);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static iARulerRepresentation *New();

  // Description:
  // The prop that is placed in the renderer.
  vtkGetObjectMacro(RulerActor, iARulerActor);
  virtual void SetScalarBarActor(iARulerActor *);

  // Description:
  // Satisfy the superclass' API.
  virtual void BuildRepresentation();
  virtual void GetSize(double size[2])
    {size[0]=2.0; size[1]=2.0;}

  // Description:
  // These methods are necessary to make this representation behave as
  // a vtkProp.
  virtual void GetActors2D(vtkPropCollection *collection);
  virtual void ReleaseGraphicsResources(vtkWindow *window);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

protected:
  iARulerRepresentation();
  ~iARulerRepresentation();

  iARulerActor *RulerActor;
private:
  iARulerRepresentation(const iARulerRepresentation &); // Not implemented
  void operator=(const iARulerRepresentation &);   // Not implemented
};

#endif //__iAVtkRulerRepresentation_h
