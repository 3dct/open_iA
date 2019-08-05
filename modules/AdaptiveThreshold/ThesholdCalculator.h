#pragma once
#include <vector>
#include "charts/iAPlotData.h"
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

struct ThresIndx {
	ThresIndx() {
		thrIndx = -std::numeric_limits<long int>::infinity();
		value = -std::numeric_limits<double>::infinity();
	}

	long int thrIndx;
	double value; 

};



class ThesholdCalculator
{
public:
	ThesholdCalculator();
	~ThesholdCalculator();

	void determineMovingAverage(const std::vector<double> &v_in, std::vector<double> &v_out, unsigned int count);
	void doubleTestSum();	
	void calculateFrequencies(size_t m_start, size_t m_end);
	void retrieveHistData(); 
	
	
	double findMaxPeak(std::vector<double>& v_ind/*, unsigned int toleranceVal*/);
	double findMinPeak(std::vector<double>& v_ind);

	//calculate min and max of the range input range
	//void calcalulateMinMax(const std::vector<double>& v_ind, unsigned int toleranceVal);

	//select values only in the range between min and max
	void specifyRange(const std::vector<double>& v_in, const std::vector<double> &vals, ParametersRanges &outRange, double xmin, double xmax);
	void testPeakDetect();
	

	//searches array in a double value and returns index and value
	ThresIndx testFindIndex(double value);
	void testSpecifyRange(const std::vector<double>& v_inRange, const std::vector<double>& v_elements, ParametersRanges& outputRanges);
	
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
	double vectorSum(const std::vector<double> &sum, size_t startInd, size_t endInd);
	
	inline bool compareDouble(double a, double b) {
		return fabs(a - b) < epsilon; 
	}

	ThresIndx findIndex(const std::vector<double>& vec, double elem);
	const double epsilon = 0.0000000001; 

	//void createDataVisualisation(const std::vector<double> v_x, const std::vector<double> v_y);
	
	QSharedPointer<iAPlotData> m_data;
	DebugHelper m_dbgHelper; 	

	std::vector<double> m_thresBinsX; 
	std::vector<double> m_freqValsY; 
	std::vector<double> m_movingFreqs; 

	QLineSeries *m_newDataSeries; 
	
};

