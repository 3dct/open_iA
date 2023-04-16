// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include "iADataSetRenderer.h"

#include <vtkSmartPointer.h>

class iAGraphData;
class iAPolyData;

class vtkActor;
class vtkGlyph3DMapper;
class vtkPolyDataMapper;
class vtkSphereSource;
class vtkTubeFilter;

//! 3D renderer for graph data, with options to adapt node and vertex size and color.
class iAGraphRenderer : public iADataSetRenderer
{
public:
	iAGraphRenderer(vtkRenderer* renderer, iAGraphData const * data);
	~iAGraphRenderer();
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

	iAAttributes const & attributes() const override;
	static iAAttributes& defaultAttributes();

	vtkSmartPointer<vtkActor> m_lineActor, m_pointActor;
	vtkSmartPointer<vtkSphereSource> m_sphereSource;
	vtkSmartPointer<vtkGlyph3DMapper> m_glyphMapper;
	vtkSmartPointer<vtkTubeFilter> m_tubeFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_lineMapper;

	iAGraphData const * m_data;

	static bool registerDefaultAttributes();
	static const bool m_sDefaultAttr;
};

//! 3D renderer for any kind of polydata.
class iAguibase_API iAPolyActorRenderer : public iADataSetRenderer
{
public:
	iAPolyActorRenderer(vtkRenderer* renderer);
	~iAPolyActorRenderer();
	void applyAttributes(QVariantMap const& values) override;
	double const* orientation() const override;
	double const* position() const override;
	void setPosition(double pos[3]) override;
	void setOrientation(double ori[3]) override;
	vtkProp3D* vtkProp() override;
	vtkPolyDataMapper* mapper();
	vtkActor* actor();

protected:
	vtkSmartPointer<vtkActor> m_polyActor;

private:
	void showDataSet() override;
	void hideDataSet() override;
	iAAttributes const& attributes() const override;
	static iAAttributes& defaultAttributes();
	Q_DISABLE_COPY(iAPolyActorRenderer);

	static bool registerDefaultAttributes();
	static const bool m_sDefaultAttr;
};

//! 3D renderer for surface mesh data.
class iAguibase_API iAPolyDataRenderer : public iAPolyActorRenderer
{
public:
	iAPolyDataRenderer(vtkRenderer* renderer, iAPolyData const * data);
	iAAABB bounds() override;
private:
	iAPolyData const * m_data;
	Q_DISABLE_COPY(iAPolyDataRenderer);
};

class iAGeometricObject;

//! 3D renderer for simple geometric objects (sphere, cube, ...).
class iAGeometricObjectRenderer : public iAPolyActorRenderer
{
public:
	iAGeometricObjectRenderer(vtkRenderer* renderer, iAGeometricObject const * data);
	iAAABB bounds() override;
private:
	iAGeometricObject const * m_data;
	Q_DISABLE_COPY(iAGeometricObjectRenderer);
};
