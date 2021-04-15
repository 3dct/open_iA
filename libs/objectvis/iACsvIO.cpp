/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iACsvIO.h"

#include <iALog.h>

#include <vtkMath.h>

#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QStringList>
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
#include <QTextCodec>
#else
#include <QStringConverter>
#endif
#include <QTextStream>

const char* iACsvIO::ColNameAutoID = "Auto_ID";
const char* iACsvIO::ColNameClassID = "Class_ID";
namespace
{
	const char* ColNamePhi = "Phi[°]";
	const char* ColNameTheta = "Theta[°]";
	const char* ColNameA11 = "a11";
	const char* ColNameA22 = "a22";
	const char* ColNameA33 = "a33";
	const char* ColNameA12 = "a12";
	const char* ColNameA13 = "a13";
	const char* ColNameA23 = "a23";
	const char* ColNameCenterX = "Xm[µm]";
	const char* ColNameCenterY = "Ym[µm]";
	const char* ColNameCenterZ = "Zm[µm]";
	const char* ColNameLength = "StraightLength[µm]";
	const char* ColNameStartX = "X1";
	const char* ColNameStartY = "Y1";
	const char* ColNameStartZ = "Z1";
	const char* ColNameEndX = "X2";
	const char* ColNameEndY = "Y2";
	const char* ColNameEndZ = "Z2";
	const char* ColNameDiameter = "Diameter";

	QString DblToString(double value)
	{
		return QString::number(value, 'f', 10);
	}
	double transformValue(QString value, uint idx, iACsvConfig const& config)
	{
		// TODO: error check?
		if (config.offset[0] != 0 && (
			(config.columnMapping.contains(iACsvConfig::CenterX) && idx == config.columnMapping[iACsvConfig::CenterX]) ||
			(config.columnMapping.contains(iACsvConfig::StartX) && idx == config.columnMapping[iACsvConfig::StartX]) ||
			(config.columnMapping.contains(iACsvConfig::EndX) && idx == config.columnMapping[iACsvConfig::EndX])))
		{
			return value.toDouble() + config.offset[0];
		}
		else if (config.offset[1] != 0 && (
			(config.columnMapping.contains(iACsvConfig::CenterY) && idx == config.columnMapping[iACsvConfig::CenterY]) ||
			(config.columnMapping.contains(iACsvConfig::StartY) && idx == config.columnMapping[iACsvConfig::StartY]) ||
			(config.columnMapping.contains(iACsvConfig::EndY) && idx == config.columnMapping[iACsvConfig::EndY])))
		{
			return value.toDouble() + config.offset[1];
		}
		else if (config.offset[2] != 0 && (
			(config.columnMapping.contains(iACsvConfig::CenterZ) && idx == config.columnMapping[iACsvConfig::CenterZ]) ||
			(config.columnMapping.contains(iACsvConfig::StartZ) && idx == config.columnMapping[iACsvConfig::StartZ]) ||
			(config.columnMapping.contains(iACsvConfig::EndZ) && idx == config.columnMapping[iACsvConfig::EndZ])))
		{
			return value.toDouble() + config.offset[2];
		}
		else if (config.columnMapping.contains(iACsvConfig::Theta) && idx == config.columnMapping[iACsvConfig::Theta] && value.toDouble() < 0)
		{
			return 2 * vtkMath::Pi() + value.toDouble();
		}
		else
		{
			return value.toDouble();
		}
	}
	double getValueAsDouble(QStringList const & values, uint index, iACsvConfig const & config)
	{
		if (index > static_cast<uint>(std::numeric_limits<int>::max()) ||
			static_cast<int>(index) > values.size())
		{
			return 0;
		}
		QString value = values[index];
		if (config.decimalSeparator != ".")
		{
			value = value.replace(config.decimalSeparator, ".");
		}
		return transformValue(value, index, config);
	}
	size_t getLineNumberForRow(iACsvConfig const& cfg, size_t row)
	{
		return cfg.skipLinesStart + (cfg.containsHeader ? 1 : 0) + row;
	}
}

iACsvIO::iACsvIO():
	m_outputMapping(new QMap<uint, uint>)
{}

bool iACsvIO::loadCSV(iACsvTableCreator & dstTbl, iACsvConfig const & cnfg_params, size_t const rowCount)
{
	m_csvConfig = cnfg_params;
	if (!QFile::exists(m_csvConfig.fileName))
	{
		LOG(lvlError, QString("Unable to open csv file '%1': File does not exist.").arg(m_csvConfig.fileName));
		return false;
	}
	QFile file(m_csvConfig.fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Unable to open file '%1': %2").arg(m_csvConfig.fileName).arg(file.errorString()));
		return false;
	}
	QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	in.setCodec(m_csvConfig.encoding.toStdString().c_str());
#else
	auto encOpt = QStringConverter::encodingForName(m_csvConfig.encoding.toStdString().c_str());
	QStringConverter::Encoding enc = encOpt.has_value() ? encOpt.value() : QStringConverter::Utf8;
	in.setEncoding(enc);
#endif
	size_t effectiveRowCount = std::min(rowCount,
		calcRowCount(in, getLineNumberForRow(m_csvConfig, 0), m_csvConfig.skipLinesEnd));
	if (effectiveRowCount <= 0)
	{
		LOG(lvlError, QString("Unable to open csv file '%1': No rows to load in the csv file!")
			.arg(m_csvConfig.fileName));
		return false;
	}

	for (size_t i = 0; i < m_csvConfig.skipLinesStart; i++)
	{
		in.readLine();
	}

	if (m_csvConfig.containsHeader)
	{
		m_fileHeaders = in.readLine().split(m_csvConfig.columnSeparator);
	}
	else
	{
		m_fileHeaders = m_csvConfig.currentHeaders;
	}
	auto selectedColIdx = computeSelectedColIdx();
	determineOutputHeaders(selectedColIdx);

	dstTbl.initialize(m_outputHeaders, effectiveRowCount);

	size_t resultRowID = 1;
	for (size_t row = 0; row < effectiveRowCount; ++row)
	{
		QString line = in.readLine();
		if (line.isEmpty())
		{
			continue;
		}
		std::vector<double> entries;
		entries.reserve(m_outputHeaders.size());
		if (m_csvConfig.addAutoID)
		{
			entries.push_back(resultRowID);
		}

		auto values = line.split(m_csvConfig.columnSeparator);
		if (values.size() < m_csvConfig.currentHeaders.size())
		{
			LOG(lvlWarn, QString("Line %1 in file '%2' (row %3 of data) only contains %4 entries, expected %5. Skipping...")
				.arg(getLineNumberForRow(m_csvConfig, row)).arg(m_csvConfig.fileName).arg(row)
				.arg(values.size()).arg(m_csvConfig.currentHeaders.size()));
			continue;
		}
		if (!m_csvConfig.addAutoID && values[0].toULongLong() != (row + 1))
		{
			LOG(lvlError, QString("ID column: Unexpected value %1, expected %2 in line %3 of file '%4' "
				"(i.e. the values are not ordered as required, the ID values need to be consecutive, starting at 1)! "
				"Please either fix the data in the CSV or use the 'Create ID' feature!")
				.arg(values[0].toULongLong()).arg(row+1)
				.arg(getLineNumberForRow(m_csvConfig, row)).arg(m_csvConfig.fileName));
			return false;
		}
		for (int valIdx : selectedColIdx)
		{
			if (valIdx >= values.size())
			{
				LOG(lvlWarn, QString("Error in line %1: Only %2 values, at least %3 expected").arg(resultRowID).arg(values.size()).arg(valIdx + 1));
				break;
			}
			QString value = values[valIdx];
			if (m_csvConfig.decimalSeparator != ".")
			{
				value = value.replace(m_csvConfig.decimalSeparator, ".");
			}
			entries.push_back(transformValue(value, valIdx, m_csvConfig));
		}
		if (m_csvConfig.computeStartEnd)
		{
			double center[3];
			center[0] = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::CenterX], m_csvConfig);
			center[1] = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::CenterY], m_csvConfig);
			center[2] = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::CenterZ], m_csvConfig);
			double phi = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::Phi], m_csvConfig);
			double theta = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::Theta], m_csvConfig);
			double radius = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::Length], m_csvConfig) * 0.5;
			double dir[3];
			dir[0] = radius * std::sin(phi) * std::cos(theta);
			dir[1] = radius * std::sin(phi) * std::sin(theta);
			dir[2] = radius * std::cos(phi);
			for (int i = 0; i < 3; ++i)
			{
				entries.push_back(center[i] + dir[i]); // start
			}
			for (int i = 0; i < 3; ++i)
			{
				entries.push_back(center[i] - dir[i]); // end
			}
		}
		if (m_csvConfig.isDiameterFixed)
		{
			entries.push_back(m_csvConfig.fixedDiameterValue);
		}
		double phi = 0.0, theta = 0.0;
		if (m_csvConfig.computeLength || m_csvConfig.computeAngles || m_csvConfig.computeCenter)
		{
			double x1 = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::StartX], m_csvConfig);
			double y1 = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::StartY], m_csvConfig);
			double z1 = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::StartZ], m_csvConfig);
			double x2 = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::EndX], m_csvConfig);
			double y2 = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::EndY], m_csvConfig);
			double z2 = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::EndZ], m_csvConfig);
			double dx = x1 - x2;
			double dy = y1 - y2;
			double dz = z1 - z2;
			if (dz < 0)
			{
				dx = x2 - x1;
				dy = y2 - y1;
				dz = z2 - z1;
			}
			if (m_csvConfig.computeLength)
			{
				double length = std::sqrt(dx * dx + dy * dy + dz * dz);
				entries.push_back(length);
			}
			if (m_csvConfig.computeCenter)
			{
				double xm = (x1 + x2) / 2.0f;
				double ym = (y1 + y2) / 2.0f;
				double zm = (z1 + z2) / 2.0f;
				entries.push_back(xm);
				entries.push_back(ym);
				entries.push_back(zm);
			}
			if (m_csvConfig.computeAngles)
			{
				if (dx == 0 && dy == 0)
				{
					phi = 0.0;
					theta = 0.0;
				}
				else
				{
					phi = asin(dy / sqrt(dx * dx + dy * dy));
					theta = acos(dz / sqrt(dx * dx + dy * dy + dz * dz));
					phi = vtkMath::DegreesFromRadians(phi);
					theta = vtkMath::DegreesFromRadians(theta);
					// locate the phi value to quadrant
					if (dx < 0)
					{
						phi = 180.0 - phi;
					}
					if (phi < 0.0)
					{
						phi = phi + 360.0;
					}
				}
				entries.push_back(phi);
				entries.push_back(theta);
			}
		}
		if (m_csvConfig.computeTensors)
		{
			if (!m_csvConfig.computeAngles)
			{
				phi = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::Phi], m_csvConfig);
				theta = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::Theta], m_csvConfig);
			}
			double rad_phi = vtkMath::RadiansFromDegrees(phi);
			double rad_theta = vtkMath::RadiansFromDegrees(theta);
			double a11 = cos(rad_phi) * cos(rad_phi) * sin(rad_theta) * sin(rad_theta);
			double a22 = sin(rad_phi) * sin(rad_phi) * sin(rad_theta) * sin(rad_theta);
			double a33 = cos(rad_theta) * cos(rad_theta);
			double a12 = cos(rad_phi) * sin(rad_theta) * sin(rad_theta) * sin(rad_phi);
			double a13 = cos(rad_phi) * sin(rad_theta) * cos(rad_theta);
			double a23 = sin(rad_phi) * sin(rad_theta) * cos(rad_theta);
			/*
			if (dx == 0 && dy == 0)
			{
				a11 = 0.0;
				a22 = 0.0;
				a12 = 0.0;
				a13 = 0.0;
				a23 = 0.0;
			}
			*/
			entries.push_back(a11);
			entries.push_back(a22);
			entries.push_back(a33);
			entries.push_back(a12);
			entries.push_back(a13);
			entries.push_back(a23);
		}
		if (m_csvConfig.addClassID)
		{
			entries.push_back(0); // class ID
		}
		dstTbl.addRow(resultRowID-1, entries);
		++resultRowID;
	}
	if (file.isOpen())
	{
		file.close();
	}
	return true;
}

void iACsvIO::determineOutputHeaders(QVector<uint> const & selectedCols)
{
	m_outputHeaders.clear();
	m_outputMapping->clear();

	//m_outputMapping.insert(iACsvConfig::ID, 0); // for now, ID is fixed to be in column 0

	for (uint key : m_csvConfig.columnMapping.keys())
	{
		int outIdx = selectedCols.indexOf(m_csvConfig.columnMapping[key]);
		if (outIdx < 0)
		{
			LOG(lvlWarn, QString("Mapped column (ID=%1, input col=%2) not selected for output.").arg(key).arg(m_csvConfig.columnMapping[key]));
		}
		else
		{
			int fullOutIdx = (m_csvConfig.addAutoID ? 1 : 0) + outIdx;
			m_outputMapping->insert(key, fullOutIdx);
		}
	}

	if (m_csvConfig.addAutoID)
	{
		m_outputHeaders.append(iACsvIO::ColNameAutoID);
	}

	for (int i = 0; i < selectedCols.size(); ++i)
	{
		m_outputHeaders.append(m_fileHeaders[selectedCols[i]]);
	}

	if (m_csvConfig.computeStartEnd)
	{
		m_outputMapping->insert(iACsvConfig::StartX, m_outputHeaders.size());
		m_outputHeaders.append(ColNameStartX);
		m_outputMapping->insert(iACsvConfig::StartY, m_outputHeaders.size());
		m_outputHeaders.append(ColNameStartY);
		m_outputMapping->insert(iACsvConfig::StartZ, m_outputHeaders.size());
		m_outputHeaders.append(ColNameStartZ);
		m_outputMapping->insert(iACsvConfig::EndX, m_outputHeaders.size());
		m_outputHeaders.append(ColNameEndX);
		m_outputMapping->insert(iACsvConfig::EndY, m_outputHeaders.size());
		m_outputHeaders.append(ColNameEndY);
		m_outputMapping->insert(iACsvConfig::EndZ, m_outputHeaders.size());
		m_outputHeaders.append(ColNameEndZ);
	}
	if (m_csvConfig.isDiameterFixed)
	{
		m_outputMapping->insert(iACsvConfig::Diameter, m_outputHeaders.size());
		m_outputHeaders.append(ColNameDiameter);
	}

	if (m_csvConfig.computeLength)
	{
		m_outputMapping->insert(iACsvConfig::Length, m_outputHeaders.size());
		m_outputHeaders.append(ColNameLength);
	}
	if (m_csvConfig.computeCenter)
	{
		m_outputMapping->insert(iACsvConfig::CenterX, m_outputHeaders.size());
		m_outputHeaders.append(ColNameCenterX);
		m_outputMapping->insert(iACsvConfig::CenterY, m_outputHeaders.size());
		m_outputHeaders.append(ColNameCenterY);
		m_outputMapping->insert(iACsvConfig::CenterZ, m_outputHeaders.size());
		m_outputHeaders.append(ColNameCenterZ);
	}
	if (m_csvConfig.computeAngles)
	{
		m_outputMapping->insert(iACsvConfig::Phi, m_outputHeaders.size());
		m_outputHeaders.append(ColNamePhi);
		m_outputMapping->insert(iACsvConfig::Theta, m_outputHeaders.size());
		m_outputHeaders.append(ColNameTheta);
	}
	if (m_csvConfig.computeTensors)
	{
		m_outputHeaders.append(ColNameA11);
		m_outputHeaders.append(ColNameA22);
		m_outputHeaders.append(ColNameA33);
		m_outputHeaders.append(ColNameA12);
		m_outputHeaders.append(ColNameA13);
		m_outputHeaders.append(ColNameA23);
	}
	if (m_csvConfig.addClassID)
	{
		m_outputHeaders.append(iACsvIO::ColNameClassID);
	}
}

QVector<uint> iACsvIO::computeSelectedColIdx()
{
	QStringList selectedHeaders = (m_csvConfig.selectedHeaders.isEmpty()) ? m_fileHeaders : m_csvConfig.selectedHeaders;
	QVector<uint> result;

	for (QString colName: selectedHeaders)
	{
		int idx = m_fileHeaders.indexOf(colName);
		if (idx >= 0)
		{
			result.append(idx);
		}
		else
		{
			LOG(lvlWarn, QString("Selected column '%1' not found in file headers '%2', skipping.").arg(colName).arg(m_fileHeaders.join(",")));
		}
	}
	return result;
}

size_t iACsvIO::calcRowCount(QTextStream& in, const size_t skipLinesStart, const size_t skipLinesEnd)
{
	// skip (unused) header lines (+1 for line containing actual column headers)
	for (size_t i = 0; i < skipLinesStart && !in.atEnd(); i++)
	{
		in.readLine();
	}

	// count remaining lines
	size_t rowCount = 0;
	while (!in.atEnd())
	{
		QString line = in.readLine();
		if (line.trimmed().isEmpty()) // skip empty lines
		{
			continue;
		}
		++rowCount;
	}
	in.seek(0);
	return rowCount - skipLinesEnd;
}

const QStringList & iACsvIO::getFileHeaders() const
{
	return m_fileHeaders;
}

const QStringList & iACsvIO::getOutputHeaders() const
{
	return m_outputHeaders;
}

QSharedPointer<QMap<uint, uint> > iACsvIO::getOutputMapping() const
{
	return m_outputMapping;
}

bool readCurvedFiberInfo(QString const & fileName, std::map<size_t, std::vector<iAVec3f> > & outMap)
{
	QFileInfo curvedInfo(fileName);
	if (!curvedInfo.exists() || !curvedInfo.isFile())
	{
		LOG(lvlWarn, QString("No curved fibre file named %1 exists.").arg(fileName));
		return false;
	}
	QFile curvedFiberPoints(curvedInfo.absoluteFilePath());
	if (!curvedFiberPoints.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		LOG(lvlError, QString("Unable to open curvedFiberPoints file: %1. Error: %2")
			.arg(curvedInfo.absoluteFilePath()).arg(curvedFiberPoints.errorString()));
		return false;
	}
	QTextStream in(&curvedFiberPoints);
	size_t lineNr = 0;
	while (!in.atEnd())
	{
		++lineNr;
		QString line = in.readLine();
		if (lineNr <= 5 || line.isEmpty())
		{
			continue;
		}

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
		QStringList valueStrList = line.split(",", QString::SkipEmptyParts);
#else
		QStringList valueStrList = line.split(",", Qt::SkipEmptyParts);
#endif
		if (valueStrList.size() < 7 || ((valueStrList.size() - 1) % 3) != 0)
		{
			LOG(lvlWarn, QString("Invalid line in curvedFiberPoints file %1, line %2: %3 - number of elements: %4")
				.arg(curvedInfo.absoluteFilePath()).arg(lineNr).arg(line).arg(valueStrList.size()));
			continue;
		}
		// TODO: better solution for Label (1..N) <-> internal fiber ID (0..N-1) mapping!!!
		size_t fiberID = valueStrList[0].toInt() - 1;
		int numOfPoints = (valueStrList.size() - 1) / 3;
		std::vector<iAVec3f> points(numOfPoints);
		for (int i = 0; i < numOfPoints; ++i)
		{
			int baseIdx = 1 + (i * 3);
			bool ok1, ok2, ok3;
			iAVec3f p(valueStrList[baseIdx].toFloat(&ok1), valueStrList[baseIdx + 1].toFloat(&ok2), valueStrList[baseIdx + 2].toFloat(&ok3));
			if (!ok1 || !ok2 || !ok3)
			{
				LOG(lvlWarn, QString("Invalid point (%1, %2, %3) in curvedFiberPoints file %4, line %5: %6 - number of elements: %7")
					.arg(valueStrList[baseIdx]).arg(valueStrList[baseIdx + 1]).arg(valueStrList[baseIdx + 2])
					.arg(curvedInfo.absoluteFilePath()).arg(lineNr).arg(line).arg(valueStrList.size()));
				continue;
			}
			points[i] = p;

		}
		outMap.insert(std::make_pair(fiberID, points));
	}
	return true;
}
