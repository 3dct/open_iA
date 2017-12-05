/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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
#include "iABatchFilter.h"

#include "iAAttributeDescriptor.h"
#include "iAConsole.h"
#include "iAFilterRegistry.h"
#include "iAStringHelper.h"

iABatchFilter::iABatchFilter():
	iAFilter("Batch...", "Image Ensembles",
		"Runs a filter on a selected set of images")
{
	AddParameter("Image folder", String, "");
	AddParameter("File mask", String, "*.mhd");
	AddParameter("Filter", String, "Image Quality");
	AddParameter("Parameters", String, "");
}

void iABatchFilter::Run(QMap<QString, QVariant> const & parameters)
{
	auto filter = iAFilterRegistry::Filter(parameters["Filter"].toString());
	if (!filter)
	{
		DEBUG_LOG(QString("Batch: Cannot run filter '%1', it does not exist!").arg(parameters["Filter"].toString()));
		return;
	}
	QMap<QString, QVariant> filterParams;
	QStringList filterParamStrs = SplitPossiblyQuotedString(parameters["Parameters"].toString());
	if (filter->Parameters().size() != filterParamStrs.size())
	{
		DEBUG_LOG(QString("Batch: Invalid number of parameters: %1 expected, %2 given!")
			.arg(filter->Parameters().size())
			.arg(filterParamStrs.size()));
		return;
	}
	for (int i=0; i<filterParamStrs.size(); ++i)
	{
		filterParams.insert(filter->Parameters()[i]->Name(), filterParamStrs[i]);
	}
	
	// TODO: for each images from given folder with given mask:


	filter->SetUp(m_cons, m_log, m_progress);
	filter->Run(filterParams);

	for (auto outValue : filter->OutputValues())
	{
		DEBUG_LOG(QString("Batch outvalue: %1=%2").arg(outValue.first).arg(outValue.second.toString()));
	}
}

IAFILTER_CREATE(iABatchFilter);