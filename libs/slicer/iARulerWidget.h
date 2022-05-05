/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <vtkBorderWidget.h>

class iARulerActor;
class iARulerRepresentation;

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
