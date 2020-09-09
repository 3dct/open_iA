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

iAMultidimensionalScaling::~iAMultidimensionalScaling()
{
	m_inputData->clear();
	delete m_inputData;

	delete m_matrixUNormalized;
	delete m_matrixProximityDis;
	delete m_configuration;

	delete m_weights;
}

void iAMultidimensionalScaling::initializeWeights()
{
	m_weights = new std::vector<double>();
}

void iAMultidimensionalScaling::startMDS(std::vector<double>* weights)
{
	m_weights = weights;
	calculateProximityDistance();

	calculateMDS(1, 1); //1,100
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
	std::vector<double> vec(m_amountOfCharas - 1);

	for (int k = 0; k < m_inputData->size(); k++)
	{
		m_amountOfElems = m_amountOfElems + m_inputData->at(k).values->size();
	}

	m_matrixUNormalized = new csvDataType::ArrayType(m_amountOfElems, vec);
}

void iAMultidimensionalScaling::normalizeMatrix()
{
	std::vector<double> maxValsForCols(m_amountOfCharas - 1);

	//skip first column & row, since these are only label numbers	
	for (int col = 0; col < m_amountOfCharas - 1; col++)
	{
		maxValsForCols[col] = -INFINITY;

		for (int ind = 0; ind < m_inputData->count(); ind++)
		{
			csvFileData currDataset = m_inputData->at(ind);

			//find max of each column for all datasets
			for (int row = 0; row < currDataset.values->size(); row++)
			{
				double curVal = currDataset.values->at(row).at(col + 1);

				if (maxValsForCols[col] < curVal)
				{
					maxValsForCols[col] = curVal;
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
				double curVal = currDataset.values->at(row).at(col + 1);

				if (!(maxValsForCols[col] == 0))
				{
					m_matrixUNormalized->at(rowU).at(col) = curVal / maxValsForCols[col];
				}
				else
				{
					m_matrixUNormalized->at(rowU).at(col) = curVal;
				}
				rowU += 1;
			}
		}
	}

	//DEBUG
	/*DEBUG_LOG("");
	DEBUG_LOG("m_matrixUNormalized");
	csvDataType::debugArrayType(m_matrixUNormalized);
	DEBUG_LOG("");*/
}

void iAMultidimensionalScaling::calculateProximityDistance()
{
	if (m_activeProxM == ProximityMetric::ArcCosineDistance)
	{
		//m_amountofCharas-1 - without the labels
		iAArcCosineDistance pd(m_weights, m_matrixUNormalized, m_amountOfCharas - 1, m_amountOfElems);
		m_matrixProximityDis = pd.calculateProximityDistance();

		//DEBUG
		/*DEBUG_LOG("");
		DEBUG_LOG("m_matrixProximityDis");
		csvDataType::debugArrayType(m_matrixProximityDis);
		DEBUG_LOG("");*/
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

	////DEBUG
	/*DEBUG_LOG(" \n m_matrixProximityDis");
	csvDataType::debugArrayType(m_matrixProximityDis);*/

	//X - configuration of points in Euclidiean space
	//initialize X with one vector filled with random values between [0,1]
	csvDataType::ArrayType* X = new csvDataType::ArrayType();
	csvDataType::initializeRandom(amountRowsProxM, dim, X);
		
	////DEBUG
	/*DEBUG_LOG(" \n init X");
	csvDataType::debugArrayType(X);*/

	// mean value of distance matrix
	double meanD = csvDataType::mean(m_matrixProximityDis);

	// move to the center
	csvDataType::addNumberSelf(X, -0.5);

	// before this step, mean distance is 1/3*sqrt(d)
	csvDataType::multiplyNumberSelf(X, 0.1 * meanD / (1.0 / 3.0 * sqrt((double)dim)));

	////DEBUG
	/*DEBUG_LOG("X");
	csvDataType::debugArrayType(X);*/

	csvDataType::ArrayType* Z = csvDataType::copy(X);
	csvDataType::ArrayType* D_ = new csvDataType::ArrayType();
	csvDataType::initialize(amountRowsProxM, amountColsProxM, D_);

	csvDataType::ArrayType* B = new csvDataType::ArrayType();
	csvDataType::initialize(amountRowsProxM, amountColsProxM, B);

	/*DEBUG_LOG("\n init Z");
	csvDataType::debugArrayType(Z);*/

	//calculate euclidean distance
	iASimilarityDistance* d = initializeDistanceMetric();
	d->calculateSimilarityDistance(X, D_);

	////DEBUG
	/*DEBUG_LOG("\n D_");
	csvDataType::debugArrayType(D_);*/

	csvDataType::ArrayType oldX_ = *X;
	csvDataType::ArrayType* result = new csvDataType::ArrayType();
	csvDataType::initialize(csvDataType::getRows(&oldX_), csvDataType::getColumns(&oldX_), result);

	//MDS iteration
	for (int it = 0; it < iterations; it++)
	{
		/*DEBUG_LOG(QString("\n old B number %1").arg(it));
		csvDataType::debugArrayType(B);*/

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

		/*DEBUG_LOG(QString("\n middle B number %1").arg(it));
		csvDataType::debugArrayType(B);*/

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

		/*DEBUG_LOG(QString("\n new B number %1").arg(it));
		csvDataType::debugArrayType(B);

		DEBUG_LOG(QString("\n old Z number %1").arg(it));
		csvDataType::debugArrayType(Z);*/

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
		d->calculateSimilarityDistance(X, D_);

		/*vectorDiff(X, &oldX_, result);
		DEBUG_LOG(QString("\n X diff number %1 ").arg(it));
		csvDataType::debugArrayType(result);
		oldX_ = *X;

		//DEBUG_LOG(QString("\n new D number %1").arg(it));
		//csvDataType::debugArrayType(D_);	*/

		//Z = X;
		Z = csvDataType::elementCopy(X);

		/*DEBUG_LOG(QString("\n new X number %1").arg(it));
		csvDataType::debugArrayType(X);

		DEBUG_LOG(QString("\n new Z number %1").arg(it));
		csvDataType::debugArrayType(Z);*/

	}

	m_configuration = X;
	
	////DEBUG
	//DEBUG_LOG("\n m_configuration");
	//csvDataType::debugArrayType(m_configuration);
}

void iAMultidimensionalScaling::vectorDiff(csvDataType::ArrayType * a, csvDataType::ArrayType * b, csvDataType::ArrayType * result)
{
	for (int i = 0; i < csvDataType::getRows(a); i++)
	{
		for (int j = 0; j < csvDataType::getColumns(a); j++)
		{
			result->at(i).at(j) = abs(a->at(i).at(j)) - abs(b->at(i).at(j));
		}
	}
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