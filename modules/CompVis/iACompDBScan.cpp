#include "iACompDBScan.h"
#include "iACompDBScanData.h"

iACompDBScan::iACompDBScan(
	iACsvDataStorage* dataStorage, std::vector<int>* amountObjectsEveryDataset, bin::BinType* datasets) :
	iACompBinning(dataStorage, datasets), m_dbData(nullptr)
{
	//test();
}

void iACompDBScan::setDataStructure(iACompHistogramTableData* datastructure)
{
	m_dbData = static_cast<iACompDBScanData*>(datastructure);
}

void iACompDBScan::calculateBins()
{
	//TODO implement DB-Scan Clustering

	QList<bin::BinType*>* binData = new QList<bin::BinType*>;  //stores MDS values
	QList<std::vector<csvDataType::ArrayType*>*>* binDataObjects =
		new QList<std::vector<csvDataType::ArrayType*>*>;  //stores data of selected objects attributes

	//double maxVal = m_dbData->getMaxVal();
	//double minVal = m_naturalBreaksData->getMinVal();

	QList<std::vector<double>>* binningStrategies =
		new QList<std::vector<double>>;  //stores number of bins for each dataset

	for (int i = 0; i < static_cast<int>(m_dbData->getAmountObjectsEveryDataset()->size()); i++)
	{  // do for every dataset

		//how to compute parameters: DBSCAN Revisited, Revisited: Why and How You Should(Still) Use DBSCAN by Schubert et.al.
		//heuristic approach: minPts = 2 · dim. --> we have 1 dimension --> minPts = 2;
		///int minPts = 2;
		//Once you know which MinPts to choose, you can determine Epsilon:
		/*float eps = ;

		dbscan( , eps, minPts);

		binData->push_back(bestBins);
		binDataObjects->push_back(bestBinsWithFiberIds);

		binningStrategies->push_back(bestCurrBinningStrategy);*/
	}

	m_dbData->setBinData(binData);
	m_dbData->setBinDataObjects(binDataObjects);
	m_dbData->calculateNumberOfObjectsInEachBin(binDataObjects);
	m_dbData->setBinBoundaries(binningStrategies);

	//m_dbData->debugBinDataObjects();
}

bin::BinType* iACompDBScan::calculateBins(bin::BinType* , int )
{
	//TODO
	return nullptr;
}