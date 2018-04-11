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
#include "iABlobImplicitFunction.h"

#include "iABlobCluster.h"
#include "iABlobManager.h"

#include <vtkObjectFactory.h>

vtkStandardNewMacro (iABlobImplicitFunction);

// Construct sphere with center at (0,0,0) and radius=0.5.
iABlobImplicitFunction::iABlobImplicitFunction()
{
	m_blobManager = 0;

	mb = NULL;
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
		mb = NULL;
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


void iABlobImplicitFunction::AddFiberInfo (double point1[3],
										   double point2[3],
										   double g)
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
	this->mb[this->mbCount].point1[0] = point1[0];
	this->mb[this->mbCount].point1[1] = point1[1];
	this->mb[this->mbCount].point1[2] = point1[2];

	this->mb[this->mbCount].point2[0] = point2[0];
	this->mb[this->mbCount].point2[1] = point2[1];
	this->mb[this->mbCount].point2[2] = point2[2];

	this->mb[this->mbCount].strength = g;

	this->mbCount++;
}

void iABlobImplicitFunction::AddFiberInfo (double x1, double y1, double z1,
										   double x2, double y2, double z2,
										   double g)
{
	double pos1[3] = {x1, y1, z1}, pos2[3] = {x2, y2, z2};
	AddFiberInfo (pos1, pos2, g);
}

double iABlobImplicitFunction::JustEvaluateFunction (double x[3])
{
	LineInfo* line;
	double value = 0;
	double pValue, pDist;
	if (this->mbCount == 0)
		return 0;

	line = this->mb;
	for (unsigned int i = 0; i < this->mbCount; i++, line++)
	{
		pValue = line->strength;
		pDist = DistancePointToLine (line->point1, line->point2, x);
		if(0 == i)
			value = pDist;
		else if(pDist <= value)
			value = pDist;
		//value += (pValue / pDist);
	}

	//printf("EvalFun: %lf, %lf, %lf = %lf\n", x[0], x[1], x[2], value);

	return value;
}

// Evaluate metaball equation
double iABlobImplicitFunction::EvaluateFunction (double x[3])
{
	double value = JustEvaluateFunction (x);

	if (m_blobManager != NULL)
	{
		QList<iABlobCluster*>* list = m_blobManager->GetListObBlobClusters();
		double otherVal;
		for (int i = 0; i < list->count(); i++)
		{
			iABlobImplicitFunction* otherFunc = list->at(i)->GetImplicitFunction ();
			if (otherFunc != this)
			{
				otherVal = otherFunc->JustEvaluateFunction(x);
				if (abs (value - otherVal) < 0.025)
					return value * (  abs(value-otherVal)/0.025  );
			}
		}
	}

	return value;
}

double iABlobImplicitFunction::EvaluateFunction (double x,
												 double y,
												 double z)
{
	double pos[3] = {x, y, z};
	return EvaluateFunction (pos);
}

// Evaluate sphere gradient.
void iABlobImplicitFunction::EvaluateGradient (double x[3], double n[3])
{
	LineInfo* line;
	double pValue;
//	double pPoint[3];
	n[1] = n[2] = n[0] = 0;

	if (this->mbCount == 0)
		return;

	line = this->mb;
	for (unsigned int i = 0; i < this->mbCount; i++, line++)
	{
		pValue = line->strength;
		n[0] += (pValue / ( (x[0] - line->point1[0]) * (x[0] - line->point1[0])));
		n[1] += (pValue / ( (x[1] - line->point1[1]) * (x[1] - line->point1[1])));
		n[2] += (pValue / ( (x[2] - line->point1[2]) * (x[2] - line->point1[2])));
	}
}

void iABlobImplicitFunction::PrintSelf (ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf (os, indent);
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
