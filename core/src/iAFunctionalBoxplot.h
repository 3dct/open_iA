/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <set>
#include <vector>

#include "iAFunction.h"

//! Class for storing a "function band", i.e. a min-max range, for creating an iAFunctionalBoxplot.
 template <typename ArgType, typename ValType>
class iAFunctionBand
{
public:
	//! merge the given function so that it also lies inside the band
	void merge(iAFunction<ArgType, ValType> const & f, size_t functionIdx);

	//! @{ getters/setters
	ValType getMin(ArgType a) const;
	ValType getMax(ArgType a) const;
	void setMin(ArgType a, ValType v);
	void setMax(ArgType a, ValType v);
	//! @}

	bool contains(ArgType idx, ValType value) const;
	bool contains(iAFunction<ArgType, ValType> const & func) const;
private:
	std::map<ArgType, std::pair<ValType, ValType>> m_Bounds;
	std::set<size_t>  m_Functions;
	// template<typename AT, typename VT>
	// friend std::ostream & operator<<(std::ostream& out, iAFunctionBand<AT, VT> const & fb);
};


//! Abstract Base for depth measures for a single function and band combination.
template <typename ArgType, typename ValType>
class iADepthMeasure
{
public:
	virtual double calculate(iAFunction<ArgType, ValType> const & curFunc,
		iAFunction<ArgType, ValType> const & limFunc1,
		iAFunction<ArgType, ValType> const & limFunc2) = 0;
};


//! Class for calculating and providing functional boxplot data for arbitrary functions
//! For details on the calculation, see
//! Lopez-Pintado, S.; Romo, J. (2009). "On the Concept of Depth for Functional
//! Data". Journal of the American Statistical Association 104 (486): 718-734.
//! Sun, Y.; Genton, M. G. (2011). "Functional boxplots". Journal of
//! Computational and Graphical Statistics 20: 316-334.
template <typename ArgType, typename ValType>
class iAFunctionalBoxplot {
public:
	// limits for the number of loops that are used during calculation
	static const unsigned long long MaxOverallFctBoxPlotLoops = 10000000000000;
	static const unsigned long long MinBandsPerFct            = 40;
	static const unsigned long long MinArgsPerBand            = 40;
	enum Index {
		LOWER,
		UPPER
	};
	//! Construction & calculation of functional boxplot data
	//! @param functions functions for which to calculate band depth
	//! @param measure the measure to use to compute the depth (see SimpleDepthMeasure and ModifiedDepthMeasure)
	//! @param maxBandSize the maximum band size to consider for band depth calculation (i.e. how
	//!    many functions at most should be combined to bands). Band depth will be calculated
	//!    as a combination of all band sizes from 2 to maxBandSize
	iAFunctionalBoxplot(std::vector<iAFunction<ArgType, ValType> *> & functions,
		iADepthMeasure<ArgType, ValType>* measure,
		size_t maxBandSize = 2);
	iAFunction<ArgType, ValType> const & getMedian() const;
	iAFunctionBand<ArgType, ValType> const & getCentralRegion() const;
	iAFunctionBand<ArgType, ValType> const & getEnvelope() const;
	std::vector<iAFunction<ArgType, ValType>* > const & getOutliers() const;
private:
	iAFunction<ArgType, ValType>* m_median;
	iAFunctionBand<ArgType, ValType> m_centralRegion;
	iAFunctionBand<ArgType, ValType> m_envelope;
	std::vector<iAFunction<ArgType, ValType> *> m_outliers;
};



#include <limits>

template <typename ArgType, typename ValType>
void iAFunctionBand<ArgType, ValType>::merge(iAFunction<ArgType, ValType> const & f, size_t funcIdx)
{
	if (m_Functions.empty())
	{
		for (auto it = f.begin(); it != f.end(); ++it)
			m_Bounds.insert(std::make_pair(it->first, std::make_pair(it->second, it->second)));
	}
	else
	{
		for (auto it = f.begin(); it != f.end(); ++it)
		{
			if (it->second < getMin(it->first))
			{
				m_Bounds[it->first].first = it->second;
			}
			if (it->second > getMax(it->first))
			{
				m_Bounds[it->first].second = it->second;
			}
		}
	}
	m_Functions.insert(funcIdx);
}

template <typename ArgType, typename ValType>
ValType iAFunctionBand<ArgType, ValType>::getMin(ArgType a) const
{
	return m_Bounds.at(a).first;
}

template <typename ArgType, typename ValType>
ValType iAFunctionBand<ArgType, ValType>::getMax(ArgType a) const
{
	return m_Bounds.at(a).second;
}

template <typename ArgType, typename ValType>
void iAFunctionBand<ArgType, ValType>::setMin(ArgType a, ValType val)
{
	m_Bounds.at(a).first = val;
}

template <typename ArgType, typename ValType>
void iAFunctionBand<ArgType, ValType>::setMax(ArgType a, ValType val)
{
	m_Bounds.at(a).second = val;
}

template <typename ArgType, typename ValType>
bool iAFunctionBand<ArgType, ValType>::contains(ArgType idx, ValType value) const
{
	return (value >= getMin(idx) && value <= getMax(idx));
}

template <typename ArgType, typename ValType>
bool iAFunctionBand<ArgType, ValType>::contains(iAFunction<ArgType, ValType> const & func) const
{
	for (auto it = func.begin(); it != func.end(); ++it)
	{
		if (!contains(it->first, it->second))
		{
			return false;
		}
	}
	return true;
}

template <typename ArgType, typename ValType>
void createFunctionBands(
	std::vector<iAFunctionBand<ArgType, ValType> > & result,
	std::vector<iAFunction<ArgType, ValType> *> const & functions,
	iAFunctionBand<ArgType, ValType> const & current,
	size_t curIdx,
	size_t curSize)
{
	iAFunctionBand<ArgType, ValType> fb(current);
	fb.merge(*functions[curIdx], curIdx);
	if (curSize == 1)
	{
		result.push_back(fb);
	}
	for (size_t i = curIdx+1; i < functions.size(); ++i)
	{
		if (curSize > 1 && curIdx+curSize-1 < functions.size())
		{
			createFunctionBands(result, functions, fb, i, curSize - 1);
		}
	}
}

template <typename ArgType, typename ValType>
void createFunctionBands(
	std::vector<iAFunctionBand<ArgType, ValType> > & result,
	std::vector<iAFunction<ArgType, ValType> *> const & functions,
	size_t size)
{
	iAFunctionBand<unsigned int, unsigned int> fb;
	for (size_t i = 0; i < functions.size()-size+1; ++i)
	{
		createFunctionBands(result, functions, fb, i, size);
	}
}

//! For comparing functions by the chosen depth measure value in an iAFunctionalBoxplot.
struct iADepthComparator
{
	bool operator()(std::pair<double, size_t> const & first, std::pair<double, size_t> const & second)
	{
		return first.first > second.first;
	}
};

//! Simple depth measure, just counts the number of bands of other function pairs which the function is fully contained in.
//! For details, see paper linked in iAFunctionalBoxplot documentation.
template <typename ArgType, typename ValType>
class iASimpleDepthMeasure: public iADepthMeasure<ArgType, ValType>
{
public:
	double calculate(iAFunction<ArgType, ValType> const & curFunc,
		iAFunction<ArgType, ValType> const & limFunc1,
		iAFunction<ArgType, ValType> const & limFunc2,
		ArgType argMin, ArgType argMax, ArgType argStepSize)
	{
		for (int a=argMin; a<argMax; a += argStepSize)
		{
			if (curFunc.get(a) < std::min(limFunc1.get(a), limFunc2.get(a)) ||
				curFunc.get(a) > std::max(limFunc1.get(a), limFunc2.get(a)))
			{
				return 0.0;
			}
		}
		return (argMax-argMin+1);
	}
};

//! Modified depth measure, sums up the percentages to which the function is contained in each band of other function pairs.
//! For details, see paper linked in iAFunctionalBoxplot documentation.
template <typename ArgType, typename ValType>
class iAModifiedDepthMeasure: public iADepthMeasure<ArgType, ValType>
{
public:
	double calculate(iAFunction<ArgType, ValType> const & curFunc,
		iAFunction<ArgType, ValType> const & limFunc1,
		iAFunction<ArgType, ValType> const & limFunc2)
	{
		double result = 0;

		for (auto it1 = limFunc1.begin(), it2 = limFunc2.begin(), it3 = curFunc.begin();
			it1 != limFunc1.end() && it2 != limFunc2.end() && it3 != curFunc.end(); ++it1, ++it2, ++it3)
		{
			if (it3->second >= std::min(it1->second, it2->second) &&
				it3->second <= std::max(it1->second, it2->second))
			{
				result += 1;
			}
		}

		return result;
	}
};

template <typename ArgType, typename ValType>
iAFunctionalBoxplot<ArgType, ValType>::iAFunctionalBoxplot(std::vector<iAFunction<ArgType, ValType> *> & functions,
	iADepthMeasure<ArgType, ValType>* measure,
	size_t
#ifndef NDEBUG // to silence compiler warning about unused parameter
		maxBandSize
#endif
	)
{
	assert(maxBandSize <= functions.size());
	assert(maxBandSize >= 2);
	assert(functions.size() >= 2);

	double * bandDepth = new double[functions.size()];

	// initialize depth to 0
	for (size_t i = 0; i < functions.size(); ++i)
	{
		bandDepth[i] = 0;
	}

	typedef std::vector<iAFunction<unsigned int, unsigned int> >::const_iterator FuncIt;
	typedef std::vector<iAFunctionBand<unsigned int, unsigned int> >::const_iterator BandIt;

	// set up sampling:

	// start at minimum counts:
	unsigned long long funcStepCnt = iAFunctionalBoxplot::MinBandsPerFct;
	unsigned long long argStepCnt = iAFunctionalBoxplot::MinArgsPerBand;

	// if we have room for more detailed calculations, do it:
	if ((functions.size() * funcStepCnt * funcStepCnt * argStepCnt) < iAFunctionalBoxplot::MaxOverallFctBoxPlotLoops)
	{
		funcStepCnt = std::min(static_cast<unsigned long long>(functions.size()),
			static_cast<unsigned long long>(std::sqrt(static_cast<double>(iAFunctionalBoxplot::MaxOverallFctBoxPlotLoops / (functions.size() * argStepCnt))))
		);

		//if ((functions.size() * funcStepCnt * funcStepCnt * argStepCnt) < iAFunctionalBoxplot::MaxOverallFctBoxPlotLoops)
		//{
		//	argStepCnt = std::min(static_cast<unsigned long long>(argMax-argMin),
		//		static_cast<unsigned long long>(iAFunctionalBoxplot::MaxOverallFctBoxPlotLoops / (functions.size() * funcStepCnt * funcStepCnt))
		//	);
		//}
	}
	// calculate final step sizes:
	unsigned long long funcStepSize = std::sqrt(static_cast<double>((functions.size()*functions.size()) / funcStepCnt));
	//unsigned long long argStepSize = (argMax-argMin) / argStepCnt;
	// factor which should normalize everything to 0..1
	//double normalizeFactor = (2 * funcStepCnt * funcStepCnt * argStepCnt) /
	//	(functions.size()*(functions.size()-1)*(argMax-argMin+1));
	double normalizeFactor = 1.0;

#pragma omp parallel for
	for (long func_nr = 0; func_nr < functions.size(); ++func_nr)
	{
		for (long func1=0; func1 < functions.size()-1; func1 += funcStepSize)
		{
			for (long func2=func1+1; func2 < functions.size(); func2 += funcStepSize)
			{
				if (func_nr != func1 && func_nr != func2)
				{
					bandDepth[func_nr] += measure->calculate(
						*functions[func_nr],
						*functions[func1],
						*functions[func2]);
				}
				else
				{
					bandDepth[func_nr] += functions.size();
				}
			}
		}
		bandDepth[func_nr] *= normalizeFactor;
	}
	std::vector < std::pair<double, size_t> > bandDepthList;

	for (size_t f = 0; f < functions.size(); ++f)
	{
		bandDepthList.push_back(std::pair<double, size_t>(bandDepth[f], f));
	}

	// order function by bd/mbd
	std::sort(bandDepthList.begin(), bandDepthList.end(), iADepthComparator());

	m_median = functions[bandDepthList[0].second];

	size_t centralRegionEnd = bandDepthList.size() / 2;
	size_t envelopeEnd = bandDepthList.size();

	// get band for first 50%
	for (size_t f = 0; f < centralRegionEnd; ++f)
	{
		m_centralRegion.merge(*functions[bandDepthList[f].second], f);
	}

	m_envelope = m_centralRegion;
	for (size_t f = centralRegionEnd; f < envelopeEnd; ++f)
	{
		m_envelope.merge(*functions[bandDepthList[f].second], f );
	}

	/*
	for (auto it = functions[0]->begin(); it != functions[0]->end(); ++it)
	{
		double iqr15 = 1.5*(m_centralRegion.getMax(it->first) - m_centralRegion.getMin(it->first));
		m_envelope.setMin(it->first, std::max(m_envelope.getMin(it->first),
			static_cast<ValType>(m_centralRegion.getMin(it->first) - iqr15 ) ) );
		m_envelope.setMax(it->first, std::min(m_envelope.getMax(it->first),
			static_cast<ValType>(m_centralRegion.getMax(it->first) + iqr15 ) ) );
		// TODO: find next data value which is still inside envelope and use that
	}
	*/

	// determine outliers -> everything outside envelope
	for (size_t f = centralRegionEnd; f < bandDepthList.size(); ++f)
	{
		if (!m_envelope.contains(*functions[bandDepthList[f].second])) {
			m_outliers.push_back(functions[bandDepthList[f].second]);
		}
	}
	delete[] bandDepth;
}

template <typename ArgType, typename ValType>
iAFunction<ArgType, ValType> const & iAFunctionalBoxplot<ArgType, ValType>::getMedian() const
{
	return *m_median;
}

template <typename ArgType, typename ValType>
iAFunctionBand<ArgType, ValType> const & iAFunctionalBoxplot<ArgType, ValType>::getCentralRegion() const
{
	return m_centralRegion;
}

template <typename ArgType, typename ValType>
iAFunctionBand<ArgType, ValType> const & iAFunctionalBoxplot<ArgType, ValType>::getEnvelope() const
{
	return m_envelope;
}

template <typename ArgType, typename ValType>
std::vector<iAFunction<ArgType, ValType> *> const & iAFunctionalBoxplot<ArgType, ValType>::getOutliers() const
{
	return m_outliers;
}
