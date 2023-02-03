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

#include <QAction>

namespace
{
	QString assembleIconName(QString const& iconName, bool checked)
	{
		return QString(":/images/%1%2.svg")
			.arg(iconName)
			.arg((checked ^ !iAMainWindow::get()->brightMode()) ? "" : "_light");
	}
	QIcon toggleIconFromName(QString const& iconName)
	{
		QIcon icon;
		icon.addPixmap(QPixmap(assembleIconName(iconName, false)), QIcon::Normal, QIcon::Off);
		icon.addPixmap(QPixmap(assembleIconName(iconName, true)), QIcon::Normal, QIcon::On);
		return icon;
	}
	void setActionIcon(QAction* action, QString const& iconName, bool toggle)
	{
		action->setIcon(toggle ? toggleIconFromName(iconName) : QIcon(assembleIconName(iconName, true)));
		action->setData(QString("%1%2").arg(toggle?"!":".").arg(iconName));
	}
	void updateActionIcon(QAction* action)
	{
		auto s = action->data().toString();
		if (s.isEmpty())
		{
			LOG(lvlWarn, QString("DEVELOPER WARNING: Dataset action %1 has no proper iconName as data!").arg(action->text()));
			return;
		}
		auto iconName = s.right(s.length() - 1);
		setActionIcon(action, iconName, s[0] == '!');
	}

	const QChar Render3DFlag('R');
	const QChar RenderOutlineFlag('B');
	const QChar RenderMagicLensFlag('L');
	const QString RenderFlagsDefault("R");
}

const QString iADataSetViewer::RenderFlags("RenderFlags");

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
	if (m_magicLensRenderer && m_magicLensRenderer->isVisible())
	{
		m_magicLensRenderer->setVisible(false);
	}
}

void iADataSetViewer::prepare(iAPreferences const& pref, iAProgress* p)
{
	Q_UNUSED(pref);
	Q_UNUSED(p);
}

void iADataSetViewer::createGUI(iAMdiChild* child, size_t dataSetIdx)
{
	if (!m_dataSet->hasMetaData(RenderFlags))   // only use default render flags if not set in dataset
	{                                               // (by loader or by derived viewer class)
		m_dataSet->setMetaData(RenderFlags, RenderFlagsDefault);
	}
	m_renderer = createRenderer(child->renderer()->renderer());
	assert(m_renderer);
	m_renderer->setAttributes(joinValues(extractValues(m_renderer->attributesWithValues()), m_dataSet->allMetaData()) );
	if (renderFlagSet(Render3DFlag))
	{
		m_renderer->setVisible(true);
	}
	if (renderFlagSet(RenderOutlineFlag))
	{
		m_renderer->setBoundsVisible(true);
	}
	m_magicLensRenderer = createRenderer(child->magicLens3DRenderer());
	if (renderFlagSet(RenderMagicLensFlag))
	{
		m_magicLensRenderer->setVisible(true);
	}
	child->renderer()->adaptSceneBoundsToNewObject(m_renderer->bounds());
	auto dsList = child->dataSetListWidget();
	QVector<QAction*> viewActions;
	viewActions.push_back(createToggleAction("3D", "3d", renderFlagSet(Render3DFlag),
		[this, child](bool checked)
		{
			setRenderFlag(Render3DFlag, checked);
			m_renderer->setVisible(checked);
			child->updateRenderer();
		}));
	viewActions.push_back(createToggleAction("Box", "box_3d_edge", renderFlagSet(RenderOutlineFlag),
		[this, child](bool checked)
		{
			setRenderFlag(RenderOutlineFlag, checked);
			m_renderer->setBoundsVisible(checked);
			child->updateRenderer();
		}));
	viewActions.push_back(createToggleAction("Magic Lens", "magic_lens_3d", renderFlagSet(RenderMagicLensFlag),
		[this, child](bool checked)
		{
			setRenderFlag(RenderMagicLensFlag, checked);
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
	viewActions.push_back(m_pickAction);

	viewActions.append(additionalActions(child));

	auto editAction = new QAction("Edit dataset and display properties");
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
			emit dataSetChanged(dataSetIdx);
		});
	setActionIcon(editAction, "edit", false);
	QVector<QAction*> editActions;
	editActions.push_back(editAction);

	auto removeAction = new QAction("Remove");
	removeAction->setToolTip("Remove dataset from display, unload from memory");
	connect(removeAction, &QAction::triggered, this,
		[this, dataSetIdx, child]()
		{
			child->dataSetListWidget()->removeDataSet(dataSetIdx);
			emit removeDataSet(dataSetIdx);
		});
	setActionIcon(removeAction, "delete", false);
	editActions.push_back(removeAction);

	connect(iAMainWindow::get(), &iAMainWindow::styleChanged, this,
	[viewActions, editActions]()
	{
		for (auto a: viewActions)
		{
			updateActionIcon(a);
		}
		for (auto a : editActions)
		{
			updateActionIcon(a);
		}
	});
	dsList->addDataSet(m_dataSet, dataSetIdx, viewActions, editActions);
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

QVariantMap iADataSetViewer::attributeValues() const
{
	QVariantMap result(m_attribValues);
	if (renderer())
	{
		result.insert(renderer()->attributeValues());
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
	addAttr(m_attributes, name, valueType, defaultValue, min, max);
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
	setActionIcon(action, iconName, true);
	action->setCheckable(true);
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
	setApplyingValues(m_attribValues, m_attributes, values);
	// merge default values to currently set values to make applying simpler:
	auto allValues = joinValues(extractValues(attributesWithValues()), m_attribValues);
	applyAttributes(allValues);
	if (renderer())
	{
		renderer()->setAttributes(values);
	}
}

void iADataSetViewer::setRenderFlag(QChar const& flag, bool enable)
{
	auto flags = m_dataSet->metaData(RenderFlags).toString();
	if (enable)
	{
		if (!flags.contains(flag))
		{
			m_dataSet->setMetaData(RenderFlags, flags + flag);
		}
	}
	else
	{
		if (flags.contains(flag))
		{
			auto str = m_dataSet->metaData(RenderFlags).toString();
			m_dataSet->setMetaData(RenderFlags, str.replace(flag, ""));
		}
	}
}

bool iADataSetViewer::renderFlagSet(QChar const& flag) const
{
	return m_dataSet->metaData(RenderFlags).toString().contains(flag);
}

void iADataSetViewer::storeState()
{
	auto attrs = attributeValues();
	attrs.insert(additionalState());
	m_dataSet->setMetaData(attrs);
}

QVariantMap iADataSetViewer::additionalState() const
{
	return QVariantMap();
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
		LOG(lvlDebug, QString("afterRenderCallback (dataSetIdx: %1)").arg(dataSetIdx));
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
		// TODO NEWIO: check - this viewer is deleted on removing the dataset, so here the object could already be deleted!
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
			LOG(lvlDebug, QString("dataSetRendered (dataSetIdx: %1)").arg(dataSetIdx));
			// viewer settings are loaded along with the dataset, so they are applied directly in the respective viewers!
			// ... check whether the dataset that triggered this signal was the last one from this collection to be rendered....
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
	// if required, provide option to immediately load tool here without waiting for dataset to finish loading/rendering
	// this could be configured via some property defined in the iATool class.
	// current tools typically require datasets to be available, so this is not implemented
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
