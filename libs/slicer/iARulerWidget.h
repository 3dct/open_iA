// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkBorderWidget.h>

class iARulerActor;
class iARulerRepresentation;

//! Widget for visualizing a ruler in the slicer.
class iARulerWidget : public vtkBorderWidget
{
public:
  static iARulerWidget *New();
  vtkTypeMacro(iARulerWidget, vtkBorderWidget);
  void PrintSelf(std::ostream& os, vtkIndent indent) override;

  //! Specify an instance of vtkWidgetRepresentation used to represent this
  //! widget in the scene. Note that the representation is a subclass of vtkProp
  //! so it can be added to the renderer independent of the widget.
  virtual void SetRepresentation(iARulerRepresentation *rep);

  iARulerRepresentation *GetScalarBarRepresentation()
    { return reinterpret_cast<iARulerRepresentation *>(this->GetRepresentation()); }

  //! Get the ScalarBar used by this Widget. One is created automatically.
  virtual void SetRulerActor(iARulerActor *actor);
  virtual iARulerActor *GetRulerActor();

  //! Can the widget be moved. On by default. If off, the widget cannot be moved around.
  // TODO: This functionality should probably be moved to the superclass.
  vtkSetMacro(Repositionable, int);
  vtkGetMacro(Repositionable, int);
  vtkBooleanMacro(Repositionable, int);

  //! Create the default widget representation if one is not set.
  void CreateDefaultRepresentation() override;

protected:
  iARulerWidget();

  int Repositionable;

  //! Handle the case of Repositionable == 0
  static void MoveAction(vtkAbstractWidget*);

  //! Set the cursor to the correct shape based on State argument
  void SetCursor(int State) override;

private:
  iARulerWidget(const iARulerWidget&) =delete;
  void operator=(const iARulerWidget&) =delete;
};
