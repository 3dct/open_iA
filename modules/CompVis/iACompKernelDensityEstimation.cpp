#include "iACompKernelDensityEstimation.h"

#include "iACompUniformBinningData.h"
#include "iACompBayesianBlocksData.h"
#include "iACompNaturalBreaksData.h"

#include <vector>

iACompKernelDensityEstimation::iACompKernelDensityEstimation(
	iACsvDataStorage* dataStorage, bin::BinType* datasets) :
	m_datasets(datasets), 
	m_dataStorage(dataStorage), 
	m_kdeData(nullptr), 
	m_maxKDE(-INFINITY), 
	m_minKDE(INFINITY)
{
	numSteps = 1000;  //(*std::minmax_element(amountObjectsEveryDataset->begin(), amountObjectsEveryDataset->end()).second); //
}

void iACompKernelDensityEstimation::setDataStructure(iACompKernelDensityEstimationData* datastructure)
{
	m_kdeData = datastructure;
}

void iACompKernelDensityEstimation::calculateCurve(
	iACompUniformBinningData* uData, iACompBayesianBlocksData* bbData, iACompNaturalBreaksData* nbData)
{
	//resulting kde-data
	QList<kdeData::kdeBins>* kdeDataUniform = new QList<kdeData::kdeBins>();
	QList<kdeData::kdeBins>* kdeDataNB = new QList<kdeData::kdeBins>();
	QList<kdeData::kdeBins>* kdeDataBB = new QList<kdeData::kdeBins>();

	//get binned data
	QList<bin::BinType*>* uDataStore = uData->getBinData();
	QList<bin::BinType*>* bbDataStore = bbData->getBinData();
	QList<bin::BinType*>* nbDataStore = nbData->getBinData();

	//data preparation
	for (size_t dataID = 0; dataID < m_datasets->size(); dataID++)
	{
		kdeData::kdeBin* result = new kdeData::kdeBin();
		calculateKDE(&m_datasets->at(dataID), result);

		kdeData::kdeBins* kdeUniform = kdeData::initializeBins(static_cast<int>(uDataStore->at(static_cast<int>(dataID))->size()));
		std::vector<double> binBoundariesUB = uData->getBinBoundaries()->at(static_cast<int>(dataID));
		calculateKDEBinning(result, uData->getMaxVal(), &binBoundariesUB, kdeUniform);

		kdeData::kdeBins* kdeNB = kdeData::initializeBins(static_cast<int>(nbDataStore->at(static_cast<int>(dataID))->size()));
		std::vector<double> binBoundariesNB = nbData->getBinBoundaries()->at(static_cast<int>(dataID));
		calculateKDEBinning(result, nbData->getMaxVal(), &binBoundariesNB, kdeNB);

		kdeData::kdeBins* kdeBB = kdeData::initializeBins(static_cast<int>(bbDataStore->at(static_cast<int>(dataID))->size()));
		std::vector<double> binBoundariesBB = bbData->getBinBoundaries()->at(static_cast<int>(dataID));
		calculateKDEBinning(result, bbData->getMaxVal(), &binBoundariesBB, kdeBB);

		kdeDataUniform->append(*kdeUniform);
		kdeDataNB->append(*kdeNB);
		kdeDataBB->append(*kdeBB);
	}

	m_kdeData->setKDEDataUniform(kdeDataUniform);
	m_kdeData->setKDEDataNB(kdeDataNB);
	m_kdeData->setKDEDataBB(kdeDataBB);
}

void iACompKernelDensityEstimation::calculateCurveUB(iACompUniformBinningData* uData)
{
	//resulting kde-data
	QList<kdeData::kdeBins>* kdeDataUniform = new QList<kdeData::kdeBins>();

	//get binned data
	QList<bin::BinType*>* uDataStore = uData->getBinData();

	for (size_t dataID = 0; dataID < m_datasets->size(); dataID++)
	{
		kdeData::kdeBin* result = new kdeData::kdeBin();
		calculateKDE(&m_datasets->at(dataID), result);

		kdeData::kdeBins* kdeUniform = kdeData::initializeBins(static_cast<int>(uDataStore->at(static_cast<int>(dataID))->size()));
		std::vector<double> binBoundariesUB = uData->getBinBoundaries()->at(static_cast<int>(dataID));
		calculateKDEBinning(result, uData->getMaxVal(), &binBoundariesUB, kdeUniform);

		kdeDataUniform->append(*kdeUniform);
	}

	m_kdeData->setKDEDataUniform(kdeDataUniform);
}

void iACompKernelDensityEstimation::calculateKDE(std::vector<double>* dataIn, kdeData::kdeBin* results)
{
	realMatrixType samples(dataIn->size(), 1);

	for (size_t i = 0; i < dataIn->size(); ++i)
	{
		samples(i) = dataIn->at(i);
	}

	// create a plottable data
	auto result = std::minmax_element(dataIn->begin(), dataIn->end());
	double xMax = *result.second;
	double xMin = *result.first;

	const realScalarType dx = (xMax - xMin) / (realScalarType)numSteps;

	//build kde
	kdeType kde(samples);

	for (indexType i = 0; i < numSteps; ++i)
	{
		realVectorType samp(1);
		realScalarType xi = xMin + i * dx;
		samp(0) = xi; 
		
		double kdeValue = kde.computePDF(samp);

		kdeData::kdePair pair = {xi, kdeValue};
		results->push_back(pair);

		if (kdeValue >= m_maxKDE)
		{
			m_maxKDE = kdeValue;		
		}

		if (kdeValue < m_minKDE)
		{
			m_minKDE = kdeValue;
		}
	}

	m_kdeData->setMaxKDEVal(m_maxKDE);
	m_kdeData->setMinKDEVal(m_minKDE);
}

void iACompKernelDensityEstimation::calculateKDEBinning(
	kdeData::kdeBin* input, double maxMDSVal, std::vector<double>* binBoundaries, kdeData::kdeBins* result)
{
	for (int pairId = 0; pairId < static_cast<int>(input->size()); pairId++)
	{
		kdeData::kdePair pair = input->at(pairId); 
		double mdsVal = pair[0];
		
		for (int bin = 0; bin < static_cast<int>(binBoundaries->size()); bin++)
		{  //look for the boundaries of each bin

			double lowerBorder;
			double upperBorder;

			if (bin < (static_cast<int>(binBoundaries->size()) - 1))
			{
				lowerBorder = binBoundaries->at(bin);
				upperBorder = binBoundaries->at(bin + 1);
			}
			else
			{
				lowerBorder = binBoundaries->at(bin);
				upperBorder = maxMDSVal;
			}

			bool greaterLowerBorder = false;
			bool smallerUpperBorder = false;
			
			if (mdsVal >= lowerBorder) greaterLowerBorder = true;
			if (mdsVal < upperBorder) smallerUpperBorder = true;

			if (greaterLowerBorder && smallerUpperBorder)
			{
				/*LOG(lvlDebug, "");
				LOG(lvlDebug, "bin" + QString::number(bin));
				LOG(lvlDebug, "lowerBorder" + QString::number(lowerBorder));
				LOG(lvlDebug, "upperBorder" + QString::number(upperBorder));
				LOG(lvlDebug, "pair:" + QString::number(pair[0]) + ", " + QString::number(pair[1]));*/

				result->at(bin).push_back(pair);
				break;
			}

			if ((bin == ((static_cast<int>(binBoundaries->size()) - 1))) && mdsVal == upperBorder)
			{
				result->at(bin).push_back(pair);
				break;
			}
		}

	}
	
	//Debug
	//kdeData::debug(result);
}