// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "vtkCorrelativeStatistics.h"
#include "vtkMultiCorrelativeStatistics.h"
#include "vtkSmartPointer.h"
#include "vtkTable.h"

#include "iACsvDataStorage.h"

#include <map>

namespace Correlation
{
	using CorrelationStore = std::map<QString, double>;
}

class iACorrelationCoefficient
{
public:

	iACorrelationCoefficient(iACsvDataStorage* dataStorage);

	//get correlation coefficients
	std::map<QString, Correlation::CorrelationStore>* getCorrelationCoefficients();

	std::map<QString, Correlation::CorrelationStore>* calculateCorrelationCoefficients(csvDataType::ArrayType* selectedData);

private:

	std::map<QString, Correlation::CorrelationStore>* calculate(vtkSmartPointer<vtkTable> data);

	vtkSmartPointer<vtkTable> toVtkTable(QList<csvFileData>* data);

	

	//stores the table with all object for all datasets
	vtkSmartPointer<vtkTable> m_inputTable;

	//data storage
	iACsvDataStorage* m_dataStorage;

	//stores amount of attributes (length,...)
	int m_numberOfAttr;

	//stores all correlation coefficients
	//for each column/attribute name a map of corresponding other column names is stored and its resulting correlation coefficient
	// Attr1 = < <Attr2, 0.9>, <Attr3, 0.1>, .... >
	std::map<QString, Correlation::CorrelationStore>* m_correlations;

	vtkSmartPointer<vtkCorrelativeStatistics> m_correlationFilter;

};
