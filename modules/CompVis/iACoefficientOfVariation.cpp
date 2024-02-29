// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACoefficientOfVariation.h"

#include "iACompHistogramTableData.h"

iACoefficientOfVariation::iACoefficientOfVariation(iACsvDataStorage* dataStorage):
	m_coeffOfVar(new std::vector<double>()),
	m_maxValForEachAttr(new std::vector<double>()),
	m_minValForEachAttr(new std::vector<double>()),
	m_attributeArray(new csvDataType::ArrayType()),
	m_inputData(dataStorage->getData())

{
	initializeAttributeArray(m_inputData, m_attributeArray);
	m_coeffOfVar = calculateVariationCoefficient(m_attributeArray);
}

std::vector<double>* iACoefficientOfVariation::recalculateCoefficentOfVariation(csvDataType::ArrayType* selectedData)
{
	return calculateVariationCoefficient(selectedData);;
}

void iACoefficientOfVariation::initializeAttributeArray(csvDataType::ArrayType* selectedData)
{
	m_attributeArray = selectedData;
}

void iACoefficientOfVariation::initializeAttributeArray(QList<csvFileData>* input, csvDataType::ArrayType* result)
{
	if (!result->empty())
	{
		return;
	}

	auto amountDatasets = input->size();
	//is for all datasets & objects the same
	int amountAttributes = input->at(0).header->size();

	for (int attrInd = 0; attrInd < amountAttributes; attrInd++)
	{
		std::vector<double> attr = std::vector<double>();

		for (qsizetype datasetInd = 0; datasetInd < amountDatasets; datasetInd++)
		{ //for all datasets
			auto amountObjects = input->at(datasetInd).values->size();

			for (size_t objInd = 0; objInd < amountObjects; objInd++)
			{ //for all objects in each dataset
				csvDataType::ArrayType* vals = input->at(datasetInd).values;
				attr.push_back(vals->at(objInd).at(attrInd));
			}
		}

		result->push_back(attr);
	}
}

std::vector<double>* iACoefficientOfVariation::calculateVariationCoefficient(csvDataType::ArrayType* arrayOfAttributes)
{
	std::vector<double>* resultCoeff = new std::vector<double>();

	for (int attrInd = 0; attrInd < ((int)arrayOfAttributes->size()); attrInd++)
	{
		double variationCoeffEmpirical;

		if (arrayOfAttributes->at(attrInd).size() == 0)
		{//there are no values
			variationCoeffEmpirical = 0.0;

		}else if(arrayOfAttributes->at(attrInd).size() == 1)
		{//there is only one value
			variationCoeffEmpirical = 1.0;
		}
		else
		{//real calculation with more than 1 value
			std::vector<double>* currAttr = &(arrayOfAttributes->at(attrInd));
			auto result = std::minmax_element(currAttr->begin(), currAttr->end());

			m_maxValForEachAttr->push_back(*result.second);
			m_minValForEachAttr->push_back(*result.first);

			double mean = calculateMean(currAttr);
			double standardDev = calculateStandardDeviation(currAttr, mean);
			double variationCoeff = (standardDev / mean);

			if (mean == 0.0)
			{
				variationCoeffEmpirical = 0.0;
			}
			else
			{
				variationCoeffEmpirical = variationCoeff / std::sqrt(arrayOfAttributes->at(attrInd).size());
			}
		}

		resultCoeff->push_back(variationCoeffEmpirical);
	}

	return resultCoeff;
}

double iACoefficientOfVariation::calculateStandardDeviation(std::vector<double>* input, double mean)
{
	double sum = 0.0;
	double n = input->size();
	for (int i = 0; i < ((int)n); i++)
	{
		sum = sum + std::pow((input->at(i) - mean), 2);
	}

	return std::sqrt((1 / (n - 1))*sum);
}

double iACoefficientOfVariation::calculateMean(std::vector<double>* input)
{
	double sum = 0;
	for (int i = 0; i < ((int)input->size()); i++)
	{
		sum = sum + input->at(i);
	}
	return (sum / ((double)input->size()));
}

std::vector<double>* iACoefficientOfVariation::getCoefficientOfVariation()
{
	return m_coeffOfVar;
}
