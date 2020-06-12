#pragma once

//CompVis
#include "iACsvDataStorage.h";

/*
The coefficient of variation (CV) is defined as the ratio of the standard deviation σ to the mean μ, c(v) = σ/μ.
It shows the extent of variability in relation to the mean of the population.
*/
class iACoefficientOfVariation
{
public:
	iACoefficientOfVariation(iACsvDataStorage* dataStorage);
	void calculateVariationCoefficient();

	//returns the latest result of the coefficient of variation calculation
	std::vector<double>* getCoefficientOfVariation();
	size_t getNumberofAttributes();

	//recalculate the coefficient of variation with the values inside input
	std::vector<double>* recalculateCoefficentOfVariation(QList<csvFileData>* input);

private:

	//store all values of each attribute in a vector
	void initializeAttributeArray(QList<csvFileData>* input, csvDataType::ArrayType* result);
	double calculateStandardDeviation(std::vector<double>* input, double mean);
	double calculateMean(std::vector<double>* input);

	//result of the coefficent of variation
	std::vector<double>* m_coeffOfVar;
	std::vector<double>* m_maxValForEachAttr;
	std::vector<double>* m_minValForEachAttr;

	//contains all values for each attribute
	csvDataType::ArrayType* m_attributeArray;

	//contains the initial dataset
	iACsvDataStorage* m_dataStorage;

	//holds the data for which the MDS will be calculated
	//list containing all csv-files
	//data = [[headerOfCSV1,valuesOfCSV1], [headerOfCSV2,valuesOfCSV2],...]
	//header = [name1,name2,...] --> Strings
	//values = [ [f1_val1,f1_val2,...], [f2_val1,f2_val2,...]]
	QList<csvFileData>* m_inputData;
};
