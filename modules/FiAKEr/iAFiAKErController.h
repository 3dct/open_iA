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

// FiAKEr:
#include "iAFiberCharData.h"            // for iAFiberSimilarity -> REFACTOR!!!
#include "iAFiberCharUIData.h"
#include "iASelectionInteractorStyle.h" // for iASelectionProvider
#include "ui_FiAKErSettings.h"

// FeatureScout:
#include <iACsvConfig.h>

// Core:
#include <iASettings.h>
#include <iAVtkWidget.h>
#include <qthelper/iAQTtoUIConnector.h>
#include <qthelper/iAWidgetSettingsMapper.h>

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QMap>
#include <QSharedPointer>

#include <vector>

class iAFiberResultsCollection;
class iAFiberCharUIData;
class iAStackedBarChart;

class iA3DColoredPolyObjectVis;
class iA3DCylinderObjectVis;

class iAChartWidget;
class iAColorTheme;
class iADockWidgetWrapper;
class iAMapper;
class iAQSplom;
class iARendererViewSync;
class iARefDistCompute;
class iASensitivityInfo;
class iAVolumeRenderer;
class iAMainWindow;
class iAMdiChild;

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

class iAFixedAspectWidget;
class iASignallingWidget;


class iAFiAKErController: public QObject, public iASelectionProvider
{
	Q_OBJECT
public:
	typedef iAQTtoUIConnector<QWidget, Ui_FIAKERSettings> iAFIAKERSettingsWidget;
	typedef std::vector<std::vector<size_t>> SelectionType;
	static const QString FIAKERProjectID;

	iAFiAKErController(iAMainWindow* mainWnd, iAMdiChild* mdiChild);

	void loadProject(QSettings const & projectFile, QString const & fileName);
	void start(QString const & path, iACsvConfig const & config, double stepShift, bool useStepData, bool showPreview, bool createCharts);
	std::vector<std::vector<size_t> > & selection() override;
	void toggleDockWidgetTitleBars();
	void toggleSettings();
	//! Load given settings.
	//! @param settings needs to be passed by value, as it's used in a lambda!
	void loadSettings(iASettings settings);
	void saveSettings(QSettings & settings);
	//! Load additional data/state - a potentially set reference or sensitivity data
	//! @param settings needs to be passed by value, as it's used in a lambda!
	void loadAdditionalData(iASettings settings, QString projectFileName);
	void saveProject(QSettings& projectFile, QString  const& fileName);
signals:
	void setupFinished();
	void referenceComputed();
	void fiberSelectionChanged(SelectionType const& selection);
private slots:
	void toggleVis(int);
	void toggleBoundingBox(int);
	void referenceToggled();
	//void previewMouseClick(Qt::MouseButton buttons, Qt::KeyboardModifiers modifiers);
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
	void distributionColorThemeChanged(int index);
	void resultColorThemeChanged(int index);
	void stackedBarColorThemeChanged(int index);
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
	void computeSensitivity();
	void resetSensitivity();
	// 3D view:
	void showMainVis(size_t resultID, bool state);

	void styleChanged();
	void selectFibersFromSensitivity(SelectionType const& selection);
private:
	bool loadReferenceInternal(iASettings settings);
	void changeDistributionSource(int index);
	void updateHistogramColors();
	QColor getResultColor(size_t resultID);
	void getResultFiberIDFromSpmID(size_t spmID, size_t & resultID, size_t & fiberID);
	void clearSelection();
	void newSelection(QString const & source);
	size_t selectionSize() const;
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
	void updateRefDistPlots();
	bool matchQualityVisActive() const;
	void updateFiberContext();
	//void startFeatureScout(int resultID, iAMdiChild* newChild);
	void visitAllVisibleVis(std::function<void(QSharedPointer<iA3DColoredPolyObjectVis>, size_t resultID)> func);
	void setClippingPlanes(QSharedPointer<iA3DColoredPolyObjectVis> vis);

	void setupMain3DView();
	void setupSettingsView();
	QWidget* setupOptimStepView();
	QWidget* setupResultListView();
	QWidget* setupProtocolView();
	QWidget* setupSelectionView();

	//! all data about the fiber characteristics optimization results that are analyzed
	QSharedPointer<iAFiberResultsCollection> m_data;
	std::vector<iAFiberCharUIData> m_resultUIs;

	QSharedPointer<iARendererViewSync> m_renderManager;
	vtkSmartPointer<iASelectionInteractorStyle> m_style;
	iAColorTheme const * m_resultColorTheme;
	iAMainWindow* m_mainWnd;
	iAMdiChild* m_mdiChild;
	size_t m_referenceID;
	SelectionType m_selection;
	vtkSmartPointer<vtkTable> m_refVisTable;
	iACsvConfig m_config;
	QString m_colorByThemeName;
	bool m_useStepData, m_showPreviews, m_showCharts;
	bool m_showFiberContext, m_mergeContextBoxes, m_showWireFrame, m_showLines;
	double m_contextSpacing;
	QString m_parameterFile; //! (.csv-)file containing eventual parameters used in creating the loaded results
	std::vector<std::vector<double>> m_paramValues;

	//! number of bins in histograms in result list
	int m_histogramBins;
	//! opacity (0..255) of current selection and of context
	int m_selectionOpacity, m_contextOpacity;
	//! factors for the diameter of the current selection and of the context (< 1 -> shrink, > 1 extend, = 1.0 no change)
	double m_diameterFactor, m_contextDiameterFactor;
	//! column index for the columns of the result list:
	int m_nameActionColumn, m_previewColumn, m_histogramColumn, m_stackedBarColumn;

	QSharedPointer<iA3DCylinderObjectVis> m_nearestReferenceVis;

	QTimer * m_playTimer;
	iARefDistCompute* m_refDistCompute;
	iAWidgetMap m_settingsWidgetMap;

	// The different views and their elements:
	std::vector<iADockWidgetWrapper*> m_views;
	enum {
		ResultListView, OptimStepChart, SPMView, ProtocolView, SelectionView, SettingsView, DockWidgetCount
	};
	// 3D View:
	void ensureMain3DViewCreated(size_t resultID);
	iAVtkWidget* m_main3DWidget;
	vtkSmartPointer<vtkRenderer> m_ren;
	QCheckBox* m_chkboxShowReference;
	QCheckBox* m_chkboxShowLines;
	QSpinBox* m_spnboxReferenceCount;
	vtkSmartPointer<vtkActor> m_refLineActor;
	QWidget* m_showReferenceWidget;
	std::vector<vtkSmartPointer<vtkActor> > m_contextActors;
	iAMapper* m_diameterFactorMapper;
	vtkSmartPointer<vtkActor> m_sampleActor;

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
	QSharedPointer<iASensitivityInfo> m_sensitivityInfo;
	void connectSensitivity();
};
