#pragma once
#include <vector>

//template <class


class ThesholdCalculator
{
public:
	ThesholdCalculator();
	~ThesholdCalculator();

	void calculateAverage(const std::vector<double> &v_in, std::vector<double> v_out, unsigned int count);
	void doubleTestSum();

private: 
	double vectorSum(const std::vector<double> &sum, size_t startInd, size_t endInd);
	void testAverage(); 
	/*double sum(); */

};

