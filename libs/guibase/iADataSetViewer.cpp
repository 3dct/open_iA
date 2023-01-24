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

#include "defines.h"    // for NotExistingChannel
#include "iAAABB.h"
#include "iADataSet.h"
#include "iADataSetListWidget.h"
#include "iADataSetRendererImpl.h"
#include "iAMdiChild.h"
#include "iARenderer.h"
#include "iAValueTypeVectorHelpers.h"

#include <iALog.h>
#include <iAMainWindow.h>
#include <iAVolumeSettings.h>

#include <vtkOpenGLRenderer.h>

iADataSetViewer::iADataSetViewer(iADataSet const * dataSet):
	m_dataSet(dataSet)
{
}

iADataSetViewer::~iADataSetViewer()
{
	if (m_renderer && m_renderer->isVisible())
	{
		m_renderer->setVisible(false);
	}
}

void iADataSetViewer::prepare(iAPreferences const& pref, iAProgress* p)
{
	Q_UNUSED(pref);
	Q_UNUSED(p);
}

void iADataSetViewer::createGUI(iAMdiChild* child, size_t newDataSetIdx)
{
	m_renderer = createRenderer(child->renderer()->renderer());
	auto dsList = child->dataSetListWidget();
	int row = dsList->addDataSet(m_dataSet, newDataSetIdx, m_renderer.get(), m_renderer.get(), hasSlicerVis());
	// TODO NEWIO: events directly connecting to specific item / QActions linked to item

	QObject::connect(child->dataSetListWidget(), &iADataSetListWidget::set3DRendererVisibility, this,
		[this, newDataSetIdx](size_t dataSetIdx, int visibility)
		{
			if (newDataSetIdx != dataSetIdx)
			{
				return;
			}
			m_renderer->setVisible(visibility);
		});
	connect(child->dataSetListWidget(), &iADataSetListWidget::setBoundsVisibility, this,
		[this, newDataSetIdx](size_t dataSetIdx, int visibility)
		{
			if (newDataSetIdx != dataSetIdx)
			{
				return;
			}
			m_renderer->setBoundsVisible(visibility);
		});
	connect(child->dataSetListWidget(), &iADataSetListWidget::set2DVisibility, this,
		[this, newDataSetIdx](size_t dataSetIdx, int visibility)
		{
			if (newDataSetIdx != dataSetIdx)
			{
				return;
			}
			setSlicerVisibility(visibility);
		});
	connect(child->dataSetListWidget(), &iADataSetListWidget::set3DMagicLensVisibility, this,
		[this, newDataSetIdx, child](size_t dataSetIdx, int visibility)
		{
			if (newDataSetIdx != dataSetIdx)
			{
				return;
			}
			if (!m_magicLensRenderer)
			{
				m_magicLensRenderer = createRenderer(child->magicLens3DRenderer());
			}
			m_magicLensRenderer->setVisible(visibility);
		});
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
	if (renderer())
	{
		result.append(renderer()->attributesWithValues());
	}
	return result;
}

iADataSetRenderer* iADataSetViewer::renderer()
{
	return m_renderer.get();
}

iADataSetRenderer const* iADataSetViewer::renderer() const
{
	return m_renderer.get();
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

std::shared_ptr<iADataSetRenderer> iADataSetViewer::createRenderer(vtkRenderer* ren)
{
	Q_UNUSED(ren);
	return {};
}

bool iADataSetViewer::hasSlicerVis() const
{
	return false;
}

void iADataSetViewer::setSlicerVisibility(bool visible)
{
	// by default we assume there is no slicer vis
	Q_UNUSED(visible);
}

void iADataSetViewer::setAttributes(QVariantMap const& values)
{
	m_attribValues = values;
	applyAttributes(values);
	if (renderer())
	{
		renderer()->setAttributes(values);
	}
	dataSetChanged();    // TODO NEWIO: maybe we can improve this logic?
}

void iADataSetViewer::setPickable(bool pickable)
{
	renderer()->setPickable(pickable);
}

void iADataSetViewer::slicerRegionSelected(double minVal, double maxVal, uint channelID)
{
	Q_UNUSED(minVal);
	Q_UNUSED(maxVal);
	Q_UNUSED(channelID);
}

uint iADataSetViewer::slicerChannelID()
{
	return NotExistingChannel;
}

void iADataSetViewer::applyAttributes(QVariantMap const& values)
{
	Q_UNUSED(values);
}


// iAGraphViewer

iAGraphViewer::iAGraphViewer(iADataSet const* dataSet) :
	iADataSetViewer(dataSet)
{}

std::shared_ptr<iADataSetRenderer> iAGraphViewer::createRenderer(vtkRenderer * ren)
{
	auto meshData = dynamic_cast<iAGraphData const*>(m_dataSet);
	return std::make_shared<iAGraphRenderer>(ren, meshData);
}



// iAMeshViewer

iAMeshViewer::iAMeshViewer(iADataSet const* dataSet) :
	iADataSetViewer(dataSet)
{}

std::shared_ptr<iADataSetRenderer> iAMeshViewer::createRenderer(vtkRenderer* ren)
{
	auto meshData = dynamic_cast<iAPolyData const*>(m_dataSet);
	return std::make_shared<iAPolyDataRenderer>(ren, meshData);
}



// iAGeometricObjectViewer

#include "iAGeometricObject.h"

iAGeometricObjectViewer::iAGeometricObjectViewer(iADataSet const* dataSet):
	iADataSetViewer(dataSet)
{}

std::shared_ptr<iADataSetRenderer> iAGeometricObjectViewer::createRenderer(vtkRenderer* ren)
{
	auto meshData = dynamic_cast<iAGeometricObject const*>(m_dataSet);
	return std::make_shared<iAGeometricObjectRenderer>(ren, meshData);
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

void iAProjectViewer::createGUI(iAMdiChild* child, size_t dataSetIdx)
{
	auto collection = dynamic_cast<iADataCollection const*>(m_dataSet);
	auto fileName = m_dataSet->metaData(iADataSet::FileNameKey).toString();
	auto afterRenderCallback = [this, child, collection, fileName, dataSetIdx]()
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
		child->removeDataSet(dataSetIdx);
		// CHECK NEWIO: also this viewer is deleted on removing the dataset, so here the object could already be deleted!
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
		auto newDataSetIdx = child->addDataSet(d);
		m_loadedDataSets.push_back(newDataSetIdx);
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