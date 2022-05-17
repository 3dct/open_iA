#pragma once


#include "iACompHistogramTableData.h"
#include "iACompKernelDensityEstimationData.h"

#include "KernelDensityEstimation/KernelDensityEstimation"

class iACompUniformBinningData;
class iACompBayesianBlocksData;
class iACompNaturalBreaksData;

class iACompKernelDensityEstimation
{
	typedef double realScalarType;
	typedef kde::Gaussian<realScalarType> kernelType;
	typedef kde::DiagonalBandwidthMatrix<realScalarType> bandwidthType;
	typedef kde::AllNeighbours<realScalarType> neighboursType;
	typedef kde::KernelDensityEstimator<kernelType, bandwidthType, neighboursType> kdeType;
	typedef typename kdeType::realVectorType realVectorType;
	typedef typename kdeType::realMatrixType realMatrixType;
	typedef typename kdeType::indexType indexType;


public:

	iACompKernelDensityEstimation(
		iACsvDataStorage* dataStorage, bin::BinType* datasets);

	void setDataStructure(iACompKernelDensityEstimationData* datastructure);

	//compute the curve for all datasets
	void calculateCurve(iACompUniformBinningData* uData, iACompBayesianBlocksData* bbData, iACompNaturalBreaksData* nbData);
	
	//compute the curve only for the uniform binning
	void calculateCurveUB(iACompUniformBinningData* uData);

private:

	//compute the KDE for an individual dataset
	void calculateKDE(std::vector<double>* dataIn, kdeData::kdeBin* results);
	
	//order kde data according to the binning of the given data
	//if a uniform binBoundaries are given, then the data is ordered according to the uniform binning
	//if bayesian blocks binBoundaries are given, then the data is ordered according to the bayesian blocks binning
	//if natural breaks binBoundaries are given, then the data is ordered according to the natural breaks binning
	void calculateKDEBinning(kdeData::kdeBin* input, double maxMDSVal, std::vector<double>* binBoundaries, kdeData::kdeBins* result);


	//array where the size of the rows is not always the same
	bin::BinType* m_datasets;

	iACsvDataStorage* m_dataStorage;

	iACompKernelDensityEstimationData* m_kdeData;

	indexType numSteps;

	//stores the maxmial kde value for all datasets
	double m_maxKDE;

	//stores the minimal kde value for all datasets
	double m_minKDE;
	
};
