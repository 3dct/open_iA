#include "iAMultidimensionalScaling.h"

//CompVis
#include "iAArcCosineDistance.h"
#include "iASimilarityDistance.h"
#include "iAMinkowskiDistance.h"
#include "iAEuclideanDistance.h"

//Debug
#include "iAConsole.h"

#include <algorithm>
#include <iostream>
#include <vector>

iAMultidimensionalScaling::iAMultidimensionalScaling(QList<csvFileData>* data) :
	m_inputData(data),
	m_amountOfElems(0),
	m_amountOfCharas(0)
{
	initializeWeights();
	initializeMatrixUNormalized();
	normalizeMatrix();
}

void iAMultidimensionalScaling::initializeWeights()
{
	m_weights = new std::vector<double>();
}

void iAMultidimensionalScaling::startMDS(std::vector<double>* weights)
{
	m_weights = weights;
	calculateProximityDistance();
	calculateMDS(1, 1);
}

void iAMultidimensionalScaling::setProximityMetric(ProximityMetric proxiName)
{
	m_activeProxM = proxiName;
}

void iAMultidimensionalScaling::setDistanceMetric(DistanceMetric disName)
{
	m_activeDisM = disName;
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
	
	//DEBUG
	//DEBUG_LOG("");
	//DEBUG_LOG("m_matrixUNormalized");
	//csvDataType::debugArrayType(m_matrixUNormalized);
	//DEBUG_LOG("");
}

void iAMultidimensionalScaling::normalizeMatrix()
{
	std::vector<double> maxValsForCols(m_amountOfCharas);

	//skip first column & row, since these are only label numbers	
	for (int col = 1; col < m_amountOfCharas; col++)
	{
		maxValsForCols[col - 1] = -INFINITY;

		for (int ind = 0; ind < m_inputData->count(); ind++)
		{
			csvFileData currDataset = m_inputData->at(ind);

			//find max of each column for all datasets
			for (int row = 0; row < currDataset.values->size(); row++)
			{
				double curVal = currDataset.values->at(row).at(col);
				if (maxValsForCols[col - 1] < curVal)
				{
					maxValsForCols[col - 1] = curVal;
				}
			}
		}

		//normalize column according to maxVal
		int rowU = 0;
		for (int ind = 0; ind < m_inputData->count(); ind++)
		{
			csvFileData currDataset = m_inputData->at(ind);

			for (int row = 0; row < currDataset.values->size(); row++)
			{
				double curVal = currDataset.values->at(row).at(col);
				if (!(maxValsForCols[col - 1] == 0))
				{
					m_matrixUNormalized->at(rowU).at(col - 1) = curVal / maxValsForCols[col - 1];
				}
				else
				{
					m_matrixUNormalized->at(rowU).at(col - 1) = curVal;
				}
				rowU += 1;
			}
		}
	}
}

void iAMultidimensionalScaling::calculateProximityDistance()
{
	if (m_activeProxM == ProximityMetric::ArcCosineDistance)
	{
		//m_amountofCharas-1 - without the labels
		iAArcCosineDistance pd(m_weights, m_matrixUNormalized, m_amountOfCharas - 1, m_amountOfElems);
		m_matrixProximityDis = pd.calculateProximityDistance();

		//DEBUG
		//DEBUG_LOG("");
		//DEBUG_LOG("m_matrixProximityDis");
		//csvDataType::debugArrayType(m_matrixProximityDis);
		//DEBUG_LOG("");
	}
}

iASimilarityDistance* iAMultidimensionalScaling::initializeDistanceMetric()
{
	iASimilarityDistance* d;
	if (m_activeDisM == DistanceMetric::EuclideanDistance)
	{
		d = new iAEuclideanDistance();
	}
	else if (m_activeDisM == DistanceMetric::MinkowskiDistance)
	{
		d = new iAMinkowskiDistance();
	}

	return d;
}

void iAMultidimensionalScaling::calculateMDS(int dim, int iterations)
{
	int amountColsProxM = csvDataType::getColumns(m_matrixProximityDis);
	int amountRowsProxM = csvDataType::getRows(m_matrixProximityDis);
	
	//X - configuration of points in Euclidiean space
	//initialize X with one vector filled with random values between [0,1]
	csvDataType::ArrayType* X = csvDataType::initializeRandom(amountRowsProxM, dim);
	csvDataType::debugArrayType(X);
	// mean value of distance matrix
	double meanD = csvDataType::mean(m_matrixProximityDis);
	// move to the center
	csvDataType::addNumberSelf(X, -0.5);
	// before this step, mean distance is 1/3*sqrt(d)
	csvDataType::multiplyNumberSelf(X, 0.1 * meanD / (1.0 / 3.0 * sqrt((double)dim)));

	//DEBUG
	/*DEBUG_LOG("");
	DEBUG_LOG("X:");
	csvDataType::debugArrayType(X);
	DEBUG_LOG("");
	*/

	csvDataType::ArrayType* Z = csvDataType::copy(X);
	csvDataType::ArrayType* D_ = csvDataType::initialize(amountRowsProxM, amountColsProxM);
	csvDataType::ArrayType* B = csvDataType::initialize(amountRowsProxM, amountColsProxM);

	//calculate euclidean distance
	iASimilarityDistance* d = initializeDistanceMetric();
	D_ = d->calculateSimilarityDistance(X, D_);
	
	//DEBUG_LOG("");
	//DEBUG_LOG("D_");
	//csvDataType::debugArrayType(D_);
	//DEBUG_LOG("");

	//MDS iteration
	for (int it = 0; it < iterations; it++)
	{
		// B = calc_B(D_,D);
		for (int r = 0; r < amountRowsProxM; r++)	
		{
			for (int c = 0; c < amountColsProxM; c++)
			{
				if ( r == c || std::fabs(D_->at(r).at(c)) < Epsilon)
				{
					B->at(r).at(c) = 0.0;
				}
				else
				{
					B->at(r).at(c) = -m_matrixProximityDis->at(r).at(c) / D_->at(r).at(c);
				}
			}
		}

		double temp;
		for (int c = 0; c < amountColsProxM; c++)
		{
			temp = 0;
			for (int r = 0; r < amountRowsProxM; r++)
			{
				temp += B->at(r).at(c);
			}

			B->at(c).at(c) = -temp;
		}

		// X = B*Z/size(D,1);
		for (int r = 0; r < csvDataType::getRows(X); r++)	
		{
			for (int xCols = 0; xCols < csvDataType::getColumns(X); xCols++)
			{
				temp = 0;
				for (int bCols = 0; bCols < csvDataType::getColumns(B); bCols++)
				{
					temp += (B->at(r).at(bCols) * Z->at(bCols).at(xCols));
				}
				X->at(r).at(xCols) = temp / (double)amountRowsProxM;
			}
		}

		//D_ = calc_D (X);
		D_ = d->calculateSimilarityDistance(X, D_);

		//Z = X;
		Z = csvDataType::elementCopy(X);
	}

	m_configuration = X;
	
	//DEBUG
	//DEBUG_LOG("");
	//DEBUG_LOG("m_configuration");
	//csvDataType::debugArrayType(m_configuration);
	//DEBUG_LOG("");
}

std::vector<double>* iAMultidimensionalScaling::getWeights()
{
	return m_weights;
}

csvDataType::ArrayType* iAMultidimensionalScaling::getResultMatrix()
{
	return m_configuration;
}

QList<csvFileData>* iAMultidimensionalScaling::getCSVFileData()
{
	return m_inputData;
}