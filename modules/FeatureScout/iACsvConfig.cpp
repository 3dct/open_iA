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
#include "iACsvConfig.h"

#include <QFile>
#include <QSettings>

namespace
{
	const char * VisualizationTypeName[iACsvConfig::VisTypeCount] =
	{
		"Labelled Volume",
		"Lines",
		"Cylinders",
		"Ellipses",
		"No Visualization"
	};
}

const QString iACsvConfig::LegacyFiberFormat("Legacy Fiber csv");
const QString iACsvConfig::LegacyVoidFormat("Legacy Pore csv");

namespace csvRegKeys
{
	static const QString SettingsName("FeatureScout");
	static const QString FormatKeyName("CSVFormats");
	static const QString SelectedHeaders = "SelectedHeaders";
	static const QString AllHeaders = "AllHeaders";
	static const QString SkipLinesStart = "SkipLinesStart";
	static const QString SkipLinesEnd = "SkipLinesEnd";
	static const QString ColSeparator = "ColumnSeparator";
	static const QString DecimalSeparator = "DecimalSeparator";
	static const QString Spacing = "Spacing";
	static const QString Unit = "Unit";
	static const QString ObjectType = "ObjectType";
	static const QString AddAutoID = "AddAutoID";
	static const QString Encoding = "Encoding";
	static const QString ComputeLength = "ComputeLength";
	static const QString ComputeAngles = "ComputeAngles";
	static const QString ComputeTensors = "ComputeTensors";
	static const QString ComputeCenter = "ComputeCenter";
	static const QString ComputeStartEnd = "ComputeStartEnd";
	static const QString ContainsHeader = "ContainsHeader";
	static const QString VisualizationType = "VisualizationType";
	static const QString ColumnMappings = "ColumnMappings";
	static const QString Offset = "Offset";
	static const QString IsDiameterFixed = "IsDiameterFixed";
	static const QString FixedDiameterValue = "FixedDiameterValue";
}

QString MapVisType2Str(iACsvConfig::VisualizationType visType)
{
	return VisualizationTypeName[visType];
}

iACsvConfig::VisualizationType MapStr2VisType(QString name)
{
	for (int i = 0; i < iACsvConfig::VisTypeCount; ++i)
	{
		if (name == VisualizationTypeName[i])
			return static_cast<iACsvConfig::VisualizationType>(i);
	}
	return iACsvConfig::UseVolume;
}

iACsvConfig::iACsvConfig() :
	fileName(""),
	encoding("System"),
	containsHeader(true),
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
	computeStartEnd(false),
	visType(UseVolume),
	cylinderQuality(12),
	segmentSkip(1),
	isDiameterFixed(false),
	fixedDiameterValue(0.0)
{
	std::fill(offset, offset + 3, 0.0);
}

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
		errorMsg = QString("Unable to open file '%1': %2!")
			.arg(fileName).arg(file.errorString());
		return false;
	}
	file.close();
	if (computeStartEnd && (
		!columnMapping.contains(iACsvConfig::CenterX) ||
		!columnMapping.contains(iACsvConfig::CenterY) ||
		!columnMapping.contains(iACsvConfig::CenterZ) ||
		!columnMapping.contains(iACsvConfig::Phi) ||
		!columnMapping.contains(iACsvConfig::Theta) ||
		!columnMapping.contains(iACsvConfig::Length)))
	{
		errorMsg = "Cannot compute start/end without center position, phi, theta and length! "
			"Please specify a mapping to the columns containing these values!";
		return false;
	}
	if ((computeLength || computeAngles || computeCenter) && (
		!columnMapping.contains(iACsvConfig::StartX) ||
		!columnMapping.contains(iACsvConfig::StartY) ||
		!columnMapping.contains(iACsvConfig::StartZ) ||
		!columnMapping.contains(iACsvConfig::EndX) ||
		!columnMapping.contains(iACsvConfig::EndY) ||
		!columnMapping.contains(iACsvConfig::EndZ)))
	{
		errorMsg = "Cannot compute length/angles/center without fully defined start and end position! "
			"Please specify a mapping to the column containing start (x, y, z) and end (x, y, z) coordinate!";
		return false;
	}
	if (computeTensors && (!computeAngles && (!columnMapping.contains(iACsvConfig::Phi) || !columnMapping.contains(iACsvConfig::Theta))))
	{
		errorMsg = "Cannot compute tensors without angles. Either enable to compute them, or specify where to find them!";
		return false;
	}
	if ((visType == Lines || visType == Cylinders) && (
		!computeStartEnd && (
			!columnMapping.contains(iACsvConfig::StartX) ||
			!columnMapping.contains(iACsvConfig::StartY) ||
			!columnMapping.contains(iACsvConfig::StartZ) ||
			!columnMapping.contains(iACsvConfig::EndX) ||
			!columnMapping.contains(iACsvConfig::EndY) ||
			!columnMapping.contains(iACsvConfig::EndZ)
		)))
	{
		errorMsg = "Visualization as Lines or Cylinders requires start and end position column, please specify where to find these!";
		return false;
	}
	if (visType == Cylinders &&
		(!columnMapping.contains(iACsvConfig::Diameter) && !isDiameterFixed) )
	{
		errorMsg = "Visualization as Cylinders requires start- and end-position as well as a diameter, please specify where to find these!";
		return false;
	}
	if (visType == Ellipses && (
		!columnMapping.contains(iACsvConfig::CenterX) ||
		!columnMapping.contains(iACsvConfig::CenterY) ||
		!columnMapping.contains(iACsvConfig::CenterZ) ||
		!columnMapping.contains(iACsvConfig::DimensionX) ||
		!columnMapping.contains(iACsvConfig::DimensionY) ||
		!columnMapping.contains(iACsvConfig::DimensionZ)))
	{
		errorMsg = "Visualization as Ellipses requires column mapping for the center point and the dimensions (in each direction), please specify where to find these!";
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
	LegacyFormat.computeStartEnd = false;
	std::fill(LegacyFormat.offset, LegacyFormat.offset + 3, 0.0);
	LegacyFormat.visType = UseVolume;
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
	LegacyFormat.columnMapping.insert(StartX,   1);
	LegacyFormat.columnMapping.insert(StartY,   2);
	LegacyFormat.columnMapping.insert(StartZ,   3);
	LegacyFormat.columnMapping.insert(EndX,     4);
	LegacyFormat.columnMapping.insert(EndY,     5);
	LegacyFormat.columnMapping.insert(EndZ,     6);
	LegacyFormat.columnMapping.insert(Length,   7);
	LegacyFormat.columnMapping.insert(Diameter, 9);
	return LegacyFormat;
}

iACsvConfig const & iACsvConfig::getLegacyPoreFormat(QString const & fileName)
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
	LegacyFormat.objectType = iAFeatureScoutObjectType::Voids;
	LegacyFormat.computeLength = false;
	LegacyFormat.computeAngles = false;
	LegacyFormat.computeTensors = false;
	LegacyFormat.computeCenter = false;
	LegacyFormat.computeStartEnd = false;
	std::fill(LegacyFormat.offset, LegacyFormat.offset + 3, 0.0);
	LegacyFormat.visType = UseVolume;
	LegacyFormat.currentHeaders = QStringList()
		<< "Label Id"
		<< "X1"	<< "Y1"	<< "Z1"
		<< "X2"	<< "Y2"	<< "Z2"
		<< "a11" << "a22" << "a33"
		<< "a12" << "a13" << "a23"
		<< "DimX" << "DimY" << "DimZ"
		<< "Phi" << "Theta"
		<< "Xm" << "Ym" << "Zm"
		<< "Volume"
		<< "Roundness"
		<< "FeretDiam"
		<< "Flatness"
		<< "VoxDimX" << "VoxDimY" << "VoxDimZ"
		<< "MajorLength"
		<< "MinorLength";
	LegacyFormat.selectedHeaders = LegacyFormat.currentHeaders;
	LegacyFormat.columnMapping.clear();
	LegacyFormat.columnMapping.insert(StartX,    1);
	LegacyFormat.columnMapping.insert(StartY,    2);
	LegacyFormat.columnMapping.insert(StartZ,    3);
	LegacyFormat.columnMapping.insert(EndX,      4);
	LegacyFormat.columnMapping.insert(EndY,      5);
	LegacyFormat.columnMapping.insert(EndZ,      6);
	LegacyFormat.columnMapping.insert(DimensionX,13);
	LegacyFormat.columnMapping.insert(DimensionY,14);
	LegacyFormat.columnMapping.insert(DimensionZ,15);
	LegacyFormat.columnMapping.insert(Phi,      16);
	LegacyFormat.columnMapping.insert(Theta,    17);
	LegacyFormat.columnMapping.insert(CenterX,  18);
	LegacyFormat.columnMapping.insert(CenterY,  19);
	LegacyFormat.columnMapping.insert(CenterZ,  20);
	LegacyFormat.columnMapping.insert(Diameter, 23);
	LegacyFormat.columnMapping.insert(Length,   28);
	return LegacyFormat;
}

QString iACsvConfig::getFormatKey(QString const & formatName)
{
	return csvRegKeys::SettingsName + "/" + csvRegKeys::FormatKeyName + "/" + formatName;
}

QStringList iACsvConfig::getListFromRegistry()
{
	QSettings settings;
	settings.beginGroup(getFormatKey(""));
	return settings.childGroups();
}

void iACsvConfig::save(QSettings & settings, QString const & formatName)
{
	settings.beginGroup(iACsvConfig::getFormatKey(formatName));
	settings.setValue(csvRegKeys::SkipLinesStart, static_cast<qulonglong>(skipLinesStart));
	settings.setValue(csvRegKeys::SkipLinesEnd, static_cast<qulonglong>(skipLinesEnd));
	settings.setValue(csvRegKeys::ColSeparator, columnSeparator);
	settings.setValue(csvRegKeys::DecimalSeparator, decimalSeparator);
	settings.setValue(csvRegKeys::ObjectType, MapObjectTypeToString(objectType));
	settings.setValue(csvRegKeys::AddAutoID, addAutoID);
	settings.setValue(csvRegKeys::Unit, unit);
	settings.setValue(csvRegKeys::Spacing, spacing);
	settings.setValue(csvRegKeys::Encoding, encoding);
	settings.setValue(csvRegKeys::ComputeLength, computeLength);
	settings.setValue(csvRegKeys::ComputeAngles, computeAngles);
	settings.setValue(csvRegKeys::ComputeTensors, computeTensors);
	settings.setValue(csvRegKeys::ComputeCenter, computeCenter);
	settings.setValue(csvRegKeys::ComputeStartEnd, computeStartEnd);
	settings.setValue(csvRegKeys::ContainsHeader, containsHeader);
	settings.setValue(csvRegKeys::IsDiameterFixed, isDiameterFixed);
	settings.setValue(csvRegKeys::FixedDiameterValue, fixedDiameterValue);
	settings.setValue(csvRegKeys::VisualizationType, MapVisType2Str(visType));
	for (int i=0; i<3; ++i)
		settings.setValue(csvRegKeys::Offset+QString::number(i), offset[i]);
	// save column mappings:
	QStringList columnMappings;
	for (auto key : columnMapping.keys())
		columnMappings.append(QString("%1:%2").arg(key).arg(columnMapping[key]));
	settings.setValue(csvRegKeys::ColumnMappings, columnMappings);
	settings.setValue(csvRegKeys::SelectedHeaders, selectedHeaders);
	settings.setValue(csvRegKeys::AllHeaders, currentHeaders);
	settings.endGroup();
}

bool iACsvConfig::load(QSettings & settings, const QString & formatName)
{
	settings.beginGroup(iACsvConfig::getFormatKey(formatName));
	QStringList allEntries = settings.allKeys();
	iACsvConfig defaultConfig;
	skipLinesStart = settings.value(csvRegKeys::SkipLinesStart, static_cast<qulonglong>(defaultConfig.skipLinesStart)).toULongLong();
	skipLinesEnd = settings.value(csvRegKeys::SkipLinesEnd, static_cast<qulonglong>(defaultConfig.skipLinesEnd)).toULongLong();
	columnSeparator = settings.value(csvRegKeys::ColSeparator, defaultConfig.columnSeparator).toString();
	decimalSeparator = settings.value(csvRegKeys::DecimalSeparator, defaultConfig.decimalSeparator).toString();
	objectType = MapStringToObjectType(settings.value(csvRegKeys::ObjectType, MapObjectTypeToString(defaultConfig.objectType)).toString());
	addAutoID = settings.value(csvRegKeys::AddAutoID, defaultConfig.addAutoID).toBool();
	computeLength = settings.value(csvRegKeys::ComputeLength, defaultConfig.computeLength).toBool();
	computeAngles = settings.value(csvRegKeys::ComputeAngles, defaultConfig.computeAngles).toBool();
	computeTensors = settings.value(csvRegKeys::ComputeTensors, defaultConfig.computeTensors).toBool();
	computeCenter = settings.value(csvRegKeys::ComputeCenter, defaultConfig.computeCenter).toBool();
	computeStartEnd = settings.value(csvRegKeys::ComputeStartEnd, defaultConfig.computeStartEnd).toBool();
	containsHeader = settings.value(csvRegKeys::ContainsHeader, defaultConfig.containsHeader).toBool();
	isDiameterFixed = settings.value(csvRegKeys::IsDiameterFixed, defaultConfig.isDiameterFixed).toBool();
	fixedDiameterValue = settings.value(csvRegKeys::FixedDiameterValue, defaultConfig.fixedDiameterValue).toDouble();
	visType = MapStr2VisType(settings.value(csvRegKeys::VisualizationType, MapVisType2Str(defaultConfig.visType)).toString());
	for (int i = 0; i<3; ++i)
		offset[i] = settings.value(csvRegKeys::Offset + QString::number(i), defaultConfig.offset[i]).toDouble();
	unit = settings.value(csvRegKeys::Unit, defaultConfig.unit).toString();
	spacing = settings.value(csvRegKeys::Spacing, defaultConfig.spacing).toDouble();
	encoding = settings.value(csvRegKeys::Encoding, defaultConfig.encoding).toString();
	selectedHeaders = settings.value(csvRegKeys::SelectedHeaders, defaultConfig.currentHeaders).toStringList();
	currentHeaders = settings.value(csvRegKeys::AllHeaders, defaultConfig.currentHeaders).toStringList();
	// load column mappings:
	columnMapping.clear();
	QStringList columnMappings = settings.value(csvRegKeys::ColumnMappings).toStringList();
	for (QString mapping : columnMappings)
	{
		uint columnKey = mapping.section(":", 0, 0).toInt();
		uint columnNumber = mapping.section(":", 1).toInt();
		columnMapping.insert(columnKey, columnNumber);
	}
	settings.endGroup();
	return !allEntries.isEmpty();
}
