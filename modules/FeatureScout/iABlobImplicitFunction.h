// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
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
	vtkIdType mbCount, mbSize, mbExt;
	double	m_center[3];

	iABlobManager* m_blobManager;

private:
	// Description:
	// Calculate distance between point and line
	double DistancePointToLine (double l1[3], double l2[3], double p[3]);

	iABlobImplicitFunction (const iABlobImplicitFunction&);		// Not implemented
	void operator= (const iABlobImplicitFunction&);	// Not implemented
};
