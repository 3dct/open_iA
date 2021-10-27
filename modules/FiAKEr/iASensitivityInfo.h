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

#include <iASensitivityData.h>

#include <iAAbortListener.h>
#include <iAProgress.h>
#include <iASettings.h>

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

	static const QString DefaultResultColorMap;

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
	iASettings m_projectToLoad;

	QSharedPointer<iASensitivityData> m_data;
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
	void setSpatialOverviewTF(int modalityIdx);
	void spatialOverviewVisibilityChanged(bool visible);
};

// Factor out as generic CSV reading class also used by iACsvIO?
//bool readParameterCSV(QString const& fileName, QString const& encoding, QString const& columnSeparator,
//	iACsvTableCreator& tblCreator, size_t resultCount, int numColumns);
