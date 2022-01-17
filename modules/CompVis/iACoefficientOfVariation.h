/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

//CompVis
#include "iACsvDataStorage.h"
#include "iACompHistogramTableData.h"

/*
The coefficient of variation (CV) is defined as the ratio of the standard deviation σ to the mean μ, c(v) = σ/μ.
It shows the extent of variability in relation to the mean of the population.
*/
class iACoefficientOfVariation
{
public:
	iACoefficientOfVariation(iACsvDataStorage* dataStorage);

	//calculates the coefficient of variation
	std::vector<double>* calculateVariationCoefficient(csvDataType::ArrayType* arrayOfAttributes);

	//returns the coefficient of variation for all values of all datasets
	std::vector<double>* getCoefficientOfVariation();

	//recalculate the coefficient of variation with the values inside input
	std::vector<double>* recalculateCoefficentOfVariation(csvDataType::ArrayType* selectedData);

private:

	//store all values of each attribute in a vector
	void initializeAttributeArray(QList<csvFileData>* input, csvDataType::ArrayType* result);
	//store all values of each attribute in a vector
	void initializeAttributeArray(csvDataType::ArrayType* selectedData);

	double calculateStandardDeviation(std::vector<double>* input, double mean);
	double calculateMean(std::vector<double>* input);

	//result of the coefficent of variation for all values of all datasets
	//stored after the first initialization to save calculation time
	std::vector<double>* m_coeffOfVar;
	std::vector<double>* m_maxValForEachAttr;
	std::vector<double>* m_minValForEachAttr;

	//contains all values for each attribute
	csvDataType::ArrayType* m_attributeArray;

	//holds the data for which the MDS will be calculated
	//list containing all csv-files
	//data = [[headerOfCSV1,valuesOfCSV1], [headerOfCSV2,valuesOfCSV2],...]
	//header = [name1,name2,...] --> Strings
	//values = [ [f1_val1,f1_val2,...], [f2_val1,f2_val2,...]]
	QList<csvFileData>* m_inputData;
};
