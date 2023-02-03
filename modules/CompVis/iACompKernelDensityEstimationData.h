// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//Debug
#include "iALog.h"

//Qt
#include <qlist.h>

#include <vector>
#include <array>

struct kdeData
{
	using kdePair = std::array<double, 2>; //<mdsValue, kdeValue>
	using kdeBin = std::vector<kdePair>; // 
	using kdeBins = std::vector<kdeBin>;

	static kdeBins* initializeBins(int numberOfBins);
	//outputs the content of kdePair
	static void debug(kdePair* pair);
	//outputs the content of the kdeBin
	static void debug(kdeBin* input);
	//outputs the content of kdeBins
	static void debug(kdeBins* input);
};

class iACompKernelDensityEstimationData
{
public:

	iACompKernelDensityEstimationData();

	//returns the the maximum MDS value in the whole dataset
	double getMaxVal();
	//returns the minimum MDS value in the whole dataset
	double getMinVal();

	//returns the the maximum MDS value in the whole dataset
	double getMaxKDEVal();
	//returns the minimum MDS value in the whole dataset
	double getMinKDEVal();
	//set the the maximum MDS value in the whole dataset
	void setMaxKDEVal(double maxKDE);
	//set the minimum MDS value in the whole dataset
	void setMinKDEVal(double minKDE);

	//get for every dataset its kde-values
	QList<kdeData::kdeBins>* getKDEDataUniform();
	QList<kdeData::kdeBins>* getKDEDataNB();
	QList<kdeData::kdeBins>* getKDEDataBB();

	std::vector<int>* getAmountObjectsEveryDataset();

	void setMaxVal(double newMax);
	void setMinVal(double newMin);
	void setKDEDataUniform(QList<kdeData::kdeBins>* newKDEData);
	void setKDEDataNB(QList<kdeData::kdeBins>* newKDEData);
	void setKDEDataBB(QList<kdeData::kdeBins>* newKDEData);
	void setAmountObjectsEveryDataset(std::vector<int>* newAmountObjectsEveryDataset);

	//returns the maximum amount of numbers in all bins --> i.e. there are maximum 5 values in one bin
	int getMaxAmountInAllBins();
	void setMaxAmountInAllBins(int newMaxAmountInAllBins);

	QList<std::vector<double>>* getObjectsPerBinUB();
	void setObjectsPerBinUB(QList<std::vector<double>>* featuresPerBinUB);

	QList<std::vector<double>>* getObjectsPerBinNB();
	void setObjectsPerBinNB(QList<std::vector<double>>* featuresPerBinNB);

	QList<std::vector<double>>* getObjectsPerBinBB();
	void setObjectsPerBinBB(QList<std::vector<double>>* featuresPerBinUB);

	QList<std::vector<double>>* getNumberOfObjectsPerBinUB();
	void setNumberOfObjectsPerBinUB(QList<std::vector<double>>* numberOfFeaturesPerBinUB);

	QList<std::vector<double>>* getNumberOfObjectsPerBinNB();
	void setNumberOfObjectsPerBinNB(QList<std::vector<double>>* numberOfFeaturesPerBinNB);

	QList<std::vector<double>>* getNumberOfObjectsPerBinBB();
	void setNumberOfObjectsPerBinBB(QList<std::vector<double>>* numberOfFeaturesPerBinBB);

	QList<std::vector<double>>* getBoundariesUB();
	void setBoundariesUB(QList<std::vector<double>>* boundariesUB);

	QList<std::vector<double>>* getBoundariesNB();
	void setBoundariesNB(QList<std::vector<double>>* boundariesNB);

	QList<std::vector<double>>* getBoundariesBB();
	void setBoundariesBB(QList<std::vector<double>>* boundariesBB);

protected:
	//maximum MDS value in all datasets
	double m_maxVal;
	//minimum MDS value in all datasets
	double m_minVal;

	//maximum KDE value in all datasets
	double m_maxKDEVal;
	//minimum MDS value in all datasets
	double m_minKDEVal;

	//vector that stores the number of elements for every dataset
	//i.e. dataset_1 stores 10 objects...
	std::vector<int>* amountObjectsEveryDataset;

	//stores the kde data binned according to uniform binning for all datasets
	QList<kdeData::kdeBins>* kdeDataUniform;
	//stores the kde data binned according to natural breaks binning for all datasets
	QList<kdeData::kdeBins>* kdeDataNB;
	//stores the kde data binned according to bayesian blocks binning for all datasets
	QList<kdeData::kdeBins>* kdeDataBB;

	//stores for each dataset for each bin how many mds values are stored computed by uniform binning
	QList<std::vector<double>>* m_objectsPerBinUB;
	//stores for each dataset for each bin how many mds values are stored computed by natural breaks binning
	QList<std::vector<double>>* m_objectsPerBinNB;
	//stores for each dataset for each bin how many mds values are stored computed by bayesian blocks binning
	QList<std::vector<double>>* m_objectsPerBinBB;

	//stores the real number of mds values (not comupted by KDE) in each bin (uniform binning) of each dataset
	QList<std::vector<double>>* m_numberOfObjectsBinUB;
	//stores the real number (not comupted by KDE) of mds values in each bin (natural breaks binning) of each dataset
	QList<std::vector<double>>* m_numberOfObjectsBinNB;
	//stores the real number of mds values (not comupted by KDE) in each bin (bayesian blocks binning) of each dataset
	QList<std::vector<double>>* m_numberOfObjectsBinBB;

	//stores for each dataset the lower boundary of each bin (uniform binning)
	QList<std::vector<double>>* m_BoundariesBinUB;
	//stores for each dataset the lower boundary of each bin (natural breaks binning)
	QList<std::vector<double>>* m_BoundariesBinNB;
	//stores for each dataset the lower boundary of each bin (bayesian blocks binning)
	QList<std::vector<double>>* m_BoundariesBinBB;
	
	//maximum amount of numbers in a bin (calculated for all bins)
	//calculated through the uniform binning appraoch!
	int m_maxAmountInAllBins;
};
