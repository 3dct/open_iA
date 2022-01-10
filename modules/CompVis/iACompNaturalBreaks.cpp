#include "iACompNaturalBreaks.h"

#include "iACompNaturalBreaksData.h"

iACompNaturalBreaks::iACompNaturalBreaks(iACsvDataStorage* dataStorage, std::vector<int>* amountObjectsEveryDataset, bin::BinType* datasets) :
	iACompBinning(dataStorage, amountObjectsEveryDataset, datasets), 
	m_naturalBreaksData(nullptr)
{
	//test();
}

void iACompNaturalBreaks::setDataStructure(iACompHistogramTableData* datastructure)
{
	m_naturalBreaksData = static_cast<iACompNaturalBreaksData*>(datastructure);
}

//calculate the binning for the data points
void iACompNaturalBreaks::calculateBins()
{
	QList<bin::BinType*>* binData = new QList<bin::BinType*>;  //stores MDS values
	QList<std::vector<csvDataType::ArrayType*>*>* binDataObjects =
		new QList<std::vector<csvDataType::ArrayType*>*>;  //stores data of selected objects attributes

	double maxVal = m_naturalBreaksData->getMaxVal();
	//double minVal = m_naturalBreaksData->getMinVal();

	QList<std::vector<double>>* binningStrategies = new QList<std::vector<double>>; //stores number of bins for each dataset

	for (int i = 0; i < m_naturalBreaksData->getAmountObjectsEveryDataset()->size(); i++)
	{  // do for every dataset

		std::vector<double> values = m_datasets->at(i);
		const int n = ((int)values.size());

		bin::BinType* bins;
		bin::BinType* bestBins;
		std::vector<csvDataType::ArrayType*>* binsWithFiberIds;
		std::vector<csvDataType::ArrayType*>* bestBinsWithFiberIds;
		FishersNaturalBreaks::LimitsContainer currBinningStrategy;
		FishersNaturalBreaks::LimitsContainer bestCurrBinningStrategy;
		
		//compute best number of bins by using goodness of variance fit
		int currentNumberOfBins = 1;
		double gvf = 0.0;  //value from 0 to 1 where 0 = No Fit and 1 = Perfect Fit.
		double bestGvf = 0.0;
		do
		{
			currentNumberOfBins += 1;

			//compute Natural Breaks
			FishersNaturalBreaks::ValueCountPairContainer sortedUniqueValueCounts;
			FishersNaturalBreaks::GetValueCountPairs(sortedUniqueValueCounts, &values[0], values.size());

			FishersNaturalBreaks::ClassifyJenksFisherFromValueCountPairs(
				currBinningStrategy, currentNumberOfBins, sortedUniqueValueCounts);
	
			//Debug Output
			/*LOG(lvlDebug, "Results of Natural Breaks: ");
			for (double breakValue : currBinningStrategy) LOG(lvlDebug, QString::number(breakValue));*/

			//calculate for each dataset the adaptive histogram according to its lower bounds of each bin
			bins = bin::initialize(currentNumberOfBins);
			binsWithFiberIds = new std::vector<csvDataType::ArrayType*>();
			for (int k = 0; k < currentNumberOfBins; k++)
			{
				csvDataType::ArrayType* init = new csvDataType::ArrayType();
				binsWithFiberIds->push_back(init);
			}

			int datasetInd = (int)values.size();

			//check for every value inside a dataset for the corresponding bin
			for (int v = 0; v < values.size(); v++)
			{
				for (int b = 0; b < currentNumberOfBins; b++)
				{
					bool inside;

					if (b < currentNumberOfBins - 1)
					{
						inside = checkRange(values.at(v), currBinningStrategy.at(b), currBinningStrategy.at(b + 1));
					}
					else
					{
						bool inside1 = checkRange(values.at(v), currBinningStrategy.at(b), maxVal);
						bool inside2 = (abs(maxVal - values.at(v)) < 1e-16);
						inside = inside1 || inside2;
					}

					if (inside)
					{
						//store MDS value
						bins->at(b).push_back(values.at(v));

						//store Fiber ID
						std::vector<double> object = m_dataStorage->getData()->at(i).values->at(v);
						csvDataType::ArrayType* data = binsWithFiberIds->at(b);
						data->push_back(object);

						break;
					}
				}

				datasetInd--;
			}

			//compute goodness of variance fit
			gvf = computeGoodnessOfVarianceFit(values, currBinningStrategy, bins);

			if (gvf > bestGvf)
			{
				bestGvf = gvf;
				bestBins = bins;
				bestBinsWithFiberIds = binsWithFiberIds;
				bestCurrBinningStrategy = currBinningStrategy;
			}
			
		} while (gvf < GFVLIMIT && currBinningStrategy.size() < values.size());

		binData->push_back(bestBins);
		binDataObjects->push_back(bestBinsWithFiberIds);

		binningStrategies->push_back(bestCurrBinningStrategy);
	}

	m_naturalBreaksData->setBinData(binData);
	m_naturalBreaksData->setBinDataObjects(binDataObjects);
	m_naturalBreaksData->calculateNumberOfObjectsInEachBin(binDataObjects);
	m_naturalBreaksData->setBinBoundaries(binningStrategies);

	//m_naturalBreaksData->debugBinDataObjects();
}

double iACompNaturalBreaks::computeGoodnessOfVarianceFit(
	std::vector<double> values, FishersNaturalBreaks::LimitsContainer currBinningStrategy, bin::BinType* bins)
{
	double gvf = 0.0;  //value from 0 to 1 where 0 = No Fit and 1 = Perfect Fit.
	
	//compute squared deviations from the values (array) mean(SDAM)
	std::sort(values.begin(), values.end());
	double valuesMean = 0.0;
	std::for_each(values.begin(), values.end(), [&](double m) { valuesMean += m; });
	valuesMean = valuesMean / values.size();
	double SDAM = 0.0;
	for (int l = 0; l < values.size(); l++)
	{
		//sum of squared deviations from values mean
		double sqDev = std::pow((values.at(l) - valuesMean), 2);
		SDAM += sqDev;
	}

	//compute the squared deviations from the bin (class) means(SDCM)
	double SDCM = 0.0;
	for (int j = 0; j < currBinningStrategy.size(); j++)
	{
		std::vector<double> valsInBin = bins->at(j);

		double binMean = 0.0;
		std::for_each(valsInBin.begin(), valsInBin.end(), [&](double m) { binMean += m; });
		binMean = binMean / valsInBin.size();
		double preSDCM = 0.0;
		for (int p = 0; p < valsInBin.size(); p++)
		{
			double sqDev2 = std::pow((valsInBin.at(p) - binMean), 2);
			preSDCM += sqDev2;
		}

		SDCM += preSDCM;
	}

	gvf = (SDAM - SDCM) / SDAM;

	return gvf;
}

bin::BinType* iACompNaturalBreaks::calculateBins(bin::BinType* data, int currData)
{
	return nullptr;
}

void iACompNaturalBreaks::test()
{
	std::vector<double> values = { 4,5,9,10 };
	const int n = 4;
	const int k = 2;

	//Generating sortedUniqueValueCounts
	FishersNaturalBreaks::ValueCountPairContainer sortedUniqueValueCounts;
	FishersNaturalBreaks::GetValueCountPairs(sortedUniqueValueCounts, &values[0], n);

	//Finding Jenks ClassBreaks
	FishersNaturalBreaks::LimitsContainer resultingbreaksArray;
	FishersNaturalBreaks::ClassifyJenksFisherFromValueCountPairs(resultingbreaksArray, k, sortedUniqueValueCounts);

	//Debug Output
	/*LOG(lvlDebug, "Results of Natural Breaks: ");
	for (double breakValue : resultingbreaksArray)
		LOG(lvlDebug, QString::number(breakValue));*/
}

//-----------------------------------------------------------------------------------------------------------//
/*This code is taken from https://www.geodms.nl/CalcNaturalBreaks
This code is written by Maarten Hilferink, © Object Vision BV, and is provided under GNU GPL v3.0 license
*/
namespace FishersNaturalBreaks
{
	SizeT GetTotalCount(const ValueCountPairContainer& vcpc)
	{
		SizeT sum = 0;
		ValueCountPairContainer::const_iterator i = vcpc.begin(), e = vcpc.end();
		for (sum = 0; i != e; ++i) sum += (*i).second;
		return sum;
	}

	void GetCountsDirect(ValueCountPairContainer& vcpc, const double* values, SizeT size)
	{
		assert(size <= BUFFER_SIZE);
		assert(size > 0);
		assert(vcpc.empty());

		double buffer[BUFFER_SIZE];

		std::copy(values, values + size, buffer);
		std::sort(buffer, buffer + size);

		double currValue = buffer[0];
		SizeT     currCount = 1;
		for (SizeT index = 1; index != size; ++index)
		{
			if (currValue < buffer[index])
			{
				vcpc.push_back(ValueCountPair(currValue, currCount));
				currValue = buffer[index];
				currCount = 1;
			}
			else
				++currCount;
		}
		vcpc.push_back(ValueCountPair(currValue, currCount));
	}

	void MergeToLeft(ValueCountPairContainer& vcpcLeft, const ValueCountPairContainer& vcpcRight, ValueCountPairContainer& vcpcDummy)
	{
		assert(vcpcDummy.empty());
		vcpcDummy.swap(vcpcLeft);
		vcpcLeft.resize(vcpcRight.size() + vcpcDummy.size());

		std::merge(vcpcRight.begin(), vcpcRight.end(), vcpcDummy.begin(), vcpcDummy.end(), vcpcLeft.begin(), CompareFirst());

		ValueCountPairContainer::iterator
			currPair = vcpcLeft.begin(),
			lastPair = vcpcLeft.end();


		ValueCountPairContainer::iterator index = currPair + 1;
		while (index != lastPair && currPair->first < index->first)
		{
			currPair = index;
			++index;
		}

		double currValue = currPair->first;
		SizeT     currCount = currPair->second;
		for (; index != lastPair; ++index)
		{
			if (currValue < index->first)
			{
				*currPair++ = ValueCountPair(currValue, currCount);
				currValue = index->first;
				currCount = index->second;
			}
			else
				currCount += index->second;
		}
		*currPair++ = ValueCountPair(currValue, currCount);
		vcpcLeft.erase(currPair, lastPair);

		vcpcDummy.clear();
	}

	struct ValueCountPairContainerArray : std::vector<ValueCountPairContainer>
	{
		void resize(SizeT k)
		{
			assert(capacity() >= k);
			while (size() < k)
			{
				push_back(ValueCountPairContainer());
				back().reserve(BUFFER_SIZE);
			}
		}

		void GetValueCountPairs(
			ValueCountPairContainer& vcpc, const double* values, SizeT size, unsigned int nrUsedContainers)
		{
			assert(vcpc.empty());
			if (size <= BUFFER_SIZE)
				GetCountsDirect(vcpc, values, size);
			else
			{
				resize(nrUsedContainers + 2);

				unsigned int m = ((unsigned int)size) / 2;

				GetValueCountPairs(vcpc, values, m, nrUsedContainers);
				GetValueCountPairs(begin()[nrUsedContainers], values + m, size - m, nrUsedContainers + 1);

				MergeToLeft(vcpc, begin()[nrUsedContainers], begin()[nrUsedContainers + 1]);
				begin()[nrUsedContainers].clear();
			}
			assert(GetTotalCount(vcpc) == size);
		}
	};

	void GetValueCountPairs(ValueCountPairContainer& vcpc, const double* values, SizeT n)
	{
		vcpc.clear();

		if (n)
		{
			ValueCountPairContainerArray vcpca;
			// max nr halving is log2(max cardinality / BUFFER_SIZE); max cardinality is SizeT(-1)
			vcpca.reserve(3 + 8 * sizeof(SizeT) - 10);
			vcpca.GetValueCountPairs(vcpc, values, n, 0);

			assert(vcpc.size());
		}
	}

	void ClassifyJenksFisherFromValueCountPairs(LimitsContainer& breaksArray, SizeT k, const ValueCountPairContainer& vcpc)
	{
		breaksArray.resize(k);
		SizeT m = vcpc.size();

		assert(k <= m); // PRECONDITION

		if (!k)
			return;

		JenksFisher jf(vcpc, k);

		if (k > 1)
		{
			jf.CalcAll();

			SizeT lastClassBreakIndex = jf.FindMaxBreakIndex(jf.m_BufSize - 1, 0, jf.m_BufSize);

			while (--k)
			{
				breaksArray[k] = vcpc[lastClassBreakIndex + k].first;
				assert(lastClassBreakIndex < jf.m_BufSize);
				if (k > 1)
				{
					jf.m_CBPtr -= jf.m_BufSize;
					lastClassBreakIndex = jf.m_CBPtr[lastClassBreakIndex];
				}
			}
			assert(jf.m_CBPtr == jf.m_CB.begin());
		}
		assert(k == 0);
		breaksArray[0] = vcpc[0].first; // break for the first class is the minimum of the dataset.
	}
}


