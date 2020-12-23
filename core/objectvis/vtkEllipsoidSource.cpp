// SOURCE: https://github.com/daviddoria/vtkEllipsoidSource.git
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkEllipsoidSource.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkEllipsoidSource.h"

#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include "vtkSmartPointer.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkLandmarkTransform.h"
#include "vtkMatrix4x4.h"
#include "vtkMath.h"
#include "vtkFloatArray.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkEllipsoidSource);

namespace //anonymous
{
  void AlignFrame(double* ray1, double* ray2, double* ray3, vtkSmartPointer<vtkTransform> transform);
}

vtkEllipsoidSource::vtkEllipsoidSource(int res)
{
  res = res < 4 ? 4 : res;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->ThetaResolution = res;
  this->PhiResolution = res;
  this->StartTheta = 0.0;
  this->EndTheta = 360.0;
  this->StartPhi = 0.0;
  this->EndPhi = 180.0;
  this->LatLongTessellation = 0;

  this->SetNumberOfInputPorts(0);

  this->EllipsoidTransform = vtkSmartPointer<vtkTransform>::New();
  this->EllipsoidTransform->PostMultiply();

  //initialize axes to defaults
  this->XAxis[0] = 1.0;
  this->XAxis[1] = 0.0;
  this->XAxis[2] = 0.0;

  this->YAxis[0] = 0.0;
  this->YAxis[1] = 1.0;
  this->YAxis[2] = 0.0;

  this->ZAxis[0] = 0.0;
  this->ZAxis[1] = 0.0;
  this->ZAxis[2] = 1.0;
}

vtkEllipsoidSource::~vtkEllipsoidSource()
{

}

int vtkEllipsoidSource::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *outputVector)
{

  // get the info object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the ouptut
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  //save the center info
  double originalCenter[3];
  originalCenter[0] = this->Center[0];
  originalCenter[1] = this->Center[1];
  originalCenter[2] = this->Center[2];

  double unitRadius = 1.0;

  //for now set the center to (0,0,0) so the scaling is around the origin
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  int i, j;
  int jStart, jEnd, numOffset;
  int numPts, numPolys;

  double x[3], n[3], deltaPhi, deltaTheta, phi, theta, radius, norm;
  double startTheta, endTheta, startPhi, endPhi;
  int base, numPoles=0, thetaResolution, phiResolution;
  vtkIdType pts[4];
  int piece =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  int numPieces =
    outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());

  if (numPieces > this->ThetaResolution)
    {
    numPieces = this->ThetaResolution;
    }

  if (piece >= numPieces)
    {
    // Although the super class should take care of this,
    // it cannot hurt to check here.
    return 1;
    }

  // I want to modify the ivars resoultion start theta and end theta,
  // so I will make local copies of them.  THese might be able to be merged
  // with the other copies of them, ...
  int localThetaResolution = this->ThetaResolution;
  double localStartTheta = this->StartTheta;
  double localEndTheta = this->EndTheta;

  while (localEndTheta < localStartTheta) //here
    {
    localEndTheta += 360.0;
    }

  deltaTheta = (localEndTheta - localStartTheta) / localThetaResolution;

  // Change the ivars based on pieces.
  int start, end;
  start = piece * localThetaResolution / numPieces;
  end = (piece+1) * localThetaResolution / numPieces;
  localEndTheta = localStartTheta + (double)(end) * deltaTheta;
  localStartTheta = localStartTheta + (double)(start) * deltaTheta;
  localThetaResolution = end - start;

  // Set things up; allocate memory
  //
  vtkDebugMacro("SphereSource Executing piece index " << piece
		  << " of " << numPieces << " pieces.");

  numPts = this->PhiResolution * localThetaResolution + 2;
  // creating triangles
  numPolys = this->PhiResolution * 2 * localThetaResolution;

  vtkSmartPointer<vtkPoints> newPoints =
      vtkSmartPointer<vtkPoints>::New();
  newPoints->Allocate(numPts);
  vtkSmartPointer<vtkFloatArray> newNormals =
      vtkSmartPointer<vtkFloatArray>::New();
  newNormals->SetNumberOfComponents(3);
  newNormals->Allocate(3*numPts);
  newNormals->SetName("Normals");

  vtkSmartPointer<vtkCellArray> newPolys =
      vtkSmartPointer<vtkCellArray>::New();
  newPolys->Allocate(newPolys->EstimateSize(numPolys, 3));

  // Create sphere
  //
  // Create north pole if needed
  if ( this->StartPhi <= 0.0 )
    {
    x[0] = this->Center[0];
    x[1] = this->Center[1];
    x[2] = this->Center[2] + unitRadius;
    newPoints->InsertPoint(numPoles,x);

    x[0] = x[1] = 0.0; x[2] = 1.0;
    newNormals->InsertTuple(numPoles,x);
    numPoles++;
    }

  // Create south pole if needed
  if ( this->EndPhi >= 180.0 )
    {
    x[0] = this->Center[0];
    x[1] = this->Center[1];
    x[2] = this->Center[2] - unitRadius;
    newPoints->InsertPoint(numPoles,x);

    x[0] = x[1] = 0.0; x[2] = -1.0;
    newNormals->InsertTuple(numPoles,x);
    numPoles++;
    }

  // Check data, determine increments, and convert to radians
  startTheta = (localStartTheta < localEndTheta ? localStartTheta : localEndTheta);
  startTheta *= vtkMath::Pi() / 180.0;
  endTheta = (localEndTheta > localStartTheta ? localEndTheta : localStartTheta);
  endTheta *= vtkMath::Pi() / 180.0;

  startPhi = (this->StartPhi < this->EndPhi ? this->StartPhi : this->EndPhi);
  startPhi *= vtkMath::Pi() / 180.0;
  endPhi = (this->EndPhi > this->StartPhi ? this->EndPhi : this->StartPhi);
  endPhi *= vtkMath::Pi() / 180.0;

  phiResolution = this->PhiResolution - numPoles;
  deltaPhi = (endPhi - startPhi) / (this->PhiResolution - 1);
  thetaResolution = localThetaResolution;
  if (fabs(localStartTheta - localEndTheta) < 360.0)
    {
    ++localThetaResolution;
    }

  deltaTheta = (endTheta - startTheta) / thetaResolution;

  jStart = (this->StartPhi <= 0.0 ? 1 : 0);
  jEnd = (this->EndPhi >= 180.0 ? this->PhiResolution - 1
  : this->PhiResolution);

  this->UpdateProgress(0.1);

  // Create intermediate points
  for (i=0; i < localThetaResolution; i++)
    {
    theta = localStartTheta * vtkMath::Pi() / 180.0 + i*deltaTheta;

    for (j=jStart; j<jEnd; j++)
      {
      phi = startPhi + j*deltaPhi;
      radius = unitRadius * sin((double)phi);
      n[0] = radius * cos((double)theta);
      n[1] = radius * sin((double)theta);
      n[2] = unitRadius * cos((double)phi);
      x[0] = n[0] + this->Center[0];
      x[1] = n[1] + this->Center[1];
      x[2] = n[2] + this->Center[2];
      newPoints->InsertNextPoint(x);

      if ( (norm = vtkMath::Norm(n)) == 0.0 )
        {
          norm = 1.0;
        }
      n[0] /= norm; n[1] /= norm; n[2] /= norm;
      newNormals->InsertNextTuple(n);
      }
    this->UpdateProgress (0.10 + 0.50*i/static_cast<float>(localThetaResolution));
    }

  // Generate mesh connectivity
  base = phiResolution * localThetaResolution;

  if (fabs(localStartTheta - localEndTheta) < 360.0)
    {
    --localThetaResolution;
    }

  if ( this->StartPhi <= 0.0 )  // around north pole
    {
    for (i=0; i < localThetaResolution; i++)
      {
      pts[0] = phiResolution*i + numPoles;
      pts[1] = (phiResolution*(i+1) % base) + numPoles;
      pts[2] = 0;
      newPolys->InsertNextCell(3, pts);
      }
    }

  if ( this->EndPhi >= 180.0 ) // around south pole
    {
    numOffset = phiResolution - 1 + numPoles;

    for (i=0; i < localThetaResolution; i++)
      {
      pts[0] = phiResolution*i + numOffset;
      pts[2] = ((phiResolution*(i+1)) % base) + numOffset;
      pts[1] = numPoles - 1;
      newPolys->InsertNextCell(3, pts);
      }
	}

  this->UpdateProgress (0.70);

  // bands in-between poles
  for (i=0; i < localThetaResolution; i++)
    {
    for (j=0; j < (phiResolution-1); j++)
      {
      pts[0] = phiResolution*i + j + numPoles;
      pts[1] = pts[0] + 1;
      pts[2] = ((phiResolution*(i+1)+j) % base) + numPoles + 1;
      if ( !this->LatLongTessellation )
        {
        newPolys->InsertNextCell(3, pts);
        pts[1] = pts[2];
        pts[2] = pts[1] - 1;
        newPolys->InsertNextCell(3, pts);
        }
      else
        {
        pts[3] = pts[2] - 1;
        newPolys->InsertNextCell(4, pts);
        }
      }
    this->UpdateProgress (0.70 + 0.30*i/static_cast<double>(localThetaResolution));
    }

  // Update ourselves and release memeory
  //

  vtkSmartPointer<vtkPolyData> spherePolyData =
      vtkSmartPointer<vtkPolyData>::New();

  newPoints->Squeeze();
  spherePolyData->SetPoints(newPoints);

  newNormals->Squeeze();
  spherePolyData->GetPointData()->SetNormals(newNormals);

  newPolys->Squeeze();
  spherePolyData->SetPolys(newPolys);

  //reset the center
  this->Center[0] = originalCenter[0];
  this->Center[1] = originalCenter[1];
  this->Center[2] = originalCenter[2];

  //this ellipsoid assumes that its major axis is aligned with the Z axis

  //create the transform
  this->EllipsoidTransform->Identity();
  this->EllipsoidTransform->Scale(this->XRadius,
                                  this->YRadius,
                                  this->ZRadius);

  //compute the ZAxis (orthogonal to XAxis and YAxis)
  vtkMath::Cross(this->XAxis, this->YAxis, this->ZAxis);

  //align the major axis
  vtkSmartPointer<vtkTransform> alignmentTransform =
        vtkSmartPointer<vtkTransform>::New();
  AlignFrame(this->XAxis, this->YAxis, this->ZAxis, alignmentTransform);

  this->EllipsoidTransform->Concatenate(alignmentTransform);

  //translate to the correct center
  this->EllipsoidTransform->Translate(this->Center[0],
                                      this->Center[1],
                                      this->Center[2]);

  //apply the transform
  vtkSmartPointer<vtkTransformPolyDataFilter> transformFilter =
      vtkSmartPointer<vtkTransformPolyDataFilter>::New();

  transformFilter->SetInputData(spherePolyData);
  transformFilter->SetTransform(this->EllipsoidTransform);
  transformFilter->Update();

  output->ShallowCopy(transformFilter->GetOutput());

  return 1;
}


//----------------------------------------------------------------------------
void vtkEllipsoidSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Theta Resolution: " << this->ThetaResolution << "\n";
  os << indent << "Phi Resolution: " << this->PhiResolution << "\n";
  os << indent << "Theta Start: " << this->StartTheta << "\n";
  os << indent << "Phi Start: " << this->StartPhi << "\n";
  os << indent << "Theta End: " << this->EndTheta << "\n";
  os << indent << "Phi End: " << this->EndPhi << "\n";
  os << indent << "Center: (" << this->Center[0] << ", "
		  << this->Center[1] << ", " << this->Center[2] << ")\n";
  os << indent
		  << "LatLong Tessellation: " << this->LatLongTessellation << "\n";

  os << indent << "X axis" << std::endl << "---------" << std::endl;
  os << indent << "Length: " << this->XRadius << std::endl;
  os << indent << "Direction: " << this->XAxis[0] << " " << this->XAxis[1] << " " << this->XAxis[2] << std::endl;
  os << std::endl;

  os << indent << "Y axis" << std::endl << "---------" << std::endl;
  os << indent << "Length: " << this->YRadius << std::endl;
  os << indent << "Direction: " << this->YAxis[0] << " " << this->YAxis[1] << " " << this->YAxis[2] << std::endl;
  os << std::endl;

  os << indent << "Z axis" << std::endl << "---------" << std::endl;
  os << indent << "Length: " << this->ZRadius << std::endl;
  os << indent << "Direction: " << this->ZAxis[0] << " " << this->ZAxis[1] << " " << this->ZAxis[2] << std::endl;
  os << std::endl;

}

namespace //anonymous
{
  void AlignFrame(double* ray1, double* ray2, double* ray3, vtkSmartPointer<vtkTransform> transform)
  {
    //This function takes two rays and finds the matrix M between them (from Ray1 to Ray2)
    vtkMath::Normalize(ray1);
    vtkMath::Normalize(ray2);
    vtkMath::Normalize(ray3);

    vtkSmartPointer<vtkLandmarkTransform> landmarkTransform =
        vtkSmartPointer<vtkLandmarkTransform>::New();
    vtkSmartPointer<vtkPoints> sourcePoints =
        vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkPoints> targetPoints =
        vtkSmartPointer<vtkPoints>::New();

    double xaxis[3] = {1,0,0};
    double yaxis[3] = {0,1,0};
    double zaxis[3] = {0,0,1};

    sourcePoints->InsertNextPoint(xaxis);
    sourcePoints->InsertNextPoint(yaxis);
    sourcePoints->InsertNextPoint(zaxis);

    targetPoints->InsertNextPoint(ray1);
    targetPoints->InsertNextPoint(ray2);
    targetPoints->InsertNextPoint(ray3);

    landmarkTransform->SetSourceLandmarks(sourcePoints);
    landmarkTransform->SetTargetLandmarks(targetPoints);
    landmarkTransform->SetModeToRigidBody();
    landmarkTransform->Update();

    vtkMatrix4x4* m = landmarkTransform->GetMatrix();

    transform->SetMatrix(m);

  } // end AlignRays
} //end anonymous namespace