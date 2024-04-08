// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkImageData.h>
#include <vtkLineSource.h>
#include <vtkPointData.h>
#include <vtkProbeFilter.h>

//! Retrieves a line profile from a specified image dataset.
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
	vtkIdType numberOfPoints() const
	{
		return m_profileData->GetNumberOfPoints();
	}
	vtkDataArray* scalars() const
	{
		return m_profileData->GetPointData()->GetScalars();
	}
private:
	vtkSmartPointer<vtkLineSource> m_lineSrc;
	vtkSmartPointer<vtkProbeFilter> m_probe;
	vtkPolyData* m_profileData;
	double m_positions[2][3];
};
