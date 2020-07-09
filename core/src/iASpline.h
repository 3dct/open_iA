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

#include <vtkActor.h>
#include <vtkParametricSpline.h>
#include <vtkParametricFunctionSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

class iASpline
{
public:
	iASpline()
	{
		m_points = vtkSmartPointer<vtkPoints>::New();
		m_points->SetDataType(VTK_DOUBLE);
		m_parametricFuncSrc = vtkSmartPointer<vtkParametricFunctionSource>::New();
		m_spline = vtkSmartPointer<vtkParametricSpline>::New();
		m_splineMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		m_splineActor = vtkSmartPointer<vtkActor>::New();

		m_spline->ParameterizeByLengthOn();
		m_parametricFuncSrc->SetScalarModeToNone();
		m_parametricFuncSrc->GenerateTextureCoordinatesOff();
		m_parametricFuncSrc->SetUResolution( 499 );
		//m_splineMapper->ImmediateModeRenderingOn();
		m_splineMapper->SetResolveCoincidentTopologyToPolygonOffset();
		m_splineActor->GetProperty()->SetColor(0.0,0.0,1.0);
	}

	void addPoint(double * pos)
	{
		// add point to the spline points
		m_points->InsertNextPoint(pos);

		// if number of point is at least 2 create, initialize and show the spline curve
		if (m_points->GetNumberOfPoints() == 2)
		{
			m_spline->SetPoints(m_points);
			m_spline->Modified();

			m_parametricFuncSrc->SetParametricFunction(m_spline);
			m_parametricFuncSrc->Update();

			m_splineMapper->SetInputConnection( m_parametricFuncSrc->GetOutputPort() ) ;
			m_splineActor->SetMapper( m_splineMapper );
			m_splineActor->SetVisibility(true);
		}
		// otherwise only update spline curve
		else if (m_points->GetNumberOfPoints() > 2)
			Modified();
	}

	void Modified()
	{
		m_spline->Modified();
	}

	vtkActor* GetActor() const
	{
		return m_splineActor;
	}

	void SetVisibility(int visibility)
	{
		m_splineActor->SetVisibility(visibility);
	}

	void Reset()
	{
		m_points->Reset();
	}

	vtkIdType GetNumberOfPoints() const
	{
		return m_points->GetNumberOfPoints();
	}

	inline void SetPoint(vtkIdType id, double x, double y, double z)
	{
		m_points->SetPoint(id, x, y, z);
	}

	inline void SetPoint(vtkIdType id, const double pos[3] )
	{
		m_points->SetPoint(id, pos);
	}

protected:
	vtkSmartPointer<vtkPoints>						m_points;			// container for all spline points
	vtkSmartPointer<vtkActor>						m_splineActor;
	vtkSmartPointer<vtkPolyDataMapper>				m_splineMapper;
	vtkSmartPointer<vtkParametricSpline>			m_spline;			// actual spline
	vtkSmartPointer<vtkParametricFunctionSource>	m_parametricFuncSrc;// source for spline function
};
