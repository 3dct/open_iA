#pragma once
#include <vector>
#include "charts/iAPlotData.h"
#include "ThresholdCalcHelper.h"
#include <QSharedPointer>
#include "DebugHelper.h"
#include <QTCharts>
#include <QtCharts/qlineseries.h>

//#include "ThresholdDefinitions.h"

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

	//ouble vectorSum(const std::vector<double>& vec, size_t startInd, size_t endInd);
	void determineMovingAverage(const std::vector<double>& v_in, std::vector<double>& v_out, unsigned int count);
	void doubleTestSum();	
	void calculateFrequencies(size_t m_start, size_t m_end);
	void retrieveHistData(); 
		

	//select values only in the range between min and max
	void specifyRange(const std::vector<double>& v_in, const std::vector<double> &vals, threshold_defs::ParametersRanges &outRange, double xmin, double xmax);
	void testPeakDetect();
	

	//searches array for a double value and returns index and value
	threshold_defs::ThresIndx testFindIndex(double value);
	void testSpecifyRange(const std::vector<double>& v_inRange, const std::vector<double>& v_elements, threshold_defs::ParametersRanges& outputRanges);
	
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




private:
		
	QSharedPointer<iAPlotData> m_data;
	DebugHelper m_dbgHelper; 
	ThresholdCalcHelper m_calcHelper; 

	std::vector<double> m_thresBinsX; 
	std::vector<double> m_freqValsY; 
	std::vector<double> m_movingFreqs; 

	QLineSeries *m_newDataSeries; 
	
};

