// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkBorderRepresentation.h>

class iARulerActor;

//! Representation data for visualizing a ruler in the slicer.
class iARulerRepresentation : public vtkBorderRepresentation
{
public:
  vtkTypeMacro(iARulerRepresentation, vtkBorderRepresentation);
  void PrintSelf(std::ostream &os, vtkIndent indent) override;
  static iARulerRepresentation *New();

  //! The prop that is placed in the renderer.
  vtkGetObjectMacro(RulerActor, iARulerActor);
  virtual void SetScalarBarActor(iARulerActor *);

  //! Satisfy the superclass' API.
  void BuildRepresentation() override;
  void GetSize(double size[2]) override
    {size[0]=2.0; size[1]=2.0;}

  //! @{ These methods are necessary to make this representation behave as a vtkProp.
  void GetActors2D(vtkPropCollection *collection) override;
  void ReleaseGraphicsResources(vtkWindow *window) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  int HasTranslucentPolygonalGeometry() override;
  //! @}

protected:
  iARulerRepresentation();
  ~iARulerRepresentation();

  iARulerActor *RulerActor;

private:
  iARulerRepresentation(const iARulerRepresentation &) =delete;
  void operator=(const iARulerRepresentation &) =delete;
};
