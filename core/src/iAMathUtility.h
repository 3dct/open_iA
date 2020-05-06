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

#include "open_iA_Core_export.h"

#include <algorithm>  // for stable_sort
#include <cassert>
#include <cmath>      // for ceil/floor
#include <cstddef>    // for size_t
#include <limits>     // for std::numeric_limits
#include <numeric>    // for std::accumulate, std::iota
#include <vector>

//! Make sure the given value is inside the given range.
//! @param min the minimum value which should be returned
//! @param max the maximum value which should be returned
//! @param value the value to check
//! @return min if the given value is smaller or equal to min,
//!         max if the given value is bigger or equal to max,
//!         or the value itself if it is in between min and max
template <typename T>
T clamp(T const min, T const max, T const value)
{
	return (value < min) ? min : ((value > max) ? max : value);
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
template <typename T>
inline T mapValue(T const * rangeSrc, T const * rangeDst, T const value)
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

//! Round a number to the nearest integer representation (by "round half away from zero" method).
//! @param number the number to round
//! @return the rounded number
template <typename T>
T round(T const & number)
{
	return number < 0.0 ? std::ceil(number - 0.5) : std::floor(number + 0.5);
}

//! Linear interpolation in a given range.
//! @param a minimum of the interpolation range
//! @param b maximum of the interpolation range
//! @param t a value from the range [0..1] specifying the interpolation position
template <typename T>
inline T linterp(const T a, const T b, const T t)
{
	return a + (b - a)*t;
}

typedef std::vector<double> FuncType;

//! Compute Gaussian function for the given value x and parameter sigma (mean = 0).
open_iA_Core_API double gaussian(double x, double sigma);

//! Compute a gaussian kernel with the given sigma (mean = 0).
open_iA_Core_API FuncType gaussianKernel(double kernelSigma, size_t kernelSteps);

//! Convolutes the given function with a Gaussian kernel with the given sigma and steps.
//! TODO: number of steps could be calculated from sigma (cut off kernel when factor gets very small)
open_iA_Core_API FuncType gaussianSmoothing(FuncType const & data, double kernelSigma, int kernelSteps);

//! Calculate first derivative of a given function.
open_iA_Core_API FuncType derivative(FuncType const & func);

//! Compute the mean of a function.
open_iA_Core_API double mean(FuncType const & func);

//! Compute the variation of a function. If known, mean can be given (to improve speed).
open_iA_Core_API double variance(FuncType const & func, double meanVal = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! Compute the standard deviation of a function. mean can be given (to improve speed).
open_iA_Core_API double standardDeviation(FuncType const & func, double meanVal = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! Compute covariance between two functions.
open_iA_Core_API double covariance(FuncType const & func1, FuncType const & func2,
	double mean1 = std::numeric_limits<double>::infinity(), double mean2 = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! Compute ranks for a given list of values.
open_iA_Core_API FuncType getNormedRanks(FuncType const& func);

//! Calculate the Pearson's correlation coefficient between two functions.
open_iA_Core_API double pearsonsCorrelationCoefficient(FuncType const & func1, FuncType const & func2);

//! Calculate the Spearman's correlation coefficient between two functions.
open_iA_Core_API double spearmansCorrelationCoefficient(FuncType const& func1, FuncType const& func2);

//! Checks whether two real values are equal, given a certain tolerance.
//! inspired by https://stackoverflow.com/a/41405501/671366
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

// source: https://stackoverflow.com/questions/1577475/c-sorting-and-keeping-track-of-indexes
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
