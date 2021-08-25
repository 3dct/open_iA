/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include <iAMatrixWidget.h> // for iADissimilarityMatrixType

#include <iAAbortListener.h>
#include <iAProgress.h>
#include <iASettings.h>

#include <QPair>
#include <QSharedPointer>
#include <QStringList>
#include <QVector>

class iAColorTheme;
class iAMdiChild;

class iACsvTableCreator;
class iAFiberResultUIData;
class iAFiberResultsCollection;
class iASensitivityGUI;

class vtkActor;
class vtkImageData;
class vtkPolyData;
class vtkRenderWindow;

class QDockWidget;

class iASensitivityData
{
public:
	iASensitivityData(QSharedPointer<iAFiberResultsCollection> data, QStringList const& paramNames,
		std::vector<std::vector<double>> const& paramValues);

	// DATA / COMPUTED DATA:
	//
	//! the "original" FIAKER data of all loaded results:
	QSharedPointer<iAFiberResultsCollection> m_data;
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

	QVector<                   //! For each result,
		QVector<               //! for each characteristic,
			QVector<double>>>  //! a histogram (bin)
		m_charHistograms;

	int m_numOfSTARSteps, m_starGroupSize;
	int m_histogramBins;
	QVector<double> paramStep;  //!< per varied parameter, the size of step performed for the STAR

	QVector<int> m_variedParams;  //!< indices of the parameters that were varied

	//! sensitivity "field" for characteristics
	QVector<                  // characteristic (index in m_charSelected)
		QVector<              // characteristics difference measure index (index in m_charDiffMeasure)
			QVector<          // variation aggregation (see iASensitivityInfo::create)
				QVector<      // parameter index (second index in paramSetValues / allParamValues)
					QVector<  // parameter set index (first index in paramSetValues)
						double>>>>>
		sensitivityField;

	//! averages over all parameter-sets of above field ("global sensitivity" for a parameter)
	QVector<              // characteristis
		QVector<          // difference measure
			QVector<      // variation aggregation
				QVector<  // parameter index
					double>>>>
		aggregatedSensitivities;

	//! sensitivity at each parameter regarding fiber count
	QVector<          // variation aggregation
		QVector<      // parameter index
			QVector<  // parameter set index
				double>>>
		sensitivityFiberCount;
	//! averages over all parameter-sets of above field ("global sensitivity" for a parameter, regarding fiber count)
	QVector<      // variation aggregation
		QVector<  // parameter index
			double>>
		aggregatedSensitivitiesFiberCount;

	//! range of the fiber counts over all results
	double m_fiberCountRange[2];
	//! histogram of the fiber counts over all results
	QVector<double> fiberCountHistogram;

	//! sensitivity "field" for dissimilarity measures
	QVector<              // dissimilarity measure (index in m_resultDissimMeasures)
		QVector<          // variation aggregation (see iASensitivityInfo::create)
			QVector<      // parameter index (second index in paramSetValues / allParamValues)
				QVector<  // parameter set index (first index in paramSetValues)
					double>>>>
		sensDissimField;

	//! averages over all parameter-sets of above field ("global sensitivity" for a parameter by dissimilarity measures)
	QVector<          // dissimilarity measure (index in m_resultDissimMeasures)
		QVector<      // variation aggregation
			QVector<  // parameter index
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
	QVector<        // characteristics index
		QVector<    // variation aggregation
		QVector<    // parameter index
		QVector<    // bin index
		QVector<    // parameter set index
		QVector<    // index of value (stores frequency values of original histogram)
		double>>>>>> charHistHist;
	*/

	//!  difference at each bin of characteristics distribution (histogram above)
	QVector<                  // characteristics index
		QVector<              // variation aggregation
			QVector<          // parameter index
				QVector<      // bin index
					QVector<  // parameter set index
						double>>>>>
		charHistVar;

	//! aggregation of differences at each bin of characteristics distribution over all parameter sets above
	QVector<              // characteristics index
		QVector<          // variation aggregation
			QVector<      // parameter index
				QVector<  // bin index
					double>>>>
		charHistVarAgg;

	//! aggregation of differences at each bin of characteristics distribution over all parameter sets above
	QVector<      // characteristics index
		QVector<  // bin index
			double>>
		charHistAvg;

	// per-object sensitivity:
	// required: 1-1 match between fibers
	// compute on the fly? spatial subdivision structure required...

	// Questions:
	// options for characteristic comparison:
	//    1. compute characteristic distribution difference
	//        - advantage: dissimilarity measure independent
	//        - disadvantage: distribution could be same even if lots of differences for single fibers
	//    2. compute matching fibers; then compute characteristic difference; then average this
	//        - advantage: represents actual differences better
	//        - disadvantage: depending on dissimilarity measure (since best match could be computed per dissimiliarity measure
	//    example: compare
	//         - result 1 with fibers a (len=5), b (len=3) and c (len=2)
	//         - result 2 with fibers A (len 3), B (len=2) and C (len=5)
	//         - best matches between result1&2: a <-> A, b <-> B, c <-> C
	//         - option 1 -> exactly the same, 1x5, 1x3, 1x2
	//         - option 2 -> length differences: 2, 1, 3

	// dissimilarity pairs:
	std::vector<std::pair<int, bool>> m_resultDissimMeasures;
	iADissimilarityMatrixType m_resultDissimMatrix;

	// unique fibers:
	//! one fiber is identified by resultID, fiberID
	using FiberKeyT = std::pair<size_t, size_t>;
	//! list of unique fibers; outside vectors: unique fibers; inside vector: "matches"
	using UniqueFibersT = std::vector<std::vector<FiberKeyT>>;

	using FiberToUniqueMapT = QMap<std::pair<size_t, size_t>, size_t>;

	UniqueFibersT m_uniqueFibers;
	FiberToUniqueMapT m_mapFiberToUnique;

	QVector<QPair<double, double>> m_resultDissimRanges;
	int m_resultDissimOptimMeasureIdx;

	QString charactName(int selCharIdx) const;
};

class iASensitivityViewState
{
public:
	virtual std::vector<size_t> const & selectedResults() const =0;
	virtual iAColorTheme const* selectedResultColorTheme() const =0;
};

class iASensitivityInfo: public QObject, public iAAbortListener
{
	Q_OBJECT
public:
	static QSharedPointer<iASensitivityInfo> create(iAMdiChild* child,
		QSharedPointer<iAFiberResultsCollection> data, QDockWidget* nextToDW,
		int histogramBins, int skipColumns, std::vector<iAFiberResultUIData> const& resultUIs,
		vtkRenderWindow* main3DWin,
		QString parameterSetFileName = QString(),
		QVector<int> const& charSelected = QVector<int>(),
		QVector<int> const& charDiffMeasure = QVector<int>(),
		iASettings const & projectFile = iASettings());
	static QSharedPointer<iASensitivityInfo> load(iAMdiChild* child,
		QSharedPointer<iAFiberResultsCollection> data, QDockWidget* nextToDW,
		iASettings const & projectFile, QString const& projectFileName,
		std::vector<iAFiberResultUIData> const& resultUIs, vtkRenderWindow* main3DWin);
	static bool hasData(iASettings const& settings);

	void saveProject(QSettings& projectFile, QString  const& fileName);

	iASensitivityData& data();

	//! the GUI elements:
	QSharedPointer<iASensitivityGUI> m_gui;

	// for interaction:
	std::vector<std::vector<size_t>> m_baseFiberSelection;
	std::vector<std::vector<size_t>> m_currentFiberSelection;

	void abort() override;
private:
	iASensitivityInfo(QSharedPointer<iAFiberResultsCollection> data,
		QString const& parameterFileName, int skipColumns, QStringList const& paramNames,
		std::vector<std::vector<double>> const& paramValues, iAMdiChild* child,
		QDockWidget* nextToDW, std::vector<iAFiberResultUIData> const& resultUIs, vtkRenderWindow* main3DWin);
	void compute(iAProgress* p);
	QString cacheFileName(QString fileName) const;
	QString dissimilarityMatrixCacheFileName() const;
	QString spatialOverviewCacheFileName() const;
	QString uniqueFiberVarCacheFileName(size_t uIdx) const;
	QString volumePercentageCacheFileName() const;
	bool readDissimilarityMatrixCache(QVector<int>& measures);
	void writeDissimilarityMatrixCache(QVector<int> const& measures) const;
	QWidget* setupMatrixView(QVector<int> const& measures);

	void updateDifferenceView();
	void computeSpatialOverview(iAProgress* p);
	void showSpatialOverview();
	
	QString m_parameterFileName;
	int m_skipColumns;
	// UI elements:
	iAMdiChild* m_child;
	QDockWidget* m_nextToDW;
	std::vector<iAFiberResultUIData> const& m_resultUIs;
	vtkRenderWindow* m_main3DWin;

	// for computation:
	bool m_aborted;
	//! "temporary" copy of project to load:
	iASettings m_projectToLoad;
	//! image for holding overview over variation per voxel
	vtkSmartPointer<vtkImageData> m_spatialOverview;

	QSharedPointer<iASensitivityData> m_data;
signals:
	void aborted();
	void resultSelected(size_t resultIdx, bool state);
	void fibersToSelect(std::vector<std::vector<size_t>> const & selection);
	void viewDifference(size_t result1, size_t result2);
public slots:
	void changeAggregation(int newAggregation);
	void changeMeasure(int newMeasure);
	void updateDissimilarity();
	void spHighlightChanged();
	void updateSPDifferenceColors();
	void updateSPHighlightColors();
	void createGUI();
	void outputBarAdded(int outType, int outIdx);
	void outputBarRemoved(int outType, int outIdx);
	void fiberSelectionChanged(std::vector<std::vector<size_t>> const& selection);
	void histoChartTypeToggled(bool checked);
	void styleChanged();
private slots:
	void dissimMatrixMeasureChanged(int);
	void dissimMatrixParameterChanged(int);
	void dissimMatrixColorMapChanged(int);
	void spPointHighlighted(size_t resultIdx, bool state);
	void spVisibleParamChanged();
	void parResultSelected(size_t resultIdx, Qt::KeyboardModifiers modifiers);
};

// Factor out as generic CSV reading class also used by iACsvIO?
//bool readParameterCSV(QString const& fileName, QString const& encoding, QString const& columnSeparator,
//	iACsvTableCreator& tblCreator, size_t resultCount, int numColumns);
