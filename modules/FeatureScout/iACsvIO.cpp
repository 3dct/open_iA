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
const char* iACsvIO::ColNamePhi = "Phi[°]";
const char* iACsvIO::ColNameTheta = "Theta[°]";
const char* iACsvIO::ColNameA11 = "a11";
const char* iACsvIO::ColNameA22 = "a22";
const char* iACsvIO::ColNameA33 = "a33";
const char* iACsvIO::ColNameA12 = "a12";
const char* iACsvIO::ColNameA13 = "a13";
const char* iACsvIO::ColNameA23 = "a23";
const char* iACsvIO::ColNameCenterX = "Xm[µm]";
const char* iACsvIO::ColNameCenterY = "Xm[µm]";
const char* iACsvIO::ColNameCenterZ = "Xm[µm]";
const char* iACsvIO::ColNameLength = "Length[µm]";

namespace
{
	double getValueAsDouble(QStringList const & values, int index, QString const & decimalSeparator)
	{
		QString value = values[index];
		if (decimalSeparator != ".")
			value = value.replace(decimalSeparator, ".");
		return value.toDouble();
	}
	QString DblToString(double value)
	{
		return QString::number(value, 'f', 10);
	}
}

iACsvIO::iACsvIO():
	m_rowCount(std::numeric_limits<size_t>::max())
{}

QStringList iACsvIO::fibreCalculation(QString const & line, size_t const colCount, size_t const rowCount)
{
	double x1, x2, y1, y2, z1, z2, dx, dy, dz, xm, ym, zm, phi, theta;
	double a11, a22, a33, a12, a13, a23;

	//getting start and EndPoints
	x1 = line.section(",", 1, 1).toFloat();
	y1 = line.section(",", 2, 2).toFloat();
	z1 = line.section(",", 3, 3).toFloat();
	x2 = line.section(",", 4, 4).toFloat();
	y2 = line.section(",", 5, 5).toFloat();
	z2 = line.section(",", 6, 6).toFloat();

	// preparing the tensor calculation
	dx = x1 - x2;
	dy = y1 - y2;
	dz = z1 - z2;
	xm = (x1 + x2) / 2.0f;
	ym = (y1 + y2) / 2.0f;
	zm = (z1 + z2) / 2.0f;

	if (dz < 0)
	{
		dx = x2 - x1;
		dy = y2 - y1;
		dz = z2 - z1;
	}

	phi = asin(dy / sqrt(dx*dx + dy * dy));
	theta = acos(dz / sqrt(dx*dx + dy * dy + dz * dz));

	a11 = cos(phi)*cos(phi)*sin(theta)*sin(theta);
	a22 = sin(phi)*sin(phi)*sin(theta)*sin(theta);
	a33 = cos(theta)*cos(theta);
	a12 = cos(phi)*sin(theta)*sin(theta)*sin(phi);
	a13 = cos(phi)*sin(theta)*cos(theta);
	a23 = sin(phi)*sin(theta)*cos(theta);

	phi = vtkMath::DegreesFromRadians(phi);
	theta = vtkMath::DegreesFromRadians(theta); // finish calculation
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
		a11 = 0.0;
		a22 = 0.0;
		a12 = 0.0;
		a13 = 0.0;
		a23 = 0.0;
	}
	size_t id = line.section(",", 0, 0).toULongLong();
	QStringList result;
	result.append(QString::number(id));

	for (int j = 1; j < 7; j++)
	{
		result.append(line.section(",", j, j));
	}
	result.append(DblToString(a11));
	result.append(DblToString(a22));
	result.append(DblToString(a33));
	result.append(DblToString(a12));
	result.append(DblToString(a13));
	result.append(DblToString(a23));
	result.append(DblToString(phi));
	result.append(DblToString(theta));
	result.append(DblToString(xm));
	result.append(DblToString(ym));
	result.append(DblToString(zm));
	for (int j = 7; j < colCount; j++)
	{
		result.append(line.section(",", j, j));
	}
	return result;
}

bool iACsvIO::loadLegacyCSV(iACsvTableCreator & dstTbl, iAFeatureScoutObjectType fid, QString const & fileName)
{
	m_csvConfig.fileName = fileName;
	m_csvConfig.columnSeparator = ",";
	m_csvConfig.decimalSeparator = ".";
	m_csvConfig.skipLinesStart = iACsvConfig::LegacyFormatStartSkipLines;
	m_csvConfig.skipLinesEnd = 0;
	switch (fid)
	{
		default:
		case iAFeatureScoutObjectType::Fibers: return loadCSV(dstTbl, LegacyFibers);
		case iAFeatureScoutObjectType::Voids:  return loadCSV(dstTbl, LegacyPores);
	}
}

bool iACsvIO::loadCSV(iACsvTableCreator & dstTbl, iACsvConfig const & cnfg_params, size_t const rowCount)
{
	m_csvConfig = cnfg_params;
	m_rowCount = rowCount;
	return loadCSV(dstTbl, NewCSV);
}

bool iACsvIO::loadCSV(iACsvTableCreator & dstTbl, ReadMode mode)
{
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
	size_t rowCount = std::min(m_rowCount, calcRowCount(in, m_csvConfig.skipLinesStart, m_csvConfig.skipLinesEnd));
	if (rowCount <= 0)
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
	auto selectedColIdx = getSelectedColIdx(m_fileHeaders, m_csvConfig.selectedHeaders, mode);
	if (!computeColumnMapping())
		return false;
	determineOutputHeaders(selectedColIdx, mode);
	int colCount = m_outputHeaders.size();

	dstTbl.initialize(m_outputHeaders, rowCount);

	size_t resultRowID = 1;
	for (size_t row = 0; row < rowCount; ++row)
	{
		QString line = in.readLine();
		if (line.isEmpty())
			continue;
		QStringList entries;
		switch (mode)
		{
			case LegacyFibers: entries = fibreCalculation(line, colCount, rowCount); break;
			case LegacyPores:
				for (int j = 0; j < colCount; j++)
					entries.append(line.section(",", j, j));
				break;
			case NewCSV:
			{
				if (m_csvConfig.addAutoID)
					entries.append(QString::number(resultRowID));

				auto values = line.split(m_csvConfig.columnSeparator);
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
					entries.append(value);
				}
				if (m_csvConfig.computeLength || m_csvConfig.computeAngles || m_csvConfig.computeTensors || m_csvConfig.computeCenter)
				{
					double x1 = getValueAsDouble(values, m_columnMapping[iACsvConfig::StartX], m_csvConfig.decimalSeparator);
					double y1 = getValueAsDouble(values, m_columnMapping[iACsvConfig::StartY], m_csvConfig.decimalSeparator);
					double z1 = getValueAsDouble(values, m_columnMapping[iACsvConfig::StartZ], m_csvConfig.decimalSeparator);
					double x2 = getValueAsDouble(values, m_columnMapping[iACsvConfig::EndX], m_csvConfig.decimalSeparator);
					double y2 = getValueAsDouble(values, m_columnMapping[iACsvConfig::EndY], m_csvConfig.decimalSeparator);
					double z2 = getValueAsDouble(values, m_columnMapping[iACsvConfig::EndZ], m_csvConfig.decimalSeparator);
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
					if (m_csvConfig.computeAngles || m_csvConfig.computeTensors)
					{
						double phi = asin(dy / sqrt(dx*dx + dy*dy));
						double theta = acos(dz / sqrt(dx*dx + dy*dy + dz*dz));
						double a11 = cos(phi)*cos(phi)*sin(theta)*sin(theta);
						double a22 = sin(phi)*sin(phi)*sin(theta)*sin(theta);
						double a33 = cos(theta)*cos(theta);
						double a12 = cos(phi)*sin(theta)*sin(theta)*sin(phi);
						double a13 = cos(phi)*sin(theta)*cos(theta);
						double a23 = sin(phi)*sin(theta)*cos(theta);
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
							a11 = 0.0;
							a22 = 0.0;
							a12 = 0.0;
							a13 = 0.0;
							a23 = 0.0;
						}
						if (m_csvConfig.computeAngles)
						{
							entries.append(DblToString(phi));
							entries.append(DblToString(theta));
						}
						if (m_csvConfig.computeTensors)
						{
							entries.append(DblToString(a11));
							entries.append(DblToString(a22));
							entries.append(DblToString(a33));
							entries.append(DblToString(a12));
							entries.append(DblToString(a13));
							entries.append(DblToString(a23));
						}
					}
				}
				break;
			}
		}
		entries.append("0"); // class ID
		dstTbl.addRow(resultRowID-1, entries);
		++resultRowID;
	}

	if (file.isOpen())
		file.close();
	return true;
}

void iACsvIO::determineOutputHeaders(QVector<int> const & selectedCols, ReadMode mode)
{
	if (mode == LegacyFibers)
	{
		m_outputHeaders = getFibreElementsName(true);
	}
	else
	{
		for (int i=0; i<selectedCols.size(); ++i)
			m_outputHeaders.append(m_fileHeaders[selectedCols[i]]);

		if (m_csvConfig.addAutoID)
			m_outputHeaders.insert(0, iACsvIO::ColNameAutoID);
		if (m_csvConfig.computeLength)
		{
			m_outputHeaders.append(iACsvIO::ColNameLength);
		}
		if (m_csvConfig.computeCenter)
		{
			m_outputHeaders.append(iACsvIO::ColNameCenterX);
			m_outputHeaders.append(iACsvIO::ColNameCenterY);
			m_outputHeaders.append(iACsvIO::ColNameCenterZ);
		}
		if (m_csvConfig.computeAngles)
		{
			m_outputHeaders.append(iACsvIO::ColNamePhi);
			m_outputHeaders.append(iACsvIO::ColNameTheta);
		}
		if (m_csvConfig.computeTensors)
		{
			m_outputHeaders.append(iACsvIO::ColNameA11);
			m_outputHeaders.append(iACsvIO::ColNameA22);
			m_outputHeaders.append(iACsvIO::ColNameA33);
			m_outputHeaders.append(iACsvIO::ColNameA12);
			m_outputHeaders.append(iACsvIO::ColNameA13);
			m_outputHeaders.append(iACsvIO::ColNameA23);
		}
	}
	m_outputHeaders.append(iACsvIO::ColNameClassID);
}

QVector<int> iACsvIO::getSelectedColIdx(QStringList const & fileHeaders, QStringList const & selectedHeaders, ReadMode mode)
{
	QVector<int> result;
	if (mode == LegacyFibers || mode == LegacyPores)
	{
		for (int i = 0; i < fileHeaders.size(); ++i)
			result.append(i);
	}
	else
	{
		for (QString colName: selectedHeaders)
		{
			int idx = fileHeaders.indexOf(colName);
			if (idx >= 0)
				result.append(idx);
			else
				DEBUG_LOG(QString("Selected column '%1' not found in file headers '%2', skipping.").arg(colName).arg(fileHeaders.join(",")));
		}
	}
	return result;
}

bool iACsvIO::computeColumnMapping()
{
	m_columnMapping.clear();
	for (int key : m_csvConfig.columnMapping.keys())
	{
		QString columnName = m_csvConfig.columnMapping[key];
		int index = m_fileHeaders.lastIndexOf(columnName);
		if (index == -1)
		{
			DEBUG_LOG(QString("Invalid column mapping: No column found with name '%1'!").arg(columnName));
			return false;
		}
		m_columnMapping.insert(key, index);
	}
	return true;
}

size_t iACsvIO::calcRowCount(QTextStream& in, const size_t skipLinesStart, const size_t skipLinesEnd)
{
	// skip (unused) header lines (+1 for line containing actual column headers)
	for (int i = 0; i < skipLinesStart + 1 && !in.atEnd(); i++)
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

QStringList iACsvIO::getFibreElementsName(bool withUnit)
{
	// TODO: overlap with dlg_FeatureScout::getNamesOfObjectCharakteristics !
	// manually define new table elements
	// using unicode to define the unit
	QString micro = QChar(0x00B5);
	QString ud = QChar(0x00B0);
	QString h2 = QChar(0x00B2);
	QString h3 = QChar(0x00B3);
	QString udegree = QString("");
	QString micro1 = QString("");
	QString micro2 = QString("");
	QString micro3 = QString("");
	if (withUnit)
	{
		udegree = QString("[%1]").arg(ud);
		micro1 = QString("[%1m]").arg(micro);
		micro2 = QString("[%1m%2]").arg(micro).arg(h2);
		micro3 = QString("[%1m%2]").arg(micro).arg(h3);
	}

	QStringList eleString;
	eleString.append("Label");                                       // 0
	eleString.append(QString("X1%1").arg(micro1));                   // 1
	eleString.append(QString("Y1%1").arg(micro1));                   // 2
	eleString.append(QString("Z1%1").arg(micro1));                   // 3
	eleString.append(QString("X2%1").arg(micro1));                   // 4
	eleString.append(QString("Y2%1").arg(micro1));                   // 5
	eleString.append(QString("Z2%1").arg(micro1));                   // 6
	eleString.append("a11");                                         // 7
	eleString.append("a22");                                         // 8
	eleString.append("a33");                                         // 9
	eleString.append("a12");                                         // 10
	eleString.append("a13");                                         // 11
	eleString.append("a23");                                         // 12
	eleString.append(QString("phi%1").arg(udegree));                 // 13
	eleString.append(QString("theta%1").arg(udegree));               // 14
	eleString.append(QString("Xm%1").arg(micro1));                   // 15
	eleString.append(QString("Ym%1").arg(micro1));                   // 16
	eleString.append(QString("Zm%1").arg(micro1));                   // 17
	if (withUnit)
	{
		eleString.append(QString("StraightLength%1").arg(micro1));   // 18
		eleString.append(QString("CurvedLength%1").arg(micro1));     // 19
	}
	else
	{
		eleString.append(QString("sL%1").arg(micro1));               // 18
		eleString.append(QString("cL%1").arg(micro1));               // 19
	}
	eleString.append(QString("Diameter%1").arg(micro1));             // 20
	eleString.append(QString("Surface%1").arg(micro2));              // 21
	eleString.append(QString("Volume%1").arg(micro3));               // 22
	if (withUnit)
	{
		eleString.append("SeparatedFiber");                          // 23
		eleString.append("CurvedFiber");                             // 24
	}
	else
	{
		eleString.append("sFiber");                                  // 23
		eleString.append("cFiber");                                  // 24
	}
	eleString.append(ColNameClassID);                                // 25

	return eleString;
}

const QStringList & iACsvIO::getFileHeaders() const
{
	return m_fileHeaders;
}

const QStringList & iACsvIO::getOutputHeaders() const
{
	return m_outputHeaders;
}
