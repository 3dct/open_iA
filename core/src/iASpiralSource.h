/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iARegularPolygonSourceEx.h"

enum CylindricityType
{
	Helix,
	Circles,
};
class iACylindricitySource : public iARegularPolygonSourceEx
{
public:
	// Standard methods for instantiation, obtaining type and printing instance values.
	static iACylindricitySource *New();
	vtkTypeMacro(iACylindricitySource, iARegularPolygonSourceEx);

	//setters/getters
	vtkSetMacro(Height, double);
	vtkGetMacro(Height, double);

	vtkSetMacro(NumWinds, int);
	vtkGetMacro(NumWinds, int);

	CylindricityType GetType() {return m_type;}
	void SetType(CylindricityType type) {m_type = type;}

protected:
	iACylindricitySource() : iARegularPolygonSourceEx(), m_type(Circles), NumWinds(0), Height(1.0) {}
	~iACylindricitySource() {}

	int RequestData(
		vtkInformation *vtkNotUsed(request),
		vtkInformationVector **vtkNotUsed(inputVector),
		vtkInformationVector *outputVector)
	{
		if (!NumWinds)
		{
			return 1;
		}
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
		int i, j;
		vtkPoints *newPoints;

		// Prepare to produce the output; create the connectivity array(s)
		newPoints = vtkPoints::New();
		newPoints->Allocate(NumberOfSides);

		int numInWind = NumberOfSides/NumWinds;
		if ( this->GeneratePolyline )
		{
			vtkCellArray * newLine = vtkCellArray::New();
			switch (m_type)
			{
			case Helix:
				newLine->Allocate(newLine->EstimateSize(1,NumberOfSides));
				newLine->InsertNextCell(NumberOfSides);
				for (i=0; i<NumberOfSides; i++)
					newLine->InsertCellPoint(i);
				break;
			case Circles:
				newLine->Allocate(newLine->EstimateSize(NumWinds, numInWind));
				for (j=0; j<NumWinds; j++)
				{
					newLine->InsertNextCell(numInWind+1);
					for (i=0; i<numInWind; i++)
						newLine->InsertCellPoint(j*numInWind + i);
					newLine->InsertCellPoint(j*numInWind); //close the polyline
				}
				break;
			}
			output->SetLines(newLine);
			newLine->Delete();
		}

		// Produce a unit vector in the plane of the polygon (i.e., perpendicular
		// to the normal)
		// Make sure the polygon normal is a unit vector
		double shiftVec[3] = {Normal[0], Normal[1], Normal[2]};
		vtkMath::MultiplyScalar(shiftVec, Height/NumberOfSides);

		//points
		double tVec1[3], tVec2[3];
		double Vec1[3], Vec2[3];
		vtkMath::Subtract(Point2, Point1, Vec1);
		vtkMath::Cross(Vec1, Normal, Vec2);
		vtkMath::Normalize(Vec1); vtkMath::Normalize(Vec2);

		double angleDelta = 2.0 * vtkMath::Pi() * NumWinds / NumberOfSides;
		// Cross with unit axis vectors and eventually find vector in the polygon plane
		for (j=0; j<NumberOfSides; j++)
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
			int shiftCoef = 0;
			switch (m_type)
			{
			case Helix:
				shiftCoef = j;
				break;
			case Circles:
				shiftCoef = (j/numInWind)*numInWind;
				break;
			}
			for (i=0; i<3; i++)
				x[i] = Center[i] - shiftCoef*shiftVec[i] + tVec1[i] + tVec2[i];
			newPoints->InsertNextPoint(x);
		}
		output->SetPoints(newPoints);
		newPoints->Delete();

		return 1;
	}

private:
	iACylindricitySource(const iACylindricitySource&);  // Not implemented.
	void operator=(const iACylindricitySource&);  // Not implemented.
protected:
		double Height;
		int NumWinds;
		CylindricityType m_type;
};
vtkStandardNewMacro(iACylindricitySource);
