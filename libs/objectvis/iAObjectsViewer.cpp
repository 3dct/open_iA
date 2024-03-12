// Copyright (c) open_iA contributors
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
	QColor DefaultColor("darkGray");    // for consistency with FeatureScout's default color
}

//! Display settings for object visualizations.
class iAObjectsRendererSettings : iASettingsObject<iAObjectsRenderer::Name, iAObjectsRendererSettings>
{
public:
	static iAAttributes& defaultAttributes()
	{
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			addAttr(attr, iAObjectsRenderer::Color, iAValueType::Color, DefaultColor);
			addAttr(attr, iAObjectsRenderer::SegmentSkip, iAValueType::Discrete, 1, 1, 1000);
			addAttr(attr, iAObjectsRenderer::NumOfCylinderSides, iAValueType::Discrete, 12, 3, 10000);
			selfRegister();
		}
		return attr;
	}
};

iAObjectsViewer::iAObjectsViewer(iADataSet* dataSet) :
	iADataSetViewer(dataSet)
{
	m_objVis = createObjectVis(dynamic_cast<iAObjectsData const*>(m_dataSet), DefaultColor);
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

iAObjectsRenderer::iAObjectsRenderer(vtkRenderer* renderer, iAObjectVis* objVis, QVariantMap const& overrideValues) :
	iADataSetRenderer(renderer),
	m_objVis(objVis),
	m_objActor(objVis ? objVis->createActor(renderer) : nullptr)
{
	setDefaultAttributes(defaultAttributes(), overrideValues);
}

iAObjectsRenderer::~iAObjectsRenderer()
{
}

void iAObjectsRenderer::applyAttributes(QVariantMap const& values)
{
	// TODO: think of ways of avoiding dynamic cast - shift down applyAttributes to object vis maybe?
	auto colorObj = dynamic_cast<iAColoredPolyObjectVis*>(m_objVis);
	if (colorObj)
	{
		colorObj->setColor(variantToColor(values[iAObjectsRenderer::Color]));
	}
	// apply segment skip and number of cylinder sides as well? but this requires recreating object vis...
}

iAAABB iAObjectsRenderer::bounds()
{
	if (!m_objVis)
	{
		static iAAABB bb{ 0,1, 0,1, 0,1 };
		return bb;
	}
	return iAAABB(m_objVis->bounds());
}

double const* iAObjectsRenderer::orientation() const
{
	if (m_objActor)
	{
	return dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor()->GetOrientation();
	}
	else
	{
		static double pos[3] = { 0, 0, 0 };
		return pos;
	}
}

double const* iAObjectsRenderer::position() const
{
	if (m_objActor)
	{
		return dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor()->GetPosition();
	}
	else
	{
		static double pos[3] = { 0, 0, 0 };
		return pos;
	}
}

void iAObjectsRenderer::setPosition(double pos[3])
{
	if (!m_objActor)
	{
		return;
	}
	dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor()->SetPosition(pos);
}

void iAObjectsRenderer::setOrientation(double ori[3])
{
	if (!m_objActor)
	{
		return;
	}
	dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor()->SetOrientation(ori);
}

vtkProp3D* iAObjectsRenderer::vtkProp()
{
	return dynamic_cast<iAPolyObjectVisActor*>(m_objActor.get())->actor();
}

iAAttributes& iAObjectsRenderer::defaultAttributes()
{
	static iAAttributes& attr = iAObjectsRendererSettings::defaultAttributes();
	return attr;
}

void iAObjectsRenderer::showDataSet()
{
	if (m_objActor)
	{
		m_objActor->show();
	}
}

void iAObjectsRenderer::hideDataSet()
{
	if (m_objActor)
	{
		m_objActor->hide();
	}
}

iAAttributes const& iAObjectsRenderer::attributes() const
{
	static iAAttributes attr;
	if (attr.isEmpty())
	{
		attr = cloneAttributes(iAObjectsRenderer::defaultAttributes());
		// TODO: if we could re-apply those parameters to existing object vis, we wouldn't need to remove them here:
		//if (!dynamic_cast<iALineObjectVis*>(m_objVis))
			removeAttribute(attr, iAObjectsRenderer::SegmentSkip);
		//if (!dynamic_cast<iACylinderObjectVis*>(m_objVis))
			removeAttribute(attr, iAObjectsRenderer::NumOfCylinderSides);
	}
	return attr;
}
