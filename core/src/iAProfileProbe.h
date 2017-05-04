/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkProbeFilter.h>
#include <vtkLineSource.h>

struct iAProfileProbe
{
public:
	iAProfileProbe(double * startPos, double * endPos, vtkImageData * imageData)
	{
		for (int i=0; i<3; i++)
		{
			positions[0][i] = startPos[i];
			positions[1][i] = endPos[i];
		}
		lineSrc = vtkLineSource::New();
		lineSrc->SetResolution(1500);
		lineSrc->SetPoint1(positions[0]);
		lineSrc->SetPoint2(positions[1]);
		probe = vtkProbeFilter::New();
		probe->SetInputConnection(lineSrc->GetOutputPort());
		probe->SetSourceData(imageData);
		probe->Update();
		profileData = probe->GetPolyDataOutput();
	}
	~iAProfileProbe()
	{
		probe->Delete();
		lineSrc->Delete();
	}
	void UpdateProbe(int ptIndex, double * newPos)
	{
		for (int i=0; i<3; i++)
			positions[ptIndex][i] = newPos[i];
		lineSrc->SetPoint1(positions[0]);
		lineSrc->SetPoint2(positions[1]);
		probe->Update();
		profileData = probe->GetPolyDataOutput();
	}
	double GetRayLength()
	{
		double comps[3] = {positions[1][0]-positions[0][0], positions[1][1]-positions[0][1], positions[1][2]-positions[0][2]};
		double sqrLen = 0;
		for (int i=0; i<3; i++)
			sqrLen += comps[i] * comps[i];
		return sqrtl(sqrLen);		
	}

public:
	vtkLineSource * lineSrc;
	vtkProbeFilter * probe;
	vtkPolyData * profileData;
	double positions[2][3];
};
