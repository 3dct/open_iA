// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iACsvConfig.h"
#include "iAObjectsData.h"

#include <iAAABB.h>
#include <iAAbortListener.h>
#include <iAProgress.h>
#include <iAVec3.h>

#include <vtkSmartPointer.h>

#include <QMap>
#include <QThread>

#include <array>
#include <memory>
#include <vector>

struct iAFiberData;

class iASPLOMData;

class vtkTable;

class QCheckBox;

//! fibervalues layout unless otherwise specified : (start[x, y, z], end[x, y, z], center[x, y, z], phi, theta, length, diameter)

//! Class for holding data about the similarity from one result to another fiber in a different dataset
class iAFiberSimilarity
{
public:
	quint32 index;
	float dissimilarity;
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
class iAFiberResult
{
public:
	//! objects table, column mapping, curved fiber data
	std::shared_ptr<iAObjectsData> objData;
	//! name of the csv file this result was loaded from
	QString fileName;
	//! name of the csv file the curved info for this file was loaded from
	QString curvedFileName;
	//! what kind of step data is available
	enum StepDataType { NoStepData=0, SimpleStepData=1, CurvedStepData=2 };
	StepDataType stepData;
	//! values for all steps, stored as: step, fiber, fibervalues
	std::vector<std::vector<std::vector<double> > > stepValues;
	//! projection error stored as fiber, step, global projection error
	std::vector<QVector<double > > projectionError;
	//! number of fibers in the dataset:
	size_t fiberCount;
// Comparison to reference:
	//! comparison data to reference for each fiber
	QVector<iARefDiffFiberData> refDiffFiber;
	//! for each similarity measure, the average over all fibers
	QVector<double> avgDifference;  // rename -> avgDissimilarity
	
	//! overall bounding box
	iAAABB bbox;
	//! bounding box per fiber:
	std::vector<iAAABB> fiberBB;
	//! fiber data objects:
	std::vector<iAFiberData> fiberData;
};

//! A collection of multiple results from one or more fiber reconstruction algorithms.
class iAFiberResultsCollection
{
public:
	static const QString FiakerFCPFormat;
	static const QString SimpleFormat;
	iAFiberResultsCollection();

	//! for each result, detailed data
	std::vector<iAFiberResult> result;
	//! SPM data.
	// TODO: deduplicate data here and in result[x].table
	std::shared_ptr<iASPLOMData> spmData;
	//! min and max of fiber count over all results
	size_t minFiberCount, maxFiberCount;
// { TODO: make private ?
	//! maximum of optimization steps in all results
	size_t optimStepMax;
	//! results folder
	QString folder;
	//! shift applied to each step
	double stepShift;
	//! type of objects (typically fibers, see iAObjectVisType)
	iAObjectVisType objectType;
// }
// Comparison to reference:
	//! for each fiber in the reference, the average match quality over all results (-1.. no match, otherwise 0..1 where 0 perfect match, 1..bad match)
	QVector<double> avgRefFiberMatch;
	//! for each difference/similarity measure, the maximum value over all results:
	QVector<double> maxAvgDifference;

	//! IDs of the computed dissimilarity measures (ref. to position in getAvailableDissimilarityMeasures / switch in getDissimilarity
	QVector<qulonglong> m_measures;

	//! mapping for the resultID and projection error column:
	uint m_resultIDColumn, m_projectionErrorColumn;

// Methods:
	bool loadData(QString const & path, iACsvConfig const & config, double stepShift, iAProgress * progress, bool& abort);
};

//! Loads a collection of results from a folder, in the background
class iAFiberResultsLoader: public QThread, public iAAbortListener
{
	Q_OBJECT
public:
	iAFiberResultsLoader(std::shared_ptr<iAFiberResultsCollection> results,
		QString const & path, iACsvConfig const & config, double stepShift);
	void run() override;
	void abort() override;
	bool isAborted() const;
	iAProgress* progress();
signals:
	void failed(QString const & path);
	void success();
private:
	iAProgress m_progress;
	std::shared_ptr<iAFiberResultsCollection> m_results;
	QString m_path;
	iACsvConfig m_config;
	double m_stepShift;
	bool m_aborted;
};

// helper functions:
void addColumn(vtkSmartPointer<vtkTable> table, double value, char const * columnName, size_t numRows);
iACsvConfig getCsvConfig(QString const & formatName);

//! merge list of bounding boxes
void mergeBoundingBoxes(iAAABB& bbox, std::vector<iAAABB> const& fiberBBs);
