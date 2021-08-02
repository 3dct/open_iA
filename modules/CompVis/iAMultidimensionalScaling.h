#pragma once

//CompVis
#include "iACsvDataStorage.h"

//Qt
#include <QList>

#include <iostream>


class iASimilarityDistance;
class iAHistogramData;

namespace MDS
{
	enum class ProximityMetric
	{
		Unknown = -1,
		ArcCosineDistance,
		NumberOfProximityMetrics
	};

	QString proximityMetric_to_string(int i);
	ProximityMetric string_to_proximityMetric(QString string);

	enum class DistanceMetric
	{
		Unknown = -1,
		EuclideanDistance,
		MinkowskiDistance,
		NumberOfDistanceMetrics
	};

	QString distanceMetric_to_string(int i);

	DistanceMetric string_to_distanceMetric(QString string);
}

class iAMultidimensionalScaling
{
   public:
	iAMultidimensionalScaling(QList<csvFileData>* data);
	~iAMultidimensionalScaling();

	std::vector<double>* getWeights();
	void startMDS(std::vector<double>* weights);
	void setProximityMetric(MDS::ProximityMetric proxiName);
	void setDistanceMetric(MDS::DistanceMetric disName);
	QList<csvFileData>* getCSVFileData();
	//return the result of the mds
	csvDataType::ArrayType* getResultMatrix();

   private:
	//initialize weight list
	void initializeWeights();

	//initialize matrixUNormalized according to the amount of objects in all csvs and all characteritics
	void initializeMatrixUNormalized();

	//1.normalize the quantitative data matrix
	void normalizeMatrix();

	//2.calculate proximity measure
	void calculateProximityDistance();

	//3.initialize which similarity metric to use for calculation
	iASimilarityDistance* initializeDistanceMetric();

	// Multidimensional scaling (MDS) with SMACOF
	// This code re-implements Michael Bronstein's SMACOF in his Matlab Toolbox for Surface Comparison and Analysis
	// The Matlab SMACOF can be downloaded at http://tosca.cs.technion.ac.il/
	//[1] A. M. Bronstein, M. M. Bronstein, R. Kimmel,"Numerical geometry of nonrigid shapes", Springer, 2008.
	void calculateMDS(int dim, int iterations);

	//holds the data for which the MDS will be calculated
	//list containing all csv-files
	//data = [[headerOfCSV1,valuesOfCSV1], [headerOfCSV2,valuesOfCSV2],...]
	//header = [name1,name2,...] --> Strings
	//values = [ [f1_val1,f1_val2,...], [f2_val1,f2_val2,...]]
	QList<csvFileData>* m_inputData;

	//amount of objects (features) inside one matrix
	int m_amountOfElems;
	//amount of characteristics of an object
	//must be the same for all csv files!
	int m_amountOfCharas;
	//normalized matrix conntaining all objects with all their characeristics
	csvDataType::ArrayType* m_matrixUNormalized;
	//proximity distance used to calculate the MDS
	csvDataType::ArrayType* m_matrixProximityDis;
	//result of the MDS
	csvDataType::ArrayType* m_configuration;
	//weights that will be used for each characteristic during the computation, each between [0,100]
	std::vector<double>* m_weights;

	MDS::DistanceMetric m_activeDisM;
	MDS::ProximityMetric m_activeProxM;

	const double Epsilon = 0.000001;
	void vectorDiff(csvDataType::ArrayType* a, csvDataType::ArrayType* b, csvDataType::ArrayType* result);
};
