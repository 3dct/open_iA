/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iADataSetRenderer.h"

#include <vtkSmartPointer.h>

class iAGraphData;
class iAPolyData;

class vtkActor;
class vtkGlyph3DMapper;
class vtkPolyDataMapper;
class vtkSphereSource;
class vtkTubeFilter;

class iAGraphRenderer : public iADataSetRenderer
{
public:
	iAGraphRenderer(vtkRenderer* renderer, iAGraphData const * data);
	void updatePointRendererPosOri();
	void applyAttributes(QVariantMap const& values) override;
	iAAABB bounds() override;
	double const* orientation() const override;
	double const* position() const override;
	void setPosition(double pos[3]) override;
	void setOrientation(double ori[3]) override;
	vtkProp3D* vtkProp() override;

private:
	void showDataSet() override;
	void hideDataSet() override;

	vtkSmartPointer<vtkActor> m_lineActor, m_pointActor;
	vtkSmartPointer<vtkSphereSource> m_sphereSource;
	vtkSmartPointer<vtkGlyph3DMapper> m_glyphMapper;
	vtkSmartPointer<vtkTubeFilter> m_tubeFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_lineMapper;

	iAGraphData const * m_data;
};

class iAPolyActorRenderer : public iADataSetRenderer
{
public:
	iAPolyActorRenderer(vtkRenderer* renderer);
	void applyAttributes(QVariantMap const& values) override;
	double const* orientation() const override;
	double const* position() const override;
	void setPosition(double pos[3]) override;
	void setOrientation(double ori[3]) override;
	vtkProp3D* vtkProp() override;

protected:
	vtkSmartPointer<vtkActor> m_polyActor;

private:
	void showDataSet() override;
	void hideDataSet() override;
};

class iAPolyDataRenderer : public iAPolyActorRenderer
{
public:
	iAPolyDataRenderer(vtkRenderer* renderer, iAPolyData const * data);
	iAAABB bounds() override;
private:
	iAPolyData const * m_data;
};

class iAGeometricObject;

class iAGeometricObjectRenderer : public iAPolyActorRenderer
{
public:
	iAGeometricObjectRenderer(vtkRenderer* renderer, iAGeometricObject const * data);
	iAAABB bounds() override;
private:
	iAGeometricObject const * m_data;
};
