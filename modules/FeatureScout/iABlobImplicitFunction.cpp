// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iABlobImplicitFunction.h"

#include "iABlobCluster.h"
#include "iABlobManager.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro (iABlobImplicitFunction);

// Construct sphere with center at (0,0,0) and radius=0.5.
iABlobImplicitFunction::iABlobImplicitFunction()
{
	m_blobManager = nullptr;

	mb = nullptr;
	mbSize = 0;
	mbCount = 0;
	mbExt = 1000;

	Resize (mbExt);
}

iABlobImplicitFunction::~iABlobImplicitFunction()
{
	Reset();
}

void iABlobImplicitFunction::Allocate (const vtkIdType sz,
									   const vtkIdType ext)
{
	mbExt = ext;
	if (mb)
	{
		delete mb;
		mb = nullptr;
	}
	mbCount = 0;
	mbSize = 0;
	Resize (sz);
}


void iABlobImplicitFunction::Resize (const vtkIdType ext)
{
	LineInfo* ptr;

	ptr = new LineInfo[mbSize + ext];
	if (mb)
	{
		// copy old into new
		memcpy (ptr, mb, sizeof (LineInfo) * mbSize);
		delete mb;
	}

	mb = ptr;
	mbSize += ext;
}

void iABlobImplicitFunction::Reset (void)
{
	Allocate (1000, 1000);
}


void iABlobImplicitFunction::AddObjectInfo (double point1[3], double point2[3], double g)
{
	if (mbCount == mbSize - 1)
	{
		Resize (mbExt);
	}

	// calc center cluster
	for (int i = 0; i < 3; i++)
	{
		m_center[i] = (point1[i] + point2[i]) / 2;
	}

	// calc other params
	mb[mbCount].point1[0] = point1[0];
	mb[mbCount].point1[1] = point1[1];
	mb[mbCount].point1[2] = point1[2];

	mb[mbCount].point2[0] = point2[0];
	mb[mbCount].point2[1] = point2[1];
	mb[mbCount].point2[2] = point2[2];

	mb[mbCount].strength = g;

	++mbCount;
}

void iABlobImplicitFunction::AddObjectInfo (double x1, double y1, double z1, double x2, double y2, double z2, double g)
{
	double pos1[3] = {x1, y1, z1}, pos2[3] = {x2, y2, z2};
	AddObjectInfo (pos1, pos2, g);
}

double iABlobImplicitFunction::JustEvaluateFunction (double x[3])
{
	LineInfo* line;
	double value = 0;
	double pDist;
	if (mbCount == 0)
	{
		return 0;
	}

	line = mb;
	for (vtkIdType i = 0; i < mbCount; i++, line++)
	{
		pDist = DistancePointToLine (line->point1, line->point2, x);
		if (i == 0 || pDist <= value)
		{
			value = pDist;
		}
	}

	return value;
}

// Evaluate metaball equation
double iABlobImplicitFunction::EvaluateFunction (double x[3])
{
	double value = JustEvaluateFunction (x);

	if (m_blobManager)
	{
		QList<iABlobCluster*>* list = m_blobManager->GetListObBlobClusters();
		double otherVal;
		for (int i = 0; i < list->count(); i++)
		{
			iABlobImplicitFunction* otherFunc = list->at(i)->GetImplicitFunction ();
			if (otherFunc != this)
			{
				otherVal = otherFunc->JustEvaluateFunction(x);
				if (std::abs (value - otherVal) < 0.025)
					return value * (  std::abs(value-otherVal)/0.025  );
			}
		}
	}

	return value;
}

// Evaluate sphere gradient.
void iABlobImplicitFunction::EvaluateGradient (double x[3], double n[3])
{
	LineInfo* line;
	double pValue;
//	double pPoint[3];
	n[1] = n[2] = n[0] = 0;

	if (mbCount == 0)
	{
		return;
	}

	line = mb;
	for (vtkIdType i = 0; i < mbCount; i++, line++)
	{
		pValue = line->strength;
		n[0] += (pValue / ( (x[0] - line->point1[0]) * (x[0] - line->point1[0])));
		n[1] += (pValue / ( (x[1] - line->point1[1]) * (x[1] - line->point1[1])));
		n[2] += (pValue / ( (x[2] - line->point1[2]) * (x[2] - line->point1[2])));
	}
}

void iABlobImplicitFunction::PrintSelf (ostream& os, vtkIndent indent)
{
	Superclass::PrintSelf (os, indent);
}

double iABlobImplicitFunction::DistancePointToLine (double l1[3],
													double l2[3],
													double p[3])
{
	double nearPoint[3], dir[3];
	double t_min, distance;

	dir[0] = l2[0] - l1[0];
	dir[1] = l2[1] - l1[1];
	dir[2] = l2[2] - l1[2];

	t_min =
		(dir[0] * (p[0] - l1[0]) + dir[1] * (p[1] - l1[1]) + dir[2] * (p[2] - l1[2])) /
		(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);

	if (t_min < 0)
		t_min = 0;
	else if (t_min > 1)
		t_min = 1;

	nearPoint[0] = l1[0] + t_min * dir[0];
	nearPoint[1] = l1[1] + t_min * dir[1];
	nearPoint[2] = l1[2] + t_min * dir[2];

	distance =
		(p[0] - nearPoint[0]) * (p[0] - nearPoint[0]) +
		(p[1] - nearPoint[1]) * (p[1] - nearPoint[1]) +
		(p[2] - nearPoint[2]) * (p[2] - nearPoint[2]);

	return distance;
}

void iABlobImplicitFunction::GetCenter (double center[3])
{
	for (int i = 0; i < 3; i++)
	{
		center[i] = m_center[i];
	}
}

void iABlobImplicitFunction::SetBlobManager (iABlobManager* blobManager)
{
	m_blobManager = blobManager;
}
