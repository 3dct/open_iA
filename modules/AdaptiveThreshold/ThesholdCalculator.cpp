#include "ThesholdCalculator.h"
#include <numeric>
#include <algorithm>
#include "iAConsole.h"


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

void ThesholdCalculator::testPeakDetect()
{
	std::vector<double> data{ 6.1, 8.0, 9.0, 14.1, 10.0,14.3, 12.1, 14.4 };
	double res = this->findMaxPeak(data); 
	DEBUG_LOG(QString("max peak %1").arg(res)) 
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

		/*	DEBUG_LOG(QString("bin %1").arg(binVals_X));
			DEBUG_LOG(QString("freq %1").arg(freq_valsY));*/
		m_thresBinsX.push_back(binVals_X);
		m_freqValsY.push_back(freq_valsY); 

		//push back some how
		////Todo Remove
		//if (b == 10) break;
		///*for(int p = 0; p < m_plots.size(); ++p)*/
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


void ThesholdCalculator::specifyRange(const std::vector<double>& v_in, std::vector<double>& v_out, double min, double max)
{
	if (v_in.size() == 0)
		throw std::invalid_argument("invalid parameter input");

	if ((min < 0) || (max == 0)) {
		throw std::invalid_argument("Invalid input parameter");
	}

	for ( auto & el :v_in ) {
		if ((el < min) && (el > max)) {
			continue; 
		}

		v_out.push_back(el);
	}
}

void ThesholdCalculator::calculateAverage(const std::vector<double> &v_in, std::vector<double> &v_out, unsigned int count)
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
