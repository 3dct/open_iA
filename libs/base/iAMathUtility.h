// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAbase_export.h"

#include <algorithm>  // for stable_sort
#include <cassert>
#include <cmath>      // for ceil/floor
#include <cstddef>    // for size_t
#include <limits>     // for std::numeric_limits
#include <numeric>    // for std::accumulate, std::iota
#include <vector>

//! Make sure the given value is inside the given range.
//! @param minVal the minimum value which should be returned
//! @param maxVal the maximum value which should be returned
//! @param value the value to check
//! @return min if the given value is smaller or equal to min,
//!         max if the given value is bigger or equal to max,
//!         or the value itself if it is in between min and max
template <typename T>
T clamp(T const minVal, T const maxVal, T const value)
{
	return (value < minVal) ? minVal : ((value > maxVal) ? maxVal : value);
}

//! Check whether a value is in a given range.
//! @param range the range given as minimum (stored at index 0) and maximum (stored at index 1)
//! @param value the value to be checked.
//! return true if value in range (bounds included, i.e. returns true if equal to minimum or maximum), false otherwise
template <typename T>
bool inRange(T const* range, T const value)
{
	return range[0] <= value && value <= range[1];
}

//! for numVal entries in two arrays, compute component-wise minimum and maximum
//! @param minVal array with numVal entries, will hold the minimum value for each component in given val1/val2 array
//! @param maxVal array with numVal entries, will hold the maximum value for each component in given val1/val2 array
//! @param val1 array with numVal entries
//! @param val2 array with numVal entries
//! @param size
//! @param numVal number of values in minVal, maxVal, val1, val2, and size
template <typename T>
void computeMinMax(T* minVal, T* maxVal, T const* val1, T const* val2, T const * size, int numVal)
{
	for (int c = 0; c < numVal; ++c)
	{
		minVal[c] = clamp(0, size[c] - 1, val1[c] <= val2[c] ? val1[c] : val2[c]);
		maxVal[c] = clamp(0, size[c] - 1, val1[c] <= val2[c] ? val2[c] : val1[c]);
	}
}

//! Map value from given interval to "norm" interval [0..1].
//! if min is bigger than max, a reverse mapping is applied
//! @param minSrcVal minimum value of source interval
//! @param maxSrcVal maximum value of source interval
//! @param value a value in source interval
//! @return the corresponding mapped value = (value - minSrcVal) / (maxSrcVal - minSrcVal)
//!         Note that if the input value is not between minSrcVal and maxSrcVal,
//!         the return value is NOT guaranteed to be in the interval 0..1. If you require
//!         the return value to be in that interval, use mapToNorm instead!
template <typename SrcType>
double mapToNormNoClamp(SrcType const minSrcVal, SrcType const maxSrcVal, SrcType const value)
{
	//assert (value >= minSrcVal && value <= maxSrcVal);
	SrcType range = maxSrcVal - minSrcVal;
	if (range == 0)
	{	// to prevent division by 0
		return 0;
	}
	double returnVal = static_cast<double>(value - minSrcVal) / range;
	return returnVal;
}

//! Map value from given interval to "norm" interval [0..1].
//! if min is bigger than max, a reverse mapping is applied
//! @param minSrcVal minimum value of source interval
//! @param maxSrcVal maximum value of source interval
//! @param value a value in source interval
//! @return the corresponding mapped value in interval [0..1]
//!     (0 if input value < minSrcVal, 1 if input value > maxSrcVal)
template <typename SrcType>
double mapToNorm(SrcType const minSrcVal, SrcType const maxSrcVal, SrcType const value)
{
	double returnVal = mapToNormNoClamp(minSrcVal, maxSrcVal, value);
	//assert(returnVal >= 0 && returnVal <= 1);
	return clamp(0.0, 1.0, returnVal);
}


//! Map values from a given range to the "normalized" range [0..1].
//! if min is bigger than max, a reverse mapping is applied
//! @param range an array of size 2 with minimum and maximum value of the source range
//! @param value the value to be mapped from the source range to the destination range
//! @return the mapped value; in case the given value is outside given min/max,
//!         the value will still be clamped to the range [0..1]
template <typename T>
inline T mapToNorm(const T * range, const T value)
{
	return mapToNorm(range[0], range[1], value);
}

//! Map value from "norm" range [0..1] to the given range.
//! @param minDstVal minimum value of destination range
//! @param maxDstVal maximum value of destination range
//! @param norm a value in range [0..1]
//! @return if norm was in [0..1], the corresponding mapped value
//!     in range [minDstVal..maxDstVal]
template <typename DstType>
DstType mapNormTo(DstType minDstVal, DstType maxDstVal, double norm)
{
	double returnVal;
	// to avoid numerical inaccuraccies:
	if (norm == 0.0)
	{
		returnVal = minDstVal;
	}
	else if (norm == 1.0)
	{
		returnVal = maxDstVal;
	}
	else
	{
		returnVal = norm * (maxDstVal - minDstVal) + minDstVal;
	}
#ifndef NDEBUG
	if (minDstVal > maxDstVal)
	{
		std::swap(minDstVal, maxDstVal);
	}
	assert (returnVal >= minDstVal && returnVal <= maxDstVal);
#endif
	return returnVal;
}

 //! Map value from one range to another.
 //! @param minSrcVal minimum value of source range
 //! @param maxSrcVal maximum value of source range
 //! @param minDstVal minimum value of destination range
 //! @param maxDstVal maximum value of destination range
 //! @param value the value to be mapped from the source range to the destination range
 //! @return if value was in range [minSrcVal..maxSrcVal], the
 //!     corresponding mapped value in range [minDstVal..maxDstVal]
template <typename SrcType, typename DstType>
DstType mapValue(SrcType minSrcVal, SrcType maxSrcVal, DstType minDstVal, DstType maxDstVal, SrcType value)
{
	return mapNormTo(minDstVal, maxDstVal, mapToNorm(minSrcVal, maxSrcVal, value));
}

//! Map value from one range to another.
//! @param rangeSrc an array of size 2 with minimum and maximum value of the source range
//! @param rangeDst  an array of size 2 with minimum and maximum value of the destination range
//! @param value the value to be mapped from the source range to the destination range
//! @return if value was in range [minSrcVal..maxSrcVal], the
//!     corresponding mapped value in range [minDstVal..maxDstVal]
template <typename SrcType, typename DstType>
inline DstType mapValue(SrcType const * rangeSrc, DstType const * rangeDst, SrcType const value)
{
	return mapValue(rangeSrc[0], rangeSrc[1], rangeDst[0], rangeDst[1], value);
}

//! Invert value in a given range.
//! Example: invertValue(range=[0, 1], value=0.2) = 0.8; invertValue(range=[1,2], value=1.3) = 1.7
//! @param range an array of size 2 with minimum and maximum value of the range
//! @param value the value to be inverted.
//! @return the inverted value
template <typename T>
T invertValue(T const * range, T const value)
{
	return range[1] + range[0] - value;
}

//#if __cplusplus >= 202002L
// TODO: replace with std::lerp when C++ standard is raised to C++20
//#endif
//! Linear interpolation in a given range.
//! @param a minimum of the interpolation range
//! @param b maximum of the interpolation range
//! @param t a value from the range [0..1] specifying the interpolation position
template <typename T>
T linterp(const T a, const T b, const T t)
{
	return a + (b - a)*t;
}

//! Compute the fractional part of a floating-point type
template <typename T>
T frac(T val)
{
	return val - std::trunc(val);
}

typedef std::vector<double> FuncType;

//! Compute Gaussian function for the given value x and parameter sigma (mean = 0).
iAbase_API double gaussian(double x, double sigma);

//! Compute a gaussian kernel with the given sigma (mean = 0).
iAbase_API FuncType gaussianKernel(double kernelSigma, size_t kernelSteps);

//! Convolutes the given function with a Gaussian kernel with the given sigma and steps.
//! TODO: number of steps could be calculated from sigma (cut off kernel when factor gets very small)
iAbase_API FuncType gaussianSmoothing(FuncType const& data, double kernelSigma, int kernelSteps);

//! Calculate first derivative of a given function.
iAbase_API FuncType derivative(FuncType const& func);

//! Compute the mean of a function.
iAbase_API double mean(FuncType const& func);

//! Compute the variation of a function. If known, mean can be given (to improve speed).
iAbase_API double variance(FuncType const & func, double meanVal = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! Compute the standard deviation of a function. mean can be given (to improve speed).
iAbase_API double standardDeviation(FuncType const & func, double meanVal = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! Compute covariance between two functions.
iAbase_API double covariance(FuncType const & func1, FuncType const & func2,
	double mean1 = std::numeric_limits<double>::infinity(), double mean2 = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! Compute ranks for a given list of values.
iAbase_API FuncType getNormedRanks(FuncType const& func);

//! Calculate the Pearson's correlation coefficient between two functions.
iAbase_API double pearsonsCorrelationCoefficient(FuncType const & func1, FuncType const & func2);

//! Calculate the Spearman's correlation coefficient between two functions.
iAbase_API double spearmansCorrelationCoefficient(FuncType const& func1, FuncType const& func2);

//! Checks whether two real values are equal, given a certain tolerance.
//! inspired by great points about floating point equals at https://stackoverflow.com/a/41405501
template <typename RealType>
bool dblApproxEqual(RealType a, RealType b, RealType tolerance = std::numeric_limits<RealType>::epsilon())
{
	RealType diff = std::abs(a-b);
	return diff < tolerance;
	//if (diff < tolerance)
	// return true;
	// TODO: Test!
	//return ((a>1 || b>1) && (diff < std::max(std::fabs(a), std::abs(b)) * tolerance));
}

//! comparison of two real type values - merge with above!
template <typename T1, typename T2>
bool isApproxEqual(T1 a, T2 b, T1 tolerance = std::numeric_limits<T1>::epsilon())
{
	T1 diff = std::fabs(a - b);
	if (diff <= tolerance)
	{
		return true;
	}
	if (diff < std::fmax(std::fabs(a), std::fabs(b)) * tolerance)
	{
		return true;
	}

	return false;
}

// source: https://stackoverflow.com/questions/1577475
template <typename T>
std::vector<size_t> sort_indexes(const std::vector<T>& v)
{
	// initialize original index locations
	std::vector<size_t> idx(v.size());
	iota(idx.begin(), idx.end(), 0);

	// sort indexes based on comparing values in v
	// using std::stable_sort instead of std::sort
	// to avoid unnecessary index re-orderings
	// when v contains elements of equal values
	std::stable_sort(idx.begin(), idx.end(),
		[&v](size_t i1, size_t i2) {return v[i1] < v[i2]; });

	return idx;
}

// TODO: policy regarding std::vector / QVector usage!
#include <QVector>

//! compute histogram from given values
// TODO: use in iAHistogramData::create
template<typename OutContainerT=QVector<double>, typename ValueT=double, typename InContainerT>
OutContainerT createHistogram(const InContainerT& inData, typename OutContainerT::size_type binCount,
	ValueT & minValue, ValueT & maxValue, bool discrete = false)
{
	if (std::isinf(minValue))
	{
		minValue = *std::min_element(inData.begin(), inData.end());
	}
	if (std::isinf(maxValue))
	{
		maxValue = *std::max_element(inData.begin(), inData.end());
	}
	if (dblApproxEqual(minValue, maxValue))
	{   // if min == max, there is only one bin - one in which all values are contained!
		return OutContainerT(1, inData.size());
	}
	if (discrete)
	{
		binCount = std::min(binCount, static_cast<typename OutContainerT::size_type>(maxValue - minValue + 1) );
	}
	OutContainerT result;
	result.fill(0, binCount);
	using IdxT = typename OutContainerT::size_type;
	for (ValueT d : inData)
	{
		IdxT bin = clamp(static_cast<IdxT>(0), binCount - 1, mapValue(minValue, maxValue, static_cast<IdxT>(0), binCount, d));
		++result[bin];
	}
	return result;
}
