#pragma once
#include <vector>
#include "charts/iAPlotData.h"
#include <QSharedPointer>
#include "DebugHelper.h"


struct HistMinMax{
	
	HistMinMax() {
		init(); 
	}
	
	void init() { yMin = 0.0; yMax = 0.0; }
	
	double yMin; 
	double yMax; 
};

class ParametersRanges {

public:
	ParametersRanges() {
		x_vals.reserve(1000);
		y_vals.reserve(1000);
	}

	void insertElem(double x, double y) {
		x_vals.push_back(x);
		y_vals.push_back(y);
	}


	const std::vector<double>& getXRange() {
		return x_vals;
	}
	const std::vector<double>& getYRange() {
		return y_vals;
	}

private:
	std::vector<double> x_vals;
	std::vector<double> y_vals;
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
	void specifyRange(const std::vector<double>& v_in, const std::vector<double> &vals, std::vector<double>& v_out, double xmin, double xmax);
	void testPeakDetect();
	void testSpecifyRange(); 


	/*TODO specify input range: min max
	-> determine peaks min max
	-> return min max
	*/
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
	QSharedPointer<iAPlotData> m_data;
	DebugHelper m_dbgHelper; 	

	std::vector<double> m_thresBinsX; 
	std::vector<double> m_freqValsY; 
	std::vector<double> m_movingFreqs; 
	
};

