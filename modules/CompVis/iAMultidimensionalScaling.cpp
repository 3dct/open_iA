#include "iAMultidimensionalScaling.h"


#include "iAArcCosineDistance.h"
//Debug
#include "iAConsole.h"

#include <vector>

iAMultidimensionalScaling::iAMultidimensionalScaling(QList<csvFileData>* data) :
	m_inputData(data),
	m_amountOfElems(0),
	m_amountOfCharas(0)
{
	initializeMatrixUNormalized();

	normalizeMatrix();

	calculateProximityDistance();
}

void iAMultidimensionalScaling::initializeMatrixUNormalized()
{
	// initialize matrixUNormalized --> different size of different input datasets
	m_amountOfCharas = m_inputData->at(0).header->size();
	std::vector<double> vec(m_amountOfCharas);

	for (int k = 0; k < m_inputData->size(); k++)
	{
		m_amountOfElems = m_amountOfElems + m_inputData->at(k).values->size();
	}

	m_matrixUNormalized = new csvDataType::ArrayType(m_amountOfElems, vec);
}

void iAMultidimensionalScaling::normalizeMatrix()
{
	std::vector<double> maxValsForCols(m_amountOfCharas);

	//skip first column & row, since these are only label numbers
	for (int row = 1; row < m_amountOfCharas; row++)
	{
		maxValsForCols[row - 1] = -INFINITY;

		for (int ind = 0; ind < m_inputData->count(); ind++)
		{
			csvFileData currDataset = m_inputData->at(ind);

			//find max of each column for all datasets
			for (int col = 0; col < currDataset.values->size(); col++)
			{
				double curVal = currDataset.values->at(col).at(row);
				if (maxValsForCols[row - 1] < curVal)
				{
					maxValsForCols[row - 1] = curVal;
				}
			}
		}

		//DEBUG_LOG("MaxValue for row " + QString::number(row) + " : " + QString::number(maxValsForCols[row - 1]));

		int colU = 0;
		//normalize column according to maxVal
		for (int ind = 0; ind < m_inputData->count(); ind++)
		{
			csvFileData currDataset = m_inputData->at(ind);

			for (int col = 0; col < currDataset.values->size(); col++)
			{
				double curVal = currDataset.values->at(col).at(row);
				if (!(maxValsForCols[row - 1] == 0))
				{
					m_matrixUNormalized->at(colU).at(row - 1) = curVal / maxValsForCols[row - 1];
				}
				else
				{
					m_matrixUNormalized->at(colU).at(row - 1) = curVal;
				}
				//DEBUG_LOG(QString::number(m_matrixUNormalized->at(colU).at(row - 1)));
				colU += 1;
			}
		}

		//DEBUG_LOG(" ");
	}
}

void iAMultidimensionalScaling::calculateProximityDistance()
{
	std::vector<double>* weights = new std::vector<double>();
	//weights->reserve(14);
	for (int i = 0; i < 15; i++)
	{
		weights->push_back(0.0714);  //.at(i) = 0.0714;
	}

	//m_amountofCharas-1 - without the labels
	iAArcCosineDistance pd(weights, m_matrixUNormalized, m_amountOfCharas-1, m_amountOfElems);

	//TODO calculate mds with iterativ method
}