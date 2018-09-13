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
#include "iACsvIO.h"

#include "iAConsole.h"

#include <vtkMath.h>

#include <QFile>
#include <QIODevice>
#include <QStringList>
#include <QTextCodec>
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
}

namespace
{
	QString DblToString(double value)
	{
		return QString::number(value, 'f', 10);
	}
	QString transformValue(QString value, int idx, iACsvConfig const & config)
	{
		if (config.offset[0] != 0 &&
			idx == config.columnMapping[iACsvConfig::CenterX] ||
			idx == config.columnMapping[iACsvConfig::StartX] ||
			idx == config.columnMapping[iACsvConfig::EndX])
			return DblToString(value.toDouble() + config.offset[0]);
		else if (config.offset[1] != 0 &&
			idx == config.columnMapping[iACsvConfig::CenterY] ||
			idx == config.columnMapping[iACsvConfig::StartY] ||
			idx == config.columnMapping[iACsvConfig::EndY])
			return DblToString(value.toDouble() + config.offset[1]);
		else if (config.offset[2] != 0 &&
			idx == config.columnMapping[iACsvConfig::CenterZ] ||
			idx == config.columnMapping[iACsvConfig::StartZ] ||
			idx == config.columnMapping[iACsvConfig::EndZ])
			return DblToString(value.toDouble() + config.offset[2]);
		else if (idx == config.columnMapping[iACsvConfig::Theta] && value < 0)
			return DblToString(2 * vtkMath::Pi() + value.toDouble());
		else
			return value;
	}
	double getValueAsDouble(QStringList const & values, int index, iACsvConfig const & config)
	{
		if (index >= values.size())
			return 0;
		QString value = values[index];
		if (config.decimalSeparator != ".")
			value = value.replace(config.decimalSeparator, ".");
		return transformValue(value, index, config).toDouble();
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
		DEBUG_LOG("Error loading csv file, file does not exist.");
		return false;
	}
	QFile file(m_csvConfig.fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		DEBUG_LOG(QString("Unable to open file: %1").arg(file.errorString()));
		return false;
	}
	QTextStream in(&file);
	in.setCodec(m_csvConfig.encoding.toStdString().c_str());
	size_t effectiveRowCount = std::min(rowCount, 
		calcRowCount(in, m_csvConfig.skipLinesStart + (cnfg_params.containsHeader ? 1 : 0), m_csvConfig.skipLinesEnd));
	if (effectiveRowCount <= 0)
	{
		DEBUG_LOG("No rows to load in the csv file!");
		return false;
	}

	for (int i = 0; i < m_csvConfig.skipLinesStart; i++)
		in.readLine();

	if (m_csvConfig.containsHeader)
		m_fileHeaders = in.readLine().split(m_csvConfig.columnSeparator);
	else
		m_fileHeaders = m_csvConfig.currentHeaders;
	auto selectedColIdx = computeSelectedColIdx();
	determineOutputHeaders(selectedColIdx);
	int colCount = m_outputHeaders.size();

	dstTbl.initialize(m_outputHeaders, effectiveRowCount);

	size_t resultRowID = 1;
	for (size_t row = 0; row < effectiveRowCount; ++row)
	{
		QString line = in.readLine();
		if (line.isEmpty())
			continue;
		QStringList entries;
		if (m_csvConfig.addAutoID)
			entries.append(QString::number(resultRowID));

		auto values = line.split(m_csvConfig.columnSeparator);
		if (values.size() < m_csvConfig.currentHeaders.size())
		{
			DEBUG_LOG(QString("Line %1 in file '%2' only contains %3 entries, expected %4. Skipping...")
				.arg(row + m_csvConfig.skipLinesStart + (m_csvConfig.containsHeader?0:1) ).arg(m_csvConfig.fileName)
				.arg(values.size()).arg(m_csvConfig.currentHeaders.size()));
			continue;
		}
		for (int valIdx : selectedColIdx)
		{
			if (valIdx >= values.size())
			{
				DEBUG_LOG(QString("Error in line %1: Only %2 values, at least %3 expected").arg(resultRowID).arg(values.size()).arg(valIdx+1));
				break;
			}
			QString value = values[valIdx];
			if (m_csvConfig.decimalSeparator != ".")
				value = value.replace(m_csvConfig.decimalSeparator, ".");
			entries.append(transformValue(value, valIdx, m_csvConfig) );
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
				entries.append(DblToString(center[i] + dir[i])); // start
			for (int i = 0; i < 3; ++i)
				entries.append(DblToString(center[i] - dir[i])); // end
		}
		if (m_csvConfig.isDiameterFixed)
		{
			entries.append(DblToString(m_csvConfig.fixedDiameterValue));
		}
		double phi, theta;
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
				double length = std::sqrt(dx*dx + dy*dy + dz*dz);
				entries.append(DblToString(length));
			}
			if (m_csvConfig.computeCenter)
			{
				double xm = (x1 + x2) / 2.0f;
				double ym = (y1 + y2) / 2.0f;
				double zm = (z1 + z2) / 2.0f;
				entries.append(DblToString(xm));
				entries.append(DblToString(ym));
				entries.append(DblToString(zm));
			}
			if (m_csvConfig.computeAngles)
			{
				phi = asin(dy / sqrt(dx*dx + dy*dy));
				theta = acos(dz / sqrt(dx*dx + dy*dy + dz*dz));
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
				if (dx == 0 && dy == 0)
				{
					phi = 0.0;
					theta = 0.0;
				}
				entries.append(DblToString(phi));
				entries.append(DblToString(theta));
			}
		}
		if (m_csvConfig.computeTensors)
		{
			if (!m_csvConfig.computeAngles)
			{
				double phi = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::Phi], m_csvConfig);
				double theta = getValueAsDouble(values, m_csvConfig.columnMapping[iACsvConfig::Theta], m_csvConfig);
			}
			double a11 = cos(phi)*cos(phi)*sin(theta)*sin(theta);
			double a22 = sin(phi)*sin(phi)*sin(theta)*sin(theta);
			double a33 = cos(theta)*cos(theta);
			double a12 = cos(phi)*sin(theta)*sin(theta)*sin(phi);
			double a13 = cos(phi)*sin(theta)*cos(theta);
			double a23 = sin(phi)*sin(theta)*cos(theta);
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
			entries.append(DblToString(a11));
			entries.append(DblToString(a22));
			entries.append(DblToString(a33));
			entries.append(DblToString(a12));
			entries.append(DblToString(a13));
			entries.append(DblToString(a23));
		}
		entries.append("0"); // class ID
		dstTbl.addRow(resultRowID-1, entries);
		++resultRowID;
	}

	if (file.isOpen())
		file.close();
	return true;
}

void iACsvIO::determineOutputHeaders(QVector<int> const & selectedCols)
{
	m_outputHeaders.clear();
	m_outputMapping->clear();

	//m_outputMapping.insert(iACsvConfig::ID, 0); // for now, ID is fixed to be in column 0

	for (int key : m_csvConfig.columnMapping.keys())
	{
		int outIdx = selectedCols.indexOf(m_csvConfig.columnMapping[key]);
		if (outIdx < 0)
		{
			DEBUG_LOG(QString("Mapped column (ID=%1, input col=%2) not selected for output.").arg(key).arg(m_csvConfig.columnMapping[key]));
		}
		else
		{
			int fullOutIdx = (m_csvConfig.addAutoID ? 1 : 0) + outIdx;
			m_outputMapping->insert(key, fullOutIdx);
		}
	}

	if (m_csvConfig.addAutoID)
		m_outputHeaders.append(iACsvIO::ColNameAutoID);

	for (int i=0; i<selectedCols.size(); ++i)
		m_outputHeaders.append(m_fileHeaders[selectedCols[i]]);

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
	m_outputHeaders.append(iACsvIO::ColNameClassID);
}

QVector<int> iACsvIO::computeSelectedColIdx()
{
	QStringList selectedHeaders = (m_csvConfig.selectedHeaders.isEmpty()) ? m_fileHeaders : m_csvConfig.selectedHeaders;
	QVector<int> result;

	for (QString colName: selectedHeaders)
	{
		int idx = m_fileHeaders.indexOf(colName);
		if (idx >= 0)
			result.append(idx);
		else
			DEBUG_LOG(QString("Selected column '%1' not found in file headers '%2', skipping.").arg(colName).arg(m_fileHeaders.join(",")));
	}
	return result;
}

size_t iACsvIO::calcRowCount(QTextStream& in, const size_t skipLinesStart, const size_t skipLinesEnd)
{
	// skip (unused) header lines (+1 for line containing actual column headers)
	for (int i = 0; i < skipLinesStart && !in.atEnd(); i++)
		in.readLine();

	// count remaining lines
	size_t rowCount = 0;
	while (!in.atEnd())
	{
		QString line = in.readLine();
		if (line.trimmed().isEmpty()) // skip empty lines
			continue;
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
