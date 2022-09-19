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

#include "iAImageDataForDisplay.h"

#include "iADataSet.h"

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
{
}

std::shared_ptr<iADataForDisplay> createDataForDisplay(iADataSet* dataSet, iAProgress* p, int numBins)
{
	auto volData = dynamic_cast<iAImageData*>(dataSet);
	if (volData)
	{
		return std::make_shared<iAImageDataForDisplay>(volData, p, numBins);
	}
	return std::make_shared<iADataForDisplay>(dataSet);
}
