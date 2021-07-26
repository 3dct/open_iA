#include "iACompHistogramCalculation.h"

//CompVis
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"
#include "iACompUniformBinningData.h"
#include "iACompBayesianBlocksData.h"
#include "iACompNaturalBreaksData.h"


//C++
#include <vector>

iACompHistogramCalculation::iACompHistogramCalculation(iAMultidimensionalScaling* mds, iACsvDataStorage* dataStorage) :
	m_mds(mds),
	m_dataStorage(dataStorage)
{

	std::vector<double>* histbinlist = csvDataType::arrayTypeToVector(m_mds->getResultMatrix());
	auto result = std::minmax_element(histbinlist->begin(), histbinlist->end());

	m_maxVal = *result.second;
	m_minVal = *result.first;

	orderDataPointsByDatasetAffiliation(histbinlist);
}

void iACompHistogramCalculation::orderDataPointsByDatasetAffiliation(std::vector<double>* histbinlist)
{
	m_datasets = new bin::BinType();
	m_amountObjectsEveryDataset = csvFileData::getAmountObjectsEveryDataset(m_mds->getCSVFileData());

	int add = 0;
	for (int i = 0; i < m_amountObjectsEveryDataset->size(); i++)
	{
		std::vector<double>::const_iterator first = histbinlist->begin() + add;
		add += m_amountObjectsEveryDataset->at(i);
		std::vector<double>::const_iterator last = histbinlist->begin() + add;

		std::vector<double> segment(first, last);
		m_datasets->push_back(segment);
	}
}

/******************************************  Uniform Binning  **********************************/
void iACompHistogramCalculation::calculateUniformBinning()
{
	//calculate uniform binning and store it
	m_uniformBinningData = new iACompUniformBinningData();
	m_uniformBinningData->setMinVal(m_minVal);
	m_uniformBinningData->setMaxVal(m_maxVal);
	m_uniformBinningData->setAmountObjectsEveryDataset(m_amountObjectsEveryDataset);

	m_uniformBinning =
		new iACompUniformBinning(m_dataStorage, m_amountObjectsEveryDataset, m_datasets);
	m_uniformBinning->setDataStructure(m_uniformBinningData);
	m_uniformBinning->setCurrentNumberOfBins(m_uniformBinningData->getInitialNumberOfBins());
	m_uniformBinning->calculateBins();
}

void iACompHistogramCalculation::calculateUniformBinning(int numberOfBins)
{
	m_uniformBinning->setCurrentNumberOfBins(numberOfBins);
	m_uniformBinning->calculateBins();
}

void iACompHistogramCalculation::calculateUniformBinningSpecificBins(bin::BinType* data, int currData, int numberOfBins)
{
	m_uniformBinning->setCurrentNumberOfBins(numberOfBins);
	m_uniformBinning->calculateBins(data, currData);
}

iACompUniformBinningData* iACompHistogramCalculation::getUniformBinningData()
{
	return m_uniformBinningData;
}

/******************************************  Bayesian Blocks  **********************************/
void iACompHistogramCalculation::calculateBayesianBlocks()
{
	m_bayesianBlocksData = new iACompBayesianBlocksData();
	m_bayesianBlocksData->setMinVal(m_minVal);
	m_bayesianBlocksData->setMaxVal(m_maxVal);
	m_bayesianBlocksData->setAmountObjectsEveryDataset(m_amountObjectsEveryDataset);
	m_bayesianBlocksData->setMaxAmountInAllBins(m_uniformBinningData->getMaxAmountInAllBins());

	m_bayesianBlocks = new iACompBayesianBlocks(m_dataStorage, m_amountObjectsEveryDataset, m_datasets);
	m_bayesianBlocks->setDataStructure(m_bayesianBlocksData);
	m_bayesianBlocks->calculateBins();
}

iACompBayesianBlocksData* iACompHistogramCalculation::getBayesianBlocksData()
{
	return m_bayesianBlocksData;
}

/******************************************  Natural Breaks  **********************************/
void iACompHistogramCalculation::calculateNaturalBreaks()
{
	 m_naturalBreaksData = new iACompNaturalBreaksData();
	m_naturalBreaksData->setMinVal(m_minVal);
	m_naturalBreaksData->setMaxVal(m_maxVal);
	m_naturalBreaksData->setAmountObjectsEveryDataset(m_amountObjectsEveryDataset);
	m_naturalBreaksData->setMaxAmountInAllBins(m_uniformBinningData->getMaxAmountInAllBins());
	
	m_naturalBreaks = new iACompNaturalBreaks(m_dataStorage, m_amountObjectsEveryDataset, m_datasets);
	m_naturalBreaks->setDataStructure(m_naturalBreaksData);
	m_naturalBreaks->calculateBins();
}

iACompNaturalBreaksData* iACompHistogramCalculation::getNaturalBreaksData()
{
	return m_naturalBreaksData;
}
