// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADataSetViewer.h"

#include "iAAABB.h"
#include "iAChannelID.h"    // for NotExistingChannel
#include "iADataSet.h"
#include "iADataSetRenderer.h"
#include "iADataSetListWidget.h"
#include "iAMdiChild.h"
#include "iAParameterDlg.h"
#include "iARenderer.h"
#include "iASlicer.h"

#include <iALog.h>
#include <iAMainWindow.h>

#include <vtkOpenGLRenderer.h>

#include <QAction>
#include <QDockWidget>

namespace
{
	const QString RenderFlagsDefault("R");
}

const QString iADataSetViewer::RenderFlags("RenderFlags");
const QChar iADataSetViewer::Render3DFlag('R');
const QChar iADataSetViewer::RenderOutlineFlag('B');
const QChar iADataSetViewer::RenderMagicLensFlag('L');
const QChar iADataSetViewer::RenderCutPlane('C');

iADataSetViewer::iADataSetViewer(iADataSet * dataSet):
	m_dataSet(dataSet),
	m_pickAction(nullptr)
{
}

iADataSetViewer::~iADataSetViewer()
{
}

void iADataSetViewer::prepare(iAProgress* p)
{
	Q_UNUSED(p);
}

void iADataSetViewer::adaptRendererSceneBounds(iAMdiChild* child)
{
	//TODO: not sure if this is best done here or somewhere else, maybe some kind of dataset manager ?
	iAAABB overallBB;
	for (auto ds : child->dataSetMap())
	{
		if (child->dataSetViewer(ds.first)->renderer())   // e.g. project files don't have a renderer...
		{
			overallBB.merge(child->dataSetViewer(ds.first)->renderer()->bounds());
		}
	}
	child->renderer()->setSceneBounds(overallBB);
}

void iADataSetViewer::createGUI(iAMdiChild* child, size_t dataSetIdx)
{
	m_child = child;
	m_dataSetIdx = dataSetIdx;
	if (!m_dataSet->hasMetaData(RenderFlags))    // only use default render flags if not set in dataset
	{                                            // (by loader or by derived viewer class)
		m_dataSet->setMetaData(RenderFlags, RenderFlagsDefault);
	}
	m_renderer = createRenderer(child->renderer()->renderer(), m_dataSet->allMetaData());
	assert(m_renderer);
	if (renderFlagSet(Render3DFlag))
	{
		m_renderer->setVisible(true);
		if (!child->renderDockWidget()->isVisible())
		{
			child->renderDockWidget()->setVisible(true);
		}
	}
	if (renderFlagSet(RenderOutlineFlag))
	{
		m_renderer->setBoundsVisible(true);
	}
	m_magicLensRenderer = createRenderer(child->magicLens3DRenderer(), m_dataSet->allMetaData());
	if (renderFlagSet(RenderMagicLensFlag))
	{
		m_magicLensRenderer->setVisible(true);
	}
	if (renderFlagSet(RenderCutPlane))
	{
		m_renderer->setCuttingPlanes(child->renderer()->slicePlanes());
	}
	adaptRendererSceneBounds(child);
	child->dataSetListWidget()->addDataSet(m_dataSet, dataSetIdx);

	auto editAction = new QAction("Edit dataset and display properties");
	connect(editAction, &QAction::triggered, this,
		[this, child, dataSetIdx]()
		{
			auto prevUnitDistance = m_dataSet->unitDistance();
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
			if (m_dataSet->unitDistance() != prevUnitDistance)
			{
				child->updatePositionMarkerSize();
				for (int s = 0; s < 3; ++s)
				{
					auto slicer = child->slicer(s);
					slicer->updateChannelMappers();  // to make possible spacing changes in image arrive in renderer
					//child->set3DSlicePlanePos(s, child->sliceNumber(s));  // private...
					slicer->resetCamera();
					slicer->update();
				}
				adaptRendererSceneBounds(child);    // automatically resets scene if necessary
			}
			child->updateRenderer();  // currently, 3D renderer properties are changed only
			emit dataSetChanged(dataSetIdx);
		});
	iAMainWindow::get()->addActionIcon(editAction, "edit");
	child->dataSetListWidget()->addAction(dataSetIdx, editAction, iADataSetListWidget::Edit);

	auto removeAction = new QAction("Remove");
	removeAction->setToolTip("Remove dataset from display, unload from memory");
	connect(removeAction, &QAction::triggered, this,
		[this, dataSetIdx, child]()
		{
			child->dataSetListWidget()->removeDataSet(dataSetIdx);
			emit removeDataSet(dataSetIdx);
		});
	iAMainWindow::get()->addActionIcon(removeAction, "delete");
	child->dataSetListWidget()->addAction(dataSetIdx, removeAction, iADataSetListWidget::Edit);

	addViewAction("3D", "3d", renderFlagSet(Render3DFlag),
		[this, child](bool checked)
		{
			setRenderFlag(Render3DFlag, checked);
			m_renderer->setVisible(checked);
			child->updateRenderer();
		});
	addViewAction("Box", "box_3d_edge", renderFlagSet(RenderOutlineFlag),
		[this, child](bool checked)
		{
			setRenderFlag(RenderOutlineFlag, checked);
			m_renderer->setBoundsVisible(checked);
			child->updateRenderer();
		});
	addViewAction("Magic Lens", "magic_lens_3d", renderFlagSet(RenderMagicLensFlag),
		[this, child](bool checked)
		{
			setRenderFlag(RenderMagicLensFlag, checked);
			m_magicLensRenderer->setVisible(checked);
			child->updateRenderer();
		});
	addViewAction("Cut Plane", "cut_plane", renderFlagSet(RenderCutPlane),
		[this, child](bool checked)
		{
			setRenderFlag(RenderCutPlane, checked);
			if (checked)
			{
				m_renderer->setCuttingPlanes(child->renderer()->slicePlanes());
				child->renderer()->setCuttingActive(true);
			}
			else
			{
				m_renderer->removeCuttingPlanes();
				// only disable renderer updates on slice plane changes if cutting is disabled for all other datasets viewers:
				bool allDisabled = true;
				for (auto d : child->dataSetMap())
				{
					auto v = child->dataSetViewer(d.first);
					if (v->renderFlagSet(RenderCutPlane))
					{
						allDisabled = false;
						break;
					}
				}
				if (allDisabled)
				{
					child->renderer()->setCuttingActive(false);
				}
			}
			child->updateRenderer();
		});
	m_pickAction = addViewAction("Pickable", "transform-move", false,
		[child, dataSetIdx](bool checked)
		{
			Q_UNUSED(checked);
			if (checked)
			{
				child->setDataSetMovable(dataSetIdx);
			}
		});
	m_pickAction->setEnabled(false);
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

std::shared_ptr<iADataSetRenderer> iADataSetViewer::createRenderer(vtkRenderer* ren, QVariantMap const & paramValues)
{
	Q_UNUSED(ren);
	Q_UNUSED(paramValues);
	return {};
}

QAction* iADataSetViewer::addViewAction(QString const& name, QString const& iconName, bool checked, std::function<void(bool)> handler)
{
	assert(m_child);
	auto action = new QAction(name);
	action->setToolTip(QString("Toggle %1").arg(name));
	action->setCheckable(true);
	action->setChecked(checked);
	iAMainWindow::get()->addActionIcon(action, iconName);
	connect(action, &QAction::triggered, this, handler);
	m_child->dataSetListWidget()->addAction(m_dataSetIdx, action, iADataSetListWidget::View);
	return action;
}

void iADataSetViewer::setAttributes(QVariantMap const& values)
{
	setApplyingValues(m_attribValues, m_attributes, values);
	// merge default values to currently set values to make applying simpler:
	auto allValues = joinValues(extractValues(attributesWithValues()), m_attribValues);
	applyAttributes(allValues);
	if (m_renderer)
	{
		m_renderer->setAttributes(values);
	}
	if (m_magicLensRenderer)
	{
		m_magicLensRenderer->setAttributes(values);
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

uint iADataSetViewer::slicerChannelID() const
{
	return NotExistingChannel;
}

void iADataSetViewer::applyAttributes(QVariantMap const& values)
{
	Q_UNUSED(values);
}
