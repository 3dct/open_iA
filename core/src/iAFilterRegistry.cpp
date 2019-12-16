/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iAFilterRegistry.h"

#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRunnerGUI.h"

void iAFilterRegistry::addFilterFactory(QSharedPointer<iAIFilterFactory> factory)
{
	m_filters.push_back(factory);
	m_runner.push_back(QSharedPointer<iAIFilterRunnerGUIFactory>(new iAFilterRunnerGUIFactory<iAFilterRunnerGUI>()));
}

void iAFilterRegistry::addFilterFactory(QSharedPointer<iAIFilterFactory> factory,
	QSharedPointer<iAIFilterRunnerGUIFactory> runner)
{
	m_filters.push_back(factory);
	m_runner.push_back(runner);
}

QVector<QSharedPointer<iAIFilterFactory>> const & iAFilterRegistry::filterFactories()
{
	return m_filters;
}

QSharedPointer<iAFilter> iAFilterRegistry::filter(QString const & name)
{
	int id = filterID(name);
	return id == -1 ? QSharedPointer<iAFilter>() : m_filters[id]->create();
}

int iAFilterRegistry::filterID(QString const & name)
{
	int cur = 0;
	for (auto filterFactory : m_filters)
	{
		auto filter = filterFactory->create();
		if (filter->name() == name)
		{
			return cur;
		}
		++cur;
	}
	DEBUG_LOG(QString("Filter '%1' not found!").arg(name));
	return -1;
}

QSharedPointer<iAIFilterRunnerGUIFactory> iAFilterRegistry::filterRunner(int filterID)
{
	return m_runner[filterID];
}

QVector<QSharedPointer<iAIFilterFactory> > iAFilterRegistry::m_filters;
QVector<QSharedPointer<iAIFilterRunnerGUIFactory> > iAFilterRegistry::m_runner;
