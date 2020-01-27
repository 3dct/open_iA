/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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

#include <vtkRegularPolygonSource.h>
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkObjectFactory.h"
#include "vtkMath.h"

class iARegularPolygonSourceEx : public vtkRegularPolygonSource
{
public:
	// Description:
	// Standard methods for instantiation, obtaining type and printing instance values.
	static iARegularPolygonSourceEx *New();
	vtkTypeMacro(iARegularPolygonSourceEx, vtkRegularPolygonSource);

	// Description:
	// Set/Get the Point1
	// origin (0,0,0).
	vtkSetVector3Macro(Point1,double);
	vtkGetVectorMacro(Point1,double,3);
	// Description:
	// Set/Get the Point2
	// origin (0,0,0).
	vtkSetVector3Macro(Point2,double);
	vtkGetVectorMacro(Point2,double,3);

protected:
	iARegularPolygonSourceEx() : vtkRegularPolygonSource()
	{
		Point1[0] = Point1[1] = Point1[2] = 0.0;
		Point2[0] = Point2[1] = Point2[2] = 0.0;
	}
	~iARegularPolygonSourceEx() {}

	int RequestData(
		vtkInformation *vtkNotUsed(request),
		vtkInformationVector **vtkNotUsed(inputVector),
		vtkInformationVector *outputVector) override
	{
		// Get the info object
		vtkInformation *outInfo = outputVector->GetInformationObject(0);

		// Get the output
		vtkPolyData *output = vtkPolyData::SafeDownCast(
			outInfo->Get(vtkDataObject::DATA_OBJECT()));

		// We only produce one piece
		if (outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
		{
			return 1;
		}

		double x[3];
		int i, j, numPts=this->NumberOfSides;
		vtkPoints *newPoints;

		// Prepare to produce the output; create the connectivity array(s)
		newPoints = vtkPoints::New();
		newPoints->Allocate(numPts);

		if ( this->GeneratePolyline )
		{
			vtkCellArray * newLine = vtkCellArray::New();
			newLine->Allocate(newLine->EstimateSize(1,numPts));
			newLine->InsertNextCell(numPts+1);
			for (i=0; i<numPts; i++)
			{
				newLine->InsertCellPoint(i);
			}
			newLine->InsertCellPoint(0); //close the polyline
			output->SetLines(newLine);
			newLine->Delete();
		}

		if ( this->GeneratePolygon )
		{
			vtkCellArray * newPoly = vtkCellArray::New();
			newPoly->Allocate(newPoly->EstimateSize(1,numPts));
			newPoly->InsertNextCell(numPts);
			for (i=0; i<numPts; i++)
			{
				newPoly->InsertCellPoint(i);
			}
			output->SetPolys(newPoly);
			newPoly->Delete();
		}

		//points
		double tVec1[3], tVec2[3];
		double Vec1[3], Vec2[3];
		vtkMath::Subtract(Point2, Point1, Vec1);
		vtkMath::Cross(Vec1, Normal, Vec2);
		vtkMath::Normalize(Vec1); vtkMath::Normalize(Vec2);

		double angleDelta = 2.0*vtkMath::Pi()/(numPts);
		// Cross with unit axis vectors and eventually find vector in the polygon plane
		for (j=0; j<numPts; j++)
		{
			for (int c=0; c<3; ++c)
			{
				tVec1[c] = Vec1[c];	tVec2[c] = Vec2[c];
			}
			double curAng = angleDelta*j;
			float cosine = std::cos(curAng);
			float sine = std::sin(curAng);
			vtkMath::MultiplyScalar(tVec1, Radius*cosine);
			vtkMath::MultiplyScalar(tVec2, Radius*sine);

			for (i=0; i<3; i++)
				x[i] = this->Center[i] + tVec1[i] + tVec2[i];
			newPoints->InsertNextPoint(x);
		}

		output->SetPoints(newPoints);
		newPoints->Delete();

		return 1;
	}

private:
	iARegularPolygonSourceEx(const iARegularPolygonSourceEx&);  // Not implemented.
	void operator=(const iARegularPolygonSourceEx&);  // Not implemented.
protected:
	double Point1[3], Point2[3];
};
vtkStandardNewMacro(iARegularPolygonSourceEx);
