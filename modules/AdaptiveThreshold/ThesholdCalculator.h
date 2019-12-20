#pragma once
#include "ThresholdCalcHelper.h"
#include "DebugHelper.h"
#include "ThresMinMaxHelper.h"

#include "charts/iAPlotData.h"

#include <QSharedPointer>
#include <QtCharts>
#include <QtCharts/qlineseries.h>

#include <vector>

class ParametersRanges; 
class QTextEdit; 


struct HistMinMax{
	
	HistMinMax() {
		init(); 
	}
	
	void init() { yMin = 0.0; yMax = 0.0; }
	
	double yMin; 
	double yMax; 
};


//resulting data structure for min and maximum threshold

class ThesholdCalculator
{
public:
	ThesholdCalculator();
	~ThesholdCalculator();

	
	void calculateMovingAverage(const std::vector<double>& v_in, std::vector<double>& v_out, unsigned int count);
	void doubleTestSum();	
	
	threshold_defs::ThresMinMax calcMinMax(const threshold_defs::ParametersRanges &ranges); 

	void retrieveHistData(); 
		

	//select values only in the range between min and max, output parameter ranges(x<double>, y<double>) 
	void specifyRange(const std::vector<double>& v_in, const std::vector<double> &vals, threshold_defs::ParametersRanges &outRange, double xmin, double xmax);
	void rangeFromParamRanges(const threshold_defs::ParametersRanges& ranges, threshold_defs::ParametersRanges& outValues, double min, double max);
	void testPeakDetect();
	

	//searches array for a double value and returns index and value
	threshold_defs::ThresIndx testFindIndex(double value);
	void testSpecifyRange(const std::vector<double>& v_inRange, const std::vector<double>& v_elements, threshold_defs::ParametersRanges& outputRanges);
	
	QString testPrintVector(); 

	
	void setData(QSharedPointer<iAPlotData>& data) {
		m_data = data; 
	}

	inline void setMovingFreqs(const std::vector<double>& freqs) {
		m_movingFreqs = freqs; 
	}

	const std::vector<double> &getThresBins() {
		return this->m_thresBinsX; 
	}

	const std::vector<double> &getFreqValsY() {
		return this->m_freqValsY; 
	}


	//iso 50 - grey value between air peak and material peak
	void determinIso50(const threshold_defs::ParametersRanges& inRanges, 
		threshold_defs::ThresMinMax& inVals);

	inline void setCalculatedResults(const threshold_defs::ThresMinMax& results) {
		m_thresResults = results; 
	}


	inline void getFirstElemInRange(const QVector<QPointF>& pts, double xmin, double xmax, QPointF* pt) {
		m_calcHelper.getFirstElemInRange(pts, xmin, xmax, pt); 
		
	}

	inline void testSort() {

		m_calcHelper.testSortPointsByIdx(); 
	}


	QPointF getPointAirPeakHalf() {
		return m_thresResults.createLokalMaxHalfPoint();
	}


	QPointF determineResultingThresholdBasedOnDecisionRule(const threshold_defs::ThresMinMax& results, QTextEdit* elem);
	const threshold_defs::ThresMinMax& getResults() const {
		return this->m_thresResults; 
	}

	void setIntersectionPoint(const QPointF& pt) {
		m_thresResults.setIntersectionPoint(pt); 
	}


	double getGreyThrPeakAir() {
		return m_thresResults.getAirPeakThr(); 
	}

	double getMaterialsThr() {
		return m_thresResults.getMaterialsThreshold(); 
	}


	double GetResultingThreshold() const { return m_thresResults.DeterminedThreshold(); }
	void SetResultingThreshold(double val) { m_thresResults.DeterminedThreshold(val) ;  }

	//normalize grey values by min max (min = 0, max = 1)
	void performGreyThresholdNormalisation(threshold_defs::ParametersRanges& ranges, double xMin, double xMax) {
		m_calcHelper.PeakgreyThresholdNormalization(ranges, xMin, xMax);
	}


	double getMaxPeakofRange(std::vector<double>& vals) {
		return m_calcHelper.findMaxPeak(vals);
	
	}


	void setNormalizedRange(const threshold_defs::ParametersRanges& normRange) {
		m_NormalizedRanges = normRange; 
	}

	const threshold_defs::ParametersRanges& getNormalizedRangedValues() const {
		return m_NormalizedRanges; 
	
	}

	inline threshold_defs::GreyThresholdPeaks const & getThrPeaksVals() const {
		return m_thresResults.getGreyThresholdPeaks(); 
	
	}

private:
	threshold_defs::ThresMinMax m_thresResults; 
	threshold_defs::ThresMinMaxHelper m_minMaxHelper;

	threshold_defs::ParametersRanges m_NormalizedRanges; 
  
	//double m_resultingThreshold; 

	QSharedPointer<iAPlotData> m_data;
	DebugHelper m_dbgHelper; 
	ThresholdCalcHelper m_calcHelper; 

	std::vector<double> m_thresBinsX; 
	std::vector<double> m_freqValsY; 
	std::vector<double> m_movingFreqs; 

	QLineSeries *m_newDataSeries; 
	
};
