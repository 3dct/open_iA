#pragma once
#include <vector>
#include "charts/iAPlotData.h"
#include <QSharedPointer>

//template <class


class ThesholdCalculator
{
public:
	ThesholdCalculator();
	~ThesholdCalculator();

	void calculateAverage(const std::vector<double> &v_in, std::vector<double> &v_out, unsigned int count);
	void doubleTestSum();

	
	void calculateFrequencies(size_t m_start, size_t m_end);

	void histData(); 
	//double findMaxPeak(const std::vector<double>& v_ind);
	
	
	double findMaxPeak(std::vector<double>& v_ind/*, unsigned int toleranceVal*/);
	double findMinPeak(std::vector<double>& v_ind);
	//calculate min and max of the range
	void calcalulateMinMax(const std::vector<double>& v_ind, unsigned int toleranceVal);

	//select values only in the range between min and max
	void specifyRange(const std::vector<double>& v_in, std::vector<double>& v_out, double min, double max);
	void testPeakDetect();
	void setData(QSharedPointer<iAPlotData>& data) {
		m_data = data; 
	}



private:
	double vectorSum(const std::vector<double> &sum, size_t startInd, size_t endInd);
	
	QSharedPointer<iAPlotData> m_data;
	//void testAverage();
	

};

