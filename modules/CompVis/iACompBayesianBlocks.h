#pragma once
#include "iACompBinning.h"

#include <vector>
#include <chrono>

class iACompBayesianBlocksData;

//#ifndef _BAYESIAN_BLOCKS_HH
//#define _BAYESIAN_BLOCKS_HH

namespace BayesianBlocks
{
	// Copyright (c) 2019 Luigi Pertoldi
	//
	// Permission is hereby granted, free of charge, to any person obtaining a copy of
	// this software and associated documentation files (the "Software"), to deal in
	// the Software without restriction, including without limitation the rights to
	// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
	// the Software, and to permit persons to whom the Software is furnished to do so,
	// subject to the following conditions:
	//
	// The above copyright notice and this permission notice shall be included in all
	// copies or substantial portions of the Software.
	//
	// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
	// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
	// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
	// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
	// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
	
	// handy aliases
	namespace bb
	{
		// data containers
		using array = std::vector<double>;
		using data_array = std::vector<double>;
		using weights_array = std::vector<int>;
		using pair = std::pair<double, int>;

		// time
		using clock = std::chrono::high_resolution_clock;
		using std::chrono::duration_cast;
		using us = std::chrono::microseconds;
	}

	//implements the core functionality of bayesian blocks
	// returns an array of the low-edges for each bin
	//This array has a size of numberOfBins + 1 
	bb::array blocks(bb::data_array data, bb::weights_array weights, const double p = 0.01, bool counter = false,
		bool benchmark = false);

	// returns an array of the low-edges for each bin
	//This array has a size of numberOfBins + 1 
	bb::array blocks(bb::data_array data, const double p = 0.01, bool counter = false, bool benchmark = false);
}

//#endif


class iACompBayesianBlocks : public iACompBinning
{
public:
	iACompBayesianBlocks(
		iACsvDataStorage* dataStorage, bin::BinType* datasets);

	//calculate the binning for the data points
	virtual void calculateBins();

	//calculates the bin datastructure for (a) specifically selected bin(s)
	virtual bin::BinType* calculateBins(bin::BinType* data, int currData);

	virtual void setDataStructure(iACompHistogramTableData* datastructure);

private:

	iACompBayesianBlocksData* m_bayesianBlocksData;
};
