// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iACompBinning.h"

//Qt
#include <QList>

#include <cmath>    // for std::isnan

iACompBinning::iACompBinning(iACsvDataStorage* dataStorage, bin::BinType* datasets) :
	m_datasets(datasets),
	m_dataStorage(dataStorage)
	
{};

bool iACompBinning::checkRange(double value, double low, double high)
{
	return ((value >= low) && (value < high));
}

std::vector<double>* iACompBinning::calculateSilhouetteCoefficient(iACompHistogramTableData* datastructure)
{
	QList<bin::BinType*>* clustersOfAllDatasets = datastructure->getBinData();
	QList<std::vector<double>>* objectsPerBinAllDatasets = datastructure->getNumberOfObjectsPerBinAllDatasets();
	std::vector<double>* resultPerDataset = new std::vector<double>();

	for (int datasetID = 0; datasetID < objectsPerBinAllDatasets->size(); datasetID++)
	{
		double silhouette = 0.0;
		//the average distance between x and other objects in a group including x
		double averageDist = 0.0;
		//the minimum average distance between x and the nearest group.
		double minAverageDist = 0.0;

		bin::BinType* clusters = clustersOfAllDatasets->at(datasetID);
		std::vector<double> numberOfPointsPerBin = objectsPerBinAllDatasets->at(datasetID);

		std::vector<double> allPoints = m_datasets->at(datasetID);

		std::vector<double>* cluster_points;
		std::vector<double>* nearest_cluster_points;

		for (int pID = 0; pID < static_cast<int>(allPoints.size()); pID++)
		{
			double thisPoint = allPoints.at(pID);

			double binID = getPointsFromCluster(clusters, thisPoint);
			cluster_points = &clusters->at(binID);
			averageDist = getASilhouette(cluster_points, thisPoint);
			
			//Get closest cluster to p
			std::vector<double> clusterBoundaries = datastructure->getBinBoundaries()->at(datasetID);
			
			double nearestClusterID = getNearestCluster(clusterBoundaries, thisPoint, datastructure->getMaxVal());
			nearest_cluster_points = &clusters->at(nearestClusterID);
			minAverageDist = getASilhouette(nearest_cluster_points, thisPoint);

			

			if ( (averageDist == 0 && minAverageDist == 0) )
			{ //capture NANs
				silhouette += 0;
			}
			else
			{
				silhouette += (minAverageDist - averageDist) / (std::max(minAverageDist, averageDist));

				if (std::isnan((minAverageDist - averageDist) / (std::max(minAverageDist, averageDist))))
				{ //capture NANs
					LOG(lvlDebug, "NAN ");
					LOG(lvlDebug, "averageDist " + QString::number(averageDist));
					LOG(lvlDebug, "minAverageDist " + QString::number(minAverageDist));
					LOG(lvlDebug, "nearestClusterID " + QString::number(nearestClusterID));
				}
			}
			
		}

		 resultPerDataset->push_back(silhouette / allPoints.size());
	}

	////DEBUG
	for (int s = 0; s < static_cast<int>(resultPerDataset->size()); s++)
	{
		LOG(lvlDebug, "silhouette for dataset " + QString::number(s) + ": " + QString::number(resultPerDataset->at(s)));
		//LOG(lvlDebug, QString::number(resultPerDataset->at(s)));
	}

	return resultPerDataset;
	
}

double iACompBinning::getPointsFromCluster(bin::BinType* clusters, double point)
{
	for (int binID = 0; binID < static_cast<int>(clusters->size()); binID++)
	{
		std::vector<double> currBin = clusters->at(binID);
		if (std::find(currBin.begin(), currBin.end(), point) != currBin.end())
		{ //found point in bin/cluster
			return binID;
		}
	}

	//Shouldn't get here
	return -1;
}

double iACompBinning::getNearestCluster(std::vector<double> clusterBoundaries, double point, double maxUpperBoundary)
{
	double min_dist = INFINITY;
	
	double nearestBinID = -1;
	double currBinID = 0;

	for (int id = 0; id < static_cast<int>(clusterBoundaries.size()); id++)
	{
		double minBoundary = clusterBoundaries.at(id);

		double maxBoundary;
		if (id != (static_cast<int>(clusterBoundaries.size())-1))
		{
			maxBoundary = clusterBoundaries.at(id + 1) * 0.99;
		}
		else
		{
			maxBoundary = maxUpperBoundary;
		}
		
		double binCentroid = minBoundary + ((minBoundary + maxBoundary) * 0.5);
		double distCentroid = abs(binCentroid - point);

		if (distCentroid < min_dist)
		{
			min_dist = distCentroid;
			nearestBinID = currBinID;
		}

		currBinID++;
	}

	return nearestBinID;
}

double iACompBinning::getASilhouette(std::vector<double>* points_from_cluster, double thisPoint)
{
	double dist = 0.0;

	for (int i = 0; i < static_cast<int>(points_from_cluster->size()); i++)
	{
		double point = points_from_cluster->at(i);

		if (point == thisPoint)
		{
			continue;
		}

		dist += abs(point - thisPoint);  //euclideanDistance for 1D
	}

	if (points_from_cluster->size() != 0)
	{
		dist /= points_from_cluster->size();
	}

	return dist;
}
