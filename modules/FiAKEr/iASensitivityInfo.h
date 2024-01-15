// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <iASensitivityData.h>

#include <iAAbortListener.h>
#include <iAProgress.h>

#include <set>

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

class iASensitivityInfo: public QObject, public iAAbortListener
{
	Q_OBJECT
public:
	static std::shared_ptr<iASensitivityInfo> create(iAMdiChild* child,
		std::shared_ptr<iAFiberResultsCollection> data, QDockWidget* nextToDW,
		int histogramBins, int skipColumns, std::vector<iAFiberResultUIData> const& resultUIs,
		vtkRenderWindow* main3DWin,
		QString parameterSetFileName = QString(),
		QVector<int> const& charSelected = QVector<int>(),
		QVector<int> const& charDiffMeasure = QVector<int>(),
		QVariantMap const & projectFile = QVariantMap());
	static std::shared_ptr<iASensitivityInfo> load(iAMdiChild* child,
		std::shared_ptr<iAFiberResultsCollection> data, QDockWidget* nextToDW,
		QVariantMap const & projectFile, QString const& projectFileName,
		std::vector<iAFiberResultUIData> const& resultUIs, vtkRenderWindow* main3DWin);
	static bool hasData(QVariantMap const& settings);

	static const QString DefaultResultColorMap;

	void saveProject(QSettings& projectFile, QString  const& fileName);

	iASensitivityData& data();

	//! the GUI elements:
	std::shared_ptr<iASensitivityGUI> m_gui;

	// for interaction:
	std::vector<std::vector<size_t>> m_baseFiberSelection;
	std::vector<std::vector<size_t>> m_currentFiberSelection;

	void abort() override;
private:
	iASensitivityInfo(std::shared_ptr<iAFiberResultsCollection> data,
		QString const& parameterFileName, int skipColumns, QStringList const& paramNames,
		std::vector<std::vector<double>> const& paramValues, iAMdiChild* child,
		QDockWidget* nextToDW, std::vector<iAFiberResultUIData> const& resultUIs, vtkRenderWindow* main3DWin);
	QWidget* setupMatrixView(QVector<int> const& measures);

	void updateDifferenceView();
	void showSpatialOverview();
	QVector<QVector<double>> currentAggregatedSensitivityMatrix();

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
	QVariantMap m_projectToLoad;

	std::shared_ptr<iASensitivityData> m_data;
	std::set<size_t> m_spatialOverviewDataSets;
signals:
	void aborted();
	void resultSelected(size_t resultIdx, bool state);
	void fibersToSelect(std::vector<std::vector<size_t>> const & selection);
	void viewDifference(size_t result1, size_t result2);
	void resultColorsChanged(QString const & themeName);
public slots:
	void changeAggregation(int newAggregation);
	void changeDistributionMeasure(int newMeasure);
	void changedCharDiffMeasure(int newMeasure);
	void updateDissimilarity();
	void spHighlightChanged();
	void updateSPDifferenceColors();
	void updateSPHighlightColors();
	void updateSpatialOverviewColors();
	void updateSPSpacing(int value);
	void createGUI();
	void outputBarAdded(int outType, int outIdx);
	void outputBarRemoved(int outType, int outIdx);
	void fiberSelectionChanged(std::vector<std::vector<size_t>> const& selection);
	void histoChartTypeToggled(bool checked);
	void styleChanged();
	void algoInfoModeChanged(int mode);
	void algoToggleArrowHeads(int state);
	void algoToggleShowHighlight(int state);
	void algoToggleMergeHighlight(int state);
	void algoSetLegendWidth(int value);
	void normalizePerOutputChanged(int state);
	void colorInOutChanged(int state);
private slots:
	void dissimMatrixMeasureChanged(int);
	void dissimMatrixParameterChanged(int);
	void dissimMatrixColorMapChanged(int);
	void spPointHighlighted(size_t resultIdx, bool state);
	void spVisibleParamChanged();
	void parResultSelected(size_t resultIdx, Qt::KeyboardModifiers modifiers);
	void setSpatialOverviewTF(int dataSetIdx);
	void spatialOverviewVisibilityChanged(bool visible);
	void setSPParameterColorMap(QString const& colorMapName);
};

// Factor out as generic CSV reading class also used by iACsvIO?
//bool readParameterCSV(QString const& fileName, QString const& encoding, QString const& columnSeparator,
//	iACsvTableCreator& tblCreator, size_t resultCount, int numColumns);
