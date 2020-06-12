#include "iACoefficientOfVariation.h"

//DEBUG
#include "iAConsole.h"

iACoefficientOfVariation::iACoefficientOfVariation(iACsvDataStorage* dataStorage):
	m_dataStorage(dataStorage), 
	m_inputData(dataStorage->getData()),
	m_attributeArray(new  csvDataType::ArrayType()),
	m_coeffOfVar(new std::vector<double>()),
	m_maxValForEachAttr(new std::vector<double>()),
	m_minValForEachAttr(new std::vector<double>())
{
	initializeAttributeArray(m_inputData, m_attributeArray);
	calculateVariationCoefficient();	
}

std::vector<double>* iACoefficientOfVariation::recalculateCoefficentOfVariation(QList<csvFileData>* input)
{
	initializeAttributeArray(input, m_attributeArray);
	calculateVariationCoefficient();
	return getCoefficientOfVariation();
}

void iACoefficientOfVariation::initializeAttributeArray(QList<csvFileData>* input, csvDataType::ArrayType* result)
{
	int amountDatasets = input->size();
	//is for all datasets & objects the same
	int amountAttributes = input->at(0).header->size();

	for (int attrInd = 0; attrInd < amountAttributes; attrInd++)
	{
		std::vector<double> attr = std::vector<double>();

		for (int datasetInd = 0; datasetInd < amountDatasets; datasetInd++)
		{ //for all datasets
			int amountObjects = input->at(datasetInd).values->size();

			for (int objInd = 0; objInd < amountObjects; objInd++)
			{ //for all objects in each dataset
				csvDataType::ArrayType* vals = input->at(datasetInd).values;
				attr.push_back(vals->at(objInd).at(attrInd));
			}
		}

		result->push_back(attr);
	}
}

void iACoefficientOfVariation::calculateVariationCoefficient()
{	
	for (int attrInd = 0; attrInd < m_attributeArray->size(); attrInd++) 
	{
		std::vector<double>* currAttr = &(m_attributeArray->at(attrInd));
		auto result = std::minmax_element(currAttr->begin(), currAttr->end());

		m_maxValForEachAttr->push_back(*result.second);
		m_minValForEachAttr->push_back(*result.first);

		double mean = calculateMean(currAttr);
		double standardDev = calculateStandardDeviation(currAttr, mean);
		
		double variationCoeff = (standardDev / mean);
		double variationCoeffEmpirical;

		if(mean == 0)
		{
			variationCoeffEmpirical = 0;
		}else
		{
			variationCoeffEmpirical = variationCoeff / std::sqrt(m_attributeArray->at(attrInd).size());
		}

		m_coeffOfVar->push_back(variationCoeffEmpirical);
	}
}

double iACoefficientOfVariation::calculateStandardDeviation(std::vector<double>* input, double mean)
{
	double sum = 0;
	double n = input->size();
	for (int i = 0; i < n; i++)
	{
		sum = sum + std::pow((input->at(i) - mean), 2);
	}

	return std::sqrt((1 / (n - 1))*sum);
}

double iACoefficientOfVariation::calculateMean(std::vector<double>* input)
{
	double sum = 0;
	for (int i = 0; i < input->size(); i++)
	{
		sum = sum + input->at(i);
	}
	return (sum / ((double)input->size()));
}

std::vector<double>* iACoefficientOfVariation::getCoefficientOfVariation()
{
	return m_coeffOfVar;
}

size_t iACoefficientOfVariation::getNumberofAttributes()
{
	return m_attributeArray->size();
}