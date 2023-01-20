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
#include "iADataSetViewer.h"

#include "iAAABB.h"
#include "iADataSet.h"
#include "iAMdiChild.h"
#include "iARenderer.h"
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

const QString iADataSetViewer::Position("Position");
const QString iADataSetViewer::Orientation("Orientation");
const QString iADataSetViewer::OutlineColor("Box Color");
const QString iADataSetViewer::Pickable("Pickable");
const QString iADataSetViewer::Shading("Shading");
const QString iADataSetViewer::AmbientLighting("Ambient lighting");
const QString iADataSetViewer::DiffuseLighting("Diffuse lighting");
const QString iADataSetViewer::SpecularLighting("Specular lighting");
const QString iADataSetViewer::SpecularPower("Specular power");

namespace
{
	const QColor OutlineDefaultColor(Qt::black);
}


iADataSetViewer::iADataSetViewer(iADataSet const * dataSet):
	m_dataSet(dataSet)
{
	addAttribute(Position, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addAttribute(Orientation, iAValueType::Vector3, variantVector<double>({ 0.0, 0.0, 0.0 }));
	addAttribute(OutlineColor, iAValueType::Color, OutlineDefaultColor);
	addAttribute(Pickable, iAValueType::Boolean, true);

	auto volumeSettings = iAMainWindow::get()->defaultVolumeSettings();
	addAttribute(AmbientLighting, iAValueType::Continuous, volumeSettings.AmbientLighting);
	addAttribute(DiffuseLighting, iAValueType::Continuous, volumeSettings.DiffuseLighting);
	addAttribute(SpecularLighting, iAValueType::Continuous, volumeSettings.SpecularLighting);
	addAttribute(SpecularPower, iAValueType::Continuous, volumeSettings.SpecularPower);
}

iADataSetViewer::~iADataSetViewer()
{}

void iADataSetViewer::prepare(iAPreferences const& pref, iAProgress* p)
{
	Q_UNUSED(pref);
	Q_UNUSED(p);
}

void iADataSetViewer::createGUI(iAMdiChild* child)
{	// by default, nothing to do
	Q_UNUSED(child);
}

QString iADataSetViewer::information() const
{
	return m_dataSet->info();
}

/*
iADataSet* iADataSetViewer::dataSet()
{
	return m_dataSet;
}
*/

void iADataSetViewer::dataSetChanged()
{}

iAAttributes iADataSetViewer::attributesWithValues() const
{
	iAAttributes result = combineAttributesWithValues(m_attributes, m_attribValues);
	// set position and orientation from current values:
	assert(result[0]->name() == Position);
	auto pos = position();
	result[0]->setDefaultValue(variantVector<double>({ pos[0], pos[1], pos[2] }));
	assert(result[1]->name() == Orientation);
	auto ori = orientation();
	result[1]->setDefaultValue(variantVector<double>({ ori[0], ori[1], ori[2] }));
	return result;
}

void iADataSetViewer::addAttribute(
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

void iADataSetViewer::setAttributes(QVariantMap const& values)
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

void iADataSetViewer::setPickable(bool pickable)
{
	m_attribValues[Pickable] = pickable;
	// TODO: maybe only apply pickable?
	applyAttributes(m_attribValues);
}

bool iADataSetViewer::isPickable() const
{
	return m_attribValues[Pickable].toBool();
}

void iADataSetViewer::setVisible(bool visible)
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

bool iADataSetViewer::isVisible() const
{
	return m_visible;
}

void iADataSetViewer::setBoundsVisible(bool visible)
{
	if (!m_outline)
	{
		m_outline = std::make_shared<iAOutlineImpl>(bounds(), m_renderer,
			m_attribValues.contains(OutlineColor) ? m_attribValues[OutlineColor].value<QColor>() : OutlineDefaultColor);
		updateOutlineTransform();
	}
	m_outline->setVisible(visible);
}

void iADataSetViewer::updateOutlineTransform()
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

void iADataSetViewer::setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3)
{
	Q_UNUSED(p1);
	Q_UNUSED(p2);
	Q_UNUSED(p3);
}

void iADataSetViewer::removeCuttingPlanes()
{}

//QWidget* iADataSetRenderer::controlWidget()
//{
//	return nullptr;
//}





// iAMeshViewer

iAMeshViewer::iAMeshViewer(iADataSet const * dataSet) : iADataSetViewer(dataSet)
{
}

void iAMeshViewer::prepare(iAPreferences const& pref, iAProgress* p)
{
	Q_UNUSED(pref);
}

void iAMeshViewer::createGUI(iAMdiChild* child)
{
	Q_UNUSED(child);
}




// iAProjectViewer

iAProjectViewer::iAProjectViewer(iADataSet const* dataSet) :
	iADataSetViewer(dataSet),
	m_numOfDataSets(0)
{
	auto collection = dynamic_cast<iADataCollection const*>(m_dataSet);
	assert(collection);
	m_numOfDataSets = collection->dataSets().size();
}

#include <iAStringHelper.h>

#include <iATool.h>
#include "iAToolRegistry.h"
#include <iAMainWindow.h>

#include <QSettings>

void iAProjectViewer::createGUI(iAMdiChild* child)
{
	auto collection = dynamic_cast<iADataCollection const*>(m_dataSet);
	auto fileName = m_dataSet->metaData(iADataSet::FileNameKey).toString();
	auto afterRenderCallback = [this, child, collection, fileName]()
	{
		// TODO NEWIO - also load viewer settings; this should happen in the creation of the viewers - consider metadata if available!
				/*
		int channel = settings.value(GetModalityKey(currIdx, "Channel"), -1).toInt();
		QString modalityRenderFlags = settings.value(GetModalityKey(currIdx, "RenderFlags")).toString();
		modalityFile = MakeAbsolute(fi.absolutePath(), modalityFile);
		QString orientationSettings = settings.value(GetModalityKey(currIdx, "Orientation")).toString();
		QString positionSettings = settings.value(GetModalityKey(currIdx, "Position")).toString();
		QString tfFileName = settings.value(GetModalityKey(currIdx, "TransferFunction")).toString();

		//loading volume settings
		iAVolumeSettings defaultSettings;
		QString Shading = settings.value(GetModalityKey(currIdx, "Shading"), defaultSettings.Shading).toString();
		QString LinearInterpolation = settings.value(GetModalityKey(currIdx, "LinearInterpolation"), defaultSettings.LinearInterpolation).toString();
		QString SampleDistance = settings.value(GetModalityKey(currIdx, "SampleDistance"), defaultSettings.SampleDistance).toString();
		QString AmbientLighting = settings.value(GetModalityKey(currIdx, "AmbientLighting"), defaultSettings.AmbientLighting).toString();
		QString DiffuseLighting = settings.value(GetModalityKey(currIdx, "DiffuseLighting"), defaultSettings.DiffuseLighting).toString();
		QString SpecularLighting = settings.value(GetModalityKey(currIdx, "SpecularLighting"), defaultSettings.SpecularLighting).toString();
		QString SpecularPower = settings.value(GetModalityKey(currIdx, "SpecularPower"), defaultSettings.SpecularPower).toString();
		QString ScalarOpacityUnitDistance = settings.value(GetModalityKey(currIdx, "ScalarOpacityUnitDistance"), defaultSettings.ScalarOpacityUnitDistance).toString();
		volSettings.RenderMode = mapRenderModeToEnum(settings.value(GetModalityKey(currIdx, "RenderMode")).toString());

		//check if vol settings are ok / otherwise use default values
		checkandSetVolumeSettings(volSettings, Shading, LinearInterpolation, SampleDistance, AmbientLighting,
			DiffuseLighting, SpecularLighting, SpecularPower, ScalarOpacityUnitDistance);

		if (!tfFileName.isEmpty())
		{
			tfFileName = MakeAbsolute(fi.absolutePath(), tfFileName);
		}
		if (modalityExists(modalityFile, channel))
		{
			LOG(lvlWarn, QString("Modality (name=%1, filename=%2, channel=%3) already exists!").arg(modalityName).arg(modalityFile).arg(channel));
		}
		else
		{
			int renderFlags = (modalityRenderFlags.contains("R") ? iAModality::MainRenderer : 0) |
				(modalityRenderFlags.contains("L") ? iAModality::MagicLens : 0) |
				(modalityRenderFlags.contains("B") ? iAModality::BoundingBox : 0) |
				(modalityRenderFlags.contains("S") ? iAModality::Slicer : 0);

			ModalityCollection mod = iAModalityList::load(modalityFile, modalityName, channel, false, renderFlags);
			if (mod.size() != 1) // we expect to load exactly one modality
			{
				LOG(lvlWarn, QString("Invalid state: More or less than one modality loaded from file '%1'").arg(modalityFile));
				return false;
			}
			mod[0]->setStringSettings(positionSettings, orientationSettings, tfFileName);
			mod[0]->setVolSettings(volSettings);
			m_modalities.push_back(mod[0]);
			emit added(mod[0]);
		}
		*/

		// all datasets loaded, continue with loading projects!
		if (!collection->settings())    // not all collections come with additional settings...
		{
			return;
		}
		auto& settings = *collection->settings().get();
		child->loadSettings(settings);
		auto tools = iAToolRegistry::toolKeys();
		auto registeredTools = iAToolRegistry::toolKeys();
		auto projectFileGroups = settings.childGroups();
		for (auto toolKey : registeredTools)
		{
			if (projectFileGroups.contains(toolKey))
			{
				auto tool = iAToolRegistry::createTool(toolKey, iAMainWindow::get(), child);
				settings.beginGroup(toolKey);
				child->addTool(toolKey, tool);
				tool->loadState(settings, fileName);
				settings.endGroup();
			}
		}
		for (auto ds : child->dataSetMap())
		{
			if (ds.second.get() == m_dataSet)
			{
				child->removeDataSet(ds.first);
				break;    // CHECK NEWIO: also this viewer is deleted on removing the dataset, so here the object could already be deleted!
			}
		}
	};
	// if no datasets available, directly load tools...
	if (collection->dataSets().empty())
	{
		afterRenderCallback();
		return;
	}
	// ...otherwise load datasets first...
	QObject::connect(child, &iAMdiChild::dataSetRendered, this,
		[this, child, collection, fileName, afterRenderCallback](size_t dataSetIdx)
		{
			static std::set<size_t> renDS;
			renDS.insert(dataSetIdx);
			if (m_loadedDataSets.size() < m_numOfDataSets)
			{
				return;
			}
			for (auto l : m_loadedDataSets)
			{
				if (renDS.find(l) == renDS.end())
				{
					return;
				}
			}
			// ... and continue with loading tools once all datasets have been rendered
			afterRenderCallback();
		});
	for (auto d : collection->dataSets())
	{
		auto dataSetIdx = child->addDataSet(d);
		m_loadedDataSets.push_back(dataSetIdx);
	}
	// TODO: provide option to immediately load tools without waiting for dataset to finish loading/rendering?
}


#include "iAVolumeViewer.h"

std::shared_ptr<iADataSetViewer> createDataSetViewer(iADataSet const* dataSet)
{
	switch (dataSet->type())
	{
	case iADataSetType::Volume: return std::make_shared<iAVolumeViewer>(dataSet);
	case iADataSetType::Mesh: [[fallthrough]];
	case iADataSetType::Graph:  return std::make_shared<iAMeshViewer>(dataSet);
	case iADataSetType::Collection: return std::make_shared<iAProjectViewer>(dataSet);
	default: return std::shared_ptr<iADataSetViewer>();
	}
}