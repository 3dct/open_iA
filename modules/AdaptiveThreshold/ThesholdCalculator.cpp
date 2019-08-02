#include "ThesholdCalculator.h"
#include <numeric>
#include <algorithm>
#include "iAConsole.h"
#include "ThresholdDefinitions.h"
#include "ChartVisHelper.h"

namespace algorithm {
	static bool greaterThan(double u, double v) {
		return u > v;
	}

	static bool smallerThan(double u, double v) {
		return u < v;
	}

}



ThesholdCalculator::ThesholdCalculator()
{
	m_newDataSeries = nullptr; 
}

ThesholdCalculator::~ThesholdCalculator()
{
}

double ThesholdCalculator::vectorSum(const std::vector<double> &vec, size_t startInd, size_t endInd)
{
	if (startInd >= vec.size() || endInd >= vec.size()) throw new std::invalid_argument("test"); 
	double tmp = 0.0f; 
	
	size_t i = startInd; 
	while (i <= endInd){
		
		tmp += vec[i];
		++i;
	}

	return tmp; 
}

ThresIndx ThesholdCalculator::findIndex(const std::vector<double>& vec, double cmpVal)
{
	ThresIndx thrInd; 
	size_t ind = 0; 
	thrInd.value = cmpVal; 

	for(const double &el:vec){
		bool isEqual = this->compareDouble(el, cmpVal); 
		if (isEqual) {
			thrInd.thrIndx = ind; 
			break;
		}

		ind++;
	}

	return thrInd; 

}


void ThesholdCalculator::testPeakDetect()
{
	std::vector<double> data{ 6.1, 8.0, 9.0, 14.1, 10.0,14.3, 12.1, 14.4 };
	double res = this->findMaxPeak(data); 
	DEBUG_LOG(QString("max peak %1").arg(res)) 
}

void ThesholdCalculator::testFindIndex(double value) {
	//						ind 0   1   2    3     4    5     6     7 
	std::vector<double> data{ 6.1, 8.0, 9.0, 14.1, 10.0,14.3, 12.1, 14.4 };
	auto res = this->findIndex(data, value);

	DEBUG_LOG(QString("ind%1 val%2").arg(res.thrIndx).arg(res.value));
}

void ThesholdCalculator::testSpecifyRange(const std::vector<double> &v_inRange, 
	const std::vector<double> &v_elements, ParametersRanges &outputRanges)
{
	 

	double x_min = 2.0; 
	double x_max = 8.0; 

	this->specifyRange(v_inRange, v_elements, outputRanges, x_min, x_max);
	DEBUG_LOG(QString("Limits(min, max) %1 %2").arg(x_min).arg(x_max)); 
	DEBUG_LOG("input vector");
	m_dbgHelper.debugVector(v_elements); 

	DEBUG_LOG("output vector");
	
	
	//m_dbgHelper.debugVector(outputRanges);
	
}

void ThesholdCalculator::performCalculation(std::vector<double> inputRange, double xmin, double xmax)
{
	//double xmin = 0;
	//double xmax = 0; 
	/*std::vector<double> vals_out; 
	specifyRange(m_thresBinsX, m_movingFreqs, vals_out, xmin, xmax);

	double min = this->findMinPeak(vals_out);
	double max = this->findMaxPeak(vals_out);*/
	//this->calcalulateMinMax(vals_out, 10);

}

void ThesholdCalculator::doubleTestSum()
{
	std::vector<double> vals = { 1, 2, 3, 4, 5, 6, 7, 8, 9 }; 
	double tmp = 0.0f; 
	tmp = this->vectorSum(vals, 0, 2);
	DEBUG_LOG(QString("su 0, 2: %1").arg(tmp));
	tmp = this->vectorSum(vals, 1, 4);
	DEBUG_LOG(QString("su 1, 4 %1").arg(tmp)); 
	tmp = this->vectorSum(vals, 4, 6);
	DEBUG_LOG(QString("su 4, 6 %1").arg(tmp)); 

}

void ThesholdCalculator::calculateFrequencies(size_t m_start, size_t m_end)
{
	//for (size_t m_i = (m_start + 1); m_i < (m_end - 1); m_i++)
	//{
	//	if (m_VolumeCount[m_i] > m_high_freq)
	//	{
	//		m_high_freq = (int) m_VolumeCount[m_i];
	//		m_high_intensity = m_i;
	//	}
	//}

	////find the low intensity peak
	//for (m_i = (m_start + 1); m_i < m_centre; m_i++)
	//{
	//	if (m_VolumeCount[m_i] > m_low_freq)
	//	{
	//		m_low_freq = m_VolumeCount[m_i];
	//		m_low_intensity = m_i;
	//	}
	//}
}



void ThesholdCalculator::retrieveHistData()
{
	if (!m_data)
		throw std::invalid_argument("Error data cannot be retrieved");

	double binVals_X;
	double freq_valsY;

	//goes over all bins;

	for (int b = 0; b < m_data->numBin(); ++b) {
		binVals_X = m_data->binStart(b);
		freq_valsY = m_data->rawData()[b];
		m_thresBinsX.push_back(binVals_X);
		m_freqValsY.push_back(freq_valsY); 

	}
}

double ThesholdCalculator::findMaxPeak(std::vector<double>& v_ind/*, unsigned int toleranceVal*/)
{
	std::sort(v_ind.begin(), v_ind.end(), algorithm::greaterThan);
	auto peak = std::adjacent_find(v_ind.begin(), v_ind.end(), std::greater<double>());
	
	if (peak == v_ind.end()) {
		--peak;
	}

	return *peak;
}

double ThesholdCalculator::findMinPeak(std::vector<double>& v_ind){
	std::sort(v_ind.begin(), v_ind.end(), algorithm::smallerThan);
	auto peak = std::adjacent_find(v_ind.begin(), v_ind.end(), std::greater<double>());

	if (peak == v_ind.end()) {
		--peak;
	}

	return *peak;
}


void ThesholdCalculator::specifyRange(const std::vector<double>& v_inRef, const std::vector<double> &vals, ParametersRanges &outRange, double xmin, double xmax)
{
	size_t vrefLengh = v_inRef.size();
	size_t valsLenght = vals.size(); 

	if ((vrefLengh == 0) || (valsLenght == 0)) {

		throw std::invalid_argument("data is empty");

	}else 
	if ( (xmin < 0) || xmax <= 0)
	{
		DEBUG_LOG(QString("size vec1 %1 size vec2 %2 xmin %3 xmax %4").arg(v_inRef.size()).
			arg(vals.size()).arg(xmin).arg(xmax));

		throw std::invalid_argument("invalid parameter input");

	}
	else if ((xmax > 65535) || (xmax <= xmin))
	{
		QString msg = QString("invalid range or out short %1").arg(xmax);
		throw std::invalid_argument(msg.toStdString().c_str());

	}else if (vrefLengh != valsLenght) {
		throw std::invalid_argument("size of input and reference vector size are not equal"); 
	}

	for (size_t ind = 0;ind < vrefLengh; ++ind) {
		double val_y = 0; 
		double el_x = v_inRef[ind]; 
		
		if ((el_x < xmin) || (el_x > xmax)) {
			continue; 
		}
		
		val_y = vals[ind]; 
		
		outRange.insertElem(el_x, val_y);
	}

}

void ThesholdCalculator::determineMovingAverage(const std::vector<double> &v_in, std::vector<double> &v_out, unsigned int count)
{
	DEBUG_LOG("Calculate average");
	
	size_t v_lengh = v_in.size();
	
	if (v_lengh == 0) {
		DEBUG_LOG("input lenght values are zero"); 
		return; 
	}

	double value = 0.0f;
	double sum = 0.0f;
	const double moving = 3.0f;
	double maxLen = v_lengh - 1;

	if ((!(count % 2) == 1) || (count > maxLen) || (count == 0))
		return; 

	for (size_t posInd = 0; posInd < v_lengh; ++posInd) {

		if (posInd < count) {
			sum = this->vectorSum(v_in,posInd,posInd+count/2);
		}
		else {
			if (posInd >= maxLen - count) {
				sum = this->vectorSum(v_in, posInd, posInd - count / 2);
			}
			else {
				size_t minPos = posInd - (count / 2);
				size_t maxPos = posInd + (count / 2);
				sum = this->vectorSum(v_in, minPos, maxPos);

				//DEBUG_LOG(QString("sum %1").arg(sum));
			}
		}


		sum /= count;
		//DEBUG_LOG(QString("Average %1").arg(sum)); 

		v_out.push_back(sum);
	}
}
