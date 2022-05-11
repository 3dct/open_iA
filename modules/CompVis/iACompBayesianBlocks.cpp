#include "iACompBayesianBlocks.h"

#include "iACompBayesianBlocksData.h"

#include <iostream>
#include <cmath>
#include <vector>
#include <map>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <stdexcept>
#include <cassert>
#include <limits>

#include <fstream>

iACompBayesianBlocks::iACompBayesianBlocks(
	iACsvDataStorage* dataStorage, bin::BinType* datasets) :
	iACompBinning(dataStorage, datasets), 
	m_bayesianBlocksData(nullptr)
{
}

void iACompBayesianBlocks::setDataStructure(iACompHistogramTableData* datastore)
{
	m_bayesianBlocksData = static_cast<iACompBayesianBlocksData*>(datastore);
}
	
void iACompBayesianBlocks::calculateBins()
{
	QList<bin::BinType*>* binData = new QList<bin::BinType*>;  //stores MDS values
	QList<std::vector<csvDataType::ArrayType*>*>* binDataObjects =
		new QList<std::vector<csvDataType::ArrayType*>*>;  //stores data of selected objects attributes

	double maxVal = m_bayesianBlocksData->getMaxVal();
	

	QList<std::vector<double>>* binningStrategies = new QList<std::vector<double>>;
	
	for (int i = 0; i < static_cast<int>(m_bayesianBlocksData->getAmountObjectsEveryDataset()->size()); i++)
	{  // do for every dataset

		std::vector<double> values = m_datasets->at(i);
		std::vector<double> binningValues = m_datasets->at(i);

		//TODO
		//change bayesian blocks computation so that min and max are used for computing lower edges for each bin
		//but the real values should be used for actual binning
		//Questions is now: Is the maxValue inside or outside the binning?
		//Problem: the last bin is always only filled with 1 value!

		//add minimum and maximum value of all datasets for each dataset for calculation of bayesian blocks
		/*
		if (std::find(binningValues.begin(), binningValues.end(), minVal) == binningValues.end())
		{
			binningValues.insert(binningValues.begin(), minVal);
			LOG(lvlDebug, "added min: " + QString::number(minVal));
		}
			
		if (std::find(binningValues.begin(), binningValues.end(), maxVal) == binningValues.end())
		{
			binningValues.push_back(maxVal);
			LOG(lvlDebug, "added max: " + QString::number(maxVal));
		}
		*/
		
		//calculate for each dataset the adaptive histogram according to its lower bounds of each bin
		auto currBinningStrategy = BayesianBlocks::blocks(binningValues, 0.01, false, true);
		
		int currentNumberOfBins = static_cast<int>(currBinningStrategy.size());
		bin::BinType* bins = bin::initialize(currentNumberOfBins);
		std::vector<csvDataType::ArrayType*>* binsWithFiberIds = new std::vector<csvDataType::ArrayType*>();
		for (int k = 0; k < currentNumberOfBins; k++)
		{
			csvDataType::ArrayType* init = new csvDataType::ArrayType();
			binsWithFiberIds->push_back(init);
		}

		//////////////////////////////////////////////////////////////////////////////////////
		//DEBUG
		/*LOG(lvlDebug, "Resulting Bins (?) = " + QString::number(currBinningStrategy.size()));
		for (int j = 0; j < currBinningStrategy.size(); j++)
		{
			LOG(lvlDebug, "value at " + QString::number(j) + " = " + QString::number(currBinningStrategy.at(j)));
		}

		LOG(lvlDebug, " ");*/
		//////////////////////////////////////////////////////////////////////////////////////
		
		int datasetInd = static_cast<int>(values.size());

		//check for every value inside a dataset for the corresponding bin	
		for (int v = 0; v < static_cast<int>(values.size()); v++)
		{
			for (int b = 0; b < currentNumberOfBins; b++)
			{
				bool inside;
				
				if (b < currentNumberOfBins-1)
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

		binData->push_back(bins);
		binDataObjects->push_back(binsWithFiberIds);

		binningStrategies->push_back(currBinningStrategy);

/*		std::ifstream fin;
		fin.open("C:/FHTools/open_iA/src/modules/CompVis/test.dat");
		if (!fin.is_open())
		{
			LOG(lvlDebug, "dat not found!");
		}

		double a;
		std::vector<double> v;
		while (fin >> a) v.push_back(a);

		LOG(lvlDebug, "v = " + QString::number(v.size()));

		std::vector<double> exp = {
			-3.48528, -1.87114, -1.36282, -0.677218, 0.659105, 1.39771, 4.06582, 5.60912, 6.17286, 7.76634, 9.91696};

		 auto r = BayesianBlocks::blocks(v, 0.01, false, true);

		 //DEBUG
		 LOG(lvlDebug, "Resulting Bins (?) = " + QString::number(r.size()));
		 for (int j = 0; j < r.size(); j++)
		 {
			 LOG(lvlDebug, "value at " + QString::number(j) + " = " + QString::number(r.at(j)));
		 }
	*/	
	}

	m_bayesianBlocksData->setBinData(binData);
	m_bayesianBlocksData->setBinDataObjects(binDataObjects);
	m_bayesianBlocksData->calculateNumberOfObjectsInEachBin(binDataObjects);
	m_bayesianBlocksData->setBinBoundaries(binningStrategies);

	//m_bayesianBlocksData->debugBinDataObjects();
}


bin::BinType* iACompBayesianBlocks::calculateBins(bin::BinType* , int )
{
	return nullptr;
}

namespace BayesianBlocks
{
	
	bb::array blocks(bb::data_array data, bb::weights_array weights, const double p, bool counter, bool benchmark)
	{
		auto start = bb::clock::now();

		// sanity checks
		if (data.size() != weights.size())
		{
			throw std::domain_error("ERROR: data and weights vectors are of different sizes");
		}

		if (data.size() == 0)
		{
			throw std::invalid_argument("ERROR: empty arrays provided as input");
		}

		if (std::find_if(weights.begin(), weights.end(), [](int& v) { return v <= 0; }) != weights.end())
		{
			throw std::domain_error("ERROR: invalid weights found in input");
		}

		if (std::unique(data.begin(), data.end()) != data.end())
		{
			throw std::invalid_argument("ERROR: duplicated values found in input");
		}

		const auto N = data.size();

		// sort and copy data
		std::vector<bb::pair> hist;
		hist.reserve(N);
		for (std::size_t i = 0; i < N; ++i) hist.emplace_back(data[i], weights[i]);
		std::sort(hist.begin(), hist.end(), [](bb::pair a, bb::pair b) { return a.first < b.first; });
		for (size_t i = 0; i < N; ++i)
		{
			data[i] = hist[i].first;
			weights[i] = hist[i].second;
		}

		// build up array with all possible bin edges
		bb::array edges(N + 1);
		edges[0] = data[0];
		for (std::size_t i = 0; i < N - 1; ++i) edges[i + 1] = (data[i] + data[i + 1]) / 2.;
		edges[N] = data[N - 1];

		assert(std::unique(edges.begin(), edges.end()) == edges.end());

		// let's use here Cash statistics and calibrated prior on number of change points
		auto cash = [](int N_k, double T_k) { return N_k * std::log(N_k / T_k); };
		auto ncp_prior = std::log(73.53 * p * std::pow(N, -0.478)) - 4;

		// arrays to store results
		bb::array last(N);
		bb::array best(N);

		auto init_time = bb::duration_cast<bb::us>(bb::clock::now() - start).count();
		start = bb::clock::now();

		// do the actual recursive computation
		for (std::size_t k = 0; k < N; ++k)
		{
			bb::array A(k + 1);
			for (std::size_t r = 0; r == 0 or r <= k; ++r)
			{
				A[r] = cash(std::accumulate(weights.begin() + r, weights.begin() + k + 1, 0), edges[k + 1] - edges[r]) +
					ncp_prior + (r == 0 ? 0 : best[r - 1]);
			}
			last[k] = std::distance(A.begin(), std::max_element(A.begin(), A.end()));
			best[k] = *(std::max_element(A.begin(), A.end()));

			if (counter)
				std::cout << '\r' << k << '/' << N << std::flush;
		}
		if (counter)
			std::cout << std::endl;

		auto loop_time = bb::duration_cast<bb::us>(bb::clock::now() - start).count();
		start = bb::clock::now();

		// iteratively find the change points
		std::vector<int> cp;
		for (auto i = N; i != 0; i = last[i - 1]) cp.push_back(i);
		cp.push_back(0);

		std::reverse(cp.begin(), cp.end());
		bb::array result(cp.size(), 0);
		std::transform(cp.begin(), cp.end(), result.begin(), [edges](size_t pos) { return edges[pos]; });

		auto end_time = bb::duration_cast<bb::us>(bb::clock::now() - start).count();

		if (benchmark)
		{
			std::cout << "init: ";
			init_time > 1000 ? std::cout << init_time / 1.E3 << " s" : std::cout << init_time << " us";
			std::cout << std::endl;
			std::cout << "loop: ";
			loop_time > 1000 ? std::cout << loop_time / 1.E3 << " s" : std::cout << loop_time << " us";
			std::cout << std::endl;
			std::cout << "end: ";
			end_time > 1000 ? std::cout << end_time / 1.E3 << " s" : std::cout << end_time << " us";
			std::cout << std::endl;
		}

		return result;
	}

	bb::array blocks(bb::data_array data, const double p, bool counter, bool benchmark)
	{
		// compute weights
		std::map<double, int> hist;
		for (auto& i : data)
		{
			if (hist.find(i) == hist.end())
				hist.emplace(i, 1);
			else
				hist[i]++;
		}

		bb::data_array x;
		bb::weights_array weights;
		for (auto& i : hist)
		{
			x.push_back(i.first);
			weights.push_back(i.second);
		}

		return BayesianBlocks::blocks(x, weights, p, counter, benchmark);
	}
}