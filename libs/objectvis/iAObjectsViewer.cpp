// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAObjectsViewer.h"

#include <iAAABB.h>

#include <iAColoredPolyObjectVis.h>
#include <iAObjectsData.h>
#include <iAObjectVisFactory.h>

#include <iADefaultSettings.h>
#include <iADataSetViewerImpl.h>    // for dataSetViewerFactoryMap

namespace
{
	QColor DefaultColor(64, 64, 64);
	const QString ColorParam("Color");
}

iAObjectsViewer::iAObjectsViewer(iADataSet* dataSet) :
	iADataSetViewer(dataSet),
	m_objVis(createObjectVis(dynamic_cast<iAObjectsData const*>(m_dataSet), DefaultColor, 12, 1))
{
}

std::shared_ptr<iADataSetRenderer> iAObjectsViewer::createRenderer(vtkRenderer* ren, QVariantMap const& paramValues)
{
	return std::make_shared<iAObjectsRenderer>(ren, m_objVis.get(), paramValues);
}

QString iAObjectsViewer::information() const
{
	auto polyObj = dynamic_cast<iAColoredPolyObjectVis*>(m_objVis.get());
	return m_dataSet->info() + (polyObj ? "\n" + polyObj->visualizationStatistics() : "");
}

iAObjectVis* iAObjectsViewer::objectVis()
{
	return m_objVis.get();
}

bool iAObjectsViewer::s_registered = []() {
	dataSetViewerFactoryMap().insert(std::make_pair(iADataSetType::Objects, createFunc<iAObjectsViewer>));
	return true;
}();



#include <iAObjectVis.h>
#include <iAPolyObjectVisActor.h>

#include <vtkActor.h>

iAObjectsRenderer::iAObjectsRenderer(vtkRenderer* renderer, iAObjectVis* objVis, QVariantMap const& overrideValues):
	iADataSetRenderer(renderer),
	m_objVis(objVis),
	m_objActor(objVis->createActor(renderer))
{
	setDefaultAttributes(defaultAttributes(), overrideValues);
}

iAObjectsRenderer::~iAObjectsRenderer()
{
}

void iAObjectsRenderer::applyAttributes(QVariantMap const& values)
{
	auto colorObj = dynamic_cast<iAColoredPolyObjectVis*>(m_objVis);
	if (colorObj)
	{
		colorObj->setColor(variantToColor(values[ColorParam]));
	}
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

class iAobjectvis_API iAObjectsRendererSettings : iASettingsObject<ObjectsRendererName, iAObjectsRendererSettings>
{
public:
	static iAAttributes& defaultAttributes()
	{
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			addAttr(attr, ColorParam, iAValueType::Color, DefaultColor);
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
