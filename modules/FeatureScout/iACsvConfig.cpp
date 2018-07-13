/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iACsvConfig.h"

#include <QFile>

iACsvConfig::iACsvConfig() :
	fileName(""),
	encoding("System"),
	skipLinesStart(LegacyFormatStartSkipLines),
	skipLinesEnd(0),
	colSeparator(";"),
	decimalSeparator("."),
	addAutoID(false),
	objectType(iAFeatureScoutObjectType::Voids),
	unit("microns"),
	spacing(0.0f),
	computeLength(false),
	computeAngles(false),
	computeTensors(false),
	containsHeader(true)
{}

bool iACsvConfig::isValid(QString & errorMsg) const
{
	if (fileName.isEmpty())
	{
		errorMsg = "Please specify a filename!";
		return false;
	}
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
	{
		errorMsg = QString("Unable to open file: %1!").arg(file.errorString());
		return false;
	}
	file.close();
	if ((computeLength || computeAngles || computeTensors) && (
		!columnMapping.contains(iACsvConfig::StartX) ||
		!columnMapping.contains(iACsvConfig::StartY) ||
		!columnMapping.contains(iACsvConfig::StartZ) ||
		!columnMapping.contains(iACsvConfig::EndX) ||
		!columnMapping.contains(iACsvConfig::EndY) ||
		!columnMapping.contains(iACsvConfig::EndZ)))
	{
		errorMsg = "Cannot compute length/angles/tensors without fully defined start and end position! "
			"Please specify a mapping to the column containing start (x, y, z) and end (x, y, z) coordinate!";
			return false;
	}
	if (selectedHeaders.size() < 1)
	{
		errorMsg = "Please select at least one column to load!";
		return false;
	}
	return true;
}