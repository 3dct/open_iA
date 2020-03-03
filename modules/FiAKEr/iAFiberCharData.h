/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iACsvConfig.h"

#include "iAProgress.h"
#include "iAvec3.h"

#include <vtkSmartPointer.h>

#include <QMap>
#include <QSharedPointer>
#include <QThread>

#include <vector>

class iASPLOMData;

class vtkTable;

class QCheckBox;

//! fibervalues layout unless otherwise specified : (start[x, y, z], end[x, y, z], center[x, y, z], phi, theta, length, diameter)

//! Class for holding data about the similarity from one result to another fiber in a different dataset
class iAFiberSimilarity
{
public:
	quint64 index;
	double dissimilarity;
	friend bool operator<(iAFiberSimilarity const & a, iAFiberSimilarity const & b);
};

QDataStream &operator<<(QDataStream &out, const iAFiberSimilarity &s);
QDataStream &operator>>(QDataStream &in, iAFiberSimilarity &s);

//! Comparison data to reference for a single result/fiber, for all steps
class iARefDiffFiberStepData
{
public:
	//! diff of fibervalues (+similarity measures)
	QVector<double> step;
};

QDataStream &operator<<(QDataStream &out, const iARefDiffFiberStepData &s);
QDataStream &operator>>(QDataStream &in, iARefDiffFiberStepData &s);

//! Comparison data to reference for a single fiber in a result
class iARefDiffFiberData
{
public:
	//! stepwise differences to reference fiber, one per diff/similarity measure (and internally then per step)
	QVector<iARefDiffFiberStepData> diff;
	//! dist to ref fibers: for each similarity measure, in order of ascending difference
	QVector<QVector<iAFiberSimilarity> > dist;
};

QDataStream &operator<<(QDataStream &out, const iARefDiffFiberData &s);
QDataStream &operator>>(QDataStream &in, iARefDiffFiberData &s);

//! Data for the result of a single run of a fiber reconstructcion algorithm.
class iAFiberCharData
{
public:
	//! the fiber data as vtkTable, mainly for the 3d visualization:
	vtkSmartPointer<vtkTable> table;
	//! mapping of the columns in m_resultTable
	QSharedPointer<QMap<uint, uint> > mapping;
	//! name of the csv file this result was loaded from
	QString fileName;
	//! name of the csv file the curved info for this file was loaded from
	QString curvedFileName;
	//! what kind of step data is available
	enum StepDataType { NoStepData=0, SimpleStepData=1, CurvedStepData=2 };
	StepDataType stepData;
	//! values for all steps, stored as: step, fiber, fibervalues
	std::vector<std::vector<std::vector<double> > > stepValues;
	//! information on curved fibers; fiber_id (size_t) maps to list of points along fiber
	std::map<size_t, std::vector<iAVec3f> > curveInfo;
	//! projection error stored as fiber, step, global projection error
	std::vector<QVector<double > > projectionError;
	//! number of fibers in the dataset:
	size_t fiberCount;
// Comparison to reference:
	//! comparison data to reference for each fiber
	QVector<iARefDiffFiberData> refDiffFiber;
	//! for each similarity measure, the average over all fibers
	QVector<double> avgDifference;
};

//! A collection of multiple results from one or more fiber reconstruction algorithms.
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
// { TODO: make private ?
	//! maximum of optimization steps in all results
	size_t optimStepMax;
	//! results folder
	QString folder;
	//! shift applied to each step
	double stepShift;
	//! type of objects (typically fibers, see iACsvConfig::VisualizationType)
	int objectType;
// }
// Comparison to reference:
	//! for each fiber in the reference, the average match quality over all results (-1.. no match, otherwise 0..1 where 0 perfect match, 1..bad match)
	QVector<double> avgRefFiberMatch;
	//! for each difference/similarity measure, the maximum value over all results:
	QVector<double> maxAvgDifference;

	//! IDs of the computed dissimilarity measures (ref. to position in getAvailableDissimilarityMeasures / switch in getDissimilarity
	QVector<size_t> m_measures;

	//! mapping for the resultID and projection error column:
	uint m_resultIDColumn, m_projectionErrorColumn;

// Methods:
	bool loadData(QString const & path, iACsvConfig const & config, double stepShift, iAProgress * progress);
};

//! Loads a collection of results from a folder, in the background
class iAFiberResultsLoader: public QThread
{
	Q_OBJECT
public:
	iAFiberResultsLoader(QSharedPointer<iAFiberResultsCollection> results,
		QString const & path, iACsvConfig const & config, double stepShift);
	void run() override;
	iAProgress* progress();
signals:
	void failed(QString const & path);
	void success();
private:
	iAProgress m_progress;
	QSharedPointer<iAFiberResultsCollection> m_results;
	QString m_path;
	iACsvConfig m_config;
	double m_stepShift;
};

// helper functions:
void addColumn(vtkSmartPointer<vtkTable> table, double value, char const * columnName, size_t numRows);
iACsvConfig getCsvConfig(QString const & formatName);
