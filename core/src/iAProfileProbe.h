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

#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkProbeFilter.h>
#include <vtkLineSource.h>

struct iAProfileProbe
{
public:
	iAProfileProbe(vtkImageData * imageData) :
		m_lineSrc(vtkSmartPointer<vtkLineSource>::New()),
		m_probe(vtkSmartPointer<vtkProbeFilter>::New())
	{
		m_lineSrc->SetResolution(1500);
		m_probe->SetInputConnection(m_lineSrc->GetOutputPort());
		m_probe->SetSourceData(imageData);
		m_profileData = m_probe->GetPolyDataOutput();
	}
	void updateProbe(int ptIndex, double const * const newPos)
	{
		for (int i = 0; i < 3; ++i)
		{
			m_positions[ptIndex][i] = newPos[i];
		}
		m_lineSrc->SetPoint1(m_positions[0]);
		m_lineSrc->SetPoint2(m_positions[1]);
	}
	void updateData()
	{
		m_probe->Update();
		m_profileData = m_probe->GetPolyDataOutput();
	}
	double rayLength() const
	{
		double comps[3] = { m_positions[1][0] - m_positions[0][0], m_positions[1][1] - m_positions[0][1], m_positions[1][2] - m_positions[0][2]};
		double sqrLen = 0;
		for (int i = 0; i < 3; ++i)
		{
			sqrLen += comps[i] * comps[i];
		}
		return sqrtl(sqrLen);
	}

public:
	vtkSmartPointer<vtkLineSource> m_lineSrc;
	vtkSmartPointer<vtkProbeFilter> m_probe;
	vtkPolyData *m_profileData;
	double m_positions[2][3];
};
