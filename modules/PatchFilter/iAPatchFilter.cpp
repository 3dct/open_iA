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
#include "iAPatchFilter.h"

#include "iAAttributeDescriptor.h"
#include "iAConnector.h"
#include "iAFilterRegistry.h"
#include "iAProgress.h"

iAPatchFilter::iAPatchFilter():
	iAFilter("Patch Filter", "Image Ensembles",
		"Create patches from an input image and apply a filter each patch.<br/>")
{
	AddParameter("Patch Count X", Discrete, 1, 1);
	AddParameter("Patch Count Y", Discrete, 1, 1);
	AddParameter("Patch Count Z", Discrete, 1, 1);
	AddParameter("Filter", String, "Image Quality");
	AddParameter("Parameters", String, "");
	AddParameter("Additional Input", String, "");
}

void iAPatchFilter::PerformWork(QMap<QString, QVariant> const & parameters)
{
	auto filter = iAFilterRegistry::Filter(parameters["Filter"].toString());
	if (!filter)
	{
		AddMsg(QString("Patch: Cannot run filter '%1', it does not exist!").arg(parameters["Filter"].toString()));
		return;
	}


}

IAFILTER_CREATE(iAPatchFilter);
