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

namespace
{
	static const QString CfgKeySettingsName("FeatureScout");
	static const QString CfgKeyFormatKeyName("CSVFormats");
	static const QString CfgKeySelectedHeaders = "SelectedHeaders";
	static const QString CfgKeyAllHeaders = "AllHeaders";
	static const QString CfgKeySkipLinesStart = "SkipLinesStart";
	static const QString CfgKeySkipLinesEnd = "SkipLinesEnd";
	static const QString CfgKeyColSeparator = "ColumnSeparator";
	static const QString CfgKeyDecimalSeparator = "DecimalSeparator";
	static const QString CfgKeySpacing = "Spacing";
	static const QString CfgKeyUnit = "Unit";
	static const QString CfgKeyObjectType = "ObjectType";
	static const QString CfgKeyAddAutoID = "AddAutoID";
	static const QString CfgKeyEncoding = "Encoding";
	static const QString CfgKeyComputeLength = "ComputeLength";
	static const QString CfgKeyComputeAngles = "ComputeAngles";
	static const QString CfgKeyComputeTensors = "ComputeTensors";
	static const QString CfgKeyComputeCenter = "ComputeCenter";
	static const QString CfgKeyComputeStartEnd = "ComputeStartEnd";
	static const QString CfgKeyContainsHeader = "ContainsHeader";
	static const QString CfgKeyVisualizationType = "VisualizationType";
	static const QString CfgKeyColumnMappings = "ColumnMappings";
	static const QString CfgKeyOffset = "Offset";
	static const QString CfgKeyIsDiameterFixed = "IsDiameterFixed";
	static const QString CfgKeyFixedDiameterValue = "FixedDiameterValue";
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
	return CfgKeySettingsName + "/" + CfgKeyFormatKeyName + "/" + formatName;
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
	settings.setValue(CfgKeySkipLinesStart, static_cast<qulonglong>(skipLinesStart));
	settings.setValue(CfgKeySkipLinesEnd, static_cast<qulonglong>(skipLinesEnd));
	settings.setValue(CfgKeyColSeparator, columnSeparator);
	settings.setValue(CfgKeyDecimalSeparator, decimalSeparator);
	settings.setValue(CfgKeyObjectType, MapObjectTypeToString(objectType));
	settings.setValue(CfgKeyAddAutoID, addAutoID);
	settings.setValue(CfgKeyUnit, unit);
	settings.setValue(CfgKeySpacing, spacing);
	settings.setValue(CfgKeyEncoding, encoding);
	settings.setValue(CfgKeyComputeLength, computeLength);
	settings.setValue(CfgKeyComputeAngles, computeAngles);
	settings.setValue(CfgKeyComputeTensors, computeTensors);
	settings.setValue(CfgKeyComputeCenter, computeCenter);
	settings.setValue(CfgKeyComputeStartEnd, computeStartEnd);
	settings.setValue(CfgKeyContainsHeader, containsHeader);
	settings.setValue(CfgKeyIsDiameterFixed, isDiameterFixed);
	settings.setValue(CfgKeyFixedDiameterValue, fixedDiameterValue);
	settings.setValue(CfgKeyVisualizationType, MapVisType2Str(visType));
	for (int i=0; i<3; ++i)
		settings.setValue(CfgKeyOffset+QString::number(i), offset[i]);
	// save column mappings:
	QStringList columnMappings;
	for (auto key : columnMapping.keys())
		columnMappings.append(QString("%1:%2").arg(key).arg(columnMapping[key]));
	settings.setValue(CfgKeyColumnMappings, columnMappings);
	settings.setValue(CfgKeySelectedHeaders, selectedHeaders);
	settings.setValue(CfgKeyAllHeaders, currentHeaders);
	settings.endGroup();
}

bool iACsvConfig::load(QSettings const & settings, const QString & formatName)
{
	QString prefix(iACsvConfig::getFormatKey(formatName));
	iACsvConfig defaultConfig;
	skipLinesStart = settings.value(prefix+CfgKeySkipLinesStart, static_cast<qulonglong>(defaultConfig.skipLinesStart)).toULongLong();
	skipLinesEnd = settings.value(prefix+CfgKeySkipLinesEnd, static_cast<qulonglong>(defaultConfig.skipLinesEnd)).toULongLong();
	columnSeparator = settings.value(prefix+CfgKeyColSeparator, defaultConfig.columnSeparator).toString();
	decimalSeparator = settings.value(prefix+CfgKeyDecimalSeparator, defaultConfig.decimalSeparator).toString();
	objectType = MapStringToObjectType(settings.value(prefix+CfgKeyObjectType, MapObjectTypeToString(defaultConfig.objectType)).toString());
	addAutoID = settings.value(prefix+CfgKeyAddAutoID, defaultConfig.addAutoID).toBool();
	computeLength = settings.value(prefix+CfgKeyComputeLength, defaultConfig.computeLength).toBool();
	computeAngles = settings.value(prefix+CfgKeyComputeAngles, defaultConfig.computeAngles).toBool();
	computeTensors = settings.value(prefix+CfgKeyComputeTensors, defaultConfig.computeTensors).toBool();
	computeCenter = settings.value(prefix+CfgKeyComputeCenter, defaultConfig.computeCenter).toBool();
	computeStartEnd = settings.value(prefix+CfgKeyComputeStartEnd, defaultConfig.computeStartEnd).toBool();
	containsHeader = settings.value(prefix+CfgKeyContainsHeader, defaultConfig.containsHeader).toBool();
	isDiameterFixed = settings.value(prefix+CfgKeyIsDiameterFixed, defaultConfig.isDiameterFixed).toBool();
	fixedDiameterValue = settings.value(prefix+CfgKeyFixedDiameterValue, defaultConfig.fixedDiameterValue).toDouble();
	visType = MapStr2VisType(settings.value(prefix+CfgKeyVisualizationType, MapVisType2Str(defaultConfig.visType)).toString());
	for (int i = 0; i<3; ++i)
		offset[i] = settings.value(prefix+CfgKeyOffset + QString::number(i), defaultConfig.offset[i]).toDouble();
	unit = settings.value(prefix+CfgKeyUnit, defaultConfig.unit).toString();
	spacing = settings.value(prefix+CfgKeySpacing, defaultConfig.spacing).toDouble();
	encoding = settings.value(prefix+CfgKeyEncoding, defaultConfig.encoding).toString();
	selectedHeaders = settings.value(prefix+CfgKeySelectedHeaders, defaultConfig.currentHeaders).toStringList();
	currentHeaders = settings.value(prefix+CfgKeyAllHeaders, defaultConfig.currentHeaders).toStringList();
	// load column mappings:
	columnMapping.clear();
	QStringList columnMappings = settings.value(prefix+CfgKeyColumnMappings).toStringList();
	for (QString mapping : columnMappings)
	{
		uint columnKey = mapping.section(":", 0, 0).toInt();
		uint columnNumber = mapping.section(":", 1).toInt();
		columnMapping.insert(columnKey, columnNumber);
	}
	// TODO: check existence of all entries?
	return settings.contains(prefix + CfgKeyVisualizationType);
}
