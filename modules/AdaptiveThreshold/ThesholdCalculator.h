#pragma once
#include <vector>
#include "charts/iAPlotData.h"
#include "ThresholdCalcHelper.h"
#include <QSharedPointer>
#include "DebugHelper.h"
#include <QTCharts>
#include <QtCharts/qlineseries.h>

class ParametersRanges; 


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
	void calculateFrequencies(size_t m_start, size_t m_end);
	threshold_defs::ThresMinMax calcMinMax(const threshold_defs::ParametersRanges &ranges); 

	void retrieveHistData(); 
		

	//select values only in the range between min and max, output parameter ranges(x<double>, y<double>) 
	void specifyRange(const std::vector<double>& v_in, const std::vector<double> &vals, threshold_defs::ParametersRanges &outRange, double xmin, double xmax);
	void testPeakDetect();
	

	//searches array for a double value and returns index and value
	threshold_defs::ThresIndx testFindIndex(double value);
	void testSpecifyRange(const std::vector<double>& v_inRange, const std::vector<double>& v_elements, threshold_defs::ParametersRanges& outputRanges);
	
	QString testPrintVector(); 

	void performCalculation(std::vector<double> inputRange, double xmin, double xmax); 


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

	void determinIso50(const threshold_defs::ParametersRanges& inRanges, 
		threshold_defs::ThresMinMax& inVals);

	inline void setThresMinMax(const threshold_defs::ThresMinMax& results) {
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

private:
	threshold_defs::ThresMinMax m_thresResults; 
  

	QSharedPointer<iAPlotData> m_data;
	DebugHelper m_dbgHelper; 
	ThresholdCalcHelper m_calcHelper; 

	std::vector<double> m_thresBinsX; 
	std::vector<double> m_freqValsY; 
	std::vector<double> m_movingFreqs; 

	QLineSeries *m_newDataSeries; 
	
};

