// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "iAvtkClipPolyData.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkExecutive.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkImplicitFunction.h"
#include "vtkIncrementalPointLocator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLine.h"
#include "vtkMergePoints.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkTriangle.h"

#include <cmath>

//VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(iAvtkClipPolyData);

void iAvtkClipPolyData::AddClipFunction(vtkImplicitFunction *func)
{
  ClipFunctions.push_back( func );
}

void iAvtkClipPolyData::RemoveClipFunction(vtkImplicitFunction* func)
{
  auto it = std::find(ClipFunctions.begin(), ClipFunctions.end(), func);
  if (it != ClipFunctions.end())
  {
    ClipFunctions.erase(it);
  }
}

bool iAvtkClipPolyData::HasClipFunctions() const
{
  return ClipFunctions.empty();
}

void iAvtkClipPolyData::ClearClipFunctions()
{
  ClipFunctions.clear();
}

//------------------------------------------------------------------------------
// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off.
iAvtkClipPolyData::iAvtkClipPolyData(vtkImplicitFunction *cf)
{
  if (cf)
  {
    this->ClipFunctions.push_back(cf);
  }
  this->InsideOut = 0;
  this->Locator = nullptr;
  this->Value = 0.0;
  this->GenerateClipScalars = 0;
  this->GenerateClippedOutput = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;

  this->SetNumberOfOutputPorts(2);

  vtkPolyData *output2 = vtkPolyData::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();
}

//------------------------------------------------------------------------------
iAvtkClipPolyData::~iAvtkClipPolyData()
{
  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }
  ClipFunctions.clear();
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
vtkMTimeType iAvtkClipPolyData::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  vtkMTimeType time;

  for (auto f: this->ClipFunctions)
  {
    time = f->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  if ( this->Locator != nullptr )
  {
    time = this->Locator->GetMTime();
    mTime = ( time > mTime ? time : mTime );
  }

  return mTime;
}

vtkPolyData *iAvtkClipPolyData::GetClippedOutput()
{
  return vtkPolyData::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

//------------------------------------------------------------------------------
//
// Clip through data generating surface.
//
int iAvtkClipPolyData::RequestData(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData *input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkIdType cellId, i, updateTime;
  vtkPoints *cellPts;
  vtkDataArray *clipScalars;
  vtkFloatArray *cellScalars;
  vtkGenericCell *cell;
  vtkCellArray *newVerts, *newLines, *newPolys, *connList=nullptr;
  vtkCellArray *clippedVerts=nullptr, *clippedLines=nullptr;
  vtkCellArray *clippedPolys=nullptr, *clippedList=nullptr;
  vtkPoints *newPoints;
  vtkIdList *cellIds;
  double s;
  vtkIdType estimatedSize, numCells=input->GetNumberOfCells();
  vtkIdType numPts=input->GetNumberOfPoints();
  vtkPoints *inPts=input->GetPoints();
  int numberOfPoints;
  vtkPointData *inPD=input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData *inCD=input->GetCellData(), *outCD = output->GetCellData();
  vtkCellData *outClippedCD = nullptr;

  vtkDebugMacro(<< "Clipping polygonal data");

  // Initialize self; create output objects
  //
  if ( numPts < 1 || inPts == nullptr )
  {
    vtkDebugMacro(<<"No data to clip");
    return 1;
  }

  if ( ClipFunctions.size() < 1 && this->GenerateClipScalars )
  {
    vtkErrorMacro(<<"Cannot generate clip scalars if no clip function defined");
    return 1;
  }

  // Determine whether we're clipping with input scalars or a clip function
  // and to necessary setup.
  if ( ClipFunctions.size() > 0 )
  {
    vtkFloatArray *tmpScalars = vtkFloatArray::New();
    tmpScalars->SetNumberOfTuples(numPts);
    inPD = vtkPointData::New();
    inPD->ShallowCopy(input->GetPointData());//copies original
    if ( this->GenerateClipScalars )
    {
      inPD->SetScalars(tmpScalars);
    }
    for ( i=0; i < numPts; i++ )
    {
      double maxValue = VTK_DOUBLE_MIN;
      for( int j = 0; j < ClipFunctions.size(); ++j )
      {
        maxValue = std::max( ClipFunctions[j]->FunctionValue(inPts->GetPoint(i)), maxValue );
      }
      tmpScalars->SetComponent(i,0,maxValue);
    }
    clipScalars = tmpScalars;
  }
  else //using input scalars
  {
    clipScalars = inPD->GetScalars();
    if ( !clipScalars )
    {
      vtkErrorMacro(<<"Cannot clip without clip function or input scalars");
      return 1;
    }
  }

  // Create objects to hold output of clip operation
  //
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  newPoints = vtkPoints::New();

  // Set the desired precision for the points in the output.
  if(this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPoints->SetDataType(input->GetPoints()->GetDataType());
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPoints->SetDataType(VTK_FLOAT);
  }
  else if(this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(VTK_DOUBLE);
  }

  newPoints->Allocate(numPts,numPts/2);
  newVerts = vtkCellArray::New();
  newVerts->AllocateEstimate(estimatedSize, 1);
  newLines = vtkCellArray::New();
  newLines->AllocateEstimate(estimatedSize, 2);
  newPolys = vtkCellArray::New();
  newPolys->AllocateEstimate(estimatedSize, 4);

  // locator used to merge potentially duplicate points
  if ( this->Locator == nullptr )
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion (newPoints, input->GetBounds());

  if ( !this->GenerateClipScalars && !input->GetPointData()->GetScalars())
  {
    outPD->CopyScalarsOff();
  }
  else
  {
    outPD->CopyScalarsOn();
  }
  outPD->InterpolateAllocate(inPD,estimatedSize,estimatedSize/2);
  outCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);

  // If generating second output, setup clipped output
  if ( this->GenerateClippedOutput )
  {
    this->GetClippedOutput()->Initialize();
    outClippedCD = this->GetClippedOutput()->GetCellData();
    outClippedCD->CopyAllocate(inCD,estimatedSize,estimatedSize/2);
    clippedVerts = vtkCellArray::New();
    clippedVerts->AllocateEstimate(estimatedSize, 1);
    clippedLines = vtkCellArray::New();
    clippedLines->AllocateEstimate(estimatedSize, 2);
    clippedPolys = vtkCellArray::New();
    clippedPolys->AllocateEstimate(estimatedSize, 4);
  }

  cellScalars = vtkFloatArray::New();
  cellScalars->Allocate(VTK_CELL_SIZE);

  // perform clipping on cells
  bool abort = false;
  updateTime = numCells/20 + 1;  // update roughly every 5%
  cell = vtkGenericCell::New();
  for (cellId=0; cellId < numCells && !abort; cellId++)
  {
    input->GetCell(cellId,cell);
    cellPts = cell->GetPoints();
    cellIds = cell->GetPointIds();
    numberOfPoints = cellPts->GetNumberOfPoints();

    // evaluate implicit cutting function
    for ( i=0; i < numberOfPoints; i++ )
    {
      s = clipScalars->GetComponent(cellIds->GetId(i),0);
      cellScalars->InsertTuple(i, &s);
    }

    switch ( cell->GetCellDimension() )
    {
      case 0: //points are generated-------------------------------
        connList = newVerts;
        clippedList = clippedVerts;
        break;

      case 1: //lines are generated----------------------------------
        connList = newLines;
        clippedList = clippedLines;
        break;

      case 2: //triangles are generated------------------------------
        connList = newPolys;
        clippedList = clippedPolys;
        break;

    } //switch

    cell->Clip(this->Value, cellScalars, this->Locator, connList, inPD, outPD, inCD, cellId, outCD,
      this->InsideOut);

    if ( this->GenerateClippedOutput )
    {
      cell->Clip(this->Value, cellScalars, this->Locator, clippedList, inPD, outPD, inCD, cellId, 
        outClippedCD, !this->InsideOut);
    }

    if ( !(cellId % updateTime) )
    {
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->CheckAbort();
    }
  } //for each cell
  cell->Delete();

  vtkDebugMacro(<<"Created: " << newPoints->GetNumberOfPoints() << " points, "
               << newVerts->GetNumberOfCells() << " verts, " << newLines->GetNumberOfCells()
               << " lines, " << newPolys->GetNumberOfCells() << " polys");

  if ( this->GenerateClippedOutput )
  {
    vtkDebugMacro(<<"Created (clipped output): " << clippedVerts->GetNumberOfCells() << " verts, "
                 << clippedLines->GetNumberOfCells() << " lines, "
                 << clippedPolys->GetNumberOfCells() << " triangles");
  }

  // Update ourselves.  Because we don't know upfront how many verts, lines,
  // polys we've created, take care to reclaim memory.
  //
  if ( ClipFunctions.size() > 0 )
  {
    clipScalars->Delete();
    inPD->Delete();
  }

  if (newVerts->GetNumberOfCells())
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();

  if (newLines->GetNumberOfCells())
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  if (newPolys->GetNumberOfCells())
  {
    output->SetPolys(newPolys);
  }
  newPolys->Delete();

  if ( this->GenerateClippedOutput )
  {
    this->GetClippedOutput()->SetPoints(newPoints);

    if (clippedVerts->GetNumberOfCells())
    {
      this->GetClippedOutput()->SetVerts(clippedVerts);
    }
    clippedVerts->Delete();

    if (clippedLines->GetNumberOfCells())
    {
      this->GetClippedOutput()->SetLines(clippedLines);
    }
    clippedLines->Delete();

    if (clippedPolys->GetNumberOfCells())
    {
      this->GetClippedOutput()->SetPolys(clippedPolys);
    }
    clippedPolys->Delete();

    this->GetClippedOutput()->GetPointData()->PassData(outPD);
    this->GetClippedOutput()->Squeeze();
  }

  output->SetPoints(newPoints);
  newPoints->Delete();
  cellScalars->Delete();

  this->Locator->Initialize();//release any extra memory
  output->Squeeze();

  return 1;
}

//------------------------------------------------------------------------------
// Specify a spatial locator for merging points. By default,
// an instance of vtkMergePoints is used.
void iAvtkClipPolyData::SetLocator(vtkIncrementalPointLocator *locator)
{
  if ( this->Locator == locator)
  {
    return;
  }

  if ( this->Locator )
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  if ( locator )
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

//------------------------------------------------------------------------------
void iAvtkClipPolyData::CreateDefaultLocator()
{
  if ( this->Locator == nullptr )
  {
    this->Locator = vtkMergePoints::New();
  }
}

//------------------------------------------------------------------------------
void iAvtkClipPolyData::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (!this->ClipFunctions.empty())
  {
    for (size_t i = 0; i < this->ClipFunctions.size(); ++i)
    {
      os << indent << "Clip Function (" << i << "): " << this->ClipFunctions[i] << "\n";
    }
  }
  else
  {
    os << indent << "Clip Function: (none)\n";
  }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  if ( this->Locator )
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Generate Clip Scalars: " << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " << (this->GenerateClippedOutput ? "On\n" : "Off\n");

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
//VTK_ABI_NAMESPACE_END
