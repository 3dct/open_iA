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
#include "iADataSetRenderer.h"

#include "iAAABB.h"
#include "iADataForDisplay.h"
#include "iAValueTypeVectorHelpers.h"

#include <iALog.h>
#include <iAMainWindow.h>
#include <iAVolumeSettings.h>

#include <vtkActor.h>
#include <vtkCubeSource.h>
#include <vtkOpenGLRenderer.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

#include <QColor>

class iAOutlineImpl
{
public:
	iAOutlineImpl(iAAABB const& box, vtkRenderer* renderer, QColor const & c): m_renderer(renderer)
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

iADataSetRenderer::iADataSetRenderer(vtkRenderer* renderer, bool defaultVisibility):
	m_renderer(renderer),
	m_visible(defaultVisibility)
{
	addAttribute(Position, iAValueType::Vector3, variantVector<double>({0.0, 0.0, 0.0}));
	addAttribute(Orientation, iAValueType::Vector3, variantVector<double>({0.0, 0.0, 0.0}));
	addAttribute(OutlineColor, iAValueType::Color, OutlineDefaultColor);
	addAttribute(Pickable, iAValueType::Boolean, true);

	auto volumeSettings = iAMainWindow::get()->defaultVolumeSettings();
	addAttribute(AmbientLighting, iAValueType::Continuous, volumeSettings.AmbientLighting);
	addAttribute(DiffuseLighting, iAValueType::Continuous, volumeSettings.DiffuseLighting);
	addAttribute(SpecularLighting, iAValueType::Continuous, volumeSettings.SpecularLighting);
	addAttribute(SpecularPower, iAValueType::Continuous, volumeSettings.SpecularPower);
}

iADataSetRenderer::~iADataSetRenderer()
{}

void iADataSetRenderer::setAttributes(QVariantMap const & values)
{
	m_attribValues = values;
	applyAttributes(values);
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
	iAAttributes result = combineAttributesWithValues(m_attributes, m_attribValues);
	// set position and orientation from current values:
	assert(result[0]->name() == Position);
	auto pos = position();
	result[0]->setDefaultValue(variantVector<double>({pos[0], pos[1], pos[2]}));
	assert(result[1]->name() == Orientation);
	auto ori = orientation();
	result[1]->setDefaultValue(variantVector<double>({ori[0], ori[1], ori[2]}));
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
	m_attributes.push_back(iAAttributeDescriptor::createParam(name, valueType, defaultValue, min, max));
	m_attribValues[name] = defaultValue;
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
{}

void iADataSetRenderer::removeCuttingPlanes()
{}

//QWidget* iADataSetRenderer::controlWidget()
//{
//	return nullptr;
//}
