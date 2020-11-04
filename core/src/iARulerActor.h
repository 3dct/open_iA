/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <vtkCoordinate.h> // For vtkViewportCoordinateMacro
#include <vtkProp.h>

class vtkAxisActor2D;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper2D;
class vtkPropCollection;
class vtkTextMapper;
class vtkTextProperty;
class vtkWindow;

class iARulerActor : public vtkProp
{
public:
  //! Instantiate the class.
  static iARulerActor *New();

  //! Standard methods for the class.
  vtkTypeMacro(iARulerActor,vtkProp);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  void SetPosition(double *coord);
  void SetSize(double *coord);

//BTX
  enum AttributeLocation
  {
    DISTANCE=0,
    XY_COORDINATES=1
  };
//ETX

  //! Specify the mode for labeling the scale axes. By default, the axes are
  //! labeled with the distance between points (centered at a distance of
  //! 0.0). Alternatively if you know that the view is down the z-axis; the
  //! axes can be labeled with x-y coordinate values.
  vtkSetClampMacro(LabelMode,int,DISTANCE,XY_COORDINATES);
  vtkGetMacro(LabelMode,int);
  void SetLabelModeToDistance() {this->SetLabelMode(DISTANCE);}
  void SetLabelModeToXYCoordinates() {this->SetLabelMode(XY_COORDINATES);}

  //! Set/Get the flags that control which of the four axes to display (top,
  //! bottom, left and right). By default, all the axes are displayed.
  vtkSetMacro(LeftAxisVisibility,int);
  vtkGetMacro(LeftAxisVisibility,int);
  vtkBooleanMacro(LeftAxisVisibility,int);
  vtkSetMacro(BottomAxisVisibility,int);
  vtkGetMacro(BottomAxisVisibility,int);
  vtkBooleanMacro(BottomAxisVisibility,int);

  //! Convenience method that turns all the axes either on or off.
  void AllAxesOn();
  void AllAxesOff();

  //! Convenience method that turns all the axes and the legend scale.
  void AllAnnotationsOn();
  void AllAnnotationsOff();

  //! Set/Get the offset of the right axis from the border. This number is expressed in
  //! pixels, and represents the approximate distance of the axes from the sides
  //! of the renderer. The default is 50.
  vtkSetClampMacro(RightBorderOffset, int, 5, VTK_INT_MAX);
  vtkGetMacro(RightBorderOffset,int);

  //! Set/Get the offset of the top axis from the border. This number is expressed in
  //! pixels, and represents the approximate distance of the axes from the sides
  //! of the renderer. The default is 30.
  vtkSetClampMacro(TopBorderOffset, int, 5, VTK_INT_MAX);
  vtkGetMacro(TopBorderOffset,int);

  //! Set/Get the offset of the left axis from the border. This number is expressed in
  //! pixels, and represents the approximate distance of the axes from the sides
  //! of the renderer. The default is 50.
  vtkSetClampMacro(LeftBorderOffset, int, 5, VTK_INT_MAX);
  vtkGetMacro(LeftBorderOffset,int);

  //! Set/Get the offset of the bottom axis from the border. This number is expressed in
  //! pixels, and represents the approximate distance of the axes from the sides
  //! of the renderer. The default is 30.
  vtkSetClampMacro(BottomBorderOffset, int, 5, VTK_INT_MAX);
  vtkGetMacro(BottomBorderOffset,int);

  //! Get/Set the corner offset. This is the offset factor used to offset the
  //! axes at the corners. Default value is 2.0.
  vtkSetClampMacro(CornerOffsetFactor, double, 1.0, 10.0);
  vtkGetMacro(CornerOffsetFactor, double);

  //! These are methods to retrieve the vtkAxisActors used to represent
  //! the four axes that form this representation. Users may retrieve and
  //! then modify these axes to control their appearance.
  vtkGetObjectMacro(LeftAxis,vtkAxisActor2D);
  vtkGetObjectMacro(BottomAxis,vtkAxisActor2D);

  //! Standard methods supporting the rendering process.
  virtual void BuildRepresentation(vtkViewport *viewport);
  void GetActors2D(vtkPropCollection*) override;
  void ReleaseGraphicsResources(vtkWindow*) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;

protected:
  iARulerActor();
  ~iARulerActor();

  int    LabelMode;
  int    AxisOffset;
  int    RightBorderOffset;
  int    TopBorderOffset;
  int    LeftBorderOffset;
  int    BottomBorderOffset;
  double CornerOffsetFactor;

  //! @{ The axes around the borders of the renderer
  vtkAxisActor2D *LeftAxis;
  vtkAxisActor2D *BottomAxis;
  //! @}

  //! @{ Control the display of the axes
  int LeftAxisVisibility;
  int BottomAxisVisibility;
  //! @}

  vtkTimeStamp         BuildTime;

  vtkCoordinate * ReferenceCoordinate;
  vtkCoordinate * PositionCoordinate;
  vtkCoordinate * SizeCoordinate;

private:
  iARulerActor(const iARulerActor&) =delete;
  void operator=(const iARulerActor&) =delete;
};
