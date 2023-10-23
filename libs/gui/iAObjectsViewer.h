// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <iADataSetRenderer.h>
#include <iADataSetViewer.h>

#include "iagui_export.h"

class iAObjectsData;
class iAObjectVis;
class iAObjectVisActor;

//! 3D Renderer for objects data in the iADataSetViewer framework
class iAgui_API iAObjectsRenderer : public iADataSetRenderer
{
public:
	iAObjectsRenderer(vtkRenderer* renderer, iAObjectsData const * data, QVariantMap const& overrideValues);
	~iAObjectsRenderer();
	void applyAttributes(QVariantMap const& values) override;
	virtual iAAABB bounds() override;
	double const* orientation() const override;
	double const* position() const override;
	void setPosition(double pos[3]) override;
	void setOrientation(double ori[3]) override;
	vtkProp3D* vtkProp() override;
	static iAAttributes& defaultAttributes();

private:
	void showDataSet() override;
	void hideDataSet() override;
	iAAttributes const& attributes() const override;
	Q_DISABLE_COPY(iAObjectsRenderer);

	std::shared_ptr<iAObjectVis> m_objVis;
	std::shared_ptr<iAObjectVisActor> m_objActor;
};

//! Viewer for objects data in the iADataSetViewer framework
class iAgui_API iAObjectsViewer : public iADataSetViewer
{
public:
	iAObjectsViewer(iADataSet* dataSet);
	std::shared_ptr<iADataSetRenderer> createRenderer(vtkRenderer* ren, QVariantMap const& paramValues) override;
private:
	static bool s_registered;
};
