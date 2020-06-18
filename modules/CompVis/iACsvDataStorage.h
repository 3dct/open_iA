#pragma once

//Debug
#include "iAConsole.h"

#include <QString.h>
#include <qlistview.h>

#include <vector>



namespace csvDataType
{
	//using ValueType = double;
	//ArrayType always has the same length of rows for each column and vice versa
	//ArrayTpe corresponds to a mxn matrix!
	//ArrayType != BinType
	using ArrayType = std::vector<std::vector<double>>;

	//initialize with zeros
	ArrayType* initialize(int rows, int columns);
	//initialize with random numbers in range [0,1]
	ArrayType* initializeRandom(int rows, int columns);
	//calculate the mean of all values
	double mean(ArrayType* input);
	// add a number to the array in space
	void addNumberSelf(ArrayType* input, double value);
	//multiply a number to the array in space
	void multiplyNumberSelf(ArrayType* input, double value);
	//copy the array
	ArrayType* copy(ArrayType* input);
	//get the rows of the array
	int getRows(ArrayType* input);
	//get the rows of the array
	int getColumns(ArrayType* input);
	//copy all elements of input to a new array
	ArrayType* elementCopy(ArrayType* input);
	//debug array
	void debugArrayType(ArrayType* input);
	//transpose array
	ArrayType* transpose(ArrayType* input);
	//convert 1xm arraytype to vector
	std::vector<double>* arrayTypeToVector(ArrayType* input);
}

struct csvFileData
{
	QStringList* header;
	csvDataType::ArrayType* values;

	//returns the amount of objects of each single csv dataset
	static std::vector<int>* getAmountObjectsEveryDataset(QList<csvFileData>* data);
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
	QStringList* getDatasetNames();

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
