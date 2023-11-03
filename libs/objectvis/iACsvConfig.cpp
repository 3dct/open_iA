// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACsvConfig.h"

#include <QFile>
#include <QSettings>

const QString iACsvConfig::FCPFiberFormat("FCP Fiber csv");
const QString iACsvConfig::FCVoidFormat("Feature Characteristics (Pore) csv");

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
//	static const QString CfgKeyAddClassID = "AddClassID";
}

iACsvConfig::iACsvConfig() :
	fileName(""),
	encoding("System"),
	containsHeader(true),
	skipLinesStart(FCPFormatStartSkipLines),
	skipLinesEnd(0),
	columnSeparator(";"),
	decimalSeparator("."),
	addAutoID(false),
	objectType(iAObjectType::Voids),
	unit("microns"),
	spacing(0.0f),
	computeLength(false),
	computeAngles(false),
	computeTensors(false),
	computeCenter(false),
	computeStartEnd(false),
	visType(iAObjectVisType::UseVolume),
	isDiameterFixed(false),
	fixedDiameterValue(0.0),
	addClassID(true)
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
	if ((visType == iAObjectVisType::Line || visType == iAObjectVisType::Cylinder) && (
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
	if (visType == iAObjectVisType::Cylinder &&
		(!columnMapping.contains(iACsvConfig::Diameter) && !isDiameterFixed) )
	{
		errorMsg = "Visualization as Cylinders requires start- and end-position as well as a diameter, please specify where to find these!";
		return false;
	}
	if (visType == iAObjectVisType::Ellipsoid && (
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

iACsvConfig const & iACsvConfig::getFCPFiberFormat(QString const & fileName)
{
	static iACsvConfig FCPFormat;
	FCPFormat.fileName = fileName;
	FCPFormat.encoding = "System";
	FCPFormat.containsHeader = false;
	FCPFormat.skipLinesStart = 5;
	FCPFormat.skipLinesEnd = 0;
	FCPFormat.columnSeparator = ",";
	FCPFormat.decimalSeparator = ".";
	FCPFormat.addAutoID = false;
	FCPFormat.objectType = iAObjectType::Fibers;
	FCPFormat.computeLength = false;
	FCPFormat.computeAngles = true;
	FCPFormat.computeTensors = true;
	FCPFormat.computeCenter = true;
	FCPFormat.computeStartEnd = false;
	std::fill(FCPFormat.offset, FCPFormat.offset + 3, 0.0);
	FCPFormat.visType = iAObjectVisType::UseVolume;
	FCPFormat.currentHeaders = QStringList() << "Label"
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
	FCPFormat.selectedHeaders = FCPFormat.currentHeaders;
	FCPFormat.columnMapping.clear();
	FCPFormat.columnMapping.insert(StartX,   1);
	FCPFormat.columnMapping.insert(StartY,   2);
	FCPFormat.columnMapping.insert(StartZ,   3);
	FCPFormat.columnMapping.insert(EndX,     4);
	FCPFormat.columnMapping.insert(EndY,     5);
	FCPFormat.columnMapping.insert(EndZ,     6);
	FCPFormat.columnMapping.insert(Length, 7);
	FCPFormat.columnMapping.insert(CurvedLength, 8);
	FCPFormat.columnMapping.insert(Diameter, 9);
	return FCPFormat;
}

iACsvConfig const& iACsvConfig::getFCVoidFormat(QString const& fileName)
{
	static iACsvConfig FCFormat;
	FCFormat.fileName = fileName;
	FCFormat.encoding = "System";
	FCFormat.containsHeader = false;
	FCFormat.skipLinesStart = 5;
	FCFormat.skipLinesEnd = 0;
	FCFormat.columnSeparator = ",";
	FCFormat.decimalSeparator = ".";
	FCFormat.addAutoID = false;
	FCFormat.objectType = iAObjectType::Voids;
	FCFormat.computeLength = false;
	FCFormat.computeAngles = false;
	FCFormat.computeTensors = false;
	FCFormat.computeCenter = false;
	FCFormat.computeStartEnd = false;
	std::fill(FCFormat.offset, FCFormat.offset + 3, 0.0);
	FCFormat.visType = iAObjectVisType::UseVolume;
	FCFormat.currentHeaders = QStringList()
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
	FCFormat.selectedHeaders = FCFormat.currentHeaders;
	FCFormat.columnMapping.clear();
	FCFormat.columnMapping.insert(StartX,    1);
	FCFormat.columnMapping.insert(StartY,    2);
	FCFormat.columnMapping.insert(StartZ,    3);
	FCFormat.columnMapping.insert(EndX,      4);
	FCFormat.columnMapping.insert(EndY,      5);
	FCFormat.columnMapping.insert(EndZ,      6);
	FCFormat.columnMapping.insert(DimensionX,13);
	FCFormat.columnMapping.insert(DimensionY,14);
	FCFormat.columnMapping.insert(DimensionZ,15);
	FCFormat.columnMapping.insert(Phi,      16);
	FCFormat.columnMapping.insert(Theta,    17);
	FCFormat.columnMapping.insert(CenterX,  18);
	FCFormat.columnMapping.insert(CenterY,  19);
	FCFormat.columnMapping.insert(CenterZ,  20);
	FCFormat.columnMapping.insert(Diameter, 23);
	FCFormat.columnMapping.insert(Length,   28);
	return FCFormat;
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
	for (int i = 0; i < 3; ++i)
	{
		settings.setValue(CfgKeyOffset + QString::number(i), offset[i]);
	}
	// save column mappings:
	QStringList columnMappings;
	for (auto key : columnMapping.keys())
	{
		columnMappings.append(QString("%1:%2").arg(key).arg(columnMapping[key]));
	}
	settings.setValue(CfgKeyColumnMappings, columnMappings);
	settings.setValue(CfgKeySelectedHeaders, selectedHeaders);
	settings.setValue(CfgKeyAllHeaders, currentHeaders);
	settings.endGroup();
}

bool iACsvConfig::load(QSettings const & settings, const QString & formatName)
{
	QString prefix(iACsvConfig::getFormatKey(formatName)+"/");
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
	for (int i = 0; i < 3; ++i)
	{
		offset[i] = settings.value(prefix + CfgKeyOffset + QString::number(i), defaultConfig.offset[i]).toDouble();
	}
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
