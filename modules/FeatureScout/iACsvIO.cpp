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

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkMath.h>
#include <vtkTable.h>

#include <QFile>
#include <QFileDialog>
#include <QIODevice>
#include <QMessageBox>
#include <QStringList>
#include <QTableWidget>
#include <QTextCodec>
#include <QTextStream>

namespace
{
	const int LegacyFormatStartSkipLines = 5;
}

const char* iACsvIO::ColNameAutoID = "Auto_ID";
const char* iACsvIO::ColNameClassID = "Class_ID";

iACsvIO::iACsvIO() :
	table(vtkSmartPointer<vtkTable>::New()),
	useCVSOnly(false),
	enableFiberTransformation(false)
{}

bool iACsvIO::loadFibreCSV(const QString &fileName)
{
	// test availability of the table and clear the table
	table->Initialize();

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QTextStream in(&file);
	size_t rowCount = calcRowCount(in, LegacyFormatStartSkipLines, 0);
	if (rowCount <= 0)
		return false;

	// todo: efficient ways to detect header lines in csv file
	// or define header information with header length and table length
	// then there is no need to calculate the table length

	// skip header lines
	in.readLine(); in.readLine(); in.readLine(); in.readLine(); in.readLine();

	QStringList eleString = getFibreElementsName(true);
	vtkSmartPointer<vtkIntArray> arrX = vtkSmartPointer<vtkIntArray>::New();
	arrX->SetName("Label");
	table->AddColumn(arrX);

	for (int i = 1; i<eleString.size(); i++)
	{
		vtkSmartPointer<vtkDoubleArray> arrX = vtkSmartPointer<vtkDoubleArray>::New();
		QByteArray byteArr = eleString.at(i).toUtf8();
		arrX->SetName(byteArr.constData());
		table->AddColumn(arrX);
	}
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName(ColNameClassID);
	table->AddColumn(arr);
	table->SetNumberOfRows(rowCount);

	//additional entries for Fibres
	int colCount = eleString.size();
	fibreCalculation(in, colCount, rowCount, true);
	file.close();
	return true;
}


void iACsvIO::fibreCalculation(QTextStream & in, size_t const colCount, size_t const rowCount, bool const useOldFeatureScoutFormat)
{
	double x1, x2, y1, y2, z1, z2, dx, dy, dz, xm, ym, zm, phi, theta;
	double a11, a22, a33, a12, a13, a23;

	for (int i = 0; i < rowCount; i++)
	{
		QString line = in.readLine();
		if (!line.isEmpty())
		{
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

			phi = (phi*180.0f) / vtkMath::Pi();
			theta = (theta*180.0f) / vtkMath::Pi(); // finish calculation
										 // locat the phi value to quadrant
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

			if ((!useCVSOnly) | enableFiberTransformation )
			{
				table->SetValue(i, 0, line.section(",", 0, 0).toInt());

				//QUICK&DIRTY: and dirty for voids to get the right values out of the csv: j<13 (7)+ comment table->SetValues 7-17
				//first entrys are those ones

				//TODO save in the first column with AUTO ID 2-8, //start colIdx ->2, end will be 8
				//
				for (int j = 1; j < 7; j++)
				{
					table->SetValue(i, j, line.section(",", j, j).toFloat());
				}
				int col_idx = 7;

				if (useOldFeatureScoutFormat)
				{
					col_idx = assignFiberValuesPart1(i, col_idx, a11, a22, a33, a12, a13, a23); // 7 - 12
					col_idx = assingFiberValuesPart_2(i, col_idx, phi, theta, xm, ym, zm);      // 13 - 17
				} // end useOldFeatureScout format

				//use the other entries of columns, using auto ID will be 8
				for (int j = 7; j < colCount; j++)
				{
					table->SetValue(i, col_idx /*j + 11*/, line.section(",", j, j).toFloat()); //TODO in dbl
					col_idx++;
				}

				//for new FeatureScout format just append it
				if (!useOldFeatureScoutFormat) {
					col_idx = assignFiberValuesPart1(i, col_idx, a11, a22, a33, a12, a13, a23);

					//13 - 17
					col_idx = assingFiberValuesPart_2(i, col_idx, phi, theta, xm, ym, zm);

				}
				table->SetValue(i, colCount - 1, 0); // with autoId +1 last column adding class information
				//TODO setup to new featureScoutFormat
			} //end !use csv only
		}
		else table->RemoveRow(i);  //Skip empty rows
	}//end for loop
}

int iACsvIO::assingFiberValuesPart_2(int i, int col_idx, double phi, double theta, double xm, double ym, double zm)
{
	table->SetValue(i, col_idx, phi);  col_idx++;   //14
	table->SetValue(i, col_idx, theta); col_idx++;  //15
	table->SetValue(i, col_idx, xm);	col_idx++;  //16
	table->SetValue(i, col_idx, ym);	col_idx++;  //17
	table->SetValue(i, col_idx, zm);	col_idx++;
	return col_idx;
}

int iACsvIO::assignFiberValuesPart1(int i, int col_idx, double a11, double a22, double a33, double a12, double a13, double a23)
{
	table->SetValue(i, col_idx, a11);  col_idx++;  //8
	table->SetValue(i, col_idx, a22); col_idx++;   //9
	table->SetValue(i, col_idx, a33); col_idx++;   //10
	table->SetValue(i, col_idx, a12); col_idx++;   //11
	table->SetValue(i, col_idx, a13);  col_idx++;  //12
	table->SetValue(i, col_idx, a23);  col_idx++;
	return col_idx;
}

bool iACsvIO::loadPoreCSV(const QString &fileName)
{
	table->Initialize();

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QTextStream in(&file);
	size_t rowCount = calcRowCount(in, LegacyFormatStartSkipLines, 0);
	if (rowCount <= 0)
		return false;

	// todo: efficient ways to detect header lines in csv file
	// or define header information with header length and table length
	// then there is no need to calculate the table length

	// skip header lines
	in.readLine(); in.readLine(); in.readLine(); in.readLine();

	// read names of elements
	QString eleLine = in.readLine();
	const char* element;
	int colCount = eleLine.count(",");

	for (int i = 0; i<colCount; i++)
	{
		vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
		QByteArray byteArr = eleLine.section(",", i, i, QString::SectionSkipEmpty).toUtf8();
		element = byteArr.constData();
		arrX->SetName(element);
		table->AddColumn(arrX);
	}
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName(iACsvIO::ColNameClassID);
	table->AddColumn(arr);
	table->SetNumberOfRows(rowCount);

	for (int i = 0; i<rowCount; ++i)
	{
		QString line = in.readLine();
		if (!line.isEmpty())
		{
			vtkVariant v = line.section(",", 0, 0).toInt();
			table->SetValue(i, 0, v.ToString()); // set pore or fibre id

			for (int j = 1; j<colCount; j++) // saving values for each col
			{
				float tmp = line.section(",", j, j).toFloat(); //TODO REMOVE
				table->SetValue(i, j, line.section(",", j, j).toFloat());
			}
			table->SetValue(i, colCount, 0);  // set Class_ID value to zero
		}
	}
	file.close();
	return true;
}

bool iACsvIO::loadConfig(const QString configName, bool & applyEN_Formating )
{
	QFile file(configName);
	return file.open(QIODevice::ReadOnly | QIODevice::Text);
}

bool iACsvIO::loadCSVCustom(iACsvConfig const & cnfg_params)
{
	m_csvConfig = cnfg_params;
	enableFiberTransformation = cnfg_params.objectType == iAFeatureScoutObjectType::Fibers;
	useCVSOnly = true;
	table->Initialize();
	return readCustomFileEntries();
}

void iACsvIO::setParams(QStringList & headers, const QVector<uint>& colIDs)
{
	m_TableHeaders = headers;
	m_colIds = colIDs;
}

void iACsvIO::debugTable(const bool useTabSeparator)
{
	std::string separator = (useTabSeparator) ? "\t": ",";
	ofstream debugfile;
	debugfile.open("C:/Users/p41883/Desktop/inputData.txt");
	if (debugfile.is_open())
	{
		vtkVariant spCol, spRow, spCN, spVal;
		spCol = table->GetNumberOfColumns();
		spRow = table->GetNumberOfRows();

		for (int i = 0; i<spCol.ToInt(); i++)
		{
			spCN = table->GetColumnName(i);
			debugfile << spCN.ToString() << separator;
		}
		debugfile << "\n";
		for (int row = 0; row < spRow.ToInt(); row++)
		{
			for (int col = 0; col < spCol.ToInt(); col++)
			{
				spVal = table->GetValue(row, col);
				debugfile << spVal.ToString() << separator; //TODO cast debug to double
			}
			debugfile << "\n";
		}
		debugfile.close();
	}
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
		in.readLine();
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

bool iACsvIO::loadCsvFile(iAFeatureScoutObjectType fid, QString const & fileName)
{
	if (!QFile::exists(fileName))
	{
		DEBUG_LOG("Error loading csv file, file does not exist.");
		return false;
	}
	switch (fid)
	{
		case iAFeatureScoutObjectType::Fibers: return loadFibreCSV(fileName);
		case iAFeatureScoutObjectType::Voids:  return loadPoreCSV(fileName);
		default:                               return false;
	}
}

vtkTable* iACsvIO::getCSVTable()
{
	return table.GetPointer();
}

bool iACsvIO::readCustomFileEntries()
{
	QFile file(m_csvConfig.fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	QTextStream in(&file);
	in.setCodec(m_csvConfig.encoding.toStdString().c_str());
	size_t rowCount = calcRowCount(in, m_csvConfig.skipLinesStart, m_csvConfig.skipLinesEnd);
	if (rowCount <= 0)
		return false;
	
	for (int i = 0; i < m_csvConfig.skipLinesStart + 1; i++)	// skip lines including header
		in.readLine();

	int colCount = 0;
	if (enableFiberTransformation)
	{
		m_TableHeaders = getFibreElementsName(true);
		colCount = m_TableHeaders.length();
	}

	//else use default header

	setColumnHeaders(m_TableHeaders);
	table->SetNumberOfRows(rowCount);

	if (!enableFiberTransformation)
	{
		if (m_colIds.length() > 0)
		{
			colCount = m_colIds.length();
		}
		else
		{
			colCount = m_TableHeaders.length();
		}
		loadPoreData(in, colCount, rowCount);
	}
	else
	{
		fibreCalculation(in, rowCount, colCount, true);
		//TODO adapt to new Featurescout csvOnly
	}

	if(file.isOpen())
		file.close();
	return true;
}

void iACsvIO::setColumnHeaders(QStringList &colHeaders)
{
	vtkSmartPointer<vtkIntArray> arrAuto = vtkSmartPointer<vtkIntArray>::New();
	if (m_csvConfig.addAutoID)
	{
		arrAuto->SetName(ColNameAutoID);
		table->AddColumn(arrAuto);
	}
	//adding headers;
	for (const auto &elLine : colHeaders)
	{
		if (!elLine.isEmpty())
		{
			vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
			QByteArray byteArr = elLine.toUtf8();
			arrX->SetName(byteArr.constData());
			table->AddColumn(arrX);
		}
	}
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName(ColNameClassID);
	table->AddColumn(arr);
}

void iACsvIO::loadPoreData(QTextStream &in, size_t const col_count, size_t const rowCount)
{
	size_t resultRowID = 1;
	for (size_t row = 0; row < (rowCount - m_csvConfig.skipLinesEnd); ++row)
	{
		QString line = in.readLine();
		if (!line.isEmpty())
		{
			table->SetValue(row, 0, resultRowID);
			int curResultCol = 1; // column index in filtered result table

			//adding entries for each col
			for (int col = 1; col<col_count + 1; col++)
			{
				if (m_colIds.contains((uint)col - 1)) //< skip rows not contained in m_colIds
				{
					QString tmp_section = line.section(m_csvConfig.colSeparator, col - 1, col - 1);
					if (!tmp_section.isEmpty())
					{
						//replace decimal separator for german input format
						if (m_csvConfig.decimalSeparator != ".")
							tmp_section = tmp_section.replace(m_csvConfig.decimalSeparator, ".");

						double tbl_value = tmp_section.toDouble();
						table->SetValue(row, curResultCol, tbl_value);
					}
					++curResultCol;
				}
			}
			table->SetValue(row, col_count + 1, 0);
			++resultRowID;
		}
		else // remove row from result if empty
			table->RemoveRow(row);
	}
}

// @{ migrated from DataTable

void iACsvIO::addLineToTable(QTableWidget* dstTbl, QStringList const & tableEntries, size_t row, bool addAutoID)
{
	dstTbl->insertRow(row);
	uint colInd = 0;
	if (addAutoID) // adding autoID column
	{
		dstTbl->setItem(row, colInd, new QTableWidgetItem(QString("%1").arg(row)));
		++colInd;
	}
	for (const auto &tableEntry : tableEntries)
	{
		dstTbl->setItem(row, colInd, new QTableWidgetItem(tableEntry));
		++colInd;
	}
}

bool  iACsvIO::readTableEntries(QTableWidget* dstTbl, const QString &fName, const uint rowCount, const QString & colSeparator,
	const uint skipLinesStart, const bool readHeaders, bool addAutoID, QString const & encoding)
{
	if (fName.isEmpty())
		return false;
	QFile file(fName);
	if (!file.open(QIODevice::ReadOnly))
	{
		QMessageBox::information(dstTbl, QObject::tr("FeatureScout"), QObject::tr("Unable to open file: %1").arg(file.errorString()));
		return false;
	}
	QTextStream in(&file);
	if (!encoding.isEmpty())
		in.setCodec(encoding.toStdString().c_str());

	//skip lines and add header to table;
	prepareHeader(dstTbl, skipLinesStart, in, readHeaders, addAutoID, colSeparator);
	readTableValues(dstTbl, rowCount, in, addAutoID, colSeparator);

	m_LastEncoding = in.codec()->name().toStdString().c_str();
	if (file.isOpen()) file.close();
	return true;
}

void iACsvIO::readTableValues(QTableWidget* dstTbl, size_t const rowCount, QTextStream &file, bool addAutoID, const QString & colSeparator)
{
	size_t row = 0;
	while (!file.atEnd() && row < rowCount)
	{
		QString el_line = file.readLine();
		auto currentEntry = el_line.split(colSeparator);
		addLineToTable(dstTbl, currentEntry, row, addAutoID);
		++row;
	}
	dstTbl->setRowCount(row);
}

void iACsvIO::prepareHeader(QTableWidget* destTbl, uint skipLinesStart, QTextStream &file, bool readHeaders, bool addAutoID, const QString & colSeparator)
{
	for (int curRow = 0; curRow < skipLinesStart; curRow++)
		file.readLine();

	QString line = file.readLine();	// header line
	if (readHeaders)
	{
		m_headerEntries = line.split(colSeparator);
		if (addAutoID)
			m_headerEntries.insert(m_headerEntries.begin(), iACsvIO::ColNameAutoID);
		destTbl->setColumnCount(m_headerEntries.length());
		destTbl->setHorizontalHeaderLabels(m_headerEntries);
	}
}

QString iACsvIO::getLastEncoding() const
{
	return m_LastEncoding;
}

const QStringList & iACsvIO::getHeaders() const
{
	return m_headerEntries;
}

// @}