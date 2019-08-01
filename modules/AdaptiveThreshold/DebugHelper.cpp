#include "DebugHelper.h"
#include "QString"
#include "iAConsole.h"

void DebugHelper::debugVector(const std::vector<double> vec)
{
	if (vec.empty()) return; 
	DEBUG_LOG("Begin vector");
	for (const double& el : vec) {
		DEBUG_LOG(QString("Vec %1").arg(el)); 
	}

	DEBUG_LOG("End vector"); 
}
