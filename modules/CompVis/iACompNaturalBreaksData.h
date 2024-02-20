// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACompHistogramTableData.h"

class iACompNaturalBreaksData : public iACompHistogramTableData
{

public:

    iACompNaturalBreaksData();

    //returns for every dataset each bin defined by its lower boundary
	virtual QList<std::vector<double>>* getBinBoundaries() override;
	//set for each dataset for each bin its lowerBoundary
	virtual void setBinBoundaries(QList<std::vector<double>>* binBoundaries) override;

    //calculate for each datasets for all their bins how many objects are contained in each one
    // the result is stored in "bin::BinType* m_numberOfObjectsPerBin"
    void calculateNumberOfObjectsInEachBin(QList<std::vector<csvDataType::ArrayType*>*>* thisBinDataObjects);

private:

    //stores for each dataset how many objects are located in each bin
    bin::BinType* m_numberOfObjectsPerBin;
};
