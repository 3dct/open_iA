#pragma once

#include <QString.h>
#include <qlistview.h>

#include <vector>

namespace csvDataType
{
	using ValueType = double;
	using ArrayType = std::vector<std::vector<ValueType>>;
}

struct csvFileData
{
	QStringList* header;
	csvDataType::ArrayType* values;
};

class iACsvDataStorage
{
   public:
	iACsvDataStorage(QStringList* csvFiles, QListView* listView);
	//read in the csv file
	QList<QStringList>* readCSV(QString csvFile);
	//store the csv file data in a csvFileData data structure
	void storeCSVToVectorArray(QList<QStringList>* list);
	//returns the data of all csv files
	QList<csvFileData>* getData();

   private:
	//fill a list with the attribute names
	void initializeHeader(QList<QStringList>* list, QStringList* headers);
	//fill a vector array with all values
	void initializeValueArray(QList<QStringList>* list, int const attrCount, csvDataType::ArrayType* values);
	//resize the file to header & values without meta info in the first columns
	void customizeCSVFile(QList<QStringList>* list);

	//name of csvFiles
	QStringList* m_filenames;

	//list containing all csv-files
	//data = [[headerOfCSV1,valuesOfCSV1], [headerOfCSV2,valuesOfCSV2],...]
	//header = [name1,name2,...] --> Strings
	//values = [ [f1_val1,f1_val2,...], [f2_val1,f2_val2,...]]	
	QList<csvFileData>* m_data;
};
