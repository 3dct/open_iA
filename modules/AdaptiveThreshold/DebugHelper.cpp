#include "DebugHelper.h"
#include "QString"
#include "iAConsole.h"

void DebugHelper::printVector(const std::vector<double> vec, unsigned int*count)
{
	if (vec.empty()) return; 
	DEBUG_LOG("Begin vector");
	
	uint indx = 0; 
	if (count){
		for (int i = 0; i < *count; i++) {
			if (i > ((*count)-1)) break;

			double val = vec[i];
			DEBUG_LOG(QString("Vec %1").arg(val));
		}

	}
	else {

	}

	
	DEBUG_LOG("End vector"); 
}

QString DebugHelper::debugVector(const std::vector<double> vec, unsigned int idxMin, unsigned int idx_max)
{
	if (vec.empty()) return QString("no values contained");
	if (idx_max == 0) return QString("invalid vector arguments");
	//if (idx_max = 0)


	uint indX = idxMin;
	double val = 0.0f; 
	QString valsOut = "Values: ";

	for (indX; indX < idx_max; indX++) {
		val = vec[indX];
		valsOut += QString("%1\t").arg(val);
	}


	return valsOut; 
}
