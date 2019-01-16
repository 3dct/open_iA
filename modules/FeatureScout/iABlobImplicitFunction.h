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

#include <vtkImplicitFunction.h>

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
	void PrintSelf (ostream& os, vtkIndent indent) override;

	static iABlobImplicitFunction* New();

	// Description:
	// Evaluate function at position x-y-z and return value
	double EvaluateFunction (double x[3]) override;
	double EvaluateFunction (double x, double y, double z) override;
	double JustEvaluateFunction (double x[3]);

	// Description:
	// Evaluate function gradient at position x-y-z and pass back vector.
	void EvaluateGradient (double x[3], double n[3]) override;

	// Description:
	// Add fiber to current cluster
	void AddObjectInfo (double point1[3], double point2[3], double g);
	void AddObjectInfo (double x1, double y1, double z1,
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
