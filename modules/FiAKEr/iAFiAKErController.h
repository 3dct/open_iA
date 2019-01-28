/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iASelectionInteractorStyle.h" // for iASelectionProvider

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QMap>
#include <QSharedPointer>

#include <vector>

class iAFiberResultsCollection;
class iAFiberCharUIData;
class iAJobListView;
class iAStackedBarChart;

class iA3DCylinderObjectVis;

class iAChartWidget;
class iAColorTheme;
class iADockWidgetWrapper;
class iAQSplom;
class iARendererManager;
class iARefDistCompute;
class iASPLOMData;
class MainWindow;

#include <vtkVersion.h>
#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
class QVTKOpenGLWidget;
typedef QVTKOpenGLWidget iAVtkWidgetClass;
#else
class QVTKWidget2;
typedef QVTKWidget2 iAVtkWidgetClass;
#endif

class vtkTable;

class QActionGroup;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QListView;
class QModelIndex;
class QSlider;
class QSpinBox;
class QStandardItemModel;
class QTimer;
class QTreeView;
class QVBoxLayout;
//class QWebEngineView;

class iAFiAKErController : public QMainWindow, public iASelectionProvider
{
	Q_OBJECT
public:
	typedef std::vector<std::vector<size_t> > SelectionType;
	iAFiAKErController(MainWindow* mainWnd);
	void start(QString const & path, QString const & configName, double stepShift);
	~iAFiAKErController();
	std::vector<std::vector<size_t> > & selection() override;
	static void loadAnalysis(MainWindow* mainWnd, QString const & folder);
signals:
	void setupFinished();
public slots:
	void toggleFullScreen();
private slots:
	void toggleVis(int);
	void toggleBoundingBox(int);
	void referenceToggled();
	void miniMouseEvent(QMouseEvent* ev);
	void optimStepSliderChanged(int);
	void mainOpacityChanged(int);
	void contextOpacityChanged(int);
	void selection3DChanged();
	void selectionSPMChanged(std::vector<size_t> const & selection);
	void selectionOptimStepChartChanged(std::vector<size_t> const & selection);
	void spmLookupTableChanged();
	void changeReferenceDisplay();
	void playPauseOptimSteps();
	void playTimer();
	void playDelayChanged(int);
	void refDistAvailable();
	void optimDataToggled(int);
	void resultsLoaded();
	void resultsLoadFailed(QString const & path);
	void visualizeCylinderSamplePoints();
	void hideSamplePoints();
	void showReferenceToggled();
	void showReferenceLinesToggled();
	void showReferenceCountChanged(int);
	void showReferenceMeasureChanged(int);
	void selectionFromListActivated(QModelIndex const &);
	void selectionDetailsItemClicked(QModelIndex const &);
	void showSpatialOverviewButton();
	void selectionModeChanged(int);
	void distributionChoiceChanged(int index);
	void histogramBinsChanged(int value);
	void distributionColorThemeChanged(QString const & colorThemeName);
	void resultColorThemeChanged(QString const & colorThemeName);
	void stackedBarColorThemeChanged(QString const & colorThemeName);
	void saveAnalysisClick();
	void loadAnalysisClick();
	void showReferenceInChartToggled();
	void distributionChartTypeChanged(int);
	void diameterFactorChanged(int);
	void showFiberContextChanged(int);
	void mergeFiberContextBoxesChanged(int);
	// result view:
	void stackedColSelect();
	void switchStackMode(bool mode);
	void colorByDistrToggled();
	void setProjectReference();
private:
	void changeDistributionSource(int index);
	void updateHistogramColors();
	QColor getResultColor(int resultID);
	void getResultFiberIDFromSpmID(size_t spmID, size_t & resultID, size_t & fiberID);
	void clearSelection();
	void newSelection(QString const & source);
	size_t selectionSize() const;
	void sortSelection(QString const & source);
	void showSelectionInPlots();
	void showSelectionInPlot(int chartID);
	void showSelectionIn3DViews();
	void showSelectionInSPM();
	bool isAnythingSelected() const;
	void loadStateAndShow();
	void addInteraction(QString const & interaction);
	void toggleOptimStepChart(int index, bool visible);
	QString diffName(int chartID) const;
	QString resultName(size_t resultID) const;
	QString stackedBarColName(int index) const;
	void setOptimStep(int optimStep);
	void showSelectionDetail();
	void hideSamplePointsPrivate();
	void showSpatialOverview();
	void setReference(size_t referenceID);
	void showMainVis(size_t resultID, int state);
	void updateRefDistPlots();
	bool matchQualityVisActive() const;
	void updateFiberContext();
	void contextSpacingChanged(double value);

	QWidget* setupMain3DView();
	QWidget* setupSettingsView();
	QWidget* setupOptimStepView();
	QWidget* setupResultListView();
	QWidget* setupProtocolView();
	QWidget* setupSelectionView();

	//! all data about the fiber characteristics optimization results that are analyzed
	QSharedPointer<iAFiberResultsCollection> m_data;
	std::vector<iAFiberCharUIData> m_resultUIs;

	QSharedPointer<iARendererManager> m_renderManager;
	vtkSmartPointer<iASelectionInteractorStyle> m_style;
	iAColorTheme const * m_resultColorTheme;
	MainWindow* m_mainWnd;
	size_t m_referenceID;
	SelectionType m_selection;
	vtkSmartPointer<vtkTable> m_refVisTable;

	QSharedPointer<iA3DCylinderObjectVis> m_nearestReferenceVis;

	vtkSmartPointer<vtkActor> m_sampleActor;
	QString m_configName;
	QTimer * m_playTimer;
	iARefDistCompute* m_refDistCompute;
	QString m_colorByThemeName;

	bool m_showFiberContext, m_mergeContextBoxes;
	double m_contextSpacing;

	// Elements of the different views:
	std::vector<iADockWidgetWrapper*> m_views;
	enum {
		JobView, ResultListView, Main3DView, OptimStepChart, SPMView, ProtocolView, SelectionView, SettingsView, DockWidgetCount
	};
	// Main Renderer:
	iAVtkWidgetClass* m_mainRenderer;
	QLabel * m_defaultOpacityLabel, *m_contextOpacityLabel, *m_diameterFactorLabel;
	QSlider* m_defaultOpacitySlider, *m_contextOpacitySlider;
	QCheckBox* m_chkboxShowReference;
	QCheckBox* m_chkboxShowLines;
	QSpinBox* m_spnboxReferenceCount;
	QComboBox* m_cmbboxSimilarityMeasure;
	vtkSmartPointer<vtkActor> m_refLineActor;
	QWidget* m_showReferenceWidget;
	std::vector<vtkSmartPointer<vtkActor> > m_contextActors;

	size_t m_projectReferenceID;

	// Results List:
	void addStackedBar(int index);
	void removeStackedBar(int index);
	iAStackedBarChart* m_stackedBarsHeaders;
	QGridLayout* m_resultsListLayout;
	QCheckBox* m_colorByDistribution;
	QComboBox* m_distributionChoice;
	QCheckBox* m_showReferenceInChart;
	QComboBox* m_distributionChartType;
	QComboBox* m_resultColorThemeChoice;

	// Scatter plot matrix:
	void setSPMColorByResult();
	iAQSplom* m_spm;

	// Optimization Steps:
	QLabel* m_currentOptimStepLabel;
	std::vector<iAChartWidget*> m_optimStepChart;
	QSlider* m_optimStepSlider;
	QVBoxLayout* m_optimChartLayout;
	std::vector<QCheckBox*> m_chartCB;
	size_t ChartCount;

	// Jobs:
	iAJobListView * m_jobs;

	// Interaction Protocol:
	QTreeView* m_interactionProtocol;
	QStandardItemModel* m_interactionProtocolModel;

	// Selections:
	QListView* m_selectionList;
	QTreeView* m_selectionDetailsTree;
	QStandardItemModel* m_selectionListModel;
	QStandardItemModel* m_selectionDetailModel;
	std::vector<SelectionType> m_selections;
};
