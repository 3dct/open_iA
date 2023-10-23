// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectsViewer.h"

#include <iAAABB.h>

#include <iAObjectsData.h>

#include "iADefaultSettings.h"
#include <iADataSetViewerImpl.h>    // for dataSetViewerFactoryMap

iAObjectsViewer::iAObjectsViewer(iADataSet* dataSet) :
	iADataSetViewer(dataSet)
{
}

std::shared_ptr<iADataSetRenderer> iAObjectsViewer::createRenderer(vtkRenderer* ren, QVariantMap const& paramValues)
{
	auto objData = dynamic_cast<iAObjectsData const*>(m_dataSet);
	return std::make_shared<iAObjectsRenderer>(ren, objData, paramValues);
}

bool iAObjectsViewer::s_registered = []() {
	dataSetViewerFactoryMap().insert(std::make_pair(iADataSetType::Objects, createFunc<iAObjectsViewer>));
	return true;
}();



#include <iAObjectVis.h>
#include <iAObjectVisFactory.h>
#include <iAPolyObjectVisActor.h>

#include <vtkActor.h>

iAObjectsRenderer::iAObjectsRenderer(vtkRenderer* renderer, iAObjectsData const* data, QVariantMap const& overrideValues):
	iADataSetRenderer(renderer),
	m_objVis(create3DObjectVis(data, QColor(255, 0, 0), 12, 1, nullptr, nullptr, nullptr)),
	m_objActor(m_objVis->createActor(renderer))
{
	Q_UNUSED(overrideValues)
}

iAObjectsRenderer::~iAObjectsRenderer()
{
}

void iAObjectsRenderer::applyAttributes(QVariantMap const& values)
{
	// apply color!
}

iAAABB iAObjectsRenderer::bounds()
{
	return m_objVis->bounds();
}

double const* iAObjectsRenderer::orientation() const
{
	return dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor()->GetOrientation();
}

double const* iAObjectsRenderer::position() const
{
	return dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor()->GetPosition();
}

void iAObjectsRenderer::setPosition(double pos[3])
{
	dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor()->SetPosition(pos);
}

void iAObjectsRenderer::setOrientation(double ori[3])
{
	dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor()->SetOrientation(ori);
}

vtkProp3D* iAObjectsRenderer::vtkProp()
{
	return dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor();
}


constexpr const char ObjectsRendererName[] = "Default Settings/Dataset Renderer: Objects";

class iAObjectsRendererSettings : iASettingsObject<ObjectsRendererName, iAObjectsRendererSettings>
{
public:
	static iAAttributes& defaultAttributes()
	{
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			addAttr(attr, "Color", iAValueType::Color, QColor(255, 0, 0));
		}
		return attr;
	}
};

iAAttributes& iAObjectsRenderer::defaultAttributes()
{
	static iAAttributes& attr = iAObjectsRendererSettings::defaultAttributes();
	return attr;
}

void iAObjectsRenderer::showDataSet()
{
	m_objActor->show();
}

void iAObjectsRenderer::hideDataSet()
{
	m_objActor->hide();
}

iAAttributes const& iAObjectsRenderer::attributes() const
{
	static iAAttributes attr;
	if (attr.isEmpty())
	{
		attr = cloneAttributes(iAObjectsRenderer::defaultAttributes());
	}
	return attr;
}
