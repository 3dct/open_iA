#include "iACsvDataStorage.h"

#include <QMessageBox>
#include <QFile>
#include <QTextStream>

#include <cstdlib>

iACsvDataStorage::iACsvDataStorage(QStringList* csvFiles) :
	m_filenames(csvFiles), 
	m_data(new QList<csvFileData>()),
	m_totalNumberOfObjects(0),
	m_MDSData(nullptr)
{
	for (int ind = 0; ind < m_filenames->size(); ind++)
	{
		QList<QStringList>* list = readCSV(m_filenames->at(ind));

		if (list == nullptr)
		{
			//QMessageBox::information(this, "CompVis",
			//	"CSV File could not be read! "
			//	"Please try again or specify another file.");
			return;
		}

		storeCSVToVectorArray(list);
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

void iACsvDataStorage::storeCSVToVectorArray(QList<QStringList>* list)
{
	struct csvFileData file;
	file.header = new QStringList();
	file.values = new csvDataType::ArrayType();

	customizeCSVFile(list);
	initializeHeader(list, file.header);
	initializeValueArray(list, file.header->size(), file.values);

	m_data->append(file);
}

void iACsvDataStorage::initializeHeader(QList<QStringList>* list, QStringList* headers)
{
	//store header Strings
	for (int ind = 0; ind < list->at(0).size(); ind++)
	{
		headers->append(list->at(0).at(ind));
	}
}

void iACsvDataStorage::initializeValueArray(
	QList<QStringList>* list, int const attrCount, csvDataType::ArrayType* values)
{
	for (int row = 1; row < (list->size()); row++)
	{
		std::vector<double> vec = std::vector<double>();
		vec.reserve(attrCount);
		for (int col = 0; col < attrCount; col++)
		{
			//values->at(row - 1).at(col) = list->at(row).at(col).toDouble();
			vec.push_back(list->at(row).at(col).toDouble());
		}
		values->push_back(vec);
	}
}

void iACsvDataStorage::customizeCSVFile(QList<QStringList>* list)
{
	//only for fiber CSVs
	for (int i = 0; i < 4; i++)
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
		result->at(i) = data->at(i).values->size();
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
		/*int rand1;
		double rand2;*/

		std::vector<double> vec(columns);
		for (int c = 0; c < columns; c++)
		{
			//rand1 = rand() * rand() + rand() * rand() + rand();
			////rand1 = rand();

			//if (rand1 < 0)
			//	rand1 = -rand1;
			//rand2 = double(rand1 % 1000001) / 1000000;

			//vec.at(c) = double(rand2);


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
		return input->at(0).size();
	}

	LOG(lvlDebug,"There is no column inside this row!");

	return -1;
}

int csvDataType::getRows(ArrayType* input)
{
	return input->size();
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

