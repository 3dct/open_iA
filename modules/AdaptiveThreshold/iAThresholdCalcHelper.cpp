// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAThresholdCalcHelper.h"

#include "iAThresholdDefinitions.h"

#include <iALog.h>
#include <iAMathUtility.h>

#include <QLine>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace
{
	bool greaterThan(double u, double v)
	{
		return u > v;
	}

	bool smallerThan(double u, double v)
	{
		return u < v;
	}

	const double dblEpsilon = 0.0000000001;
	const float  fltEpsilon = 0.0000000001f;
}

double iAThresholdCalcHelper::findMaxPeak(std::vector<double>& v_ind) const
{
	std::sort(v_ind.begin(), v_ind.end(), greaterThan);
	auto peak = std::adjacent_find(v_ind.begin(), v_ind.end(), std::greater<double>());

	if (peak == v_ind.end())
	{
		--peak;
	}

	return *peak;
}

double iAThresholdCalcHelper::findMinPeak(std::vector<double>& v_ind) const
{
	std::sort(v_ind.begin(), v_ind.end(), smallerThan);
	auto peak = std::adjacent_find(v_ind.begin(), v_ind.end(), std::less<double>());

	if (peak == v_ind.end())
	{
		--peak;
	}

	return *peak;
}

double iAThresholdCalcHelper::vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd)
{
	if (startInd >= vec.size() || endInd >= vec.size())
	{
		throw std::invalid_argument("test");
	}
	double tmp = 0.0f;

	size_t i = startInd;
	while (i <= endInd)
	{
		tmp += vec[i];
		++i;
	}

	return tmp;
}

threshold_defs::iAThresIndx iAThresholdCalcHelper::findIndex(const std::vector<double>& vec, double cmpVal) const
{
	threshold_defs::iAThresIndx thrInd;
	long ind = 0;
	thrInd.value = cmpVal;
	if (vec.empty())
	{
		thrInd.thrIndx = -1;
	}

	for (const double& el : vec)
	{
		bool isEqual = dblApproxEqual(el, cmpVal, dblEpsilon);
		if (isEqual)
		{
			thrInd.thrIndx = ind;
			break;
		}
		else
		{
			thrInd.thrIndx = -1;
		}
		ind++;
	}
	return thrInd;
}

threshold_defs::iAThresMinMax iAThresholdCalcHelper::calculateLocalePeaks(const threshold_defs::iAParametersRanges& inRanges) const
{
	std::vector<double> yRange = inRanges.getYRange();
	std::vector<double> xRange = inRanges.getXRange();

	//or copy this because the array is sorted
	double y_max = this->findMaxPeak(yRange);
	double y_min = this->findMinPeak(yRange);

	const auto indMax = this->findIndex(inRanges.getYRange(), y_max);
	const auto indMin = this->findIndex(inRanges.getYRange(), y_min);

	if ((indMin.thrIndx == -1) || (indMax.thrIndx == -1))
	{
		throw std::invalid_argument("index out of range");
	}

	double x_max = xRange[indMax.thrIndx];
	double x_min = xRange[indMin.thrIndx];

	threshold_defs::iAThresMinMax thrMinMax;

	thrMinMax.FreqPeakLokalMaxY(y_max);
	thrMinMax.FreqPeakMinY(y_min);
	thrMinMax.PeakMinXThreshold(x_min);
	thrMinMax.LokalMaxPeakThreshold_X(x_max);
	return thrMinMax;
}

void iAThresholdCalcHelper::determinIso50andGlobalMax(const threshold_defs::iAParametersRanges& inRanges, threshold_defs::iAThresMinMax &inVals)
{
	//min_Max of a Range
	//detect peak max
	//iso 50 is between air and material peak - grauwert

	std::vector<double> freqRangesY = inRanges.getYRange();
	double maxRange = this->findMaxPeak(freqRangesY); //maximum

	threshold_defs::iAThresIndx indMinMax = findIndex(inRanges.getYRange(), maxRange);
	if (indMinMax.thrIndx < 0)
	{
		return;
	}

	double maxPeakThres = inRanges.getXRange()[indMinMax.thrIndx];
	double Iso50Val = (maxPeakThres + inVals.LokalMaxPeakThreshold_X()) * 0.5f;
	inVals.setMaterialsThreshold(maxPeakThres);
	inVals.Iso50ValueThr(Iso50Val);
}


void iAThresholdCalcHelper::getFirstElemInRange(const QVector <QPointF>& in, float xmin, float xmax, QPointF* result)
{
	if (!result)
	{
		throw std::invalid_argument("null argument QPointF");
	}
	if (in.empty())
	{
		result = nullptr;
		return;
	}
	try
	{
		QVector<QPointF> ranges = in;
		sortPointsByX(ranges);
		bool contained = false;
		QPointF pt_tmp;

		for (const QPointF& pt : ranges)
		{
			if (this->checkInRange(pt, xmin, xmax))
			{
				contained = true;
				pt_tmp = pt;
				break;
			}
		}

		if (contained)
		{
			result->setX(pt_tmp.x());
			result->setY(pt_tmp.y());
		}
		else
		{
			result = nullptr;
		}
	}
	catch (std::bad_alloc& /*ba*/)
	{
		LOG(lvlInfo, "error calculation elem by ranges faild in memory");
		throw;
	}
}

void iAThresholdCalcHelper::PeakgreyThresholdNormalization(threshold_defs::iAParametersRanges& ranges, double greyThrPeakAir, double greyThrPeakMax)
{
	if (greyThrPeakAir < std::numeric_limits<double>::min() || (greyThrPeakAir > std::numeric_limits<double>::max()))
	{
		LOG(lvlInfo, QString("grey value threshold invalid %1").arg(greyThrPeakAir));
		return;
	}

	if (greyThrPeakAir > greyThrPeakMax)
	{
		LOG(lvlInfo, "grey value of air peak must be smaller than matierial peak, please change order");
		return;
	}

	std::vector<double> tmp_ranges_x = ranges.getXRange();
	if (tmp_ranges_x.empty())
	{
		return;
	}

	for (double& val: tmp_ranges_x)
	{
		//LOG(lvlInfo, QString("before %1").arg(val));
		val = mapToNorm(greyThrPeakAir, greyThrPeakMax, val);
		//LOG(lvlInfo, QString("after %1").arg(val))
	}

	ranges.setXVals(tmp_ranges_x);

}

bool iAThresholdCalcHelper::checkInRange(const QPointF& pt, float min, float max)
{
	float xval = static_cast<float>(pt.x());
	bool isInRangeMin = false;
	bool isInRangeMax = false;

	if (!(min < max))
	{
		throw std::invalid_argument("error comparing values");
	}

	isInRangeMin = (dblApproxEqual(min, xval, fltEpsilon)) || (min < xval);
	isInRangeMax = (dblApproxEqual(max, xval, fltEpsilon)) || (xval < max);

	return (isInRangeMax && isInRangeMin);

}
