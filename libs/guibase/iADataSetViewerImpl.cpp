// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iADataSetViewerImpl.h"

#include "iAPolyData.h"
#include "iADataSetRendererImpl.h"



// iAGraphViewer

iAGraphViewer::iAGraphViewer(iADataSet* dataSet) :
	iADataSetViewer(dataSet)
{}

std::shared_ptr<iADataSetRenderer> iAGraphViewer::createRenderer(vtkRenderer* ren, QVariantMap const& paramValues)
{
	auto meshData = dynamic_cast<iAGraphData const*>(m_dataSet);
	return std::make_shared<iAGraphRenderer>(ren, meshData, paramValues);
}



// iAMeshViewer

iAMeshViewer::iAMeshViewer(iADataSet* dataSet) :
	iADataSetViewer(dataSet)
{}

std::shared_ptr<iADataSetRenderer> iAMeshViewer::createRenderer(vtkRenderer* ren, QVariantMap const& paramValues)
{
	auto meshData = dynamic_cast<iAPolyData const*>(m_dataSet);
	return std::make_shared<iAPolyDataRenderer>(ren, meshData, paramValues);
}



// iAGeometricObjectViewer

#include "iAGeometricObject.h"

iAGeometricObjectViewer::iAGeometricObjectViewer(iADataSet* dataSet) :
	iADataSetViewer(dataSet)
{
	auto meshData = dynamic_cast<iAGeometricObject*>(m_dataSet);
	for (auto a : meshData->objectProperties())
	{
		addAttribute(a->name(), a->valueType(), a->defaultValue(), a->min(), a->max());
	}
}

std::shared_ptr<iADataSetRenderer> iAGeometricObjectViewer::createRenderer(vtkRenderer* ren, QVariantMap const& paramValues)
{
	auto meshData = dynamic_cast<iAGeometricObject const*>(m_dataSet);
	return std::make_shared<iAGeometricObjectRenderer>(ren, meshData, paramValues);
}

void iAGeometricObjectViewer::applyAttributes(QVariantMap const& values)
{
	auto meshData = dynamic_cast<iAGeometricObject*>(m_dataSet);
	meshData->applyAttributes(values);
}



// iAProjectViewer

iAProjectViewer::iAProjectViewer(iADataSet* dataSet) :
	iADataSetViewer(dataSet),
	m_numOfDataSets(0)
{
	auto collection = dynamic_cast<iADataCollection const*>(m_dataSet);
	assert(collection);
	m_numOfDataSets = collection->dataSets().size();
}


#include "iAMdiChild.h"
#include "iAMainWindow.h"
#include "iATool.h"
#include "iAToolRegistry.h"

#include <iAStringHelper.h>

#include <QSettings>

void iAProjectViewer::createGUI(iAMdiChild* child, size_t dataSetIdx)
{
	auto collection = dynamic_cast<iADataCollection const*>(m_dataSet);
	auto fileName = m_dataSet->metaData(iADataSet::FileNameKey).toString();
	auto afterRenderCallback = [child, collection, fileName, dataSetIdx]()
		{
			// all datasets loaded, continue with loading projects!
			if (!collection->settings())    // not all collections come with additional settings...
			{
				return;
			}
			auto& settings = *collection->settings().get();
			child->loadSettings(settings);
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
		};
	// if no datasets available, directly load tools...
	if (collection->dataSets().empty())
	{
		afterRenderCallback();
		return;
	}
	// ...otherwise load datasets first...
	QObject::connect(child, &iAMdiChild::dataSetRendered, this,
		[this, afterRenderCallback](size_t dataSetIdx)
		{
			// viewer settings are loaded along with the dataset, so they are applied directly in the respective viewers!
			// ... check whether the dataset that triggered this signal was the last one from this collection to be rendered....
			m_renderedDataSets.insert(dataSetIdx);
			if (m_loadedDataSets.size() < m_numOfDataSets)
			{
				return;
			}
			for (auto l : m_loadedDataSets)
			{
				if (m_renderedDataSets.find(l) == m_renderedDataSets.end())
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

std::map<iADataSetType, iADataSetViewerCreateFuncPtr>& dataSetViewerFactoryMap()
{
	static std::map<iADataSetType, iADataSetViewerCreateFuncPtr> m;
	if (m.empty())    // not thread-safe!
	{
		m.insert(std::make_pair(iADataSetType::Volume, createFunc<iAVolumeViewer>));
		m.insert(std::make_pair(iADataSetType::Mesh, createFunc<iAMeshViewer>));
		m.insert(std::make_pair(iADataSetType::GeometricObject, createFunc<iAGeometricObjectViewer>));
		m.insert(std::make_pair(iADataSetType::Graph, createFunc<iAGraphViewer>));
		m.insert(std::make_pair(iADataSetType::Collection, createFunc<iAProjectViewer>));
	}
	return m;
}

std::shared_ptr<iADataSetViewer> createDataSetViewer(iADataSet* dataSet)
{
	if (dataSetViewerFactoryMap().find(dataSet->type()) == dataSetViewerFactoryMap().end())
	{
		return std::shared_ptr<iADataSetViewer>();
	}
	return dataSetViewerFactoryMap()[dataSet->type()](dataSet);
}
