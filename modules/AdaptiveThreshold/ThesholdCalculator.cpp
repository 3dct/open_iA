#include "ThesholdCalculator.h"
#include <numeric>
#include <algorithm>
#include "iAConsole.h"
#include "ThresholdDefinitions.h"
#include "ChartVisHelper.h"



ThesholdCalculator::ThesholdCalculator()
{
	m_newDataSeries = nullptr; 
}

ThesholdCalculator::~ThesholdCalculator()
{
}

//double ThesholdCalculator::vectorSum(const std::vector<double> &vec, size_t startInd, size_t endInd)
//{
//	if (startInd >= vec.size() || endInd >= vec.size()) throw new std::invalid_argument("test"); 
//	double tmp = 0.0f; 
//	
//	size_t i = startInd; 
//	while (i <= endInd){
//		
//		tmp += vec[i];
//		++i;
//	}
//
//	return tmp; 
//}



void ThesholdCalculator::testPeakDetect()
{
	std::vector<double> data{ 6.1, 8.0, 9.0, 14.1, 10.0,14.3, 12.1, 14.4 };
	double res = m_calcHelper.findMaxPeak(data); 
	DEBUG_LOG(QString("max peak %1").arg(res)) 
}

threshold_defs::ThresIndx ThesholdCalculator::testFindIndex(double value) {
	//						ind 0   1   2    3     4    5     6     7 
	std::vector<double> data{ 6.1, 8.0, 9.0, 14.1, 10.0,14.3, 12.1, 14.4 };
	auto res = m_calcHelper.findIndex(data, value);
	return res;
}
	
	

void ThesholdCalculator::testSpecifyRange(const std::vector<double> &v_inRange, 
	const std::vector<double> &v_elements, threshold_defs::ParametersRanges &outputRanges)
{
	double x_min = 2.0; 
	double x_max = 8.0; 

	this->specifyRange(v_inRange, v_elements, outputRanges, x_min, x_max);
	DEBUG_LOG(QString("Limits(min, max) %1 %2").arg(x_min).arg(x_max)); 
	DEBUG_LOG("input vector");
	m_dbgHelper.printVector(v_elements,nullptr); 

	DEBUG_LOG("output vector");
}

QString ThesholdCalculator::testPrintVector()
{
	uint min = 4; 
	uint max = 7; 
	std::vector<double> vals = { 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	QString out = QString("min %1").arg(min);
	out += QString("max %1 \n").arg(max);
	out += m_dbgHelper.debugVector(vals, min, max); 
	return out; 
}


void ThesholdCalculator::determinIso50(const threshold_defs::ParametersRanges& inRanges, threshold_defs::ThresMinMax& inVals)
{
	try
	{
		m_calcHelper.determinIso50andGlobalMax(inRanges, inVals);
	}
	catch (std::invalid_argument & /*iaex*/) {
	
		throw; 
	}
	
	
	
}


QPointF ThesholdCalculator::determineResultingThresholdBasedOnDecisionRule(const threshold_defs::ThresMinMax& results, QTextEdit *elem)
{	

	return m_minMaxHelper.determineThresholdResultsPointXY(results, elem);
}

void ThesholdCalculator::doubleTestSum()
{
	std::vector<double> vals = { 1, 2, 3, 4, 5, 6, 7, 8, 9 }; 
	double tmp = 0.0f; 
	tmp = m_calcHelper.vectorSum(vals, 0, 2);
	DEBUG_LOG(QString("su 0, 2: %1").arg(tmp));
	tmp = m_calcHelper.vectorSum(vals, 1, 4);
	DEBUG_LOG(QString("su 1, 4 %1").arg(tmp)); 
	tmp = m_calcHelper.vectorSum(vals, 4, 6);
	DEBUG_LOG(QString("su 4, 6 %1").arg(tmp)); 

}





threshold_defs::ThresMinMax ThesholdCalculator::calcMinMax(const threshold_defs::ParametersRanges& ranges)
{
	try {
		return m_calcHelper.calculateLocalePeaks(ranges);
	}
	catch (std::invalid_argument& iaex) {
		throw iaex; 
	}
}

void ThesholdCalculator::retrieveHistData()
{
	if (!m_data)
		throw std::invalid_argument("Error histogram data cannot be retrieved");

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




void ThesholdCalculator::specifyRange(const std::vector<double>& v_inRef, const std::vector<double> &vals, threshold_defs::ParametersRanges &outRange, double xmin, double xmax)
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

	//this is hardcoded
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


void ThesholdCalculator::rangeFromParamRanges(const threshold_defs::ParametersRanges& ranges, threshold_defs::ParametersRanges& outValues, double min, double max) {
	try {
		this->specifyRange(ranges.getXRange(), ranges.getYRange(), outValues, min, max);
	}
	catch (std::invalid_argument& /*iave*/) {
		throw; 
	}
}


void ThesholdCalculator::calculateMovingAverage(const std::vector<double> &v_in, std::vector<double> &v_out, unsigned int count)
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
			sum = m_calcHelper.vectorSum(v_in,posInd,posInd+count/2);
		}
		else {
			if (posInd >= maxLen - count) {
				sum = m_calcHelper.vectorSum(v_in, posInd, posInd - count / 2);
			}
			else {
				size_t minPos = posInd - (count / 2);
				size_t maxPos = posInd + (count / 2);
				sum = m_calcHelper.vectorSum(v_in, minPos, maxPos);
			}
		}


		sum /= count;
		v_out.push_back(sum);
	}
}
