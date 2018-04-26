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
#include "iACsvIO.h"

#include "iAConsole.h"
#include "iAMathUtility.h"    // for Pi

#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkTable.h>

#include <QFile>
#include <QFileDialog>
#include <QIODevice>
#include <QStringList>
#include <QTextStream>


iACsvIO::iACsvIO() :
	table(vtkSmartPointer<vtkTable>::New()),
	m_colSeparator(","),
	m_decimalSeparator("."),
	m_EN_Values(true),
	m_EL_ID(1),
	m_FileName(""),
	m_tableWidth(0),
	m_useEndLine(false),
	m_endLine(0),
	useCVSOnly(false),
	enableFiberTransformation(false)
{
	this->setDefaultConfigPath(); 
}


bool iACsvIO::LoadFibreCSV(const QString &fileName)
{
	// test availability of the table and clear the table
	table->Initialize();

	// calculate the length of objects in csv file for defining the vtkTable
	int tableLength = CalcTableLength(fileName, nullptr);
	if (tableLength <= 0)
		return false;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	QTextStream in(&file);

	// todo: efficient ways to detect header lines in csv file
	// or define header information with header length and table length
	// then there is no need to calculate the table length

	// header lines
	in.readLine(); in.readLine(); in.readLine(); in.readLine();

	// names of elements
	QString eleLine = in.readLine();
	int eleWidth = eleLine.count(",");
	const char* element;

	QStringList eleString = GetFibreElementsName(true);
	vtkSmartPointer<vtkIntArray> arrX = vtkSmartPointer<vtkIntArray>::New();
	arrX->SetName("Label");
	table->AddColumn(arrX);

	for (int i = 1; i<eleString.size(); i++)
	{
		vtkSmartPointer<vtkDoubleArray> arrX = vtkSmartPointer<vtkDoubleArray>::New();
		QByteArray byteArr = eleString.at(i).toUtf8();
		element = byteArr.constData();
		arrX->SetName(element);
		table->AddColumn(arrX);
	}
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName("Class_ID");
	table->AddColumn(arr);
	table->SetNumberOfRows(tableLength);

	//additional entries for Fibres
	int colCount = eleString.size(); 
	FibreTranformation(in, eleWidth, tableLength, colCount);
	file.close();
	return true;

	//double x1, x2, y1, y2, z1, z2, dx, dy, dz, xm, ym, zm, phi, theta;
	//double a11, a22, a33, a12, a13, a23;

	//for (int i = 0; i<tableLength; i++)
	//{
	//	QString line = in.readLine();
	//	if (!line.isEmpty())
	//	{
	//		x1 = line.section(",", 1, 1).toFloat();
	//		y1 = line.section(",", 2, 2).toFloat();
	//		z1 = line.section(",", 3, 3).toFloat();
	//		x2 = line.section(",", 4, 4).toFloat();
	//		y2 = line.section(",", 5, 5).toFloat();
	//		z2 = line.section(",", 6, 6).toFloat();

	//		// preparing the tensor calculation
	//		dx = x1 - x2;
	//		dy = y1 - y2;
	//		dz = z1 - z2;
	//		xm = (x1 + x2) / 2.0f;
	//		ym = (y1 + y2) / 2.0f;
	//		zm = (z1 + z2) / 2.0f;

	//		if (dz<0)
	//		{
	//			dx = x2 - x1;
	//			dy = y2 - y1;
	//			dz = z2 - z1;
	//		}

	//		phi = asin(dy / sqrt(dx*dx + dy*dy));
	//		theta = acos(dz / sqrt(dx*dx + dy*dy + dz*dz));

	//		a11 = cos(phi)*cos(phi)*sin(theta)*sin(theta);
	//		a22 = sin(phi)*sin(phi)*sin(theta)*sin(theta);
	//		a33 = cos(theta)*cos(theta);
	//		a12 = cos(phi)*sin(theta)*sin(theta)*sin(phi);
	//		a13 = cos(phi)*sin(theta)*cos(theta);
	//		a23 = sin(phi)*sin(theta)*cos(theta);

	//		phi = (phi*180.0f) / Pi;
	//		theta = (theta*180.0f) / Pi; // finish calculation
	//									   // locat the phi value to quadrant
	//		if (dx<0)
	//		{
	//			phi = 180.0 - phi;
	//		}

	//		if (phi<0.0)
	//		{
	//			phi = phi + 360.0;
	//		}

	//		if (dx == 0 && dy == 0)
	//		{
	//			phi = 0.0;
	//			theta = 0.0;
	//			a11 = 0.0;
	//			a22 = 0.0;
	//			a12 = 0.0;
	//			a13 = 0.0;
	//			a23 = 0.0;
	//		}


	//		table->SetValue(i, 0, line.section(",", 0, 0).toInt());

	//		//QUICK&DIRTY: and dirty for voids to get the right values out of the csv: j<13 (7)+ comment table->SetValues 7-17
	//		for (int j = 1; j<7; j++)
	//		{
	//			table->SetValue(i, j, line.section(",", j, j).toFloat());
	//		}

	//		table->SetValue(i, 7, a11);
	//		table->SetValue(i, 8, a22);
	//		table->SetValue(i, 9, a33);
	//		table->SetValue(i, 10, a12);
	//		table->SetValue(i, 11, a13);
	//		table->SetValue(i, 12, a23);
	//		table->SetValue(i, 13, phi);
	//		table->SetValue(i, 14, theta);
	//		table->SetValue(i, 15, xm);
	//		table->SetValue(i, 16, ym);
	//		table->SetValue(i, 17, zm);

	//		for (int j = 7; j<eleWidth; j++)
	//		{
	//			table->SetValue(i, j + 11, line.section(",", j, j).toFloat());
	//		}

	//		table->SetValue(i, eleString.size() - 1, 0);
	//	}
	//}
	//
	//file.close();
	//return true;
	
}


void iACsvIO::FibreTranformation(QTextStream &in, int eleWidth, int tableLength, const int colCount/* &eleString*/) {

	double x1, x2, y1, y2, z1, z2, dx, dy, dz, xm, ym, zm, phi, theta;
	double a11, a22, a33, a12, a13, a23;

	for (int i = 0; i < tableLength; i++)
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

			phi = (phi*180.0f) / Pi;
			theta = (theta*180.0f) / Pi; // finish calculation
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

			if ((!useCVSOnly) | enableFiberTransformation ) {
				table->SetValue(i, 0, line.section(",", 0, 0).toInt());

				//QUICK&DIRTY: and dirty for voids to get the right values out of the csv: j<13 (7)+ comment table->SetValues 7-17
				//first entrys are those ones
				//save in the first column with AUTO ID 2-8, //start colIdx ->2, end will be 8
				for (int j = 1; j < 7; j++)
				{
					table->SetValue(i, j, line.section(",", j, j).toFloat());
				}
					int col_idx = 7; 
					 

					//7
					table->SetValue(i, col_idx, a11);  col_idx++; 
					//8
					table->SetValue(i, col_idx, a22); col_idx++;
					//9
					table->SetValue(i, col_idx, a33); col_idx++; 
					//10
					
					table->SetValue(i, col_idx, a12); col_idx++; 
					//11
					table->SetValue(i, col_idx, a13);  col_idx++;
					//12
					table->SetValue(i, col_idx, a23);  col_idx++; 

					//13	
					table->SetValue(i, col_idx, phi);  col_idx++; 
					//14

					table->SetValue(i, col_idx, theta); col_idx++; 
					//15
					table->SetValue(i, col_idx, xm);	col_idx++; 
					//16

					table->SetValue(i, col_idx, ym);	col_idx++; 
					//17

					table->SetValue(i, col_idx, zm);	col_idx++;

					//original
					//table->SetValue(i, 7, a11);  //7
					//table->SetValue(i, 8, a22);	 //8
					//table->SetValue(i, 9, a33);  //9
					//table->SetValue(i, 10, a12);  //10
					//table->SetValue(i, 11, a13);  //11
					//table->SetValue(i, 12, a23);  //12
					//table->SetValue(i, 13, phi);  //13	
					//table->SetValue(i, 14, theta); //14
					//table->SetValue(i, 15, xm);		//15
					//table->SetValue(i, 16, ym);		//16
					//table->SetValue(i, 17, zm);		//17


					//use the other entries of columns, using auto ID will be 8
					for (int j = 7; j < eleWidth; j++)
					{
						table->SetValue(i, col_idx /*j + 11*/, line.section(",", j, j).toFloat()); //TODO in dbl
						col_idx++; 
					}

					table->SetValue(i, colCount - 1, 0); // with autoId +1 last column adding class information
			}//end !use csv only
		}
		else table->RemoveRow(i);  //Skip empty rows
	}
}

bool iACsvIO::LoadPoreCSV(const QString &fileName)
{
	table->Initialize();
	// calculate the length of objects in csv file for defining the vtkTable
	int tableLength = CalcTableLength(fileName,  nullptr);
	if (tableLength <= 0)
		return false;
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;

	QTextStream in(&file);
	// todo: efficient ways to detect header lines in csv file
	// or define header information with header length and table length
	// then there is no need to calculate the table length

	// header lines
	in.readLine(); in.readLine(); in.readLine(); in.readLine();

	// names of elements
	QString eleLine = in.readLine();
	const char* element;
	// count elements
	int tableWidth = eleLine.count(",");

	//vtkSmartPointer<vtkIntArray> arrX = vtkSmartPointer<vtkIntArray>::New();
	//arrX->SetName("Label");
	//table->AddColumn(arrX);
	//read out entrys
	for (int i = 0; i<tableWidth; i++)
	{
		vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
		QByteArray byteArr = eleLine.section(",", i, i, QString::SectionSkipEmpty).toUtf8();
		element = byteArr.constData();
		arrX->SetName(element);
		table->AddColumn(arrX);
	}
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName("Class_ID");
	table->AddColumn(arr);
	table->SetNumberOfRows(tableLength);

	for (int i = 0; i<tableLength; ++i)
	{
		QString line = in.readLine();
		if (!line.isEmpty())
		{
			vtkVariant v = line.section(",", 0, 0).toInt();
			//set pore or fibre id 
			table->SetValue(i, 0, v.ToString());

			//saving values for each col
			for (int j = 1; j<tableWidth; j++)
			{
				float tmp = line.section(",", j, j).toFloat(); //TODO REMOVE
				table->SetValue(i, j, line.section(",", j, j).toFloat());
			}
			// set Class_ID value to zero 
			table->SetValue(i, tableWidth, 0);
		}
	}
	file.close();
	return true;
}

bool iACsvIO::loadConfig(const QString configName, bool & applyEN_Formating )
{
	QFile file(configName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return false;
	else return true; 

}

bool iACsvIO::loadCSVCustom(csvConfig::configPararams &cnfg_params)
{
	if (!cnfg_params.paramsValid) {
		return false; 
	}
	setTableParams(cnfg_params);
	return loadCsv_WithConfig();
}


void iACsvIO::setParams(QStringList & headers, const QVector<uint>& colIDs, uint TableWidth)
{
	this->setTableHeaders(headers);
	this->setColIDs(colIDs);
	this->setTableWidth(TableWidth);
}

void iACsvIO::debugTable()
{
	ofstream debugfile;
	debugfile.open("C:/Users/p41883/Desktop/inputData.txt");
	if (debugfile.is_open()) {
		vtkVariant spCol, spRow, spCN, spVal;
		spCol = this->table->GetNumberOfColumns();
		spRow = this->table->GetNumberOfRows();

		for (int i = 0; i<spCol.ToInt(); i++) {
			spCN = this->table->GetColumnName(i);
			debugfile << spCN.ToString() << ",";
		}
		debugfile << "\n";
		for (int row = 0; row < spRow.ToInt(); row++) {
			for (int col = 0; col < spCol.ToInt(); col++) {
				spVal = this->table->GetValue(row, col);
				debugfile << spVal.ToString() << ","; //TODO cast debug to double
			}
			debugfile << "\n";
		}
		debugfile.close();
	}

}

void iACsvIO::setDefaultConfigPath()
{
	configPath = "D:/OpenIa_TestDaten/TestInput/config/";
}

long iACsvIO::CalcTableLength(const QString &fileName,const int *nrHeadersToSkip)
{
	// skip lines which are not headers
	// todo: to find another efficient way to count the lines in a file
	if (fileName.isEmpty())
		return 0;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return 0;

	long int TableLength = 0;

	// read header lines, need specification each time the structure of the csv file changes
	//TODO change reading header lines
	//default for feature scout
	if (!nrHeadersToSkip) {
		file.readLine();
		file.readLine();
		file.readLine();
		file.readLine();
		file.readLine();
	}
	else {
		const int headersToSkip = *nrHeadersToSkip; 
		for (int i = 0; i < headersToSkip; i++) {
			file.readLine();
		}
	}

	while (!file.atEnd())
	{
		file.readLine();
		++TableLength; // count text line
	}
	file.close();
	return TableLength;
}


QStringList iACsvIO::GetFibreElementsName(bool withUnit)
{
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
	eleString.append("Label");							// 0	
	eleString.append(QString("X1%1").arg(micro1));		// 1
	eleString.append(QString("Y1%1").arg(micro1));		// 2
	eleString.append(QString("Z1%1").arg(micro1));		// 3
	eleString.append(QString("X2%1").arg(micro1));		// 4
	eleString.append(QString("Y2%1").arg(micro1));		// 5
	eleString.append(QString("Z2%1").arg(micro1));		// 6
	eleString.append("a11");							// 7
	eleString.append("a22");							// 8
	eleString.append("a33");							// 9
	eleString.append("a12");							// 10
	eleString.append("a13");							// 11
	eleString.append("a23");							// 12
	eleString.append(QString("phi%1").arg(udegree));	// 13
	eleString.append(QString("theta%1").arg(udegree));	// 14
	eleString.append(QString("Xm%1").arg(micro1));		// 15
	eleString.append(QString("Ym%1").arg(micro1));		// 16
	eleString.append(QString("Zm%1").arg(micro1));		// 17

														////QUICK&DIRTY to set the right labeling for voids
														//eleString.append("ID");							// 0	
														//eleString.append("Volume");		// 1
														//eleString.append("DimX");		// 2
														//eleString.append("DimY");		// 3
														//eleString.append("DimZ");		// 4
														//eleString.append("PositionX");		// 5
														//eleString.append("PositionY");		// 6
														//eleString.append("PositionZ");							// 7
														//eleString.append("ShapeFactor");							// 8
														//eleString.append("a33");							// 9
														//eleString.append("a12");							// 10
														//eleString.append("a13");							// 11
														//eleString.append("a23");							// 12
														//eleString.append(QString("phi%1").arg(udegree));	// 13
														//eleString.append(QString("theta%1").arg(udegree));	// 14
														//eleString.append(QString("Xm%1").arg(micro1));		// 15
														//eleString.append(QString("Ym%1").arg(micro1));		// 16
														//eleString.append(QString("Zm%1").arg(micro1));		// 17

	if (withUnit)
	{
		eleString.append(QString("StraightLength%1").arg(micro1));		// 18
		eleString.append(QString("CurvedLength%1").arg(micro1));		// 19
	}
	else
	{
		eleString.append(QString("sL%1").arg(micro1));		// 18
		eleString.append(QString("cL%1").arg(micro1));		// 19

	}
	eleString.append(QString("Diameter%1").arg(micro1));	// 20
	eleString.append(QString("Surface%1").arg(micro2));		// 21
	eleString.append(QString("Volume%1").arg(micro3));		// 22
	if (withUnit)
	{
		eleString.append("SperatedFiber");								// 23
		eleString.append("CurvedFiber");								// 24
	}
	else
	{
		eleString.append("sFiber");								// 23
		eleString.append("cFiber");								// 24
	}
	eleString.append("Class_ID");							// 25

	return eleString;
}

bool iACsvIO::LoadCsvFile(iAObjectAnalysisType fid, QString const & fileName)
{
	if (!QFile::exists(fileName))
	{
		DEBUG_LOG("Error loading csv file, file does not exist.");
		return false;
	}
	switch (fid)
	{
		case INDIVIDUAL_FIBRE_VISUALIZATION: return LoadFibreCSV(fileName);
		case INDIVIDUAL_PORE_VISUALIZATION:  return LoadPoreCSV(fileName);
		default:                             return false;
	}
}

vtkTable* iACsvIO::GetCSVTable()
{
	return table.GetPointer();
}



//loading csv into table 
bool iACsvIO::loadCsv_WithConfig(){
	this->useCVSOnly = true; 
	table->Initialize();
	
	bool retFlag = false; 
	//read entries with selected headers
	this->readCustomFileEntries(this->m_FileName, this->m_rowsToSkip, this->m_TableHeaders, this->m_colIds, this->m_EN_Values, retFlag);
	return retFlag; 
}

void iACsvIO::setTableParams(csvConfig::configPararams &csv_Params)
{
	this->m_FileName = csv_Params.fileName;
	this->m_rowsToSkip = csv_Params.startLine;

	if (csv_Params.file_seperator == csvConfig::csvSeparator::Colunm) {
		this->m_colSeparator = ";";
	}
	else if (csv_Params.file_seperator == csvConfig::csvSeparator::Comma) {
		this->m_colSeparator = ",";
	}

	//english decimal format
	if (!(csv_Params.csv_Inputlanguage == (csvConfig::inputLang::EN))) {
		this->m_decimalSeparator = ".";
		this->m_EN_Values = false;
	}

	//endline to Skip
	this->m_useEndLine = csv_Params.useEndline; 
	if (this->m_useEndLine) {
		this->m_endLine = csv_Params.endLine; 
	
	}

	if (csv_Params.inputObjectType == csvConfig::CTInputObjectType::Fiber) {
		this->enableFiberTransformation = true; 
	}
	else this->enableFiberTransformation = false; 

}

void iACsvIO::readCustomFileEntries(const QString &fileName, const int rows_toSkip, const QStringList &m_Headers, QVector<uint> colSelEntries, bool En_values, bool &retFlag) {
	int tableWidth = 0; 
	long tableLength = 0; 
	this->m_EL_ID = 1;  //reset to 1 -> ID starts with 1!
	tableLength = CalcTableLength(fileName, (&rows_toSkip));
	/*if (this->m_useEndLine) {
		this->m_endLine = tableLength -1 ;
	}*/
	
	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		retFlag = false;
		return;
	}

	QTextStream in(&file);

	//skip lines including header 
	for (int i = 0; i < rows_toSkip; i++) {
		QString tmp = in.readLine();
	}

	int col_count = 0;
	if (enableFiberTransformation) {
		this->m_TableHeaders = 	this->GetFibreElementsName(true);
		this->m_tableWidth = m_colIds.length();
		col_count = this->m_TableHeaders.length(); 
	}
	
	//else use default header

	setColumnHeaders(this->m_TableHeaders);

	table->SetNumberOfRows(tableLength);
	QString line = "";
	QString tmp_section = "";
	tableWidth = this->m_tableWidth; 

	
	

	//load pores
	if (!enableFiberTransformation) {

		if (this->m_colIds.length() > 0) {
			col_count = this->m_colIds.length();
		}
		else {
			col_count = this->m_TableHeaders.length();
		}

		loadPoreData(tableLength, line, in, tableWidth, tmp_section, col_count);
	} //TODO FIberTransformation
	else {
		//loadPoreData(tableLength, line, in, tableWidth, tmp_section, col_count);
		this->FibreTranformation(in, tableWidth, tableLength,col_count);
	}
	
	if(file.isOpen())
	file.close();
	retFlag = true; 
}

void iACsvIO::setColumnHeaders(QStringList &colHeaders)
{
	QByteArray byteArr;
	const char* element;

	vtkSmartPointer<vtkIntArray> arrAuto = vtkSmartPointer<vtkIntArray>::New();
	if(!this->enableFiberTransformation){
	arrAuto->SetName("Auto_ID");
	table->AddColumn(arrAuto);
	}
	//adding headers; 
	for (const auto &elLine : colHeaders) {
		if (!elLine.isEmpty()) {
			vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
			byteArr = elLine.toUtf8();
			element = byteArr.constData();
			arrX->SetName(element);
			table->AddColumn(arrX);
		}
	}


	//read entries;
	vtkSmartPointer<vtkIntArray> arr = vtkSmartPointer<vtkIntArray>::New();
	arr->SetName("Class_ID");
	table->AddColumn(arr);
}

void iACsvIO::loadPoreData(long tableLength, QString &line, QTextStream &in, int tableWidth, QString &tmp_section, int col_count)
{
	double tbl_value = 0.0;

	vtkVariant ID_val;

	//use separate col count index 
	int cur_Colcount = 1;

	for (long row = 0; row<tableLength; ++row) 
	{   
		//skip endline if activated
		if (m_useEndLine && (row > m_endLine))
			break;

		line = in.readLine();
		if (!line.isEmpty())
		{
			cur_Colcount = 1;
			table->SetValue(row, 0, this->m_EL_ID );
			
			//adding entries for each col 
			for (int col = 1; col<tableWidth + 1; col++)
			{
			

				//skip rows
				if (m_colIds.contains((uint)col - 1))
				{
					tmp_section = line.section(m_colSeparator, col - 1, col - 1);
					if (!tmp_section.isEmpty())
					{

						//replace decimal separator for german input format 
						if (!this->m_EN_Values)
							tmp_section = tmp_section.replace(",", m_decimalSeparator);

						tbl_value = tmp_section.toDouble();
						table->SetValue(row, cur_Colcount, tbl_value);
					}cur_Colcount++;

				}

			}

			table->SetValue(row, col_count + 1, 0);
			this->m_EL_ID++;
		} // exclude white spaces if row is empty
		else table->RemoveRow(row);
	}
}



