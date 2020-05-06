#pragma once

#include "iACsvDataStorage.h"

#include "qlist.h"

//class iACsvDataStorage;

class iAMultidimensionalScaling
{
   public:
	iAMultidimensionalScaling(QList<csvFileData>* data);

   private:
	   //initialize matrixUNormalized according to the amount of objects in all csvs and all characteritics
	   void initializeMatrixUNormalized();
	   
	   //1.normalize the quantitative data matrix
	   void normalizeMatrix();

	   //2.calculate proximity measure
	   void calculateProximityDistance();

	   //holds the data for which the MDS will be calculated
	   //list containing all csv-files
	   //data = [[headerOfCSV1,valuesOfCSV1], [headerOfCSV2,valuesOfCSV2],...]
	   //header = [name1,name2,...] --> Strings
	   //values = [ [f1_val1,f1_val2,...], [f2_val1,f2_val2,...]]	
	   QList<csvFileData>* m_inputData;
	   
	   //amount of objects (features) inside one matrix
	   int m_amountOfElems;
	   //amount of characteristics of an object
	   //must be the same for all csv files!
	   int m_amountOfCharas;
	   //normalized matrix conntaining all objects with all their characeristics
	   csvDataType::ArrayType* m_matrixUNormalized;
};