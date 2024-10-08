// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACorrelationCoefficient.h"

//vtk
#include <vtkCorrelativeStatistics.h>
#include <vtkDoubleArray.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkStringArray.h>
#include <vtkTable.h>

#include <QList>

#include <limits>

iACorrelationCoefficient::iACorrelationCoefficient(iACsvDataStorage* dataStorage):
	m_inputTable(nullptr),
	m_dataStorage(dataStorage),
	m_numberOfAttr(0),
	m_correlations(new std::map <QString, Correlation::CorrelationStore>()),
	m_correlationFilter(vtkSmartPointer<vtkCorrelativeStatistics>::New())
{
	vtkSmartPointer<vtkTable> data = toVtkTable(m_dataStorage->getData());
	m_correlations = calculate(data);
}

std::map<QString, Correlation::CorrelationStore>* iACorrelationCoefficient::calculate(vtkSmartPointer<vtkTable> data)
{
	std::map<QString, Correlation::CorrelationStore>* result = new std::map <QString, Correlation::CorrelationStore>();

	m_correlationFilter->SetInputData(vtkStatisticsAlgorithm::INPUT_DATA, data);

	Correlation::CorrelationStore corStore = Correlation::CorrelationStore();

	// Select all columns in inputData
	for (int c = 0; c < data->GetNumberOfColumns(); c++)
	{
		corStore.clear();

		for (int d = 0; d < data->GetNumberOfColumns(); d++)
		{
			if (c == d)
			{
				corStore.insert({ data->GetColumnName(d), std::numeric_limits<double>::infinity() });
				continue;
			}

			m_correlationFilter->AddColumnPair(data->GetColumnName(c), data->GetColumnName(d));

			m_correlationFilter->SetLearnOption(true);
			m_correlationFilter->SetDeriveOption(true);
			m_correlationFilter->SetAssessOption(false);
			m_correlationFilter->SetTestOption(false);
			m_correlationFilter->Update();

			vtkMultiBlockDataSet* outputMetaDS = vtkMultiBlockDataSet::SafeDownCast(m_correlationFilter->GetOutputDataObject(vtkStatisticsAlgorithm::OUTPUT_MODEL));
			vtkTable* derivedTable = vtkTable::SafeDownCast(outputMetaDS->GetBlock(1));

			corStore.insert({ data->GetColumnName(d), derivedTable->GetValue(0, derivedTable->GetNumberOfColumns() - 1).ToDouble() });

			m_correlationFilter->ResetAllColumnStates();
			m_correlationFilter->ResetRequests();
		}

		/*for (auto it = corStore.cbegin(); it != corStore.cend(); ++it)
		{
			LOG(lvlDebug,it->first + " = " + QString::number(it->second));
		}*/

		result->insert({ data->GetColumnName(c), corStore });
	}

	//DEBUG
	/*for (auto it1 = result->cbegin(); it1 != result->cend(); ++it1)
	{
		LOG(lvlDebug,"");
		LOG(lvlDebug,it1->first + " : ");
		Correlation::CorrelationStore aa = it1->second;
		for (auto it = aa.cbegin(); it != aa.cend(); ++it)
		{
			LOG(lvlDebug,it->first + " = " + QString::number(it->second));
		}
		LOG(lvlDebug,"");
		LOG(lvlDebug,"#############################################################");
	}*/

	return result;
}

vtkSmartPointer<vtkTable> iACorrelationCoefficient::toVtkTable(QList<csvFileData>* data)
{
	// data preparation
	QStringList attrNames = *m_dataStorage->getAttributeNamesWithoutLabel();
	m_numberOfAttr = static_cast<int>(attrNames.size()); //amount of attributes


	m_inputTable = vtkSmartPointer<vtkTable>::New();

	// Set the labels
	auto labels = vtkSmartPointer<vtkStringArray>::New();

	//set amount of attributes
	for (int i = 0; i < m_numberOfAttr; i++)
	{
		labels->InsertNextValue(attrNames.at(i).toStdString());

		auto arrIndex = vtkSmartPointer<vtkDoubleArray>::New();
		arrIndex->SetName(attrNames.at(i).toStdString().c_str());
		m_inputTable->AddColumn(arrIndex);
	}

	//calculate amount of objects(fibers)/rows
	int numberOfRows = 0;
	for (int i = 0; i < data->size(); i++)
	{
		numberOfRows += csvDataType::getRows(data->at(i).values);
	}
	m_inputTable->SetNumberOfRows(numberOfRows);

	int row = 0;
	//fill table with data
	for (int i = 0; i < ((int)data->size()); i++)
	{//for all datasets
		for (int dataInd = 0; dataInd < ((int)data->at(i).values->size()); dataInd++)
		{ //for all values
			for (int attrInd = 1; attrInd < m_numberOfAttr + 1; attrInd++)
			{//for all attributes but without the label attribute

				int col = attrInd - 1;
				double val = data->at(i).values->at(dataInd).at(attrInd);

				m_inputTable->SetValue(row, col, vtkVariant(val));
			}
			row++;
		}
	}

	return m_inputTable;
}

std::map<QString, Correlation::CorrelationStore>* iACorrelationCoefficient::getCorrelationCoefficients()
{
	return m_correlations;
}

std::map<QString, Correlation::CorrelationStore>* iACorrelationCoefficient::calculateCorrelationCoefficients(csvDataType::ArrayType* selectedData)
{
	QStringList attrNames = *m_dataStorage->getAttributeNamesWithoutLabel();

	auto labels = vtkSmartPointer<vtkStringArray>::New();
	auto table = vtkSmartPointer<vtkTable>::New();

	for (int i = 0; i < attrNames.size(); i++)
	{
		labels->InsertNextValue(attrNames.at(i).toStdString());

		auto arrIndex = vtkSmartPointer<vtkDoubleArray>::New();
		arrIndex->SetName(attrNames.at(i).toStdString().c_str());

		table->AddColumn(arrIndex);
	}

	//calculate amount of objects(fibers)/rows
	int numberOfRows = csvDataType::getColumns(selectedData);
	table->SetNumberOfRows(numberOfRows);

	for (int col = 0; col < table->GetNumberOfColumns(); col++)
	{
		for (int r = 0; r < table->GetNumberOfRows(); r++)
		{
			table->SetValue(r, col, selectedData->at(col+1).at(r));
		}
	}

	return calculate(table);
}
