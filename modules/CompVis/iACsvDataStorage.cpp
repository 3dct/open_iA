#include "iACsvDataStorage.h"

#include <QMessageBox>
#include "QFile.h"
#include "QTextStream.h"

iACsvDataStorage::iACsvDataStorage(QStringList* csvFiles, QListView* listView) :
	m_filenames(csvFiles),
	m_data(new QList<csvFileData>())
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
	//start with 1 --> in col 0 the header strings are specified
	for (int col = 1; col < (list->size()); col++)
	{
		values->push_back(std::vector<double>(attrCount, 0));
		for (int row = 0; row < attrCount; row++)
		{
			values->at(col - 1).at(row) = list->at(col).at(row).toDouble();
		}
	}

	//DEBUG
	/*for (int t= 0; t < attrCount; t++)
	{
		DEBUG_LOG(QString::number(values->at(0).at(t)));
	}
	for (int t = 0; t < attrCount; t++)
	{
		DEBUG_LOG(QString::number(values->at(values->size()-1).at(t)));
	}*/
	
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
csvDataType::ArrayType* csvDataType::initialize(int columns, int rows)
{
	std::vector<double> vec(rows, 0);
	return new csvDataType::ArrayType(columns, vec);
}

csvDataType::ArrayType* csvDataType::initializeRandom(int columns, int rows)
{
	ArrayType* result = new ArrayType(columns);
	for (int c = 0; c < columns; c++)
	{
		int rand1;
		double rand2;

		std::vector<double> vec(rows);
		for (int r = 0; r < rows; r++)
		{
			rand1 = rand() * rand() + rand() * rand() + rand();
			if (rand1 < 0)
				rand1 = -rand1;
			rand2 = double(rand1 % 1000001) / 1000000;

			vec.at(r) = double(rand2);
		}
		result->at(c) = vec;
	}

	return result;
}

double csvDataType::mean(ArrayType* input)
{
	int rows = getRows(input);
	int columns = getColumns(input);

	double x = 0;
	for (int c = 0; c < columns; c++)
	{
		for (int r = 0; r < rows; r++)
		{
			x += input->at(c).at(r);
		}
	}

	return x / rows / columns;
}

void csvDataType::addNumberSelf(ArrayType* input, double value)
{
	int cols = getColumns(input);
	int rows = getRows(input);

	for (int c = 0; c < cols; c++)
	{
		for (int r = 0; r < rows; r++)
		{
			input->at(c).at(r) += value;
		}
	}
}

void csvDataType::multiplyNumberSelf(ArrayType* input, double value)
{
	int cols = getColumns(input);
	int rows = getRows(input);

	for (int c = 0; c < cols; c++)
	{
		for (int r = 0; r < rows; r++)
		{
			input->at(c).at(r) *= value;
		}
	}
}

csvDataType::ArrayType* csvDataType::copy(ArrayType* input)
{
	int cols = getColumns(input);
	int rows = getRows(input);

	ArrayType* result = initialize(cols, rows);
	for (int c = 0; c < cols; c++)
	{
		for (int r = 0; r < rows; r++)
		{
			result->at(c).at(r) = input->at(c).at(r);
		}
	}
	return result;
}

int csvDataType::getRows(ArrayType* input)
{
	if (input->size() > 0)
	{
		return input->at(0).size();
	}

	DEBUG_LOG("There is no row inside this column!");

	return -1;
}

int csvDataType::getColumns(ArrayType* input)
{
	return input->size();
}

csvDataType::ArrayType* csvDataType::elementCopy(ArrayType* input)
{
	int cols = getColumns(input);
	int rows = getRows(input);
	ArrayType* result = initialize(cols,rows);
	
	for (int c = 0; c < cols; c++)
	{
		for (int r = 0; r < rows; r++)
		{
			result->at(c).at(r) = input->at(c).at(r);
		}
	}

	return result;
}

csvDataType::ArrayType* csvDataType::transpose(ArrayType* input)
{
	int amountCols = getColumns(input);
	int amountRows = getRows(input);

	ArrayType* result = initialize(amountRows, amountCols);
	
	for (int c = 0; c < amountCols; c++)
	{
		for (int r = 0; r < amountRows; r++)
		{
			result->at(r).at(c) = input->at(c).at(r);
		}
	}

	return result;
}

std::vector<double>* csvDataType::arrayTypeToVector(ArrayType* input)
{
	int amountCols = getColumns(input);
	int amountRows = getRows(input);

	std::vector<double>* result = new std::vector<double>(amountRows);

	if (amountCols <= 0 || amountCols > 1)
	{
		return nullptr;
	}

	for (int col1 = 0; col1 < amountCols; col1++)
	{
		for (int r1 = 0; r1 < amountRows; r1++)
		{
			result->at(r1) = input->at(col1).at(r1);
		}
	}

	return result;
}

void csvDataType::debugArrayType(ArrayType* input)
{
	int amountCols = getColumns(input);
	int amountRows = getRows(input);
	//DEBUG
	DEBUG_LOG("Columns: " + QString::number(amountCols));
	DEBUG_LOG("Rows: " + QString::number(amountRows));

	DEBUG_LOG("Matrix: ");
	for (int col1 = 0; col1 < amountCols; col1++)
	{
		DEBUG_LOG("Column " + QString::number(col1) + ":");
		for (int r1 = 0; r1 < input->at(col1).size(); r1++)
		{
			DEBUG_LOG("  Row " + QString::number(r1) + ": " + QString::number(input->at(col1).at(r1)));
		}
	}
}

