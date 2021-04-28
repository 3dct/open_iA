#include "iACompBinning.h"

//Qt
#include "qlist.h"

iACompBinning::iACompBinning(iACsvDataStorage* dataStorage, std::vector<int>* amountObjectsEveryDataset, bin::BinType* datasets):
	m_dataStorage(dataStorage),
	m_datasets(datasets)
{};

bool iACompBinning::checkRange(double value, double low, double high)
{
	return ((value >= low) && (value < high));
}