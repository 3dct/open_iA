/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenb�ck, *
*                     Artem & Alexander Amirkhanov, B. Fr�hler                        *
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
* Contact: FH O� Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstra�e 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 

// .NAME vtkMetaballs - implicit function for a sphere
// .SECTION Description
// vtkMetaballs computes the implicit function and/or gradient for a sphere.
// vtkMetaballs is a concrete implementation of vtkImplicitFunction.

#pragma once
#ifndef IABLOBIMPLICITFUNCTION_H
#define IABLOBIMPLICITFUNCTION_H

#include <vtkAlgorithm.h>
#include <vtkImplicitFunction.h>
#include <vtkPoints.h>
#include <vtkDoubleArray.h>
#include <vtkPlaneCollection.h>

class iABlobManager;
class iABlobCluster;

typedef struct
{
	double point1[3];	// point of the line
	double point2[3];	// point of the line
	double strength;	// strength of field
} LineInfo;


class iABlobImplicitFunction : public vtkImplicitFunction
{
public:
	vtkTypeMacro (iABlobImplicitFunction, vtkImplicitFunction);
	void PrintSelf (ostream& os, vtkIndent indent);

	static iABlobImplicitFunction* New();

	// Description:
	// Evaluate function at position x-y-z and return value
	double	EvaluateFunction (double x[3]);
	double	EvaluateFunction (double x, double y, double z);
	double	JustEvaluateFunction (double x[3]);

	// Description:
	// Evaluate function gradient at position x-y-z and pass back vector.
	void	EvaluateGradient (double x[3], double n[3]);

	// Description:
	// Add fiber to current cluster
	void	AddFiberInfo (double point1[3], double point2[3], double g);
	void	 AddFiberInfo (double x1, double y1, double z1,
	                       double x2, double y2, double z2, double g);
	// Description
	// Reset information about fibres
	void	Reset();

	// Description
	// Get center of cluster
	void	GetCenter (double center[3]);

	void	SetBlobManager (iABlobManager* blobManager);

protected:
	iABlobImplicitFunction();
	~iABlobImplicitFunction();

	void Allocate (const vtkIdType sz, const vtkIdType ext = 1000);
	void Resize (const vtkIdType ext);

	LineInfo* mb;
	unsigned int mbCount, mbSize;
	unsigned int mbExt;
	double	m_center[3];

	iABlobManager* m_blobManager;

private:
	// Description:
	// Calculate distance between point and line
	double DistancePointToLine (double l1[3], double l2[3], double p[3]);

	iABlobImplicitFunction (const iABlobImplicitFunction&);		// Not implemented
	void operator= (const iABlobImplicitFunction&);	// Not implemented
};

#endif // __iAVtkBlobImplicitFunction_h