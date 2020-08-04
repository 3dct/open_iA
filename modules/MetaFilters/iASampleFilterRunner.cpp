
/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASampleFilterRunner.h"

#include "iASampleParameterNames.h"

#include "iAConsole.h"
#include "iAFilter.h"
#include "iAFilterRegistry.h"

iASampleFilterRunner::iASampleFilterRunner(
	QMap<QString, QVariant> const& parameters,
	QVector<iAConnector*> input):
	m_parameters(parameters),
	m_input(input)
{
	assert(m_parameters[spnAlgorithmType].toString() == atBuiltIn);
}

QString iASampleFilterRunner::output() const
{
	return "";
}

bool iASampleFilterRunner::success() const
{
	return true;
}

double iASampleFilterRunner::duration() const
{
	return 0.0;
}

void iASampleFilterRunner::run()
{
	QString filterName = m_parameters[spnFilter].toString();
	auto filter = iAFilterRegistry::filter(filterName);
	if (!filter)
	{
		QString msg = QString("Filter '%1' does not exist!").arg(filterName);
		DEBUG_LOG(msg);
		return;
	}
	for (auto in : m_input)
	{
		filter->addInput(in);
	}
	//QObject::connect(&progress, &iAProgress::progress, ... , &::progress);
	//filter->setProgress(&progress);
	filter->run(m_parameters);
}