/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include <cassert>
#include <cmath>

// consistently define isNaN/isInf:
#if (defined(_MSC_VER) && _MSC_VER <= 1600)
	#define isNaN(x) _isnan(x)
	#define isInf(x) (!_finite(x))
#elif (defined(__GNUG__))
	#define isNaN(x) std::isnan(x)
	#define isInf(x) std::isinf(x)
#else
	#define isNaN(x) isnan(x)
	#define isInf(x) isinf(x)
#endif

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
	//assert (value >= minSrcVal && value <= maxSrcVal);
	SrcType range = maxSrcVal - minSrcVal;
	if (range == 0)
	{	// to prevent division by 0
		return 0;
	}
	double returnVal = static_cast<double>(value - minSrcVal) / range;
	assert(returnVal >= 0 && returnVal <= 1);
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
