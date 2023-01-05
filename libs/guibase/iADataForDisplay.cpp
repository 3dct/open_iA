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
#include "iADataForDisplay.h"

#include "iADataSet.h"
#include "iAMdiChild.h"

#include "iAToolRegistry.h"

iADataForDisplay::iADataForDisplay(iADataSet* dataSet):
	m_dataSet(dataSet)
{
}

iADataForDisplay::~iADataForDisplay()
{}

void iADataForDisplay::show(iAMdiChild* child)
{	// by default, nothing to do
	Q_UNUSED(child);
}

QString iADataForDisplay::information() const
{
	return m_dataSet->info();
}

iADataSet* iADataForDisplay::dataSet()
{
	return m_dataSet;
}

void iADataForDisplay::applyPreferences(iAPreferences const& prefs)
{
	Q_UNUSED(prefs);
}

void iADataForDisplay::updatedPreferences()
{}

void iADataForDisplay::dataSetChanged()
{}



iADataSetViewer::iADataSetViewer(iADataSet const * dataSet) : m_dataSet(dataSet) {}

iADataSetViewer::~iADataSetViewer() {}

void iADataSetViewer::prepare(iAPreferences const& pref) {
	Q_UNUSED(pref);
}



iAVolumeViewer::iAVolumeViewer(iADataSet const * dataSet) : iADataSetViewer(dataSet)
{
}

void iAVolumeViewer::prepare(iAPreferences const& pref)
{
	Q_UNUSED(pref);
	//auto volData = dynamic_cast<iAImageData const*>(m_dataSet);
	//if (volData)
	//{
	//p.get(), m_preferences.HistogramBins);
}

void iAVolumeViewer::createGUI(iAMdiChild* child)
{
	Q_UNUSED(child);
}



iAMeshViewer::iAMeshViewer(iADataSet const * dataSet) : iADataSetViewer(dataSet)
{
}

void iAMeshViewer::prepare(iAPreferences const& pref)
{
	Q_UNUSED(pref);
}

void iAMeshViewer::createGUI(iAMdiChild* child)
{
	Q_UNUSED(child);
}



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
#include <iAMainWindow.h>

#include <QSettings>

void iAProjectViewer::createGUI(iAMdiChild* child)
{
	auto collection = dynamic_cast<iADataCollection const*>(m_dataSet);
	auto fileName = m_dataSet->metaData(iADataSet::FileNameKey).toString();
	auto afterRenderCallback = [this, child, collection, fileName]()
	{
		// all datasets loaded, continue with loading projects!
		auto const & settings = collection->settings();
		child->loadSettings(settings);
		auto tools = iAToolRegistry::toolKeys();
		auto registeredTools = iAToolRegistry::toolKeys();
		auto& projectFile = collection->settings();
		auto projectFileGroups = projectFile.childGroups();
		for (auto toolKey : registeredTools)
		{
			if (projectFileGroups.contains(toolKey))
			{
				auto tool = iAToolRegistry::createTool(toolKey, iAMainWindow::get(), child);
				projectFile.beginGroup(toolKey);
				child->addTool(toolKey, tool);
				tool->loadState(projectFile, fileName);
				projectFile.endGroup();
			}
		}
		// CHECK NEWIO: we could remove the viewer here from the mdi child (no API for that though - yet)
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