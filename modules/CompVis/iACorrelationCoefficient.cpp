#include "iACorrelationCoefficient.h"

//vtk
#include "vtkDoubleArray.h"
#include "vtkStringArray.h"

#include "vtkDataObject.h"

#include "vtkMultiBlockDataSet.h"

iACorrelationCoefficient::iACorrelationCoefficient(iACsvDataStorage* dataStorage):
	m_dataStorage(dataStorage),
	m_correlationFilter(vtkSmartPointer<vtkCorrelativeStatistics>::New()),
	m_correlations(new std::map <QString, Correlation::CorrelationStore>())
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
				corStore.insert({ data->GetColumnName(d), INFINITY });
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

		/*for (Correlation::CorrelationStore::iterator it = corStore.begin(); it != corStore.end(); ++it)
		{
			DEBUG_LOG(it->first + " = " + QString::number(it->second));
		}*/

		result->insert({ data->GetColumnName(c), corStore });
	}

	//DEBUG
	/*for (std::map <QString, Correlation::CorrelationStore>::iterator it1 = result->begin(); it1 != result->end(); ++it1)
	{
		DEBUG_LOG("");
		DEBUG_LOG(it1->first + " : ");
		Correlation::CorrelationStore aa = it1->second;
		for (Correlation::CorrelationStore::iterator it = aa.begin(); it != aa.end(); ++it)
		{
			DEBUG_LOG(it->first + " = " + QString::number(it->second));
		}
		DEBUG_LOG("");
		DEBUG_LOG("#############################################################");
	}*/

	return result;
}

vtkSmartPointer<vtkTable> iACorrelationCoefficient::toVtkTable(QList<csvFileData>* data)
{
	// data preparation
	m_numberOfAttr = data->at(0).header->size()-1; //amount of attributes
	QStringList attrNames = *m_dataStorage->getData()->at(0).header;
	attrNames.erase(attrNames.begin());//remove label attributes

	m_inputTable = vtkSmartPointer<vtkTable>::New();

	// Set the labels
	vtkSmartPointer<vtkStringArray> labels = vtkSmartPointer<vtkStringArray>::New();

	//set amount of attributes
	for (int i = 0; i < m_numberOfAttr; i++)
	{
		labels->InsertNextValue(attrNames.at(i).toStdString());

		vtkSmartPointer<vtkDoubleArray> arrIndex = vtkSmartPointer<vtkDoubleArray>::New();
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
	for (int i = 0; i < data->size(); i++)
	{//for all datasets
		for (int dataInd = 0; dataInd < data->at(i).values->size(); dataInd++)
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
	QStringList attrNames = *m_dataStorage->getData()->at(0).header;

	vtkSmartPointer<vtkStringArray> labels = vtkSmartPointer<vtkStringArray>::New();
	vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();

	for (int i = 0; i < attrNames.size(); i++)
	{
		labels->InsertNextValue(attrNames.at(i).toStdString());

		vtkSmartPointer<vtkDoubleArray> arrIndex = vtkSmartPointer<vtkDoubleArray>::New();
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