#include "iACompKernelDensityEstimationData.h"

iACompKernelDensityEstimationData::iACompKernelDensityEstimationData() : 
	m_maxVal(-1),
	m_minVal(-1), 
	m_maxKDEVal(-1),
	m_minKDEVal(-1),
	amountObjectsEveryDataset(new std::vector<int>()), 
	kdeDataUniform(nullptr),
	kdeDataNB(nullptr),
	kdeDataBB(nullptr),
	m_objectsPerBinUB(nullptr),
	m_objectsPerBinNB(nullptr),
	m_objectsPerBinBB(nullptr),
	m_maxAmountInAllBins(0)
{
}

double iACompKernelDensityEstimationData::getMaxVal()
{
	return m_maxVal;
}

double iACompKernelDensityEstimationData::getMinVal()
{
	return m_minVal;
}

QList<kdeData::kdeBins>* iACompKernelDensityEstimationData::getKDEDataUniform()
{
	return kdeDataUniform;
}

QList<kdeData::kdeBins>* iACompKernelDensityEstimationData::getKDEDataNB()
{
	return kdeDataNB;
}

QList<kdeData::kdeBins>* iACompKernelDensityEstimationData::getKDEDataBB()
{
	return kdeDataBB;
}

std::vector<int>* iACompKernelDensityEstimationData::getAmountObjectsEveryDataset()
{
	return amountObjectsEveryDataset;
}



void iACompKernelDensityEstimationData::setMaxVal(double newMax)
{
	m_maxVal = newMax;
}

void iACompKernelDensityEstimationData::setMinVal(double newMin)
{
	m_minVal = newMin;
}

void iACompKernelDensityEstimationData::setKDEDataUniform(QList<kdeData::kdeBins>* newKDEData)
{
	kdeDataUniform = newKDEData;
}

void iACompKernelDensityEstimationData::setKDEDataNB(QList<kdeData::kdeBins>* newKDEData)
{
	kdeDataNB = newKDEData;
}

void iACompKernelDensityEstimationData::setKDEDataBB(QList<kdeData::kdeBins>* newKDEData)
{
	kdeDataBB = newKDEData;
}

void iACompKernelDensityEstimationData::setAmountObjectsEveryDataset(std::vector<int>* newAmountObjectsEveryDataset)
{
	amountObjectsEveryDataset = newAmountObjectsEveryDataset;
}

int iACompKernelDensityEstimationData::getMaxAmountInAllBins()
{
	return m_maxAmountInAllBins;
}
void iACompKernelDensityEstimationData::setMaxAmountInAllBins(int newMaxAmountInAllBins)
{
	m_maxAmountInAllBins = newMaxAmountInAllBins;
}


double iACompKernelDensityEstimationData::getMaxKDEVal()
{
	return m_maxKDEVal;
}

double iACompKernelDensityEstimationData::getMinKDEVal()
{
	return m_minKDEVal;
}

void iACompKernelDensityEstimationData::setMaxKDEVal(double maxKDE)
{
	m_maxKDEVal = maxKDE;
}

void iACompKernelDensityEstimationData::setMinKDEVal(double minKDE)
{
	m_minKDEVal = minKDE;
}

QList<std::vector<double>>* iACompKernelDensityEstimationData::getObjectsPerBinUB()
{
	return m_objectsPerBinUB;
}

void iACompKernelDensityEstimationData::setObjectsPerBinUB(QList<std::vector<double>>* featuresPerBinUB)
{
	m_objectsPerBinUB = featuresPerBinUB;
}

QList<std::vector<double>>* iACompKernelDensityEstimationData::getObjectsPerBinNB()
{
	return m_objectsPerBinNB;
}

void iACompKernelDensityEstimationData::setObjectsPerBinNB(QList<std::vector<double>>* featuresPerBinNB)
{
	m_objectsPerBinNB = featuresPerBinNB;
}

QList<std::vector<double>>* iACompKernelDensityEstimationData::getObjectsPerBinBB()
{
	return m_objectsPerBinBB;
}
void iACompKernelDensityEstimationData::setObjectsPerBinBB(QList<std::vector<double>>* featuresPerBinUB)
{
	m_objectsPerBinBB = featuresPerBinUB;
}

/************************** kde methods ***************************************/

kdeData::kdeBins* kdeData::initializeBins(int numberOfBins)
{
	kdeData::kdeBins* data = new std::vector<kdeBin>(numberOfBins);
	return data;
}

void kdeData::debug(kdePair* pair)
{
	LOG(lvlDebug, "mdsValue = " + QString::number(pair->at(0)) + " : kdeValue = " + QString::number(pair->at(1)));
}

void kdeData::debug(kdeData::kdeBin* input)
{
	LOG(lvlDebug, " ");
	LOG(lvlDebug, "Number of kdePairs: " + QString::number(input->size()));
	LOG(lvlDebug, "kdePairs: ");
	for (size_t i = 0; i < input->size(); i++)
	{
		kdePair p = input->at(i);
		kdeData::debug(&p);
	}
	LOG(lvlDebug, " ");
}

void kdeData::debug(kdeBins* input)
{
	LOG(lvlDebug, "--------------------------");
	LOG(lvlDebug, "Number of kdeBins: " + QString::number(input->size()));
	LOG(lvlDebug, "kdeBins: ");
	for (size_t i = 0; i < input->size(); i++)
	{
		kdeData::debug(&input->at(i));
	}
	LOG(lvlDebug, "--------------------------");
}