/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include <cassert>
#include <cmath>   // for ceil/floor
#include <cstddef> // for size_t
#include <limits>  // for std::numeric_limits
#include <vector>

/**
 * make sure the given value is inside the given interval
 * @param min the minimum value which should be returned
 * @param max the maximum value which should be returned
 * @param val the value to check
 * @return min if the given value smaller than min, max if
 *         given value bigger than max, value if it is in
 *         between min and max
 */
template <typename T>
T clamp(T const min, T const max, T const val)
{
	return (val < min) ? min : ((val > max) ? max : val);
}

/**
 * apply minmax normalization
 * if min is bigger than max, a reverse mapping is applied
 * @param minSrcVal minimum value of source interval
 * @param maxSrcVal maximum value of source interval
 * @param value a value in source interval
 * @return the corresponding mapped value = value - minSrcVal / (maxSrcVal - minSrcVal)
 */
template <typename SrcType>
double minMaxNormalize(SrcType const minSrcVal, SrcType const maxSrcVal, SrcType const value)
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

/**
 * map value from given interval to "norm" interval [0..1]
 * if min is bigger than max, a reverse mapping is applied
 * @param minSrcVal minimum value of source interval
 * @param maxSrcVal maximum value of source interval
 * @param value a value in source interval
 * @return if norm was in interval [minSrcVal..maxSrcVal], the
 *     corresponding mapped value in interval [0..1]
 */
template <typename SrcType>
double mapToNorm(SrcType const minSrcVal, SrcType const maxSrcVal, SrcType const value)
{
	double returnVal = minMaxNormalize(minSrcVal, maxSrcVal, value);
	//assert(returnVal >= 0 && returnVal <= 1);
	return clamp(0.0, 1.0, returnVal);
}

template <typename T>
inline T get_t(const T& v, const T& rangeStart, const T& rangeLen)
{
	if (rangeLen == 0)
		return rangeStart;
	return (v - rangeStart) / rangeLen;
}

template <typename T>
inline T mapToNorm(const T * range, const T v)
{
	return mapToNorm(range[0], range[1], v);
}

/**
 * map value from "norm" interval [0..1] to the given interval
 * @param minDstVal minimum value of destination interval
 * @param maxDstVal maximum value of destination interval
 * @param norm a value in interval [0..1]
 * @return if norm was in [0..1], the corresponding mapped value
 *     in interval [minDstVal..maxDstVal]
 */
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
	if ( minDstVal > maxDstVal )
		std::swap( minDstVal, maxDstVal );
	assert (returnVal >= minDstVal && returnVal <= maxDstVal);
	return returnVal;
}

/**
  * map value from one interval to another
  * @param minSrcVal minimum value of source interval
  * @param maxSrcVal maximum value of source interval
  * @param minDstVal minimum value of destination interval
  * @param maxDstVal maximum value of destination interval
  * @param value a value in source interval
  * @return if value was in interval [minSrcVal..maxSrcVal], the
  *     corresponding mapped value in interval [minDstVal..maxDstVal]
  */
template <typename SrcType, typename DstType>
DstType mapValue(SrcType minSrcVal, SrcType maxSrcVal, DstType minDstVal, DstType maxDstVal, SrcType value)
{
	return mapNormTo(minDstVal, maxDstVal, mapToNorm(minSrcVal, maxSrcVal, value));
}

template <typename T>
inline T mapValue(T const * rangeSrc, T const * rangeDst, T const val)
{
	return mapValue(rangeSrc[0], rangeSrc[1], rangeDst[0], rangeDst[1], val);
}

template <typename T>
T invertValue(T const * range, T const val)
{
	return range[1] + range[0] - val;
}

/**
  * round a number to the nearest integer representation (by "round half away from zero" method)
  * @param number the number to round
  * @return the rounded number
  */
template <typename T>
T round(T const & number)
{
	return number < 0.0 ? std::ceil(number - 0.5) : std::floor(number + 0.5);
}

/**
  * linear interpolation in a given range
  * @param a minimum of the interpolation range
  * @param b maximum of the interpolation range
  * @param t a value from the interval [0..1] specifying the interpolation position
  */
template <typename T>
inline T linterp(const T a, const T b, const T t)
{
	return a + (b - a)*t;
}

typedef std::vector<double> FuncType;

//! compute Gaussian function for the given value x and parameter sigma (mean = 0)
open_iA_Core_API double gaussian(double x, double sigma);

//! compute a gaussian kernel with the given sigma (mean = 0)
open_iA_Core_API FuncType gaussianKernel(double kernelSigma, size_t kernelSteps);

//! convolutes the given function with a Gaussian kernel with the given sigma and steps
//! TODO: number of steps could be calculated from sigma (cut off kernel when factor gets very small)
open_iA_Core_API FuncType gaussianSmoothing(FuncType const & data, double kernelSigma, int kernelSteps);

//! calculate first derivative of a given function
open_iA_Core_API FuncType derivative(FuncType const & func);

//! compute the mean of a function
open_iA_Core_API double mean(FuncType const & func);

//! compute the variation of a function. mean can be given (to improve speed)
open_iA_Core_API double variance(FuncType const & func, double meanVal = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! compute the standard deviation of a function. mean can be given (to improve speed)
open_iA_Core_API double standardDeviation(FuncType const & func, double meanVal = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! compute covariance between two functions
open_iA_Core_API double covariance(FuncType const & func1, FuncType const & func2,
	double mean1 = std::numeric_limits<double>::infinity(), double mean2 = std::numeric_limits<double>::infinity(), bool correctDF = true);

//! calculate the Pearson's correlation coefficient between two functions
open_iA_Core_API double pearsonsCorrelationCoefficient(FuncType const & func1, FuncType const & func2);

//! checks whether two boolean values are equal, given a certain tolerance:
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
