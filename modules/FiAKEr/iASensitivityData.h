// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iAMatrixWidget.h> // for iADissimilarityMatrixType

#include <vtkSmartPointer.h>

#include <QPair>
#include <QStringList>
#include <QVector>

#include <memory>
#include <vector>

class iAFiberResultsCollection;

class iAProgress;
class iAColorTheme;

class vtkImageData;

using qvectorsizetype = size_t;

class iASensitivityData
{
public:
	iASensitivityData(std::shared_ptr<iAFiberResultsCollection> data, QStringList const& paramNames,
		std::vector<std::vector<double>> const& paramValues);
	//! compute characteristics
	void compute(iAProgress* progress);
	//! compute voxelized spatial overview over sensitivity (utilizing unique fibers)
	void computeSpatialOverview(iAProgress* p);
	//! name of the cache file for the spatial overview image
	QString spatialOverviewCacheFileName() const;
	//! name of the cache file for the average fiber/voxel image (basically ~ mean objects)
	QString averageFiberVoxelCacheFileName() const;
	//! name of the cache file for the dissimilarity matrix
	QString dissimilarityMatrixCacheFileName() const;
	//! abort the sensitivity computation in case one is running
	void abort();
	//! name of characteristic with given index
	QString charactName(int charIdx) const;

	// DATA / COMPUTED DATA:
	//
	//! the "original" FIAKER data of all loaded results:
	std::shared_ptr<iAFiberResultsCollection> m_data;
	//! the names of all parameters
	QStringList m_paramNames;
	//! "points" in parameter space at which the sensitivity was computed
	//! (only the "centers" of the STARs)
	//! first index: parameter set; second index: parameter
	QVector<QVector<double>> paramSetValues;
	//! all samples points (i.e. all values from paramSetValues + points sampled for STAR around these points)
	//! NOTE: due to legacy reasons, swapped index order in comparison to paramSetValues!
	// TODO: unify index order?
	std::vector<std::vector<double>> m_paramValues;
	//! range of each parameter
	std::vector<double> m_paramMin, m_paramMax;
	//! indices of features for which sensitivity was computed
	QVector<int> m_charSelected;
	//! which difference measures were used for distribution comparison
	QVector<int> m_charDiffMeasure;
	//! for which dissimilarity measure sensitivity was computed
	//QVector<int> dissimMeasure;

	//! characteristics histogram for each result and characteristic
	QVector<          // For each result,
		QVector<      // for each characteristic (index in _selected_ characteristics, not in overall!),
		QVector<      // characteristic histogram bin
		double>>>
		m_charHistograms;

	int m_numOfSTARSteps, m_starGroupSize;
	int m_histogramBins;
	QVector<double> paramStep;  //!< per varied parameter, the size of step performed for the STAR

	QVector<int> m_variedParams;  //!< indices of the parameters that were varied

	//! sensitivity "field" for characteristics distributions
	QVector<          // characteristic (index in m_charSelected)
		QVector<      // characteristics distribution difference measure index (index in m_charDiffMeasure)
		QVector<      // variation aggregation (see iASensitivityInfo::create)
		QVector<      // parameter index (second index in paramSetValues / allParamValues)
		QVector<      // parameter set index (first index in paramSetValues)
		double>>>>>
		sensitivityField;

	//! averages over all parameter-sets of above char. distribution field ("global sensitivity" for a parameter)
	QVector<          // characteristis
		QVector<      // characteristics distribution difference measure
		QVector<      // variation aggregation
		QVector<      // parameter index
		double>>>>
		aggregatedSensitivities;

	//! sensitivity "field" for characteristics - pairwise match differences
	QVector<          // characteristic (index in m_charSelected)
		//QVector<      // characteristics difference measure index
		QVector<      // variation aggregation (see iASensitivityInfo::create)
		QVector<      // parameter index (second index in paramSetValues / allParamValues)
		QVector<      // parameter set index (first index in paramSetValues)
		double>>>>
		sensitivityFieldPWDiff;

	//! averages over all parameter-sets of above pairwise match diff. field ("global sensitivity" for a parameter)
	QVector<          // characteristis
		//QVector<      // difference measure
		QVector<      // variation aggregation
		QVector<      // parameter index
		double>>>
		aggregatedSensitivitiesPWDiff;

	//! sensitivity at each parameter regarding fiber count
	QVector<          // variation aggregation
		QVector<      // parameter index
		QVector<      // parameter set index
		double>>>
		sensitivityFiberCount;
	//! averages over all parameter-sets of above field ("global sensitivity" for a parameter, regarding fiber count)
	QVector<          // variation aggregation
		QVector<      // parameter index
		double>>
		aggregatedSensitivitiesFiberCount;

	//! range of the fiber counts over all results
	double m_fiberCountRange[2];
	//! histogram of the fiber counts over all results
	QVector<double> fiberCountHistogram;

	//! sensitivity "field" for dissimilarity measures
	QVector<          // dissimilarity measure (index in m_resultDissimMeasures)
		QVector<      // variation aggregation (see iASensitivityInfo::create)
		QVector<      // parameter index (second index in paramSetValues / allParamValues)
		QVector<      // parameter set index (first index in paramSetValues)
		double>>>>
		sensDissimField;

	//! averages over all parameter-sets of above field ("global sensitivity" for a parameter by dissimilarity measures)
	QVector<          // dissimilarity measure (index in m_resultDissimMeasures)
		QVector<      // variation aggregation
		QVector<      // parameter index
		double>>>
		aggregatedSensDissim;

	//! ranges of dissimilarity metrics (index1: dissimilarity metric; index2: min/max)
	QVector<QPair<double, double>> m_dissimRanges;
	//! histograms of dissimilarity metrics (index1: dissimilarity metric; index2: bin)
	QVector<QVector<double>> m_dissimHistograms;

	// Histogram "variation" (i.e. average of differences in frequency)
	// TODO: think about other measures (variation, ...) for differences between bins?

	/* not used for now - TODO: actual histogram of histograms
	//! distribution of characteristics distributions ("histogram of histograms") across:
	QVector<          // characteristics index
		QVector<      // variation aggregation
		QVector<      // parameter index
		QVector<      // bin index
		QVector<      // parameter set index
		QVector<      // index of value (stores frequency values of original histogram)
		double>>>>>> charHistHist;
	*/

	//!  difference at each bin of characteristics distribution (histogram above)
	QVector<          // characteristics index
		QVector<      // variation aggregation
		QVector<      // parameter index
		QVector<      // bin index
		QVector<      // parameter set index
		double>>>>>
		charHistVar;

	//! aggregation of differences at each bin of characteristics distribution over all parameter sets above
	QVector<          // characteristics index
		QVector<      // variation aggregation
		QVector<      // parameter index
		QVector<      // bin index
		double>>>>
		charHistVarAgg;

	//! aggregation of differences at each bin of characteristics distribution over all parameter sets above
	QVector<          // characteristics index
		QVector<      // bin index
		double>>
		charHistAvg;

	//! the dissimilarity measures that were used in computing the below dissimilarity matrix
	std::vector<std::pair<int, bool>> m_resultDissimMeasures;
	//! dissimilarity information for all result pairs
	iADissimilarityMatrixType m_resultDissimMatrix;
	//! the ranges of result dissimilarity pairs: (result1, result2) -> dissimilarity
	QVector<QPair<double, double>> m_resultDissimRanges;

	// unique fibers:
	//! one fiber is identified by resultID, fiberID
	using FiberKeyT = std::pair<size_t, size_t>;
	//! data structure for list of unique fibers; outside vectors: unique fibers; inside vector: "matches"
	using UniqueFibersT = std::vector<std::vector<FiberKeyT>>;
	//! data structure to speed up unique fiber finding process: stores the unique fiber ID for every <resultID, fiberID> pair
	using FiberToUniqueMapT = QMap<std::pair<size_t, size_t>, size_t>;

	//! list of unique fibers
	UniqueFibersT m_uniqueFibers;
	//! map to speed up unique fiber finding process
	FiberToUniqueMapT m_mapFiberToUnique;

	//! the dissimilarity measure that should be used to "optimize" computation of other dissimilarity measures
	//! (by only taking the first n matches according to this measure)
	//! probably unused currently, due to other optimization (by only considering fibers with matching bounding boxes)
	int m_resultDissimOptimMeasureIdx;

	//! image for holding overview over variation per voxel
	vtkSmartPointer<vtkImageData> m_spatialOverview;
	
	vtkSmartPointer<vtkImageData> m_averageFiberVoxel;

private:
	QString cacheFileName(QString fileName) const;
	QString uniqueFiberVarCacheFileName(size_t uIdx) const;
	QString resultFiberCacheFileName(size_t uIdx) const;
	QString volumePercentageCacheFileName() const;
	bool readDissimilarityMatrixCache(QVector<int>& measures);
	void writeDissimilarityMatrixCache(QVector<int> const& measures) const;

	double characteristicsDifference(int charIdx, qvectorsizetype r1Idx, qvectorsizetype r2Idx, int measureIdx);

	bool m_aborted;
};

class iASensitivityViewState
{
public:
	virtual std::vector<size_t> const& selectedResults() const = 0;
	virtual iAColorTheme const* selectedResultColorTheme() const = 0;
};
