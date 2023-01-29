#include "iACompHistogramCalculation.h"

//CompVis
#include "iACsvDataStorage.h"
#include "iAMultidimensionalScaling.h"
#include "iACompUniformBinningData.h"
#include "iACompBayesianBlocksData.h"
#include "iACompNaturalBreaksData.h"
#include "iACompKernelDensityEstimationData.h"
#include "iACompBinning.h"


//C++
#include <vector>

iACompHistogramCalculation::iACompHistogramCalculation(iACsvDataStorage* dataStorage, bool mdsComputed) :
	m_dataStorage(dataStorage),
	m_amountObjectsEveryDataset(new std::vector<int>()),
	m_uniformBinningData(nullptr),
	m_uniformBinning(nullptr),
	m_bayesianBlocksData(nullptr),
	m_bayesianBlocks(nullptr),
	m_naturalBreaksData(nullptr),
	m_naturalBreaks(nullptr),
	m_densityEstimationData(nullptr),
	m_densityEstimation(nullptr)
{

	//read in MDS values for loaded datasets
	std::vector<double>* histbinlist = new std::vector<double>();

	if (mdsComputed)
	{ //data originates from multidimensional distributions and was reduced via MDS to 1D
		histbinlist = csvDataType::arrayTypeToVector(dataStorage->getMDSData());
		orderDataPointsByDatasetAffiliation(histbinlist);
	}
	else
	{ //data originates from univariate distributions
		
		QList<csvFileData>* datasets = dataStorage->getData();
		
		//set datasets which stores the values for the binning calculations
		m_datasets = new bin::BinType();

		for (int i = 0; i < datasets->size(); i++)
		{
			csvFileData dataset = datasets->at(i);
			
			std::vector<double> distribution = std::vector<double>();

			for (int row = 0; row < static_cast<int>(dataset.values->size()); row++)
			{
				distribution.push_back(dataset.values->at(row).at(1));
			}

			m_datasets->push_back(distribution);

			histbinlist->insert(histbinlist->end(), distribution.begin(), distribution.end());
		}

		m_amountObjectsEveryDataset = csvFileData::getAmountObjectsEveryDataset(m_dataStorage->getData());
	}
	 
	auto result = std::minmax_element(histbinlist->begin(), histbinlist->end());

	m_maxVal = *result.second;
	m_minVal = *result.first;

	dataStorage->setMaxVal(m_maxVal);
	dataStorage->setMinVal(m_minVal);
}

void iACompHistogramCalculation::orderDataPointsByDatasetAffiliation(std::vector<double>* histbinlist)
{
	m_datasets = new bin::BinType();
	m_amountObjectsEveryDataset = csvFileData::getAmountObjectsEveryDataset(m_dataStorage->getData());

	int add = 0;
	for (int i = 0; i < static_cast<int>(m_amountObjectsEveryDataset->size()); i++)
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
	m_uniformBinningData->computeSturgesRule();

	m_uniformBinning =
		new iACompUniformBinning(m_dataStorage, m_datasets);
	m_uniformBinning->setDataStructure(m_uniformBinningData);
	m_uniformBinning->setCurrentNumberOfBins(m_uniformBinningData->getInitialNumberOfBins());
	m_uniformBinning->calculateBins();
	
	//evaluate binning
	LOG(lvlDebug, "Uniform Binning");
	m_uniformBinning->calculateSilhouetteCoefficient(m_uniformBinningData);
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

	m_bayesianBlocks = new iACompBayesianBlocks(m_dataStorage, m_datasets);
	m_bayesianBlocks->setDataStructure(m_bayesianBlocksData);
	m_bayesianBlocks->calculateBins();

	//evaluate binning
	LOG(lvlDebug, "Bayesian Blocks");
	m_bayesianBlocks->calculateSilhouetteCoefficient(m_bayesianBlocksData);
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
	
	m_naturalBreaks = new iACompNaturalBreaks(m_dataStorage, m_datasets);
	m_naturalBreaks->setDataStructure(m_naturalBreaksData);
	m_naturalBreaks->calculateBins();

	//evaluate binning
	LOG(lvlDebug, "Natural Breaks");
	m_naturalBreaks->calculateSilhouetteCoefficient(m_naturalBreaksData);
}

iACompNaturalBreaksData* iACompHistogramCalculation::getNaturalBreaksData()
{
	return m_naturalBreaksData;
}

/******************************************  Kernel Density Estimation  **********************************/
void iACompHistogramCalculation::calculateDensityEstimation()
{
	m_densityEstimationData = new iACompKernelDensityEstimationData();
	m_densityEstimationData->setMinVal(m_minVal);
	m_densityEstimationData->setMaxVal(m_maxVal);
	m_densityEstimationData->setAmountObjectsEveryDataset(m_amountObjectsEveryDataset);
	m_densityEstimationData->setMaxAmountInAllBins(m_uniformBinningData->getMaxAmountInAllBins());
	m_densityEstimationData->setObjectsPerBinUB(m_uniformBinningData->getNumberOfObjectsPerBinAllDatasets());
	m_densityEstimationData->setObjectsPerBinNB(m_naturalBreaksData->getNumberOfObjectsPerBinAllDatasets());
	m_densityEstimationData->setObjectsPerBinBB(m_bayesianBlocksData->getNumberOfObjectsPerBinAllDatasets());
	m_densityEstimationData->setNumberOfObjectsPerBinUB(m_uniformBinningData->getNumberOfObjectsPerBinAllDatasets());
	m_densityEstimationData->setNumberOfObjectsPerBinNB(m_naturalBreaksData->getNumberOfObjectsPerBinAllDatasets());
	m_densityEstimationData->setNumberOfObjectsPerBinBB(m_bayesianBlocksData->getNumberOfObjectsPerBinAllDatasets());
	m_densityEstimationData->setBoundariesUB(m_uniformBinningData->getBinBoundaries());
	m_densityEstimationData->setBoundariesNB(m_naturalBreaksData->getBinBoundaries());
	m_densityEstimationData->setBoundariesBB(m_bayesianBlocksData->getBinBoundaries());


	m_densityEstimation = new iACompKernelDensityEstimation(m_dataStorage, m_datasets);
	m_densityEstimation->setDataStructure(m_densityEstimationData);
	m_densityEstimation->calculateCurve(m_uniformBinningData, m_bayesianBlocksData, m_naturalBreaksData);
}

void iACompHistogramCalculation::recalculateDensityEstimationUniformBinning()
{
	m_densityEstimationData->setNumberOfObjectsPerBinUB(m_uniformBinningData->getNumberOfObjectsPerBinAllDatasets());
	m_densityEstimationData->setNumberOfObjectsPerBinUB(m_uniformBinningData->getNumberOfObjectsPerBinAllDatasets());
	m_densityEstimationData->setBoundariesUB(m_uniformBinningData->getBinBoundaries());

	m_densityEstimation->setDataStructure(m_densityEstimationData);
	m_densityEstimation->calculateCurveUB(m_uniformBinningData);
}

iACompKernelDensityEstimationData* iACompHistogramCalculation::getKernelDensityEstimationData()
{
	return m_densityEstimationData;
}
