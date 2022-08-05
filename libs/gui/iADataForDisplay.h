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
#pragma once

#include "iAgui_export.h"

#include <QString>

#include <memory>

class iADataSet;
class iAMdiChild;

//! Base class for data linked to a dataset required for displaying it,
//! in addition to the dataset itself (e.g., the histogram for a volume dataset)
//! TODO: Find a better name!
class iAgui_API iADataForDisplay
{
public:
	iADataForDisplay(iADataSet* dataSet);
	//! called from GUI thread when the data computation (via createDataForDisplay method below) is complete
	virtual void show(iAMdiChild* child);
	//! called when the dataset is removed and its related controls should close down
	virtual ~iADataForDisplay();
	//! Get information to display about the dataset
	virtual QString information() const;
protected:
	iADataSet* const dataSet();
private:
	iADataSet* m_dataSet;
};

std::shared_ptr<iADataForDisplay> createDataForDisplay(iADataSet* dataSet, int numBins);
