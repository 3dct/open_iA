#include "ThesholdCalculator.h"
#include <numeric>
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
		DEBUG_LOG(QString("el %1").arg(vec[i])); 
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

void ThesholdCalculator::calculateAverage(const std::vector<double> &v_in, std::vector<double> v_out, unsigned int count)
{
	size_t v_lengh = v_in.size();
	double value = 0.0f;
	double sum = 0.0f;
	const double moving = 3.0f;
	double maxLen = v_lengh - 1;
	//double tmp;

	//check odd
	if ((!(count % 2) == 1) || (count > maxLen) || (count == 0))
		return; 


	for (size_t posInd = 0; posInd < v_lengh; ++posInd) {

		if (posInd < count) {
			sum = this->vectorSum(v_in,posInd,posInd+count);
		}
		else
			if (posInd >= maxLen-count) {
				sum = this->vectorSum(v_in,posInd, posInd - count);
			}
			else {
				size_t minPos = posInd - (count / 2);
				size_t maxPos = posInd + (count / 2); 
				sum = this->vectorSum(v_in, minPos / 2, maxPos / 2);

				DEBUG_LOG(QString("sum %1").arg(sum));
			}
		
		sum /= count;
		v_out.push_back(sum);
	}
}
