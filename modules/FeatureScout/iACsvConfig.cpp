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
	columnSeparator(";"),
	decimalSeparator("."),
	addAutoID(false),
	objectType(iAFeatureScoutObjectType::Voids),
	unit("microns"),
	spacing(0.0f),
	computeLength(false),
	computeAngles(false),
	computeTensors(false),
	computeCenter(false),
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
	if ((computeLength || computeAngles || computeTensors || computeCenter) && (
		!columnMapping.contains(iACsvConfig::StartX) ||
		!columnMapping.contains(iACsvConfig::StartY) ||
		!columnMapping.contains(iACsvConfig::StartZ) ||
		!columnMapping.contains(iACsvConfig::EndX) ||
		!columnMapping.contains(iACsvConfig::EndY) ||
		!columnMapping.contains(iACsvConfig::EndZ)))
	{
		errorMsg = "Cannot compute length/angles/tensors/center without fully defined start and end position! "
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

iACsvConfig const & iACsvConfig::getLegacyFiberFormat(QString const & fileName)
{
	static iACsvConfig LegacyFormat;
	LegacyFormat.fileName = fileName;
	LegacyFormat.encoding = "System";
	LegacyFormat.containsHeader = false;
	LegacyFormat.skipLinesStart = 5;
	LegacyFormat.skipLinesEnd = 0;
	LegacyFormat.columnSeparator = ",";
	LegacyFormat.decimalSeparator = ".";
	LegacyFormat.addAutoID = false;
	LegacyFormat.objectType = iAFeatureScoutObjectType::Fibers;
	LegacyFormat.computeLength = false;
	LegacyFormat.computeAngles = true;
	LegacyFormat.computeTensors = true;
	LegacyFormat.computeCenter = true;
	LegacyFormat.currentHeaders = QStringList() << "Label"
		<< "X1[µm]"
		<< "Y1[µm]"
		<< "Z1[µm]"
		<< "X2[µm]"
		<< "Y2[µm]"
		<< "Z2[µm]"
		<< "StraightLength[µm]"
		<< "CurvedLength[µm]"
		<< "Diameter[µm]"
		<< "Surface[µm²]"
		<< "Volume[µm³]"
		<< "SeperatedFibre"
		<< "CurvedFibre";
	LegacyFormat.selectedHeaders = LegacyFormat.currentHeaders;
	LegacyFormat.columnMapping.clear();
	LegacyFormat.columnMapping.insert(StartX,   LegacyFormat.currentHeaders[1]);
	LegacyFormat.columnMapping.insert(StartY,   LegacyFormat.currentHeaders[2]);
	LegacyFormat.columnMapping.insert(StartZ,   LegacyFormat.currentHeaders[3]);
	LegacyFormat.columnMapping.insert(EndX,     LegacyFormat.currentHeaders[4]);
	LegacyFormat.columnMapping.insert(EndY,     LegacyFormat.currentHeaders[5]);
	LegacyFormat.columnMapping.insert(EndZ,     LegacyFormat.currentHeaders[6]);
	LegacyFormat.columnMapping.insert(Length,   LegacyFormat.currentHeaders[7]);
	LegacyFormat.columnMapping.insert(Diameter, LegacyFormat.currentHeaders[9]);
	return LegacyFormat;
}

iACsvConfig const & iACsvConfig::getLegacyPoreFormat(QString const & fileName)
{
	static iACsvConfig LegacyFormat;
	LegacyFormat.fileName = fileName;
	LegacyFormat.encoding = "System";
	LegacyFormat.containsHeader = true;
	LegacyFormat.skipLinesStart = 4;
	LegacyFormat.skipLinesEnd = 0;
	LegacyFormat.columnSeparator = ",";
	LegacyFormat.decimalSeparator = ".";
	LegacyFormat.addAutoID = false;
	LegacyFormat.objectType = iAFeatureScoutObjectType::Fibers;
	LegacyFormat.computeLength = false;
	LegacyFormat.computeAngles = false;
	LegacyFormat.computeTensors = false;
	LegacyFormat.computeCenter = false;
	LegacyFormat.currentHeaders.clear();
	LegacyFormat.selectedHeaders.clear();
	LegacyFormat.columnMapping.clear();
	return LegacyFormat;
}