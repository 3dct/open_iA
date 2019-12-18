#pragma once
//#include <iAConsole.h>
#include <vector>

class QString; 

class DebugHelper
{
public:
	/*DebugHelper(); */
	void printVector(const std::vector<double> vec, unsigned int *count); 

	QString debugVector(const std::vector<double> vec, unsigned int v_min, unsigned int v_max);

};

