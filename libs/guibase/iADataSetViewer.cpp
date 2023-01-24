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
#include "iAParameterDlg.h"
#include "iARenderer.h"
#include "iAValueTypeVectorHelpers.h"

#include <iALog.h>
#include <iAMainWindow.h>
#include <iAVolumeSettings.h>

#include <vtkOpenGLRenderer.h>


namespace
{
	QPixmap pixmapFromName(QString iconName, bool checked)
	{
		return QPixmap(QString(":/images/%1%2.svg")
			.arg(iconName)
			.arg((checked ^ !iAMainWindow::get()->brightMode()) ? "" : "_light"));
	}
	QIcon iconFromName(QString iconName)
	{
		QIcon icon;
		icon.addPixmap(pixmapFromName(iconName, false), QIcon::Normal, QIcon::Off);
		icon.addPixmap(pixmapFromName(iconName, true), QIcon::Normal, QIcon::On);
		return icon;
	}
}

iADataSetViewer::iADataSetViewer(iADataSet * dataSet):
	m_dataSet(dataSet),
	m_pickAction(nullptr)
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

void iADataSetViewer::createGUI(iAMdiChild* child, size_t dataSetIdx)
{
	m_renderer = createRenderer(child->renderer()->renderer());
	assert(m_renderer);
	auto dsList = child->dataSetListWidget();
	m_actions.push_back(createToggleAction("3D", "eye", true,
		[this, child](bool checked)
		{
			m_renderer->setVisible(checked);
			child->updateRenderer();
		}));
	m_actions.push_back(createToggleAction("Box", "eye", true,
		[this, child](bool checked)
		{
			m_renderer->setBoundsVisible(checked);
			child->updateRenderer();
		}));
	m_actions.push_back(createToggleAction("Magic Lens", "eye", true,
		[this, child](bool checked)
		{
			if (!m_magicLensRenderer)
			{
				if (!checked)
				{
					return;
				}
				m_magicLensRenderer = createRenderer(child->magicLens3DRenderer());
			}
			m_magicLensRenderer->setVisible(checked);
			child->updateRenderer();
		}));
	m_pickAction = createToggleAction("Pickable", "transform-move", false,
		[this, child, dataSetIdx](bool checked) {
			Q_UNUSED(checked);
			if (checked)
			{
				child->setDataSetMovable(dataSetIdx);
			}
		});
	m_pickAction->setEnabled(false);

	auto editAction = new QAction("Edit dataset properties");
	connect(editAction, &QAction::triggered, this,
		[this, child, dataSetIdx]()
		{
			auto params = attributesWithValues();
			params.prepend(iAAttributeDescriptor::createParam("Name", iAValueType::String, m_dataSet->name()));
			QString description;
			if (m_dataSet->hasMetaData(iADataSet::FileNameKey))
			{
				description = iADataSet::FileNameKey + ": " + m_dataSet->metaData(iADataSet::FileNameKey).toString();
			}
			iAParameterDlg dlg(child, "Dataset parameters", params, description);
			if (dlg.exec() != QDialog::Accepted)
			{
				return;
			}
			auto newName = dlg.parameterValues()["Name"].toString();
			if (m_dataSet->name() != newName)
			{
				m_dataSet->setMetaData(iADataSet::NameKey, newName);
				child->dataSetListWidget()->setName(dataSetIdx, newName);
			}
			setAttributes(dlg.parameterValues());
			child->updateRenderer();  // currently, 3D renderer properties are changed only
			/*
	// TODO: reset camera in 3D renderer / slicer when the spacing of modality was changed
	// TODO: maybe instead of reset, apply a zoom corresponding to the ratio of spacing change?
	m_renderer->updateSlicePlanes(newSpacing);
	m_renderer->renderer()->ResetCamera();
	m_renderer->update();
	for (int s = 0; s < 3; ++s)
	{
		set3DSlicePlanePos(s, sliceNumber(s));
		slicer(s)->renderer()->ResetCamera();
		slicer(s)->update();
	}
	*/
			emit dataSetChanged();
		});
	editAction->setIcon(iconFromName("edit"));
	editAction->setData("edit");
	m_actions.push_back(editAction);

	m_actions.push_back(m_pickAction);
	m_actions.append(additionalActions(child));

	connect(iAMainWindow::get(), &iAMainWindow::styleChanged, this,
	[this]()
	{
		for (auto a: m_actions)
		{
			auto iconName = a->data().toString();
			if (!iconName.isEmpty())
			{
				a->setIcon(iconFromName(iconName));
			}
			else
			{
				LOG(lvlWarn, QString("DEVELOPER WARNING: Dataset action %1 has no proper iconName as data!").arg(a->text()));
			}
		}
	});
	dsList->addDataSet(m_dataSet, dataSetIdx, m_actions);
}

QString iADataSetViewer::information() const
{
	return m_dataSet->info();
}

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

QAction* iADataSetViewer::createToggleAction(QString const& name, QString const& iconName, bool checked, std::function<void(bool)> handler)
{
	auto action = new QAction(name);
	action->setToolTip(QString("Toggle %1").arg(name));
	action->setIcon(iconFromName(iconName));
	action->setCheckable(true);
	action->setData(iconName);
	action->setChecked(checked);
	connect(action, &QAction::triggered, this, handler);
	return action;
}

QVector<QAction*> iADataSetViewer::additionalActions(iAMdiChild* child)
{
	Q_UNUSED(child);
	return {};
}

void iADataSetViewer::setAttributes(QVariantMap const& values)
{
	m_attribValues = values;
	applyAttributes(values);
	if (renderer())
	{
		renderer()->setAttributes(values);
	}
}

void iADataSetViewer::setPickable(bool pickable)
{
	QSignalBlocker sb(m_pickAction);
	m_pickAction->setChecked(pickable);
	renderer()->setPickable(pickable);
}

void iADataSetViewer::setPickActionVisible(bool visible)
{
	m_pickAction->setEnabled(visible);
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

iAGraphViewer::iAGraphViewer(iADataSet * dataSet) :
	iADataSetViewer(dataSet)
{}

std::shared_ptr<iADataSetRenderer> iAGraphViewer::createRenderer(vtkRenderer * ren)
{
	auto meshData = dynamic_cast<iAGraphData const*>(m_dataSet);
	return std::make_shared<iAGraphRenderer>(ren, meshData);
}



// iAMeshViewer

iAMeshViewer::iAMeshViewer(iADataSet * dataSet) :
	iADataSetViewer(dataSet)
{}

std::shared_ptr<iADataSetRenderer> iAMeshViewer::createRenderer(vtkRenderer* ren)
{
	auto meshData = dynamic_cast<iAPolyData const*>(m_dataSet);
	return std::make_shared<iAPolyDataRenderer>(ren, meshData);
}



// iAGeometricObjectViewer

#include "iAGeometricObject.h"

iAGeometricObjectViewer::iAGeometricObjectViewer(iADataSet * dataSet):
	iADataSetViewer(dataSet)
{}

std::shared_ptr<iADataSetRenderer> iAGeometricObjectViewer::createRenderer(vtkRenderer* ren)
{
	auto meshData = dynamic_cast<iAGeometricObject const*>(m_dataSet);
	return std::make_shared<iAGeometricObjectRenderer>(ren, meshData);
}


// iAProjectViewer

iAProjectViewer::iAProjectViewer(iADataSet * dataSet) :
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

std::shared_ptr<iADataSetViewer> createDataSetViewer(iADataSet * dataSet)
{
	switch (dataSet->type())
	{
	case iADataSetType::Volume: return std::make_shared<iAVolumeViewer>(dataSet);
	case iADataSetType::Mesh: return std::make_shared<iAMeshViewer>(dataSet);
	case iADataSetType::Graph: return std::make_shared<iAGraphViewer>(dataSet);
	case iADataSetType::Collection: return std::make_shared<iAProjectViewer>(dataSet);
	default: return std::shared_ptr<iADataSetViewer>();
	}
}