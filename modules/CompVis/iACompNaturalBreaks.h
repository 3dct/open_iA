// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACompBinning.h"

#include <assert.h>
#include <vector>
#include <algorithm>

class iACompNaturalBreaksData;

namespace FishersNaturalBreaks
{
	typedef std::size_t                  SizeT;
	typedef SizeT                        CountType;
	typedef std::pair<double, CountType> ValueCountPair;
	typedef std::vector<double>          LimitsContainer;
	typedef std::vector<ValueCountPair>  ValueCountPairContainer;

	// helper funcs
	template <typename T> T Min(T a, T b) { return (a < b) ? a : b; }

	SizeT GetTotalCount(const ValueCountPairContainer& vcpc);

	// helper struct JenksFisher
	struct JenksFisher  // captures the intermediate data and methods for the calculation of Natural Class Breaks.
	{
		JenksFisher(const ValueCountPairContainer& vcpc, SizeT k) :
			m_M(vcpc.size()),
			m_K(k),
			m_BufSize(vcpc.size() - (k - 1)),
			m_PrevSSM(m_BufSize),
			m_CurrSSM(m_BufSize),
			m_CB(m_BufSize * (m_K - 1)),
			m_CBPtr(),
			m_NrCompletedRows(0)
		{
			m_CumulValues.reserve(vcpc.size());
			double cwv = 0;
			CountType cw = 0, w;

			for (SizeT i = 0; i != m_M; ++i)
			{
				assert(!i ||
					vcpc[i].first > vcpc[i - 1].first);  // PRECONDITION: the value sequence must be strictly increasing

				w = vcpc[i].second;
				assert(w > 0);  // PRECONDITION: all weights must be positive

				cw += w;
				assert(cw > w || !i);  // No overflow? No loss of precision?

				cwv += w * vcpc[i].first;
				m_CumulValues.push_back(ValueCountPair(cwv, cw));
				if (i < m_BufSize)
					m_PrevSSM[i] = cwv * cwv / cw;  // prepare SSM for first class. Last (k-1) values are omitted
			}
		}

		double GetW(SizeT b, SizeT e)
		// Gets sum of weighs for elements b..e.
		{
			assert(b);  // First element always belongs to class 0, thus queries should never include it.
			assert(b <= e);
			assert(e < m_M);

			double res = m_CumulValues[e].second;
			res -= m_CumulValues[b - 1].second;
			return res;
		}

		double GetWV(SizeT b, SizeT e)
		// Gets sum of weighed values for elements b..e
		{
			assert(b);
			assert(b <= e);
			assert(e < m_M);

			double res = m_CumulValues[e].first;
			res -= m_CumulValues[b - 1].first;
			return res;
		}

		double GetSSM(SizeT b, SizeT e)
		// Gets the Squared Mean for elements b..e, multiplied by weight.
		// Note that n*mean^2 = sum^2/n when mean := sum/n
		{
			double res = GetWV(b, e);
			return res * res / GetW(b, e);
		}

		SizeT FindMaxBreakIndex(SizeT i, SizeT bp, SizeT ep)
		// finds CB[i+m_NrCompletedRows] given that
		// the result is at least bp+(m_NrCompletedRows-1)
		// and less than          ep+(m_NrCompletedRows-1)
		// Complexity: O(ep-bp) <= O(m)
		{
			assert(bp < ep);
			assert(bp <= i);
			assert(ep <= i + 1);
			assert(i < m_BufSize);
			assert(ep <= m_BufSize);

			double minSSM = m_PrevSSM[bp] + GetSSM(bp + m_NrCompletedRows, i + m_NrCompletedRows);
			SizeT foundP = bp;
			while (++bp < ep)
			{
				double currSSM = m_PrevSSM[bp] + GetSSM(bp + m_NrCompletedRows, i + m_NrCompletedRows);
				if (currSSM > minSSM)
				{
					minSSM = currSSM;
					foundP = bp;
				}
			}
			m_CurrSSM[i] = minSSM;
			return foundP;
		}

		void CalcRange(SizeT bi, SizeT ei, SizeT bp, SizeT ep)
		// find CB[i+m_NrCompletedRows]
		// for all i>=bi and i<ei given that
		// the results are at least bp+(m_NrCompletedRows-1)
		// and less than            ep+(m_NrCompletedRows-1)
		// Complexity: O(log(ei-bi)*Max((ei-bi),(ep-bp))) <= O(m*log(m))
		{
			assert(bi <= ei);

			assert(ep <= ei);
			assert(bp <= bi);

			if (bi == ei)
				return;
			assert(bp < ep);

			SizeT mi = (bi + ei) / 2;
			SizeT mp = FindMaxBreakIndex(mi, bp, Min<SizeT>(ep, mi + 1));

			assert(bp <= mp);
			assert(mp < ep);
			assert(mp <= mi);

			// solve first half of the sub-problems with lower 'half' of possible outcomes
			CalcRange(bi, mi, bp, Min<SizeT>(mi, mp + 1));

			m_CBPtr[mi] = mp;  // store result for the middle element.

			// solve second half of the sub-problems with upper 'half' of possible outcomes
			CalcRange(mi + 1, ei, mp, ep);
		}

		void CalcAll()
		// complexity: O(m*log(m)*k)
		{
			if (m_K >= 2)
			{
				m_CBPtr = m_CB.begin();
				for (m_NrCompletedRows = 1; m_NrCompletedRows < m_K - 1; ++m_NrCompletedRows)
				{
					CalcRange(0, m_BufSize, 0, m_BufSize);  // complexity: O(m*log(m))

					m_PrevSSM.swap(m_CurrSSM);
					m_CBPtr += m_BufSize;
				}
			}
		}

		SizeT m_M, m_K, m_BufSize;
		ValueCountPairContainer m_CumulValues;

		std::vector<double> m_PrevSSM;
		std::vector<double> m_CurrSSM;
		std::vector<SizeT> m_CB;
		std::vector<SizeT>::iterator m_CBPtr;

		SizeT m_NrCompletedRows;
	};

	// GetValueCountPairs
	//
	// GetValueCountPairs sorts chunks of values and then merges them in order to minimize extra memory and work when many values are equal.
	// This is done recursively while retaining used intermediary buffers in order to minimize heap allocations.
	const SizeT BUFFER_SIZE = 1024;

	struct CompareFirst
	{
		bool operator()(const ValueCountPair& lhs, const ValueCountPair& rhs)
		{
			return lhs.first < rhs.first;
		}
	};

	void GetCountsDirect(ValueCountPairContainer& vcpc, const double* values, SizeT size);
	void MergeToLeft(ValueCountPairContainer& vcpcLeft, const ValueCountPairContainer& vcpcRight, ValueCountPairContainer& vcpcDummy);
	void GetValueCountPairs(ValueCountPairContainer& vcpc, const double* values, SizeT n);
	void ClassifyJenksFisherFromValueCountPairs(LimitsContainer& breaksArray, SizeT k, const ValueCountPairContainer& vcpc);
}


class iACompNaturalBreaks : public iACompBinning
{

public:
	iACompNaturalBreaks(iACsvDataStorage* dataStorage, bin::BinType* datasets);

	//calculate the binning for the data points
	void calculateBins() override;

	//calculates the bin datastructure for (a) specifically selected bin(s)
	bin::BinType* calculateBins(bin::BinType* data, int currData) override;

	void setDataStructure(iACompHistogramTableData* datastructure) override;

private:

	//testing Fishers Natural Breaks implementation
	void test();

	//compute the Goodness of Variance Fit to determine the best number of bins
	double computeGoodnessOfVarianceFit(
		std::vector<double> values, FishersNaturalBreaks::LimitsContainer currBinningStrategy, bin::BinType* bins);

	iACompNaturalBreaksData* m_naturalBreaksData;

	//Goodness Of Variance Fit has a value from 0 to 1 where 0 = No Fit and 1 = Perfect Fit.
	//GFVLIMIT is the limit the gfv has to have before stopping the computation of a good number of Breaks/Bins/Classes
	const double GFVLIMIT = 0.99;

};
