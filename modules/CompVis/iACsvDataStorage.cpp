#include "iACsvDataStorage.h"

//Debug
#include "iAConsole.h"

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
	}
	*/
}

void iACsvDataStorage::customizeCSVFile(QList<QStringList>* list)
{
	//only for fiber CSVs
	for (int i = 0; i < 4; i++)
	{
		list->removeAt(0);
	}
}

QList<csvFileData>* iACsvDataStorage::getData()
{
	return m_data;
}