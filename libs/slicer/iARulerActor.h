// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkCoordinate.h> // For vtkViewportCoordinateMacro
#include <vtkProp.h>

class vtkAxisActor2D;
class vtkPropCollection;
class vtkWindow;

//! An actor for visualizing a ruler in the slicer.
//! Displays either a vertical or horizontal ruler using a vtkAxisActor2D,
//! depending on the aspect ratio of the widget.
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

  //! @{
  //! vertical (left) / horizontal (bottom) axis; only one of the two is shown at a time, depending on the widget's aspect ratio
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
