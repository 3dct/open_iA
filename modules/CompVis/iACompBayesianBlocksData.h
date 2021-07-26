#pragma once
#include "iACompHistogramTableData.h"

class iACompBayesianBlocksData : public iACompHistogramTableData
{
public:

	iACompBayesianBlocksData();

	//returns for every dataset each bin defined by its lower boundary
	virtual QList<std::vector<double>>* getBinRange();

	//calculate for each datasets for all their bins how many objects are contained in each one
	// the result is stored in "bin::BinType* m_numberOfObjectsPerBin"
	void calculateNumberOfObjectsInEachBin(QList<std::vector<csvDataType::ArrayType*>*>* thisBinDataObjects);

	//set for each dataset for each bin its lowerBoundary and upperBoundary
	void setBinBoundaries(QList<std::vector<double>>* binBoundaries);

private:

	//stores for each dataset how many objects are located in each bin
	bin::BinType* m_numberOfObjectsPerBin;

	//stores for each bin of each dataset its boundaries, where [lowerBound, upperBound[
	QList<std::vector<double>>* m_binsBoundaries;

};
