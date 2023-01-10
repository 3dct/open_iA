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
#pragma once

//Debug
#include <iALog.h>

//QT
#include <QSharedPointer>
#include <QString>

#include <vector>

//vtk
#include "vtkSmartPointer.h"

class iACsvIO;
struct iACsvConfig;
class iACompHistogramTableData;
class dlg_CSVInput;
class vtkTable;

namespace csvDataType
{
	//using ValueType = double;
	//ArrayType always has the same length of rows for each column and vice versa
	//ArrayTpe corresponds to a mxn matrix!
	//ArrayType != BinType
	using ArrayType = std::vector<std::vector<double>>;

	//initialize with zeros
	void initialize(int rows, int columns, csvDataType::ArrayType* result);
	//initialize with random numbers in range [0,1]
	void initializeRandom(int rows, int columns, csvDataType::ArrayType* result);
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
	iACsvDataStorage(QStringList* csvFiles, int headerLineNumber);
	//read in the csv file
	QList<QStringList>* readCSV(QString csvFile);
	//store the csv file data in a csvFileData data structure
	void storeCSVToVectorArray(QList<QStringList>* list, int headerLineNumber);
	//returns the data of all csv files
	QList<csvFileData>* getData();

	//returns the names of the attributes without the label attribute
	QStringList* getAttributeNamesWithoutLabel();
	//returns the names of ALL attributes
	QStringList* getAttributeNames();

	QStringList* getDatasetNames();

	//returns the total number of objects in all datasets
	int getTotalNumberOfObjects();

	//get the minimum value of all distributions/csv files
	double getMinVal();
	//set the minimum value of all distributions/csv files
	void setMinVal(double minVal);
	//get the maximum value of all distributions/csv files
	double getMaxVal();
	//set the maximum value of all distributions/csv files
	void setMaxVal(double maxVal);

	/*** MDS Calculation ***/
	//get the 1d mds matrix result
	csvDataType::ArrayType* getMDSData();
	//set the 1d mds matrix result
	void setMDSData(csvDataType::ArrayType* mdsData);

	/*** 3D Rendering ***/
	std::vector<vtkSmartPointer<vtkTable>> const & getObjectTables();
	std::vector<QSharedPointer<QMap<uint, uint>>> const & getOutputMappings();
	std::vector<iACsvConfig> const & getCsvConfigs();

   private:
	//fill a list with the attribute names
	   void initializeHeader(QList<QStringList>* list, QStringList* headers, int headerLineNumber);
	//fill a vector array with all values
	void initializeValueArray(QList<QStringList>* list, int const attrCount, csvDataType::ArrayType* values);
	//resize the file to header & values without meta info in the first columns
	void customizeCSVFile(QList<QStringList>* list, int headerLineNumber);
	//creates a vtkTable storing the object that should be drawn and their mapping
	void initializeObjectTableFor3DRendering();

	//name of csvFiles
	QStringList* m_filenames;

	//list containing all csv-files
	//data = [[headerOfCSV1,valuesOfCSV1], [headerOfCSV2,valuesOfCSV2],...]
	//header = [name1,name2,...] --> Strings
	//values = [ [f1_val1,f1_val2,...], [f2_val1,f2_val2,...]]
	QList<csvFileData>* m_data;

	//stores the overall number of objects of all datasets
	int m_totalNumberOfObjects;

	//stores the resulting 1D matrix of the mds computation
	csvDataType::ArrayType* m_MDSData;

	/*** Initialization for Rendering with iAobjectvis***/
	std::vector<vtkSmartPointer<vtkTable>> m_objectTables;
	std::vector<QSharedPointer<QMap<uint, uint>>> m_outputMappings;
	std::vector<iACsvConfig> m_csvConfigs;

	//minimum value of all distributions/csv files
	double m_minVal;
	//maximum value of all distributions/csv file
	double m_maxVal;

};
