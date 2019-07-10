#include "ThesholdCalculator.h"
#include <numeric>
#include <algorithm>
#include "iAConsole.h"


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
	/*for (size_t i = startInd; i < endInd+1; ++i) {*/
	size_t i = startInd; 
	while (i <= endInd){
		//DEBUG_LOG(QString("el %1").arg(vec[i])); 
		tmp += vec[i];
		++i;
	}

	return tmp; 
}

void ThesholdCalculator::testAverage()
{

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



void ThesholdCalculator::findMaxPeak(const std::vector<double>& v_ind/*, unsigned int toleranceVal*/)
{
	auto peak = std::adjacent_find(v_ind.begin(), v_ind.end(), std::greater<double>());
	If(peak == v_ind.end()) {
		--peak;
	
	}

	return *peak;
}

void ThesholdCalculator::specifyRange(const std::vector<double>& v_in, const std::vector<double>& v_ind, double min, double max)
{
	if (v_in.size() == 0)
		throw std::invalid_argument("invalid parameter input");

	if (min < 0) || (max == 0) {
		throw std::invalid_argument("Invalid input parameter");
	}

	for (const double& el :v_in ) {
		if (el < min) && (el > max) {
			continue; 
		}

		v_ind.push_back(el); 
	}
}

void ThesholdCalculator::calculateAverage(const std::vector<double> &v_in, std::vector<double> &v_out, unsigned int count)
{
	DEBUG_LOG("Calculate average");
	
	size_t v_lengh = v_in.size();
	
	if (v_lengh == 0) {
		DEBUG_LOG("input values are zero"); 
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
