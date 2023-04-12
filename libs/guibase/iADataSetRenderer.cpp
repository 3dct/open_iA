// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADataSetRenderer.h"

#include "iAAABB.h"
#include "iAValueTypeVectorHelpers.h"

#include <iALog.h>
#include <iAMainWindow.h>

#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
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
		m_mapper->SetInputConnection(m_cubeSource->GetOutputPort());
		m_actor->GetProperty()->SetRepresentationToWireframe();
		m_actor->GetProperty()->SetShading(false);
		m_actor->GetProperty()->SetOpacity(1);
		//m_actor->GetProperty()->SetLineWidth(2);
		m_actor->GetProperty()->SetAmbient(1.0);
		m_actor->GetProperty()->SetDiffuse(0.0);
		m_actor->GetProperty()->SetSpecular(0.0);
		setColor(c);
		m_actor->SetPickable(false);
		m_actor->SetMapper(m_mapper);
		renderer->AddActor(m_actor);
	}
	void setVisible(bool visible)
	{
		if (visible)
		{
			m_renderer->AddActor(m_actor);
		}
		else
		{
			m_renderer->RemoveActor(m_actor);
		}
	}
	void setOrientationAndPosition(QVector<double> pos, QVector<double> ori)
	{
		assert(pos.size() == 3);
		assert(ori.size() == 3);
		m_actor->SetPosition(pos.data());
		m_actor->SetOrientation(ori.data());
	}
	void setBounds(iAAABB const& box)
	{
		m_cubeSource->SetBounds(
			box.minCorner().x(), box.maxCorner().x(),
			box.minCorner().y(), box.maxCorner().y(),
			box.minCorner().z(), box.maxCorner().z()
		);
	}
	void setColor(QColor const& c)
	{
		m_actor->GetProperty()->SetColor(c.redF(), c.greenF(), c.blueF());
	}
	QColor color() const
	{
		auto rgb = m_actor->GetProperty()->GetColor();
		return QColor(rgb[0], rgb[1], rgb[2]);
	}

private:
	vtkNew<vtkCubeSource> m_cubeSource;
	vtkNew<vtkPolyDataMapper> m_mapper;
	vtkNew<vtkActor> m_actor;
	vtkRenderer* m_renderer;
};


namespace
{
	const QColor OutlineDefaultColor(Qt::black);
}




const QString iADataSetRenderer::Position("Position");
const QString iADataSetRenderer::Orientation("Orientation");
const QString iADataSetRenderer::OutlineColor("Box Color");
const QString iADataSetRenderer::Pickable("Pickable");
const QString iADataSetRenderer::Shading("Shading");
const QString iADataSetRenderer::AmbientLighting("Ambient lighting");
const QString iADataSetRenderer::DiffuseLighting("Diffuse lighting");
const QString iADataSetRenderer::SpecularLighting("Specular lighting");
const QString iADataSetRenderer::SpecularPower("Specular power");

iADataSetRenderer::iADataSetRenderer(vtkRenderer* renderer) :
	m_renderer(renderer),
	m_visible(false)
{
	addAttribute(Position, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addAttribute(Orientation, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addAttribute(OutlineColor, iAValueType::Color, OutlineDefaultColor);
	addAttribute(Pickable, iAValueType::Boolean, true);
	// generic lighting settings:
	addAttribute(AmbientLighting, iAValueType::Continuous, 0.2);
	addAttribute(DiffuseLighting, iAValueType::Continuous, 0.5);
	addAttribute(SpecularLighting, iAValueType::Continuous, 0.7);
	addAttribute(SpecularPower, iAValueType::Continuous, 10.0);
}

iADataSetRenderer::~iADataSetRenderer()
{
	// cannot call virtual functions in destructor -> setVisible(false) leads to crash!
	if (m_outline)
	{
		m_outline->setVisible(false);
	}
}

void iADataSetRenderer::setAttributes(QVariantMap const& values)
{
	setApplyingValues(m_attribValues, m_attributes, values);
	// merge default values to currently set values to make applying simpler:
	auto allValues = joinValues(extractValues(attributesWithValues()), m_attribValues);
	applyAttributes(allValues);
	if (m_outline)
	{
		m_outline->setBounds(bounds());	// only potentially changes for volume currently; maybe use signals instead?
		m_outline->setColor(m_attribValues[OutlineColor].value<QColor>());
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
	return combineAttributesWithValues(m_attributes, attributeValues());
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
			m_attribValues.contains(OutlineColor) ? m_attribValues[OutlineColor].value<QColor>() : OutlineDefaultColor);
		updateOutlineTransform();
	}
	m_outline->setVisible(visible);
}

void iADataSetRenderer::addAttribute(
	QString const& name, iAValueType valueType, QVariant defaultValue, double min, double max)
{
#ifndef NDEBUG
	for (auto attr : m_attributes)
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

void iADataSetRenderer::setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3)
{
	Q_UNUSED(p1);
	Q_UNUSED(p2);
	Q_UNUSED(p3);
}

void iADataSetRenderer::removeCuttingPlanes()
{}

//QWidget* iADataSetRenderer::controlWidget()
//{
//	return nullptr;
//}
