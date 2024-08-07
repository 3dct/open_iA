// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAThresholdCalculator.h"

#include "iAChartVisHelper.h"
#include "iAThresholdDefinitions.h"

#include <iALog.h>

#include <numeric>
#include <algorithm>

iAThresholdCalculator::iAThresholdCalculator()
{
	m_newDataSeries = nullptr;
}

void iAThresholdCalculator::testPeakDetect()
{
	std::vector<double> data{ 6.1, 8.0, 9.0, 14.1, 10.0,14.3, 12.1, 14.4 };
	double res = m_calcHelper.findMaxPeak(data);
	LOG(lvlInfo, QString("max peak %1").arg(res));
}

threshold_defs::iAThresIndx iAThresholdCalculator::testFindIndex(double value) {
	//						ind 0   1   2    3     4    5     6     7
	std::vector<double> data{ 6.1, 8.0, 9.0, 14.1, 10.0,14.3, 12.1, 14.4 };
	auto res = m_calcHelper.findIndex(data, value);
	return res;
}

void iAThresholdCalculator::testSpecifyRange(const std::vector<double> &v_inRange,
	const std::vector<double> &v_elements, threshold_defs::iAParametersRanges &outputRanges)
{
	double x_min = 2.0;
	double x_max = 8.0;

	this->specifyRange(v_inRange, v_elements, outputRanges, x_min, x_max);
	LOG(lvlInfo, QString("Limits(min, max) %1 %2").arg(x_min).arg(x_max));
}

void iAThresholdCalculator::determinIso50(const threshold_defs::iAParametersRanges& inRanges, threshold_defs::iAThresMinMax& inVals)
{
	m_calcHelper.determinIso50andGlobalMax(inRanges, inVals);
}

QPointF iAThresholdCalculator::determineResultingThresholdBasedOnDecisionRule(const threshold_defs::iAThresMinMax& results, QTextEdit *elem)
{
	return m_minMaxHelper.determineThresholdResultsPointXY(results, elem);
}

void iAThresholdCalculator::doubleTestSum()
{
	std::vector<double> vals = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	double tmp = 0.0f;
	tmp = m_calcHelper.vectorSum(vals, 0, 2);
	LOG(lvlInfo, QString("su 0, 2: %1").arg(tmp));
	tmp = m_calcHelper.vectorSum(vals, 1, 4);
	LOG(lvlInfo, QString("su 1, 4 %1").arg(tmp));
	tmp = m_calcHelper.vectorSum(vals, 4, 6);
	LOG(lvlInfo, QString("su 4, 6 %1").arg(tmp));

}

threshold_defs::iAThresMinMax iAThresholdCalculator::calcMinMax(const threshold_defs::iAParametersRanges& ranges)
{
	return m_calcHelper.calculateLocalePeaks(ranges);
}

void iAThresholdCalculator::retrieveHistData()
{
	if (!m_data)
	{
		throw std::invalid_argument("Error histogram data cannot be retrieved");
	}

	for (size_t idx = 0; idx < m_data->valueCount(); ++idx)
	{
		double binVals_X = m_data->xValue(idx);
		double freq_valsY = m_data->yValue(idx);
		m_thresBinsX.push_back(binVals_X);
		m_freqValsY.push_back(freq_valsY);
	}
}

void iAThresholdCalculator::specifyRange(const std::vector<double>& v_inRef, const std::vector<double> &vals, threshold_defs::iAParametersRanges &outRange, double xmin, double xmax)
{
	size_t vrefLengh = v_inRef.size();
	size_t valsLenght = vals.size();

	if ((vrefLengh == 0) || (valsLenght == 0))
	{
		throw std::invalid_argument("data is empty");
	}
	else if ( (xmin < 0) || xmax <= 0)
	{
		LOG(lvlInfo, QString("size vec1 %1 size vec2 %2 xmin %3 xmax %4").arg(v_inRef.size()).
			arg(vals.size()).arg(xmin).arg(xmax));
		throw std::invalid_argument("invalid parameter input");
	}
	//this is hardcoded
	else if ((xmax > 65535) || (xmax <= xmin))
	{
		QString msg = QString("invalid range or out short %1").arg(xmax);
		throw std::invalid_argument(msg.toStdString().c_str());
	}
	else if (vrefLengh != valsLenght)
	{
		throw std::invalid_argument("size of input and reference vector size are not equal");
	}

	for (size_t ind = 0;ind < vrefLengh; ++ind)
	{
		double val_y = 0;
		double el_x = v_inRef[ind];

		if ((el_x < xmin) || (el_x > xmax))
		{
			continue;
		}
		val_y = vals[ind];
		outRange.insertElem(el_x, val_y);
	}
}


void iAThresholdCalculator::rangeFromParamRanges(const threshold_defs::iAParametersRanges& ranges,
	threshold_defs::iAParametersRanges& outValues, double min, double max)
{
	this->specifyRange(ranges.getXRange(), ranges.getYRange(), outValues, min, max);
}


void iAThresholdCalculator::calculateMovingAverage(const std::vector<double> &v_in, std::vector<double> &v_out, unsigned int count)
{
	//LOG(lvlInfo, "Calculate average");
	if (v_in.size() == 0)
	{
		LOG(lvlInfo, "input length values are zero");
		return;
	}

	double sum = 0.0f;
	double maxLen = v_in.size() - 1;

	if ((count % 2) != 1 || count > maxLen || count < 3)
	{
		LOG(lvlInfo, QString("Moving average size was specified as %1, but it must be >= 3, < %2 and odd!").arg(count).arg(maxLen));
		return;
	}

	for (size_t posInd = 0; posInd < v_in.size(); ++posInd)
	{
		if (posInd < count)
		{
			sum = m_calcHelper.vectorSum(v_in,posInd,posInd+count/2);
		}
		else
		{
			if (posInd >= maxLen - count)
			{
				sum = m_calcHelper.vectorSum(v_in, posInd, posInd - count / 2);
			}
			else
			{
				size_t minPos = posInd - (count / 2);
				size_t maxPos = posInd + (count / 2);
				sum = m_calcHelper.vectorSum(v_in, minPos, maxPos);
			}
		}
		sum /= count;
		v_out.push_back(sum);
	}
}
