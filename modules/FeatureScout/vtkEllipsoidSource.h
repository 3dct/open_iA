// SOURCE: https://github.com/daviddoria/vtkEllipsoidSource.git
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkEllipsoidSource.h,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkEllipsoidSource - create a polygonal ellipsoid centered at the origin
// .SECTION Description
// vtkEllipsoidSource creates an ellipsoid (represented by polygons) centered at 
// the origin. The orientation is completely specified by the major axis direction.
// The minor and major axis lengths can be set.

#ifndef __vtkEllipsoidSource_h
#define __vtkEllipsoidSource_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkSmartPointer.h"

class vtkTransform;
class vtkSphereSource;

#define VTK_MAX_SPHERE_RESOLUTION 1024

class vtkEllipsoidSource : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(vtkEllipsoidSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkEllipsoidSource *New();

  // Description:
  // Set the center of the sphere. Default is 0,0,0.
  vtkSetVector3Macro(Center,double);
  vtkGetVectorMacro(Center,double,3);

  // Description:
  // Set the number of points in the longitude direction (ranging from
  // StartTheta to EndTheta).
  vtkSetClampMacro(ThetaResolution,int,3,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(ThetaResolution,int);

  // Description:
  // Set the number of points in the latitude direction (ranging
  // from StartPhi to EndPhi).
  vtkSetClampMacro(PhiResolution,int,3,VTK_MAX_SPHERE_RESOLUTION);
  vtkGetMacro(PhiResolution,int);

  // Description:
  // Set the starting longitude angle. By default StartTheta=0 degrees.
  vtkSetClampMacro(StartTheta,double,0.0,360.0);
  vtkGetMacro(StartTheta,double);

  // Description:
  // Set the ending longitude angle. By default EndTheta=360 degrees.
  vtkSetClampMacro(EndTheta,double,0.0,360.0);
  vtkGetMacro(EndTheta,double);

  // Description:
  // Set the starting latitude angle (0 is at north pole). By default
  // StartPhi=0 degrees.
  vtkSetClampMacro(StartPhi,double,0.0,360.0);
  vtkGetMacro(StartPhi,double);

  // Description:
  // Set the ending latitude angle. By default EndPhi=180 degrees.
  vtkSetClampMacro(EndPhi,double,0.0,360.0);
  vtkGetMacro(EndPhi,double);

  // Description:
  // Cause the sphere to be tessellated with edges along the latitude
  // and longitude lines. If off, triangles are generated at non-polar
  // regions, which results in edges that are not parallel to latitude and
  // longitude lines. If on, quadrilaterals are generated everywhere
  // except at the poles. This can be useful for generating a wireframe
  // sphere with natural latitude and longitude lines.
  vtkSetMacro(LatLongTessellation,int);
  vtkGetMacro(LatLongTessellation,int);
  vtkBooleanMacro(LatLongTessellation,int);
  
  // Description:
  // Set the X radius.
  vtkSetMacro(XRadius, double);
  vtkGetMacro(XRadius, double);
  
  // Description:
  // Set the Y radius.
  vtkSetMacro(YRadius, double);
  vtkGetMacro(YRadius, double);
  
  // Description:
  // Set the Z radius.
  vtkSetMacro(ZRadius, double);
  vtkGetMacro(ZRadius, double);
  
  // Description:
  // Set the direction of the axis initially aligned with the X axis.
  vtkSetVector3Macro(XAxis, double);
  vtkGetVector3Macro(XAxis, double);
  
  // Description:
  // Set the direction of the axis initially aligned with the Y axis.
  vtkSetVector3Macro(YAxis, double);
  vtkGetVector3Macro(YAxis, double);
	
protected:
  vtkEllipsoidSource(int res=8);
  ~vtkEllipsoidSource();
  
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  double Center[3];
  int ThetaResolution;
  int PhiResolution;
  double StartTheta;
  double EndTheta;
  double StartPhi;
  double EndPhi;
  int LatLongTessellation;
  
  double XRadius;
  double YRadius;
  double ZRadius;

private:
  vtkEllipsoidSource(const vtkEllipsoidSource&);  // Not implemented.
  void operator=(const vtkEllipsoidSource&);  // Not implemented.
  
  vtkSmartPointer<vtkTransform> EllipsoidTransform;
  
  double XAxis[3];
  double YAxis[3];
  double ZAxis[3];
};

#endif
