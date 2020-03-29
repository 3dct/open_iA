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

#include "iAThresholdCalcHelper.h"
#include "iAThreshMinMaxHelper.h"

#include <charts/iAPlotData.h>

#include <QSharedPointer>
#include <QtCharts>
#include <QtCharts/qlineseries.h>

#include <vector>

class iAParametersRanges;
class QTextEdit;

//resulting data structure for min and maximum threshold
class iAThresholdCalculator
{
public:
	iAThresholdCalculator();

	void calculateMovingAverage(const std::vector<double>& v_in, std::vector<double>& v_out, unsigned int count);
	void doubleTestSum();
	threshold_defs::iAThresMinMax calcMinMax(const threshold_defs::iAParametersRanges &ranges);
	void retrieveHistData();

	//! select values only in the range between min and max, output parameter ranges(x<double>, y<double>)
	void specifyRange(const std::vector<double>& v_in, const std::vector<double> &vals, threshold_defs::iAParametersRanges &outRange, double xmin, double xmax);
	void rangeFromParamRanges(const threshold_defs::iAParametersRanges& ranges, threshold_defs::iAParametersRanges& outValues, double min, double max);
	void testPeakDetect();

	//! searches array for a double value and returns index and value
	threshold_defs::iAThresIndx testFindIndex(double value);
	void testSpecifyRange(const std::vector<double>& v_inRange, const std::vector<double>& v_elements, threshold_defs::iAParametersRanges& outputRanges);

	void setData(QSharedPointer<iAPlotData>& data)
	{
		m_data = data;
	}

	inline void setMovingFreqs(const std::vector<double>& freqs)
	{
		m_movingFreqs = freqs;
	}

	const std::vector<double> &getThresBins()
	{
		return this->m_thresBinsX;
	}

	const std::vector<double> &getFreqValsY()
	{
		return this->m_freqValsY;
	}


	//iso 50 - grey value between air peak and material peak
	void determinIso50(const threshold_defs::iAParametersRanges& inRanges,
		threshold_defs::iAThresMinMax& inVals);

	inline void setCalculatedResults(const threshold_defs::iAThresMinMax& results)
	{
		m_thresResults = results;
	}

	inline void getFirstElemInRange(const QVector<QPointF>& pts, double xmin, double xmax, QPointF* pt)
	{
		m_calcHelper.getFirstElemInRange(pts, xmin, xmax, pt);
	}

	inline void testSort()
	{
		m_calcHelper.testSortPointsByIdx();
	}

	QPointF getPointAirPeakHalf()
	{
		return m_thresResults.createLokalMaxHalfPoint();
	}

	QPointF determineResultingThresholdBasedOnDecisionRule(const threshold_defs::iAThresMinMax& results, QTextEdit* elem);
	const threshold_defs::iAThresMinMax& getResults() const
	{
		return this->m_thresResults;
	}

	void setIntersectionPoint(const QPointF& pt)
	{
		m_thresResults.setIntersectionPoint(pt);
	}

	double getGreyThrPeakAir()
	{
		return m_thresResults.getAirPeakThr();
	}

	double getMaterialsThr()
	{
		return m_thresResults.getMaterialsThreshold();
	}

	double GetResultingThreshold() const { return m_thresResults.DeterminedThreshold(); }
	void SetResultingThreshold(double val) { m_thresResults.DeterminedThreshold(val) ;  }

	//normalize grey values by min max (min = 0, max = 1)
	void performGreyThresholdNormalisation(threshold_defs::iAParametersRanges& ranges, double xMin, double xMax)
	{
		m_calcHelper.PeakgreyThresholdNormalization(ranges, xMin, xMax);
	}

	double getMaxPeakofRange(std::vector<double>& vals)
	{
		return m_calcHelper.findMaxPeak(vals);
	}

	void setNormalizedRange(const threshold_defs::iAParametersRanges& normRange)
	{
		m_NormalizedRanges = normRange;
	}

	const threshold_defs::iAParametersRanges& getNormalizedRangedValues() const
	{
		return m_NormalizedRanges;
	}

	inline threshold_defs::iAGreyThresholdPeaks const & getThrPeaksVals() const
	{
		return m_thresResults.getGreyThresholdPeaks();
	}

private:
	threshold_defs::iAThresMinMax m_thresResults;
	threshold_defs::iAThreshMinMaxHelper m_minMaxHelper;

	threshold_defs::iAParametersRanges m_NormalizedRanges;

	QSharedPointer<iAPlotData> m_data;
	iAThresholdCalcHelper m_calcHelper;

	std::vector<double> m_thresBinsX;
	std::vector<double> m_freqValsY;
	std::vector<double> m_movingFreqs;

	QLineSeries *m_newDataSeries;
};

