// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADataSetRenderer.h"

#include "iAAABB.h"
#include "iAValueTypeVectorHelpers.h"
#include "iAvtkActorHelper.h"  // for showActor

#include <iAMainWindow.h>
#include <iAvtkSourcePoly.h>

#include <vtkCommand.h>
#include <vtkCubeSource.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>

#include <QColor>

//! Holds VTK classes necessary for viewing an outline for a given box.
class iAOutlineImpl
{
public:
	iAOutlineImpl(iAAABB const& box, vtkRenderer* renderer, QColor const& c) : m_renderer(renderer)
	{
		setBounds(box);
		m_cube.actor->GetProperty()->SetRepresentationToWireframe();
		m_cube.actor->GetProperty()->SetShading(false);
		m_cube.actor->GetProperty()->SetOpacity(1);
		//m_actor->GetProperty()->SetLineWidth(2);
		m_cube.actor->GetProperty()->SetAmbient(1.0);
		m_cube.actor->GetProperty()->SetDiffuse(0.0);
		m_cube.actor->GetProperty()->SetSpecular(0.0);
		setColor(c);
		m_cube.actor->SetPickable(false);
		renderer->AddActor(m_cube.actor);
	}
	void setVisible(bool visible)
	{
		showActor(m_renderer, m_cube.actor, visible);
	}
	void setOrientationAndPosition(QVector<double> pos, QVector<double> ori)
	{
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_cube.actor->SetPosition(pos.data());
		m_cube.actor->SetOrientation(ori.data());
	}
	void setBounds(iAAABB const& box)
	{
		m_cube.source->SetBounds(
			box.minCorner().x(), box.maxCorner().x(),
			box.minCorner().y(), box.maxCorner().y(),
			box.minCorner().z(), box.maxCorner().z()
		);
	}
	void setColor(QColor const& c)
	{
		m_cube.actor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
	}
	QColor color() const
	{
		auto rgb = m_cube.actor->GetProperty()->GetColor();
		return QColor(rgb[0], rgb[1], rgb[2]);
	}

private:
	iAvtkSourcePoly<vtkCubeSource> m_cube;
	vtkRenderer* m_renderer;
};


namespace
{
	const QColor OutlineDefaultColor(Qt::black);
}




iADataSetRenderer::iADataSetRenderer(vtkRenderer* renderer) :
	m_renderer(renderer),
	m_visible(false)
{
	// if renderer is deleted for whatever reason, make sure it isn't accessed anymore
	m_renderObserverTag = renderer->AddObserver(vtkCommand::DeleteEvent, this, &iADataSetRenderer::clearRenderer);
}

iAAttributes const & iADataSetRenderer::attributes() const
{
	return defaultAttributes();
}

void iADataSetRenderer::setDefaultAttributes(iAAttributes const& defaultAttr, QVariantMap const& overrideValues)
{
	setAttributes(joinValues(extractValues(defaultAttr), overrideValues));
}

iAAttributes& iADataSetRenderer::defaultAttributes()
{
	static iAAttributes attr;
	if (attr.isEmpty())
	{
		addAttr(attr, Position, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
		addAttr(attr, Orientation, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
		addAttr(attr, OutlineColor, iAValueType::Color, colorToVariant(OutlineDefaultColor));
		addAttr(attr, Pickable, iAValueType::Boolean, true);
		// generic lighting settings:
		addAttr(attr, AmbientLighting, iAValueType::Continuous, 0.2);
		addAttr(attr, DiffuseLighting, iAValueType::Continuous, 0.5);
		addAttr(attr, SpecularLighting, iAValueType::Continuous, 0.7);
		addAttr(attr, SpecularPower, iAValueType::Continuous, 10.0);

		// addAttr(attr, ShowSlicers, iAValueType::Boolean, false);                // whether objects are cut at the current slice plane of each slicer (currently applies for all volume datasets; should be moved to individual dataset renderers, to be able to selectively enable it for each dataset)
	}
	return attr;
}

void iADataSetRenderer::clearRenderer()
{
	m_renderer = nullptr;
}

iADataSetRenderer::~iADataSetRenderer()
{
	if (m_renderer)
	{   // if this dataset renderer is cleaned up before renderer, de-register observer, otherwise invalid memory is accessed:
		m_renderer->RemoveObserver(m_renderObserverTag);
	}
	// cannot call virtual functions in destructor -> setVisible(false) leads to crash! -> needs to be done in derived classes.
	if (m_outline)
	{
		m_outline->setVisible(false);
	}
}

void iADataSetRenderer::setAttributes(QVariantMap const& values)
{
	setApplyingValues(m_attribValues, attributes(), values);
	// merge default values to currently set values to make applying simpler:
	auto allValues = joinValues(extractValues(attributesWithValues()), m_attribValues);
	applyAttributes(allValues);
	if (m_outline)
	{
		m_outline->setBounds(bounds());	// only potentially changes for volume currently; maybe use signals instead?
		m_outline->setColor(variantToColor(m_attribValues[OutlineColor]));
		updateOutlineTransform();
	}
}

void iADataSetRenderer::setPickable(bool pickable)
{
	m_attribValues[Pickable] = pickable;
	// TODO: maybe only apply pickable?
	applyAttributes(m_attribValues);
}

bool iADataSetRenderer::isPickable() const
{
	return m_attribValues[Pickable].toBool();
}

iAAttributes iADataSetRenderer::attributesWithValues() const
{
	return combineAttributesWithValues(attributes(), attributeValues());
}

QVariantMap iADataSetRenderer::attributeValues() const
{
	QVariantMap result(m_attribValues);
	// set position and orientation from current values:
	auto pos = position();
	result[Position] = variantVector<double>({ pos[0], pos[1], pos[2] });
	auto ori = orientation();
	result[Orientation] = variantVector<double>({ ori[0], ori[1], ori[2] });
	return result;
}

void iADataSetRenderer::setVisible(bool visible)
{
	m_visible = visible;
	if (m_visible)
	{
		showDataSet();
	}
	else
	{
		hideDataSet();
	}
}

bool iADataSetRenderer::isVisible() const
{
	return m_visible;
}

void iADataSetRenderer::setBoundsVisible(bool visible)
{
	if (!m_outline)
	{
		m_outline = std::make_shared<iAOutlineImpl>(bounds(), m_renderer,
			m_attribValues.contains(OutlineColor) ? variantToColor(m_attribValues[OutlineColor]) : OutlineDefaultColor);
		updateOutlineTransform();
	}
	m_outline->setVisible(visible);
}


vtkRenderer* iADataSetRenderer::vtkRen() const
{
	return m_renderer;
}

/*
void iADataSetRenderer::addAttribute(
	QString const& name, iAValueType valueType, QVariant defaultValue, double min, double max)
{
#ifndef NDEBUG
	for (auto attr : attributes())
	{
		if (attr->name() == name)
		{
			LOG(lvlWarn, QString("iADataSetRenderer::addAttribute: Attribute with name %1 already exists!").arg(name));
		}
	}
#endif
	addAttr(m_attributes, name, valueType, defaultValue, min, max);
	m_attribValues[name] = valueType == iAValueType::Categorical
		? selectedOption(defaultValue.toStringList())
		: defaultValue;
}
*/

void iADataSetRenderer::updateOutlineTransform()
{
	if (!m_outline)
	{
		return;
	}
	QVector<double> pos(3), ori(3);
	for (int i = 0; i < 3; ++i)
	{
		pos[i] = position()[i];
		ori[i] = orientation()[i];
	}
	m_outline->setOrientationAndPosition(pos, ori);
}

void iADataSetRenderer::setCuttingPlanes(std::array<vtkPlane*, 3> p)
{
	Q_UNUSED(p);
}

void iADataSetRenderer::removeCuttingPlanes()
{}

void iADataSetRenderer::addCuttingPlane(vtkPlane* p)
{
	Q_UNUSED(p);
}

void iADataSetRenderer::removeCuttingPlane(vtkPlane* p)
{
	Q_UNUSED(p);
}

//QWidget* iADataSetRenderer::controlWidget()
//{
//	return nullptr;
//}
