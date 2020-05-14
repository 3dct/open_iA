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

// FiAKEr:
#include "iAChangeableCameraWidget.h"
#include "iAFiberCharData.h"            // for iAFiberSimilarity -> REFACTOR!!!
#include "iASavableProject.h"
#include "iASelectionInteractorStyle.h" // for iASelectionProvider
#include "ui_FiAKErSettings.h"

// FeatureScout:
#include <iACsvConfig.h>

// Core:
#include <iASettings.h>
#include <iAVtkWidget.h>
#include <qthelper/iAQTtoUIConnector.h>

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QMap>
#include <QSharedPointer>

#include <vector>

class iAFiberResultsCollection;
class iAFiberCharUIData;
class iAJobListView;
class iAStackedBarChart;

// Sensitivity:
class iAMatrixWidget;
class iAParameterListView;

class iA3DColoredPolyObjectVis;
class iA3DCylinderObjectVis;

class iAChartWidget;
class iAColorTheme;
class iACsvVectorTableCreator;
class iADockWidgetWrapper;
class iAFileChooserWidget;
class iAMapper;
class iAQSplom;
class iARendererManager;
class iARefDistCompute;
class iASPLOMData;
class iAVolumeRenderer;
class MainWindow;
class MdiChild;

class vtkColorTransferFunction;
class vtkCubeSource;
class vtkPiecewiseFunction;
class vtkImageData;
class vtkTable;

class QActionGroup;
class QButtonGroup;
class QCheckBox;
class QComboBox;
class QGridLayout;
class QLabel;
class QLineEdit;
class QListView;
class QModelIndex;
class QSettings;
class QSlider;
class QSpinBox;
class QStandardItemModel;
class QTimer;
class QTreeView;
class QVBoxLayout;

// To be able to put non-QObject derived class in settingsWidgetMap
class iAQCheckBoxVector: public QObject, public QVector<QCheckBox*> { };
class iAQLineEditVector: public QObject, public QVector<QLineEdit*> { };

class iAVtkQtWidget;
class iAFixedAspectWidget;
class iASignallingWidget;

//! UI elements for each result
class iAFiberCharUIData
{
public:
	iAVtkQtWidget* vtkWidget = nullptr;
	QSharedPointer<iA3DColoredPolyObjectVis> mini3DVis;
	QSharedPointer<iA3DColoredPolyObjectVis> main3DVis;
	iAChartWidget* histoChart;
	iAStackedBarChart* stackedBars;
	iAFixedAspectWidget* previewWidget = nullptr;
	iASignallingWidget* nameActions;
	QWidget* topFiller, * bottomFiller;
	//! index where the plots for this result start
	size_t startPlotIdx;
};


class iAResultPairInfo
{
public:
	iAResultPairInfo();
	iAResultPairInfo(int measureCount);
	// average dissimilarity, per dissimilarity measure
	QVector<double> avgDissim;

	// for every fiber, and every dissimilarity measure, the n best matching fibers (in descending match quality)
	QVector<QVector<QVector<iAFiberSimilarity>>> fiberDissim;
};

using iADissimilarityMatrixType = QVector<QVector<iAResultPairInfo>>;

class iAFiAKErController: public QObject, public iASelectionProvider
{
	Q_OBJECT
public:
	typedef iAQTtoUIConnector<QWidget, Ui_FIAKERSettings> iAFIAKERSettingsWidget;
	typedef std::vector<std::vector<size_t> > SelectionType;
	static const QString FIAKERProjectID;

	iAFiAKErController(MainWindow* mainWnd, MdiChild* mdiChild);

	void loadProject(QSettings const & projectFile, QString const & fileName);
	void start(QString const & path, iACsvConfig const & config, double stepShift, bool useStepData, bool showPreview);
	std::vector<std::vector<size_t> > & selection() override;
	void toggleDockWidgetTitleBars();
	void toggleSettings();
	//! Load given settings.
	//! @param settings needs to be passed by value, as it's used in a lambda!
	void loadSettings(iASettings settings);
	void saveSettings(QSettings & settings);
	//! Load potential reference.
	//! @param settings needs to be passed by value, as it's used in a lambda!
	void loadReference(iASettings settings);
	void saveProject(QSettings& projectFile, QString  const& fileName);
signals:
	void setupFinished();
	void referenceComputed();
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
	void showReferenceInChartToggled();
	void linkPreviewsToggled();
	void distributionChartTypeChanged(int);
	void diameterFactorChanged(int);
	void contextDiameterFactorChanged(int);
	void contextSpacingChanged(double value);
	void showFiberContextChanged(int);
	void mergeFiberContextBoxesChanged(int);
	void showWireFrameChanged(int);
	void showLinesChanged(int);
	void showBoundingBoxChanged(int);
	void updateBoundingBox();
	// result view:
	void stackedColSelect();
	void switchStackMode(bool mode);
	void colorByDistrToggled();
	void exportDissimilarities();
	void sortByCurrentWeighting();
	// settings view:
	void update3D();
	void applyRenderSettings();
	// sensitivity:
	void sensitivitySlot();
	void dissimMatrixMeasureChanged(int);
	void dissimMatrixParameterChanged(int);
	void dissimMatrixColorMapChanged(int);
private:
	bool loadReferenceInternal(iASettings settings);
	void changeDistributionSource(int index);
	void updateHistogramColors();
	QColor getResultColor(size_t resultID);
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
	void toggleOptimStepChart(size_t index, bool visible);
	QString diffName(size_t chartID) const;
	QString resultName(size_t resultID) const;
	QString stackedBarColName(int index) const;
	void setOptimStep(int optimStep);
	void showSelectionDetail();
	void hideSamplePointsPrivate();
	void showSpatialOverview();
	void setReference(size_t referenceID, std::vector<std::pair<int, bool>> measures, int optimizationMeasure, int bestMeasure);
	void showMainVis(size_t resultID, int state);
	void updateRefDistPlots();
	bool matchQualityVisActive() const;
	void updateFiberContext();
	void startFeatureScout(int resultID, MdiChild* newChild);
	void visitAllVisibleVis(std::function<void(QSharedPointer<iA3DColoredPolyObjectVis>, size_t resultID)> func);
	void setClippingPlanes(QSharedPointer<iA3DColoredPolyObjectVis> vis);

	void setupMain3DView();
	void setupSettingsView();
	QWidget* setupOptimStepView();
	QWidget* setupResultListView();
	QWidget* setupProtocolView();
	QWidget* setupSelectionView();
	QWidget* setupMatrixView(iACsvVectorTableCreator& tblCreator, QVector<int> const & measures);

	//! all data about the fiber characteristics optimization results that are analyzed
	QSharedPointer<iAFiberResultsCollection> m_data;
	std::vector<iAFiberCharUIData> m_resultUIs;

	QSharedPointer<iARendererManager> m_renderManager;
	vtkSmartPointer<iASelectionInteractorStyle> m_style;
	iAColorTheme const * m_resultColorTheme;
	MainWindow* m_mainWnd;
	MdiChild* m_mdiChild;
	size_t m_referenceID;
	SelectionType m_selection;
	vtkSmartPointer<vtkTable> m_refVisTable;
	iACsvConfig m_config;
	QString m_colorByThemeName;
	bool m_useStepData, m_showPreviews;
	bool m_showFiberContext, m_mergeContextBoxes, m_showWireFrame, m_showLines;
	double m_contextSpacing;
	QString m_parameterFile; //! (.csv-)file containing eventual parameters used in creating the loaded results

	QSharedPointer<iA3DCylinderObjectVis> m_nearestReferenceVis;

	vtkSmartPointer<vtkActor> m_sampleActor;
	QTimer * m_playTimer;
	iARefDistCompute* m_refDistCompute;
	QMap<QString, QObject*> m_settingsWidgetMap;

	// The different views and their elements:
	std::vector<iADockWidgetWrapper*> m_views;
	enum {
		JobView, ResultListView, OptimStepChart, SPMView, ProtocolView, SelectionView, SettingsView, DockWidgetCount
	};
	// 3D View:
	iAVtkWidget* m_main3DWidget;
	vtkSmartPointer<vtkRenderer> m_ren;
	QCheckBox* m_chkboxShowReference;
	QCheckBox* m_chkboxShowLines;
	QSpinBox* m_spnboxReferenceCount;
	vtkSmartPointer<vtkActor> m_refLineActor;
	QWidget* m_showReferenceWidget;
	std::vector<vtkSmartPointer<vtkActor> > m_contextActors;
	iAMapper* m_diameterFactorMapper;

	vtkSmartPointer<vtkCubeSource> m_customBoundingBoxSource;
	vtkSmartPointer<vtkPolyDataMapper> m_customBoundingBoxMapper;
	vtkSmartPointer<vtkActor> m_customBoundingBoxActor;

	// Results List:
	void addStackedBar(int index);
	void removeStackedBar(int index);
	void updateResultList();
	iAStackedBarChart* m_stackedBarsHeaders;
	QGridLayout* m_resultsListLayout;
	QCheckBox* m_colorByDistribution;
	QComboBox* m_distributionChoice;
	iAQCheckBoxVector m_showResultVis;
	iAQCheckBoxVector m_showResultBox;
	QMap<size_t, int> m_resultListSorting;

	// Settings View:
	void addChartCB();
	iAFIAKERSettingsWidget* m_settingsView;
	// 3D view part
	iAQLineEditVector m_teBoundingBox;
	bool m_cameraInitialized;

	// Optimization steps part
	iAQCheckBoxVector m_chartCB;

	// Scatter plot matrix:
	void setSPMColorByResult();
	iAQSplom* m_spm;

	// Optimization Steps:
	QLabel* m_currentOptimStepLabel;
	std::vector<iAChartWidget*> m_optimStepChart;
	QSlider* m_optimStepSlider;
	QVBoxLayout* m_optimChartLayout;
	size_t m_chartCount;

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

	// Sensitivity
	iADissimilarityMatrixType m_dissimilarityMatrix;
	iAMatrixWidget* m_matrixWidget;
	iAParameterListView* m_parameterListView;

	QString dissimilarityMatrixCacheFileName();
	bool readDissimilarityMatrixCache(QVector<int>& measures);
	void writeDissimilarityMatrixCache(QVector<int> const& measures);
};
