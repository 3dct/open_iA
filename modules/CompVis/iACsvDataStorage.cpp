/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iACsvDataStorage.h"

//CompVis
#include "iACompVisOptions.h"

//iAobjectvis
#include "dlg_CSVInput.h"
#include "iACsvConfig.h"
#include "iACsvIO.h"
#include "iACsvVtkTableCreator.h"

//QT
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

//vtk
#include "vtkTable.h"


#include <cstdlib>

iACsvDataStorage::iACsvDataStorage(QStringList* csvFiles, int headerLineNumber) :
	m_filenames(csvFiles),
	m_data(new QList<csvFileData>()),
	m_totalNumberOfObjects(0),
	m_MDSData(nullptr),
	m_minVal(0.0),
	m_maxVal(0.0)
{
	for (int ind = 0; ind < m_filenames->size(); ind++)
	{
		QList<QStringList>* list = readCSV(m_filenames->at(ind));

		if (list == nullptr)
		{
			LOG(lvlError, QString("Unable to read file '%1'").arg(m_filenames->at(ind)));
			return;
		}
		
		//create data structure for MDS and CompVis
		storeCSVToVectorArray(list, headerLineNumber);

	
		//create data structure for 3DVis for library iAobjectvis
		if (iACompVisOptions::getShow3DViews())
		{
			initializeObjectTableFor3DRendering();
		}

	}

	//calculate overall number of objects of all datasets
	std::vector<int>* objectsPerDataset = csvFileData::getAmountObjectsEveryDataset(m_data);
	int sum = 0;
	for(int i = 0; i < ((int)objectsPerDataset->size()); i++)
	{
		sum += objectsPerDataset->at(i);
	}

	m_totalNumberOfObjects = sum;
}

void iACsvDataStorage::initializeObjectTableFor3DRendering()
{
	dlg_CSVInput* dlg = new dlg_CSVInput(false);
	if (dlg->exec() != QDialog::Accepted)
	{
		return;
	}
	
	iACsvConfig csvConfig = dlg->getConfig();
	iACsvVtkTableCreator creator;
	iACsvIO* io = new iACsvIO();
	
	if (!io->loadCSV(creator, csvConfig))
	{
		return;
	}

	vtkSmartPointer<vtkTable> objectTable = creator.table();
	m_objectTables.push_back(objectTable);
	m_outputMappings.push_back(io->getOutputMapping());
	m_csvConfigs.push_back(csvConfig);
}

QList<QStringList>* iACsvDataStorage::readCSV(QString csvFile)
{
	QFile f(csvFile);

	if (!f.open(QIODevice::ReadOnly))
	{
		return nullptr;
	}

	QTextStream ts(&f);
	QList<QStringList> list;

	// read entire file and parse lines into list of stringlist's
	while (!ts.atEnd())
	{
		QStringList li = ts.readLine().split(",");

		//remove last element when empty
		QString lastElem = li.at(li.size() - 1);
		if (lastElem == QString(""))
		{
			li.removeAt(li.size() - 1);
		}

		list.append(li);
	}

	f.close();  // done with file

	return new QList<QStringList>(list);
}

void iACsvDataStorage::storeCSVToVectorArray(QList<QStringList>* list, int headerLineNumber)
{
	struct csvFileData file;
	file.header = new QStringList();
	file.values = new csvDataType::ArrayType();

	customizeCSVFile(list, headerLineNumber);
	initializeHeader(list, file.header, headerLineNumber);
	initializeValueArray(list, file.header->size(), file.values);

	m_data->append(file);
}

void iACsvDataStorage::initializeHeader(QList<QStringList>* list, QStringList* headers, int headerLineNumber)
{
	if (headerLineNumber == 0)
	{//when there are no attribute labels
		std::vector<double> attrNames = std::vector<double>();
		for (int col = 0; col < (list->at(0).size()); col++)
		{
			headers->append(QString("Attr" + QString::number(col)));
		}
	}
	else
	{
		//store header Strings
		for (int ind = 0; ind < list->at(0).size(); ind++)
		{
			headers->append(list->at(0).at(ind));
		}

		//remove header from original data
		list->removeAt(0);
	}
}

void iACsvDataStorage::initializeValueArray(
	QList<QStringList>* list, int const attrCount, csvDataType::ArrayType* values)
{
	int colNumber = list->at(0).size();
	int rowNumber = list->size();

	if (colNumber == 1)
	{
		for (int row = 0; row < rowNumber; row++)
		{
			//for each feature

			std::vector<double> vec = std::vector<double>();
			vec.reserve(attrCount);
			
			vec.push_back(row); //add id
			vec.push_back(list->at(row).at(0).toDouble()); //add value
			
			//store the feature's values
			values->push_back(vec);
		}
	}
	else
	{
		for (int row = 0; row < (list->size()); row++)
		{
			std::vector<double> vec = std::vector<double>();
			vec.reserve(attrCount);
			for (int col = 0; col < attrCount; col++)
			{
				vec.push_back(list->at(row).at(col).toDouble());
			}
			values->push_back(vec);
		}
	}
}

void iACsvDataStorage::customizeCSVFile(QList<QStringList>* list, int headerLineNumber)
{
	//remove unnecessary lines of code before the line storing the attribute names
	for (int i = 0; i < headerLineNumber; i++)
	{
		list->removeAt(0);
	}
}

QStringList* iACsvDataStorage::getDatasetNames()
{
	return m_filenames;
}

QList<csvFileData>* iACsvDataStorage::getData()
{
	return m_data;
}

int iACsvDataStorage::getTotalNumberOfObjects()
{
	return m_totalNumberOfObjects;
}

//returns the names of the attributes without the label attribute
QStringList* iACsvDataStorage::getAttributeNamesWithoutLabel()
{
	QStringList attrNamesWithoutLabel = *(this->getData()->at(0).header);
	attrNamesWithoutLabel.removeFirst();

	QStringList* newList = new QStringList(attrNamesWithoutLabel);

	return newList;
}

QStringList* iACsvDataStorage::getAttributeNames()
{
	return this->getData()->at(0).header;
}


double iACsvDataStorage::getMinVal()
{
	return m_minVal;
}

void iACsvDataStorage::setMinVal(double minVal)
{
	m_minVal = minVal;
}

double iACsvDataStorage::getMaxVal()
{
	return m_maxVal;
}

void iACsvDataStorage::setMaxVal(double maxVal)
{
	m_maxVal = maxVal;
}

/*********************** 3D Rendering ******************************************/
std::vector<vtkSmartPointer<vtkTable>> const & iACsvDataStorage::getObjectTables()
{
	return m_objectTables;
}

std::vector<QSharedPointer<QMap<uint, uint>>> const & iACsvDataStorage::getOutputMappings()
{
	return m_outputMappings;
}

std::vector<iACsvConfig> const & iACsvDataStorage::getCsvConfigs()
{
	return m_csvConfigs;
}

/*********************** store data computed by MDS ******************************************/
csvDataType::ArrayType* iACsvDataStorage::getMDSData()
{
	return m_MDSData;
}

void iACsvDataStorage::setMDSData(csvDataType::ArrayType* mdsData)
{
	m_MDSData = mdsData;
}

/*********************** csvFileData methods ******************************************/
std::vector<int>* csvFileData::getAmountObjectsEveryDataset(QList<csvFileData>* data)
{
	int amountDatasets = data->size();
	std::vector<int>* result = new std::vector<int>(amountDatasets);

	for (int i = 0; i < amountDatasets; i++)
	{
		result->at(i) = static_cast<int>(data->at(i).values->size());
	}
	
	return result;
}


/***********************  csvDataType methods  ******************************************/
void csvDataType::initialize(int rows, int columns, csvDataType::ArrayType* result)
{
	/*std::vector<double> vec(columns, 0.0);
	return csvDataType::ArrayType(rows, vec);*/

	//TODO here is problem of memory leak!!!!!!!!!!!!!
	result->reserve(rows);

	for(int i = 0;  i < rows; i++)
	{
		std::vector<double> vec = std::vector<double>();
		vec.reserve(columns);

		for (int j = 0; j < columns; j++)
		{
			vec.push_back(0.0);
		}
		result->push_back(vec);
	}

}

void csvDataType::initializeRandom(int rows, int columns, csvDataType::ArrayType* result)
{
	double rand2 = 1.0 / rows;
	for (int r = 0; r < rows; r++)
	{
		std::vector<double> vec(columns);
		for (int c = 0; c < columns; c++)
		{
			//do not initialize random to reproduce the results of the MDS
			vec.at(c) = double(rand2);

		}
		rand2 += (1.0 / rows);

		result->push_back(vec);
	}
}

double csvDataType::mean(ArrayType* input)
{
	int rows = getRows(input);
	int columns = getColumns(input);

	double x = 0;

	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < columns; c++)
		{
			x += input->at(r).at(c);
		}
	}

	return x / rows / columns;
}

void csvDataType::addNumberSelf(ArrayType* input, double value)
{
	int cols = getColumns(input);
	int rows = getRows(input);

	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			input->at(r).at(c) += value;
		}
	}
}

void csvDataType::multiplyNumberSelf(ArrayType* input, double value)
{
	int cols = getColumns(input);
	int rows = getRows(input);

	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			input->at(r).at(c) *= value;
		}
	}
}

csvDataType::ArrayType* csvDataType::copy(ArrayType* input)
{
	int cols = getColumns(input);
	int rows = getRows(input);

	ArrayType* result = new ArrayType(); 
	initialize(rows, cols, result);

	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			result->at(r).at(c) = input->at(r).at(c);
		}
	}
	return result;
}

int csvDataType::getColumns(ArrayType* input)
{
	if (input->size() > 0)
	{
		return static_cast<int>(input->at(0).size());
	}

	LOG(lvlDebug,"There is no column inside this row!");

	return -1;
}

int csvDataType::getRows(ArrayType* input)
{
	return static_cast<int>(input->size());
}

csvDataType::ArrayType* csvDataType::elementCopy(ArrayType* input)
{
	int cols = getColumns(input);
	int rows = getRows(input);
	ArrayType* result = new ArrayType();
	initialize(rows, cols, result);
	
	for (int r = 0; r < rows; r++)
	{
		for (int c = 0; c < cols; c++)
		{
			result->at(r).at(c) = input->at(r).at(c);
		}
	}

	return result;
}

csvDataType::ArrayType* csvDataType::transpose(ArrayType* input)
{
	int amountCols = getColumns(input);
	int amountRows = getRows(input);

	ArrayType* result = new ArrayType();
	initialize(amountRows, amountCols, result);

	for (int r = 0; r < amountRows; r++)
	{
		for (int c = 0; c < amountCols; c++)
		{
			result->at(c).at(r) = input->at(r).at(c);
		}
	}

	return result;
}

std::vector<double>* csvDataType::arrayTypeToVector(ArrayType* input)
{
	int amountCols = getColumns(input);
	int amountRows = getRows(input);

	if (amountCols <= 0 || amountCols > 1)
	{
		return nullptr;
	}

	std::vector<double>* result = new std::vector<double>(amountRows);

	for (int r1 = 0; r1 < amountRows; r1++)
	{
		for (int col1 = 0; col1 < amountCols; col1++)
		{
			result->at(r1) = input->at(r1).at(col1);
		}

	}
	return result;
}

void csvDataType::debugArrayType(ArrayType* input)
{
	int amountCols = getColumns(input);
	int amountRows = getRows(input);

	QString output;

	//DEBUG
	LOG(lvlDebug,"Columns: " + QString::number(amountCols));
	LOG(lvlDebug,"Rows: " + QString::number(amountRows));

	LOG(lvlDebug,"Matrix: ");
	for (int r1 = 0; r1 < amountRows; r1++)
	{
		//LOG(lvlDebug,"Row " + QString::number(r1) + ":");

		for (int col1 = 0; col1 < amountCols; col1++)
		{
			//LOG(lvlDebug,"Column " + QString::number(col1) + ": " + QString::number(input->at(r1).at(col1)));
			//LOG(lvlDebug,QString("Column %1 : %2").arg(col1).arg(input->at(r1).at(col1)));
			QString number = QString::number(input->at(r1).at(col1));
			if(number.size() == 1)
			{
				number += ".000000";
			}
			int pos = number.lastIndexOf(QChar('.'));
			output += number.left(pos + 7) + ", ";
		}

		output += "\n";
	}

	LOG(lvlDebug,output);
}

