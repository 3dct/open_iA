/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
* *********************************************************************************** *
* This program is free software: you can redistribute it and/or modify it under the   *
* terms of the GNU General Public License as published by the Free Software           *
* Foundation, either version 3 of the License, or (at your option) any later version. *
*                                                                                     *
* This program is distributed in the hope that it will be useful, but WITHOUT ANY     *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A     *
* PARTICULAR PURPOSE.  See the GNU General Public License for more details.           *
*                                                                                     *
* You should have received a copy of the GNU General Public License along with this   *
* program.  If not, see http://www.gnu.org/licenses/                                  *
* *********************************************************************************** *
* Contact: FH OÖ Forschungs & Entwicklungs GmbH, Campus Wels, CT-Gruppe,              *
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email: c.heinzl@fh-wels.at       *
* ************************************************************************************/
#pragma once

#include "iAProgress.h"

#include "iAvec3.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QSharedPointer>
#include <QThread>

#include <vector>

class iASPLOMData;

class iACsvConfig;

class vtkTable;

class QCheckBox;

//! fibervalues layout unless otherwise specified : (start[x, y, z], end[x, y, z], center[x, y, z], phi, theta, length, diameter)

//! Class for holding data about the similarity from one result to another fiber in a different dataset
class iAFiberSimilarity
{
public:
	size_t index;
	double similarity;
	friend bool operator<(iAFiberSimilarity const & a, iAFiberSimilarity const & b);
};

//! Comparison data to reference for a single result/fiber, for all time steps
class iARefDiffFiberTimeData
{
public:
	//! diff of fibervalues (+similarity measures)
	std::vector<double> timestep;
};

//! Comparison data to reference for a single fiber in a result
class iARefDiffFiberData
{
public:
	//! differences to reference fiber, one per diff/similarity measure
	std::vector<iARefDiffFiberTimeData> diff;
	//! dist to ref fibers: for each similarity measure, in order of ascending difference
	std::vector<std::vector<iAFiberSimilarity> > dist;
};

//! Data for a single fiber characterization result
class iAFiberCharData
{
public:
	static const int FiberValueCount = 13;
	//! the fiber data as vtkTable, mainly for the 3d visualization:
	vtkSmartPointer<vtkTable> table;
	//! mapping of the columns in m_resultTable
	QSharedPointer<QMap<uint, uint> > mapping;
	//! name of the csv file this result was loaded from
	QString fileName;
	//! values for all timesteps, stored as: timestep, fiber, fibervalues
	std::vector<std::vector<std::vector<double> > > timeValues;
	//! information on curved fibers; fiber_id (size_t) maps to list of points along fiber
	std::map<size_t, std::vector<iAVec3f> > curveInfo;
	//! projection error stored as fiber, timestep, global projection error
	std::vector<std::vector<double > > projectionError;
	//! number of fibers in the dataset:
	size_t fiberCount;
// Comparison to reference:
	//! comparison data to reference for each fiber
	std::vector<iARefDiffFiberData> refDiffFiber;
	//! for each similarity measure, the average over all fibers
	std::vector<double> avgDifference;
};

class iAFiberResultsCollection
{
public:
	static const QString LegacyFormat;
	static const QString SimpleFormat;
	iAFiberResultsCollection();

	//! for each result, detailed data
	std::vector<iAFiberCharData> result;
	//! SPM data
	QSharedPointer<iASPLOMData> spmData;
	//! min and max of fiber count over all results
	size_t minFiberCount, maxFiberCount;
	//! maximum of optimization steps in all results
	int optimStepMax;
	//! results folder
	QString folder;
// Comparison to reference:
	//! for each fiber in the reference, the average match quality over all results (-1.. no match, otherwise 0..1 where 0 perfect match, 1..bad match)
	std::vector<double> avgRefFiberMatch;
	//! for each difference/similarity measure, the maximum value over all results:
	std::vector<double> maxAvgDifference;
	//! type of objects (typically fibers, see iACsvConfig::VisualizationType)
	int objectType;

// Methods:
	bool loadData(QString const & path, QString const & configName, double stepShift, iAProgress * progress);
};

class iAFiberResultsLoader: public QThread
{
	Q_OBJECT
public:
	iAFiberResultsLoader(QSharedPointer<iAFiberResultsCollection> results, QString const & path, QString const & configName, double stepShift);
	void run() override;
	iAProgress* progress();
signals:
	void failed(QString const & path);
private:
	iAProgress m_progress;
	QSharedPointer<iAFiberResultsCollection> m_results;
	QString m_path;
	QString m_configName;
	double m_stepShift;
};

// helper functions:
void addColumn(vtkSmartPointer<vtkTable> table, float value, char const * columnName, size_t numRows);
iACsvConfig getCsvConfig(QString const & csvFile, QString const & formatName);
