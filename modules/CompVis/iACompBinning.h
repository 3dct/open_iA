// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//CompVis
#include "iACompHistogramTableData.h"

//Qt
#include <QList>

class iAMultidimensionalScaling;

class iACompBinning
{

public:

	iACompBinning(iACsvDataStorage* dataStorage, bin::BinType* datasets);

	//calculate the binning for the data points
	virtual void calculateBins() = 0;
	
	//calculates the bin datastructure for (a) specifically selected bin(s)
	virtual bin::BinType* calculateBins(bin::BinType* data, int currData) = 0;

	virtual void setDataStructure(iACompHistogramTableData* datastructure) = 0;

	
	/*** Internal evaluation methods ***/

	/**
	 * @brief The silhouette coefficient is the most common way to combine the metrics of cohesion and separation of a clustering algorithm in a single measure.
	 * It is defined in the interval [-1; 1] for each example in our data set. 
	 * Value beetwen -1 and 1 where 1 means that the object is assigned to the best possible group,
	 * 0 - the object is located between two groups, and -1 - wrong assignment of the object
	 * ["Evaluation Metrics for Unsupervised Learning Algorithms" by Palacio & Nino, 2019]
	 * @param datastructure - contains the all information required to compute the cluster evaluation measure
	 * @return for each dataset contains the silhouette coefficient for the clustering
	*/
	std::vector<double>* calculateSilhouetteCoefficient(iACompHistogramTableData* datastructure);

protected:

	//checks if the value lies inside an interval [low,high[
	bool checkRange(double value, double low, double high);


	//array where the size of the rows is not always the same
	//store all mds values for each dataset
	bin::BinType* m_datasets;

	iACsvDataStorage* m_dataStorage;

private:

	/**
	 * @brief get all points of the cluster the specific point belongs to
	 * @param clusters - all clusters of a dataset
	 * @param point - the point for which the cluster is looked for
	 * @return - the points of the cluster, the point belongs to
	*/
	double getPointsFromCluster(bin::BinType* clusters, double point);

	/**
	 * @brief compute the silhouette of a specifc point to all other points of his cluster by using the euclidean distance
	 * @param points_from_cluster - the cluster this specific point belongs to
	 * @param thisPoint - the point for which the silhouette is computed
	 * @return - the silhouette/distance from thisPoint to the other points contained in the cluster points_from_cluster
	*/
	double getASilhouette(std::vector<double>* points_from_cluster, double thisPoint);


	double getNearestCluster(std::vector<double> clusterBoundaries, double point, double maxUpperBoundary);

};
