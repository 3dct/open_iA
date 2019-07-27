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
#include "iAFiAKErController.h"

#include "iAFiberCharData.h"
#include "iAFiberData.h"     // for samplePoints
#include "iAJobListView.h"
#include "iARefDistCompute.h"
#include "iAStackedBarChart.h"

// FeatureScout:
#include "iA3DCylinderObjectVis.h"
#include "iA3DEllipseObjectVis.h"
#include "iACsvConfig.h"
#include "iAFeatureScoutModuleInterface.h"
#include "iAVectorPlotData.h"

// Core:
#include "charts/iAChartWidget.h"
#include "charts/iAHistogramData.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAScatterPlot.h" // for selection mode: iAScatterPlot::Rectangle
#include "charts/iAQSplom.h"
#include "charts/iASPLOMData.h"
#include "iAColorTheme.h"
#include "iAConnector.h"
#include "iAConsole.h"
#include "iALookupTable.h"
#include "iALUT.h"
#include "iAMapperImpl.h"
#include "iAMathUtility.h"
#include "iAModuleDispatcher.h"
#include "iARendererManager.h"
#include "iAStringHelper.h"
#include "iATransferFunction.h"
#include "iAVolumeRenderer.h"
#include "iAVtkWidget.h"
#include "io/iAITKIO.h"
#include "io/iAFileChooserWidget.h"
#include "io/iAFileUtils.h"
#include "mainwindow.h"
#include "mdichild.h"
#include "qthelper/iADockWidgetWrapper.h"
#include "qthelper/iAFixedAspectWidget.h"
#include "qthelper/iASignallingWidget.h"

#include <vtkColorTransferFunction.h>
#include <vtkCubeSource.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageData.h>
#include <vtkLine.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkTable.h>
#include <vtkVersion.h>
#include <vtkVertexGlyphFilter.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QListView>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QMouseEvent>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QStandardItemModel>
#include <QTimer>
#include <QTreeView>

#include <QtGlobal> // for QT_VERSION

#include <array>

namespace
{
	const int DockWidgetMargin = 3;
	const int SettingSpacing = 4;

	const int HistogramMinWidth = 80;
	const int StackedBarMinWidth = 70;
	const int DefaultPlayDelay = 1000;
	int HistogramBins = 20;
	int SelectionOpacity = iA3DLineObjectVis::DefaultSelectionOpacity;
	int ContextOpacity = iA3DLineObjectVis::DefaultContextOpacity;
	const double MinDiameterFactor = 0.02;
	const double MaxDiameterFactor = 2;
	const int MinFactorSliderVal = 1;
	const int MaxFactorSliderVal = 100;
	double DiameterFactor = 1.0;
	double ContextDiameterFactor = 1.0;
	const size_t NoPlotsIdx = std::numeric_limits<size_t>::max();
	const size_t NoResult = NoPlotsIdx;
	const QString ModuleSettingsKey("FIAKER");
	const QString RefMarker(" (Reference)");

	const QString ProjectFileFolder("Folder");
	const QString ProjectFileFormat("Format");
	const QString ProjectFileReference("Reference");
	const QString ProjectFileStepShift("StepShift");
	
	const QString DefaultResultColorTheme("Brewer Accent (max. 8)");
	const QString DefaultStackedBarColorTheme("Material red (max. 10)");

	const int DistributionRefAlpha = 80;
	const QColor OptimStepMarkerColor(192, 0, 0);
	const QColor SelectionColor(0, 0, 0);
	const QColor ReferenceColor(235, 235, 235);
	QColor StandardBackgroundColor;

	enum ResultListColumns
	{
		NameActionColumn,
		PreviewColumn,
		StackedBarColumn,
		HistogramColumn
	};

}

//! UI elements for each result
class iAFiberCharUIData
{
public:
	iAVtkWidget* vtkWidget;
	QSharedPointer<iA3DColoredPolyObjectVis> mini3DVis;
	QSharedPointer<iA3DColoredPolyObjectVis> main3DVis;
	QCheckBox* cbBoundingBox;
	QCheckBox* cbShow;
	iAChartWidget* histoChart;
	iAStackedBarChart* stackedBars;
	iAFixedAspectWidget* previewWidget;
	iASignallingWidget* nameActions;
	QLabel* nameLabel;
	QWidget* topFiller, * bottomFiller;
	//! index where the plots for this result start
	size_t startPlotIdx;
};

iAFiAKErController::iAFiAKErController(MainWindow* mainWnd) :
	m_resultColorTheme(iAColorThemeManager::instance().theme(DefaultResultColorTheme)),
	m_mainWnd(mainWnd),
	m_spm(new iAQSplom()),
	m_referenceID(NoResult),
	m_playTimer(new QTimer(this)),
	m_refDistCompute(nullptr),
	m_renderManager(new iARendererManager()),
	m_colorByThemeName(iALUT::GetColorMapNames()[0]),
	m_showFiberContext(false),
	m_mergeContextBoxes(false),
	m_contextSpacing(0.0)
{
	setDockOptions(AllowNestedDocks | AllowTabbedDocks);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif
	setMinimumSize(600, 400);
	setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
	connect(mainWnd, &MainWindow::fullScreenToggled, this, &iAFiAKErController::toggleFullScreen);
}

void iAFiAKErController::toggleFullScreen()
{
	QWidget* mdiSubWin = qobject_cast<QWidget*>(parent());
	if (m_mainWnd->isFullScreen())
		mdiSubWin->setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	else
		mdiSubWin->setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
	mdiSubWin->show();
}

void iAFiAKErController::start(QString const & path, QString const & configName, double stepShift)
{
	m_configName = configName;
	m_jobs = new iAJobListView(DockWidgetMargin);
	m_views.resize(DockWidgetCount);
	m_views[JobView] = new iADockWidgetWrapper(m_jobs, "Jobs", "foeJobs");
	addDockWidget(Qt::BottomDockWidgetArea, m_views[JobView]);

	m_data = QSharedPointer<iAFiberResultsCollection>(new iAFiberResultsCollection());
	auto resultsLoader = new iAFiberResultsLoader(m_data, path, configName, stepShift);
	connect(resultsLoader, SIGNAL(finished()), this, SLOT(resultsLoaded()));
	connect(resultsLoader, SIGNAL(failed(QString const &)), this, SLOT(resultsLoadFailed(QString const &)));
	m_jobs->addJob("Loading results...", resultsLoader->progress(), resultsLoader);
	resultsLoader->start();
}

void iAFiAKErController::resultsLoadFailed(QString const & path)
{
	QMessageBox::warning(m_mainWnd, "Fiber Analytics",
		QString("Could not load data in folder '%1'. Make sure it is in the right format!").arg(path));
	delete parent(); // deletes QMdiSubWindow which this widget is child of
	return;
}

void iAFiAKErController::resultsLoaded()
{
	m_resultUIs.resize(m_data->result.size());
	m_selection.resize(m_data->result.size());

	auto main3DView = setupMain3DView();
	auto settingsView = setupSettingsView();
	auto optimStepsView = setupOptimStepView();
	auto resultListView = setupResultListView();
	auto protocolView = setupProtocolView();
	auto selectionView = setupSelectionView();

	m_views.resize(DockWidgetCount);
	m_views[ResultListView] = new iADockWidgetWrapper(resultListView, "Result list", "foeResultList");
	m_views[Main3DView]     = new iADockWidgetWrapper(main3DView, "3D view", "foe3DView");
	m_views[OptimStepChart] = new iADockWidgetWrapper(optimStepsView, "Optimization Steps", "foeTimeSteps");
	m_views[SPMView]        = new iADockWidgetWrapper(m_spm, "Scatter Plot Matrix", "foeSPM");
	m_views[ProtocolView]   = new iADockWidgetWrapper(protocolView, "Interactions", "foeInteractions");
	m_views[SelectionView]  = new iADockWidgetWrapper(selectionView, "Selections", "foeSelections");
	m_views[SettingsView]   = new iADockWidgetWrapper(settingsView, "Settings", "foeSettings");

	splitDockWidget(m_views[JobView], m_views[ResultListView], Qt::Vertical);
	splitDockWidget(m_views[ResultListView], m_views[Main3DView], Qt::Horizontal);
	splitDockWidget(m_views[ResultListView], m_views[OptimStepChart], Qt::Vertical);
	splitDockWidget(m_views[Main3DView], m_views[SPMView], Qt::Horizontal);
	splitDockWidget(m_views[ResultListView], m_views[ProtocolView], Qt::Vertical);
	splitDockWidget(m_views[Main3DView], m_views[SelectionView], Qt::Vertical);
	splitDockWidget(m_views[Main3DView], m_views[SettingsView], Qt::Vertical);

	loadStateAndShow();
}

iAFiAKErController::~iAFiAKErController()
{
	if (parent())
	{
		QSettings settings;
		settings.setValue(ModuleSettingsKey + "/maximized", isMaximized());
		if (!isMaximized())
			settings.setValue(ModuleSettingsKey + "/geometry", qobject_cast<QWidget*>(parent())->geometry());
		settings.setValue(ModuleSettingsKey + "/state", saveState());
	}
}

QWidget * iAFiAKErController::setupMain3DView()
{
	m_mainRenderer = new iAVtkWidget();
	auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	m_ren = vtkSmartPointer<vtkRenderer>::New();
	m_ren->SetBackground(1.0, 1.0, 1.0);
	renWin->SetAlphaBitPlanes(1);
	m_ren->SetUseDepthPeeling(true);
	m_ren->SetMaximumNumberOfPeels(1000);
	renWin->AddRenderer(m_ren);
	m_mainRenderer->SetRenderWindow(renWin);
	m_renderManager->addToBundle(m_ren);
	m_style = vtkSmartPointer<iASelectionInteractorStyle>::New();
	m_style->setSelectionProvider(this);
	m_style->assignToRenderWindow(renWin);
	m_style->setRenderer(m_ren);
	connect(m_style.GetPointer(), &iASelectionInteractorStyle::selectionChanged, this, &iAFiAKErController::selection3DChanged);

	m_showReferenceWidget = new QWidget();
	m_chkboxShowReference = new QCheckBox("Show ");
	m_spnboxReferenceCount = new QSpinBox();
	m_spnboxReferenceCount->setValue(1);
	m_spnboxReferenceCount->setMinimum(1);
	m_spnboxReferenceCount->setMaximum(1);
	m_showReferenceWidget->setLayout(new QHBoxLayout());
	m_showReferenceWidget->layout()->setContentsMargins(0, 0, 0, 0);
	m_showReferenceWidget->layout()->setSpacing(SettingSpacing);
	m_chkboxShowLines = new QCheckBox("Connect");
	connect(m_chkboxShowReference, &QCheckBox::stateChanged, this, &iAFiAKErController::showReferenceToggled);
	connect(m_spnboxReferenceCount, SIGNAL(valueChanged(int)), this, SLOT(showReferenceCountChanged(int)));
	connect(m_chkboxShowLines, &QCheckBox::stateChanged, this, &iAFiAKErController::showReferenceLinesToggled);
	m_showReferenceWidget->layout()->addWidget(m_chkboxShowReference);
	m_showReferenceWidget->layout()->addWidget(m_spnboxReferenceCount);
	m_showReferenceWidget->layout()->addWidget(new QLabel("nearest ref. fibers"));
	m_showReferenceWidget->layout()->addWidget(m_chkboxShowLines);
	m_showReferenceWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QWidget* mainRendererContainer = new QWidget();
	mainRendererContainer->setLayout(new QVBoxLayout());
	mainRendererContainer->layout()->setContentsMargins(DockWidgetMargin, DockWidgetMargin, DockWidgetMargin, DockWidgetMargin);
	mainRendererContainer->layout()->addWidget(m_mainRenderer);
	mainRendererContainer->layout()->addWidget(m_showReferenceWidget);
	return mainRendererContainer;
}

QWidget* iAFiAKErController::setupSettingsView()
{
	m_defaultOpacitySlider = new QSlider(Qt::Horizontal);
	m_defaultOpacitySlider->setMinimum(0);
	m_defaultOpacitySlider->setMaximum(255);
	m_defaultOpacitySlider->setValue(SelectionOpacity);
	connect(m_defaultOpacitySlider, &QSlider::valueChanged, this, &iAFiAKErController::mainOpacityChanged);
	m_defaultOpacityLabel = new QLabel(QString::number(SelectionOpacity, 'f', 2));

	m_contextOpacitySlider = new QSlider(Qt::Horizontal);
	m_contextOpacitySlider->setMinimum(0);
	m_contextOpacitySlider->setMaximum(255);
	m_contextOpacitySlider->setValue(ContextOpacity);
	connect(m_contextOpacitySlider, &QSlider::valueChanged, this, &iAFiAKErController::contextOpacityChanged);
	m_contextOpacityLabel = new QLabel(QString::number(ContextOpacity, 'f', 2));

	m_diameterFactorMapper = new iALinearMapper(MinDiameterFactor, MaxDiameterFactor, MinFactorSliderVal, MaxFactorSliderVal);
	auto diameterFactorSlider = new QSlider(Qt::Horizontal);
	diameterFactorSlider->setMinimum(MinFactorSliderVal);
	diameterFactorSlider->setMaximum(MaxFactorSliderVal);
	int factorSliderValue = static_cast<int>(m_diameterFactorMapper->srcToDst(DiameterFactor));
	diameterFactorSlider->setValue(factorSliderValue);
	connect(diameterFactorSlider, &QSlider::valueChanged, this, &iAFiAKErController::diameterFactorChanged);
	m_diameterFactorLabel = new QLabel(QString::number(m_diameterFactorMapper->dstToSrc(factorSliderValue), 'f', 2));
	
	auto contextDiameterFactorSlider = new QSlider(Qt::Horizontal);
	contextDiameterFactorSlider->setMinimum(MinFactorSliderVal);
	contextDiameterFactorSlider->setMaximum(MaxFactorSliderVal);
	int contextFactorSlider = static_cast<int>(m_diameterFactorMapper->srcToDst(ContextDiameterFactor));
	contextDiameterFactorSlider->setValue(contextFactorSlider);
	connect(contextDiameterFactorSlider, &QSlider::valueChanged, this, &iAFiAKErController::contextDiameterFactorChanged);
	m_contextDiameterFactorLabel = new QLabel(QString::number(m_diameterFactorMapper->dstToSrc(contextFactorSlider), 'f', 2));

	QWidget* sliderWidget = new QWidget();
	auto sliderWidgetLayout = new QGridLayout();
	sliderWidget->setLayout(sliderWidgetLayout);
	sliderWidgetLayout->setContentsMargins(0, 0, 0, 0);
	sliderWidgetLayout->setSpacing(SettingSpacing);
	sliderWidgetLayout->addWidget(new QLabel("Main Opacity:"), 0, 0);
	sliderWidgetLayout->addWidget(m_defaultOpacitySlider, 0, 1);
	sliderWidgetLayout->addWidget(m_defaultOpacityLabel, 0, 2);
	sliderWidgetLayout->addWidget(new QLabel("Context Opacity:"), 1, 0);
	sliderWidgetLayout->addWidget(m_contextOpacitySlider, 1, 1);
	sliderWidgetLayout->addWidget(m_contextOpacityLabel, 1, 2);
	sliderWidgetLayout->addWidget(new QLabel("General diameter mod. factor:"), 2, 0);
	sliderWidgetLayout->addWidget(diameterFactorSlider, 2, 1);
	sliderWidgetLayout->addWidget(m_diameterFactorLabel, 2, 2);
	sliderWidgetLayout->addWidget(new QLabel("Context diameter mod. factor:"), 3, 0);
	sliderWidgetLayout->addWidget(contextDiameterFactorSlider, 3, 1);
	sliderWidgetLayout->addWidget(m_contextDiameterFactorLabel, 3, 2);

	QWidget* fiberContextWidget = new QWidget();
	auto fiberContextLayout = new QVBoxLayout();
	fiberContextWidget->setLayout(fiberContextLayout);
	fiberContextLayout->setContentsMargins(0, 0, 0, 0);
	fiberContextLayout->setSpacing(SettingSpacing);
	QCheckBox* cbShowFiberContext = new QCheckBox("Show selected fiber context");
	connect(cbShowFiberContext, &QCheckBox::stateChanged, this, &iAFiAKErController::showFiberContextChanged);
	fiberContextLayout->addWidget(cbShowFiberContext);
	QCheckBox* cbMergeFiberContextBoxes = new QCheckBox("Merge fiber context boxes");
	connect(cbMergeFiberContextBoxes, &QCheckBox::stateChanged, this, &iAFiAKErController::mergeFiberContextBoxesChanged);
	fiberContextLayout->addWidget(cbMergeFiberContextBoxes);
	QWidget* fiberContextSpacingWidget = new QWidget();
	auto fiberContextSpacingLayout = new QHBoxLayout();
	fiberContextSpacingWidget->setLayout(fiberContextSpacingLayout);
	fiberContextSpacingLayout->setContentsMargins(0, 0, 0, 0);
	fiberContextSpacingLayout->setSpacing(SettingSpacing);
	QDoubleSpinBox* sbContextSpacing = new QDoubleSpinBox();
	sbContextSpacing->setRange(0.0, 1000.0);
	sbContextSpacing->setValue(0.0);
	fiberContextSpacingLayout->addWidget(new QLabel("Context Spacing"));
	fiberContextSpacingLayout->addWidget(sbContextSpacing);
	fiberContextLayout->addWidget(fiberContextSpacingWidget);
	connect(sbContextSpacing, SIGNAL(valueChanged(double)), this, SLOT(contextSpacingChanged(double)));


	QWidget* moreButtons = new QWidget();
	moreButtons->setLayout(new QHBoxLayout());
	moreButtons->layout()->setContentsMargins(0, 0, 0, 0);
	moreButtons->layout()->setSpacing(SettingSpacing);
	auto showSampledCylinder = new QPushButton("Sample Fiber");
	auto hideSampledCylinder = new QPushButton("Hide Sample Points");
	auto spatialOverviewButton = new QPushButton("Spatial Overview");
	connect(showSampledCylinder, &QPushButton::pressed, this, &iAFiAKErController::visualizeCylinderSamplePoints);
	connect(hideSampledCylinder, &QPushButton::pressed, this, &iAFiAKErController::hideSamplePoints);
	connect(spatialOverviewButton, &QPushButton::pressed, this, &iAFiAKErController::showSpatialOverviewButton);
	moreButtons->layout()->addWidget(showSampledCylinder);
	moreButtons->layout()->addWidget(hideSampledCylinder);
	moreButtons->layout()->addWidget(spatialOverviewButton);

	auto selectionModeChoice = new QComboBox();
	selectionModeChoice->addItem("Rubberband Rectangle (Endpoints)");
	selectionModeChoice->addItem("Single Fiber Click");
	connect(selectionModeChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(selectionModeChanged(int)));
	auto selectionModeWidget = new QWidget();
	selectionModeWidget->setLayout(new QHBoxLayout());
	selectionModeWidget->layout()->setContentsMargins(0, 0, 0, 0);
	selectionModeWidget->layout()->setSpacing(SettingSpacing);
	selectionModeWidget->layout()->addWidget(new QLabel("Selection Mode:"));
	selectionModeWidget->layout()->addWidget(selectionModeChoice);

	auto metricLabel = new QLabel("Metric:");
	m_cmbboxSimilarityMeasure = new QComboBox();
	auto similarityMeasures = iARefDistCompute::getSimilarityMeasureNames();
	for (auto name: similarityMeasures)
		m_cmbboxSimilarityMeasure->addItem(name);
	m_cmbboxSimilarityMeasure->setCurrentIndex(iARefDistCompute::BestSimilarityMeasure);
	connect(m_cmbboxSimilarityMeasure, SIGNAL(currentIndexChanged(int)), this, SLOT(showReferenceMeasureChanged(int)));
	auto metricChoice = new QWidget();
	metricChoice->setLayout(new QHBoxLayout());
	metricChoice->layout()->setContentsMargins(0, 0, 0, 0);
	metricChoice->layout()->setSpacing(SettingSpacing);
	metricChoice->layout()->addWidget(metricLabel);
	metricChoice->layout()->addWidget(m_cmbboxSimilarityMeasure);

	QGroupBox* main3DViewSettings = new QGroupBox("Main 3D View");
	main3DViewSettings->setLayout(new QVBoxLayout());
	main3DViewSettings->layout()->setContentsMargins(SettingSpacing, SettingSpacing, SettingSpacing, SettingSpacing);
	main3DViewSettings->layout()->setSpacing(SettingSpacing);
	main3DViewSettings->layout()->addWidget(sliderWidget);
	main3DViewSettings->layout()->addWidget(fiberContextWidget);
	main3DViewSettings->layout()->addWidget(selectionModeWidget);
	main3DViewSettings->layout()->addWidget(metricChoice);
	main3DViewSettings->layout()->addWidget(moreButtons);

	QWidget* playControls = new QWidget();
	playControls->setLayout(new QHBoxLayout());
	playControls->layout()->setContentsMargins(0, 0, 0, 0);
	playControls->layout()->setSpacing(SettingSpacing);
	QSpinBox* stepDelayInput = new QSpinBox();
	stepDelayInput->setMinimum(100);
	stepDelayInput->setMaximum(10000);
	stepDelayInput->setSingleStep(100);
	stepDelayInput->setValue(DefaultPlayDelay);
	playControls->layout()->addWidget(new QLabel("Delay (ms)"));
	playControls->layout()->addWidget(stepDelayInput);
	m_playTimer->setInterval(DefaultPlayDelay);
	connect(m_playTimer, &QTimer::timeout, this, &iAFiAKErController::playTimer);
	connect(stepDelayInput, SIGNAL(valueChanged(int)), this, SLOT(playDelayChanged(int)));

	m_optimStepChart.resize(iAFiberCharData::FiberValueCount + iARefDistCompute::SimilarityMeasureCount + 1);

	auto dataChooser = new QScrollArea();
	dataChooser->setWidgetResizable(true);
	auto comboBoxContainer = new QWidget();
	dataChooser->setWidget(comboBoxContainer);
	comboBoxContainer->setLayout(new QVBoxLayout());
	ChartCount = iAFiberCharData::FiberValueCount + iARefDistCompute::SimilarityMeasureCount + 1;
	m_chartCB.resize(ChartCount);
	for (int chartID = 0; chartID < ChartCount; ++chartID)
	{
		m_chartCB[chartID] = new QCheckBox(diffName(chartID));
		m_chartCB[chartID]->setChecked(chartID == ChartCount - 1);
		m_chartCB[chartID]->setEnabled(chartID == ChartCount - 1);
		m_chartCB[chartID]->setProperty("chartID", chartID);
		connect(m_chartCB[chartID], &QCheckBox::stateChanged, this, &iAFiAKErController::optimDataToggled);
		comboBoxContainer->layout()->addWidget(m_chartCB[chartID]);
	}
	size_t curPlotStart = 0;
	for (int resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto & d = m_data->result[resultID];
		if (!d.projectionError.empty())
		{
			m_resultUIs[resultID].startPlotIdx = curPlotStart;
			curPlotStart += d.fiberCount;
		}
		else
			m_resultUIs[resultID].startPlotIdx = NoPlotsIdx;
	}

	QGroupBox* optimStepSettings = new QGroupBox("Optimization Steps Animation");
	optimStepSettings->setLayout(new QVBoxLayout());
	optimStepSettings->layout()->setContentsMargins(SettingSpacing, SettingSpacing, SettingSpacing, SettingSpacing);
	optimStepSettings->layout()->setSpacing(SettingSpacing);
	optimStepSettings->layout()->addWidget(playControls);
	optimStepSettings->layout()->addWidget(dataChooser);

	auto histogramBinInput = new QSpinBox();
	histogramBinInput->setMinimum(5);
	histogramBinInput->setMaximum(1000);
	histogramBinInput->setValue(HistogramBins);
	connect(histogramBinInput, SIGNAL(valueChanged(int)), this, SLOT(histogramBinsChanged(int)));
	auto histoBinInputWidget = new QWidget();
	histoBinInputWidget->setLayout(new QHBoxLayout());
	histoBinInputWidget->layout()->setContentsMargins(0, 0, 0, 0);
	histoBinInputWidget->layout()->setSpacing(SettingSpacing);
	histoBinInputWidget->layout()->addWidget(new QLabel("Histogram Bins:"));
	histoBinInputWidget->layout()->addWidget(histogramBinInput);

	m_showReferenceInChart = new QCheckBox("Show Reference in Distribution Histograms");
	m_showReferenceInChart->setChecked(true);
	connect(m_showReferenceInChart, &QCheckBox::stateChanged, this, &iAFiAKErController::showReferenceInChartToggled);

	m_distributionChartType = new QComboBox();
	m_distributionChartType->addItem("Bar Chart");
	m_distributionChartType->addItem("Line Graph");
	connect(m_distributionChartType, SIGNAL(currentIndexChanged(int)), this, SLOT(distributionChartTypeChanged(int)));

	auto distributionChartTypeWidget = new QWidget();
	distributionChartTypeWidget->setLayout(new QHBoxLayout());
	distributionChartTypeWidget->layout()->setContentsMargins(0, 0, 0, 0);
	distributionChartTypeWidget->layout()->setSpacing(SettingSpacing);
	distributionChartTypeWidget->layout()->addWidget(new QLabel("Distribution Plot Type:"));
	distributionChartTypeWidget->layout()->addWidget(m_distributionChartType);

	auto stackedBarColorThemeChoice = new QComboBox();
	stackedBarColorThemeChoice->addItems(iAColorThemeManager::instance().availableThemes());
	stackedBarColorThemeChoice->setCurrentText(DefaultStackedBarColorTheme);
	connect(stackedBarColorThemeChoice, SIGNAL(currentIndexChanged(QString const &)), this, SLOT(stackedBarColorThemeChanged(QString const &)));

	auto stackedBarColorThemeWidget = new QWidget();
	stackedBarColorThemeWidget->setLayout(new QHBoxLayout());
	stackedBarColorThemeWidget->layout()->setContentsMargins(0, 0, 0, 0);
	stackedBarColorThemeWidget->layout()->setSpacing(SettingSpacing);
	stackedBarColorThemeWidget->layout()->addWidget(new QLabel("Stacked Bar Colors:"));
	stackedBarColorThemeWidget->layout()->addWidget(stackedBarColorThemeChoice);

	QGroupBox* resultListSettings = new QGroupBox("Result List");
	resultListSettings->setLayout(new QVBoxLayout());
	resultListSettings->layout()->setContentsMargins(SettingSpacing, SettingSpacing, SettingSpacing, SettingSpacing);
	resultListSettings->layout()->setSpacing(SettingSpacing);
	resultListSettings->layout()->addWidget(histoBinInputWidget);
	resultListSettings->layout()->addWidget(m_showReferenceInChart);
	resultListSettings->layout()->addWidget(distributionChartTypeWidget);
	resultListSettings->layout()->addWidget(stackedBarColorThemeWidget);

	auto distrColorThemeChoice = new QComboBox();
	distrColorThemeChoice->addItems(iALUT::GetColorMapNames());
	distrColorThemeChoice->setCurrentIndex(0);
	connect(distrColorThemeChoice, SIGNAL(currentIndexChanged(QString const &)), this, SLOT(distributionColorThemeChanged(QString const &)));
	auto distrColorThemeChoiceWidget = new QWidget();
	distrColorThemeChoiceWidget->setLayout(new QHBoxLayout());
	distrColorThemeChoiceWidget->layout()->setContentsMargins(0, 0, 0, 0);
	distrColorThemeChoiceWidget->layout()->setSpacing(SettingSpacing);
	distrColorThemeChoiceWidget->layout()->addWidget(new QLabel("Distribution Colors:"));
	distrColorThemeChoiceWidget->layout()->addWidget(distrColorThemeChoice);

	auto resultColorThemeChoice = new QComboBox();
	resultColorThemeChoice->addItems(iAColorThemeManager::instance().availableThemes());
	resultColorThemeChoice->setCurrentText(DefaultResultColorTheme);
	connect(resultColorThemeChoice, SIGNAL(currentIndexChanged(QString const &)), this, SLOT(resultColorThemeChanged(QString const &)));
	auto resultColorThemeChoiceWidget = new QWidget();
	resultColorThemeChoiceWidget->setLayout(new QHBoxLayout());
	resultColorThemeChoiceWidget->layout()->setContentsMargins(0, 0, 0, 0);
	resultColorThemeChoiceWidget->layout()->setSpacing(SettingSpacing);
	resultColorThemeChoiceWidget->layout()->addWidget(new QLabel("Result Colors:"));
	resultColorThemeChoiceWidget->layout()->addWidget(resultColorThemeChoice);

	auto fileChooser = new iAFileChooserWidget(this, iAFileChooserWidget::FileNameOpen);
	connect(fileChooser, &iAFileChooserWidget::fileNameChanged, this, &iAFiAKErController::loadVolume);
	auto volumeDatasetWidget = new QWidget();
	volumeDatasetWidget->setLayout(new QHBoxLayout());
	volumeDatasetWidget->layout()->setContentsMargins(0, 0, 0, 0);
	volumeDatasetWidget->layout()->setSpacing(SettingSpacing);
	volumeDatasetWidget->layout()->addWidget(new QLabel("Reference Volume:"));
	volumeDatasetWidget->layout()->addWidget(fileChooser);

	QWidget* saveLoadAnalysisWidget = new QWidget();
	saveLoadAnalysisWidget->setLayout(new QHBoxLayout());
	saveLoadAnalysisWidget->layout()->setContentsMargins(0, 0, 0, 0);
	saveLoadAnalysisWidget->layout()->setSpacing(SettingSpacing);
	auto saveAnalysisButton = new QPushButton("Save Analysis");
	connect(saveAnalysisButton, &QPushButton::pressed, this, &iAFiAKErController::saveAnalysisClick);
	auto loadAnalysisButton = new QPushButton("Load Analysis");
	connect(loadAnalysisButton, &QPushButton::pressed, this, &iAFiAKErController::loadAnalysisClick);
	saveLoadAnalysisWidget->layout()->addWidget(saveAnalysisButton);
	saveLoadAnalysisWidget->layout()->addWidget(loadAnalysisButton);

	QGroupBox* globalSettings = new QGroupBox("Global Settings");
	globalSettings->setLayout(new QVBoxLayout());
	globalSettings->layout()->setContentsMargins(SettingSpacing, SettingSpacing, SettingSpacing, SettingSpacing);
	globalSettings->layout()->setSpacing(SettingSpacing);
	globalSettings->layout()->addWidget(resultColorThemeChoiceWidget);
	globalSettings->layout()->addWidget(distrColorThemeChoiceWidget);
	globalSettings->layout()->addWidget(volumeDatasetWidget);
	globalSettings->layout()->addWidget(saveLoadAnalysisWidget);

	QWidget* leftSettingsWidget = new QWidget();
	leftSettingsWidget->setLayout(new QVBoxLayout());
	leftSettingsWidget->layout()->setContentsMargins(DockWidgetMargin, DockWidgetMargin, DockWidgetMargin, DockWidgetMargin);
	leftSettingsWidget->layout()->setSpacing(SettingSpacing);
	leftSettingsWidget->layout()->addWidget(main3DViewSettings);
	leftSettingsWidget->layout()->addWidget(resultListSettings);
	leftSettingsWidget->layout()->addWidget(globalSettings);

	QWidget* settingsView = new QWidget();
	settingsView->setLayout(new QHBoxLayout());
	settingsView->layout()->setContentsMargins(DockWidgetMargin, DockWidgetMargin, DockWidgetMargin, DockWidgetMargin);
	settingsView->layout()->setSpacing(SettingSpacing);
	settingsView->layout()->addWidget(leftSettingsWidget);
	settingsView->layout()->addWidget(optimStepSettings);

	return settingsView;
}

QWidget* iAFiAKErController::setupOptimStepView()
{
	auto chartContainer = new QWidget();
	m_optimChartLayout = new QVBoxLayout();
	chartContainer->setLayout(m_optimChartLayout);
	chartContainer->setSizeIncrement(QSizePolicy::Expanding, QSizePolicy::Expanding);
	m_optimStepSlider = new QSlider(Qt::Horizontal);
	m_optimStepSlider->setMinimum(0);
	m_optimStepSlider->setMaximum(m_data->optimStepMax - 1);
	m_optimStepSlider->setValue(m_data->optimStepMax - 1);
	m_currentOptimStepLabel = new QLabel("");
	m_currentOptimStepLabel->setText(QString::number(m_data->optimStepMax - 1));
	connect(m_optimStepSlider, &QSlider::valueChanged, this, &iAFiAKErController::optimStepSliderChanged);
	QPushButton* playPauseButton = new QPushButton("Play");
	connect(playPauseButton, &QPushButton::pressed, this, &iAFiAKErController::playPauseOptimSteps);

	QWidget* optimStepsCtrls = new QWidget();
	optimStepsCtrls->setLayout(new QHBoxLayout());
	optimStepsCtrls->layout()->addWidget(m_optimStepSlider);
	optimStepsCtrls->layout()->addWidget(m_currentOptimStepLabel);
	optimStepsCtrls->layout()->addWidget(playPauseButton);
	optimStepsCtrls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QWidget* optimStepsView = new QWidget();
	optimStepsView->setLayout(new QVBoxLayout());
	optimStepsView->layout()->setContentsMargins(DockWidgetMargin, DockWidgetMargin, DockWidgetMargin, DockWidgetMargin);
	optimStepsView->layout()->addWidget(chartContainer);
	optimStepsView->layout()->addWidget(optimStepsCtrls);
	return optimStepsView;
}

namespace
{
	QSharedPointer<iA3DColoredPolyObjectVis> create3DVis(vtkRenderer* renderer,
		vtkSmartPointer<vtkTable> table, QSharedPointer<QMap<uint, uint> > mapping, QColor const & color, int objectType,
		std::map<size_t, std::vector<iAVec3f> > & curvedFiberData)
	{
		switch (objectType)
		{
		case iACsvConfig::Ellipses:  return QSharedPointer<iA3DColoredPolyObjectVis>(new iA3DEllipseObjectVis(renderer, table, mapping, color));
		default:
		case iACsvConfig::Cylinders: return QSharedPointer<iA3DColoredPolyObjectVis>(new iA3DCylinderObjectVis(renderer, table, mapping, color, curvedFiberData));
		}
	}
}

QWidget* iAFiAKErController::setupResultListView()
{
	size_t commonPrefixLength = 0, commonSuffixLength = 0;
	QString baseName0;
	for (int resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		QString baseName = QFileInfo(m_data->result[resultID].fileName).baseName();
		if (resultID > 0)
		{
			commonPrefixLength = std::min(commonPrefixLength, static_cast<size_t>(greatestCommonPrefixLength(baseName, baseName0)));
			commonSuffixLength = std::min(commonSuffixLength, static_cast<size_t>(greatestCommonSuffixLength(baseName, baseName0)));
		}
		else
		{
			commonPrefixLength = baseName.length();
			commonSuffixLength = baseName.length();
			baseName0 = baseName;
		}
	}
	if (commonPrefixLength == commonSuffixLength)
	{
		commonSuffixLength = 0;
	}
	auto resultListScrollArea = new QScrollArea();
	resultListScrollArea->setWidgetResizable(true);
	auto resultList = new QWidget();
	resultListScrollArea->setWidget(resultList);
	m_resultsListLayout = new QGridLayout();
	m_resultsListLayout->setSpacing(2);
	m_resultsListLayout->setContentsMargins(DockWidgetMargin, DockWidgetMargin, DockWidgetMargin, DockWidgetMargin);
	m_resultsListLayout->setColumnStretch(PreviewColumn, 1);
	m_resultsListLayout->setColumnStretch(StackedBarColumn, m_data->result.size());
	m_resultsListLayout->setColumnStretch(HistogramColumn, 2 * m_data->result.size());

	auto colorTheme = iAColorThemeManager::instance().theme(DefaultStackedBarColorTheme);
	m_stackedBarsHeaders = new iAStackedBarChart(colorTheme, true);
	m_stackedBarsHeaders->setMinimumWidth(StackedBarMinWidth);
	auto headerFiberCountAction = new QAction("Fiber Count", nullptr);
	headerFiberCountAction->setProperty("colID", 0);
	headerFiberCountAction->setCheckable(true);
	headerFiberCountAction->setChecked(true);
	connect(headerFiberCountAction, &QAction::triggered, this, &iAFiAKErController::stackedColSelect);
	m_stackedBarsHeaders->contextMenu()->addAction(headerFiberCountAction);
	connect(m_stackedBarsHeaders, &iAStackedBarChart::switchedStackMode, this, &iAFiAKErController::switchStackMode);

	auto nameActionsLabel = new QLabel("Name/Actions");
	nameActionsLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	auto previewLabel = new QLabel("Preview");
	previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_distributionChoice = new QComboBox();
	QStringList paramNames;
	for (int curIdx = 0; curIdx < m_data->spmData->numParams() - 1; ++curIdx)
		paramNames.push_back(QString("%1 Distribution").arg(m_data->spmData->parameterName(curIdx)));
	m_distributionChoice->addItems(paramNames);
	m_distributionChoice->addItem("Match Quality");
	m_distributionChoice->setCurrentIndex((*m_data->result[0].mapping)[iACsvConfig::Length]);
	connect(m_distributionChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(distributionChoiceChanged(int)));
	m_distributionChoice->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_colorByDistribution = new QCheckBox("Color by");
	connect(m_colorByDistribution, &QCheckBox::stateChanged, this, &iAFiAKErController::colorByDistrToggled);

	auto histHeader = new QWidget();
	histHeader->setLayout(new QHBoxLayout());
	histHeader->layout()->setContentsMargins(0, 0, 0, 0);
	histHeader->layout()->addWidget(m_colorByDistribution);
	histHeader->layout()->addWidget(m_distributionChoice);

	m_resultsListLayout->addWidget(nameActionsLabel, 0, NameActionColumn);
	m_resultsListLayout->addWidget(previewLabel, 0, PreviewColumn);
	m_resultsListLayout->addWidget(m_stackedBarsHeaders, 0, StackedBarColumn);
	m_resultsListLayout->addWidget(histHeader, 0, HistogramColumn);

	for (int resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto & d = m_data->result.at(resultID);
		auto & uiData = m_resultUIs[resultID];

		uiData.previewWidget = new iAFixedAspectWidget();
		uiData.vtkWidget = uiData.previewWidget->vtkWidget();
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		renWin->SetAlphaBitPlanes(1);
		auto ren = vtkSmartPointer<vtkRenderer>::New();
		m_renderManager->addToBundle(ren);
		ren->SetBackground(1.0, 1.0, 1.0);
		ren->SetUseDepthPeeling(true);
		ren->SetMaximumNumberOfPeels(10);
		renWin->AddRenderer(ren);
		uiData.vtkWidget->SetRenderWindow(renWin);
		uiData.vtkWidget->setProperty("resultID", resultID);

		uiData.cbShow = new QCheckBox("Show");
		uiData.cbShow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		uiData.cbShow->setProperty("resultID", resultID);
		uiData.cbBoundingBox = new QCheckBox("Box");
		uiData.cbBoundingBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		uiData.cbBoundingBox->setProperty("resultID", resultID);

		QString name = QFileInfo(d.fileName).baseName();
		name = name.mid(commonPrefixLength, name.size() - commonPrefixLength - commonSuffixLength);

		uiData.nameActions = new iASignallingWidget();
		uiData.nameActions->setAutoFillBackground(true);
		uiData.nameActions->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		uiData.nameActions->setLayout(new QVBoxLayout());
		uiData.nameActions->layout()->setContentsMargins(0, 0, 0, 0);
		uiData.nameActions->layout()->setSpacing(5);
		uiData.nameLabel = new QLabel(name);
		uiData.nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		uiData.topFiller = new QWidget();
		uiData.topFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		uiData.bottomFiller = new QWidget();
		uiData.bottomFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		uiData.nameActions->layout()->addWidget(uiData.topFiller);
		uiData.nameActions->layout()->addWidget(uiData.nameLabel);
		uiData.nameActions->layout()->addWidget(uiData.cbShow);
		uiData.nameActions->layout()->addWidget(uiData.cbBoundingBox);
		uiData.nameActions->layout()->addWidget(uiData.bottomFiller);

		uiData.stackedBars = new iAStackedBarChart(colorTheme);
		uiData.stackedBars->setMinimumWidth(StackedBarMinWidth);
		uiData.stackedBars->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		connect(m_stackedBarsHeaders, &iAStackedBarChart::weightsChanged, uiData.stackedBars, &iAStackedBarChart::setWeights);

		uiData.histoChart = new iAChartWidget(resultList, "Fiber Length", "");
		uiData.histoChart->setShowXAxisLabel(false);
		uiData.histoChart->setMinimumWidth(HistogramMinWidth);
		uiData.histoChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		m_resultsListLayout->addWidget(uiData.nameActions, resultID + 1, NameActionColumn);
		m_resultsListLayout->addWidget(uiData.previewWidget, resultID + 1, PreviewColumn);
		m_resultsListLayout->addWidget(uiData.stackedBars, resultID + 1, StackedBarColumn);
		m_resultsListLayout->addWidget(uiData.histoChart, resultID + 1, HistogramColumn);

		uiData.mini3DVis = create3DVis(ren, d.table, d.mapping, getResultColor(resultID), m_data->objectType, d.curveInfo);
		uiData.main3DVis = create3DVis(m_ren, d.table, d.mapping, getResultColor(resultID), m_data->objectType, d.curveInfo);
		uiData.mini3DVis->setColor(getResultColor(resultID));
		uiData.mini3DVis->show();
		ren->ResetCamera();

		uiData.previewWidget->setProperty("resultID", resultID);
		uiData.stackedBars->setProperty("resultID", resultID);
		uiData.histoChart->setProperty("resultID", resultID);
		uiData.nameActions->setProperty("resultID", resultID);
		connect(uiData.previewWidget, &iAFixedAspectWidget::dblClicked, this, &iAFiAKErController::referenceToggled);
		connect(uiData.stackedBars, &iAStackedBarChart::dblClicked, this, &iAFiAKErController::referenceToggled);
		connect(uiData.histoChart, &iAChartWidget::dblClicked, this, &iAFiAKErController::referenceToggled);
		connect(uiData.nameActions, &iASignallingWidget::dblClicked, this, &iAFiAKErController::referenceToggled);
		connect(uiData.vtkWidget, &iAVtkWidget::mouseEvent, this, &iAFiAKErController::miniMouseEvent);
		connect(uiData.cbShow, &QCheckBox::stateChanged, this, &iAFiAKErController::toggleVis);
		connect(uiData.cbBoundingBox, &QCheckBox::stateChanged, this, &iAFiAKErController::toggleBoundingBox);
	}
	resultList->setLayout(m_resultsListLayout);
	addStackedBar(0);
	changeDistributionSource((*m_data->result[0].mapping)[iACsvConfig::Length]);
	return resultListScrollArea;
}

QWidget * iAFiAKErController::setupProtocolView()
{
	m_interactionProtocol = new QTreeView();
	m_interactionProtocol->setHeaderHidden(true);
	m_interactionProtocolModel = new QStandardItemModel();
	m_interactionProtocol->setModel(m_interactionProtocolModel);
	QWidget* protocolView = new QWidget();
	protocolView->setLayout(new QHBoxLayout());
	protocolView->layout()->setContentsMargins(DockWidgetMargin, DockWidgetMargin, DockWidgetMargin, DockWidgetMargin);
	protocolView->layout()->addWidget(m_interactionProtocol);
	return protocolView;
}

QWidget * iAFiAKErController::setupSelectionView()
{
	m_selectionListModel = new QStandardItemModel();
	m_selectionList = new QListView();
	m_selectionList->setModel(m_selectionListModel);
	connect(m_selectionList, &QListView::clicked, this, &iAFiAKErController::selectionFromListActivated);
	auto selectionListWrapper = new QWidget();
	selectionListWrapper->setLayout(new QVBoxLayout());
	selectionListWrapper->layout()->addWidget(new QLabel("Selections:"));
	selectionListWrapper->layout()->addWidget(m_selectionList);
	m_selectionDetailModel = new QStandardItemModel();
	m_selectionDetailsTree = new QTreeView();
	m_selectionDetailsTree->setHeaderHidden(true);
	m_selectionDetailsTree->setModel(m_selectionDetailModel);
	connect(m_selectionDetailsTree, &QTreeView::clicked, this, &iAFiAKErController::selectionDetailsItemClicked);
	auto selectionDetailWrapper = new QWidget();
	selectionDetailWrapper->setLayout(new QVBoxLayout());
	selectionDetailWrapper->layout()->addWidget(new QLabel("Details:"));
	selectionDetailWrapper->layout()->addWidget(m_selectionDetailsTree);
	auto selectionView = new QWidget();
	selectionView->setLayout(new QHBoxLayout());
	selectionView->layout()->setContentsMargins(DockWidgetMargin, DockWidgetMargin, DockWidgetMargin, DockWidgetMargin);
	selectionView->layout()->addWidget(selectionListWrapper);
	selectionView->layout()->addWidget(selectionDetailWrapper);
	return selectionView;
}

void iAFiAKErController::loadStateAndShow()
{
	addInteraction(QString("Loaded %1 results in folder %2").arg(m_data->result.size()).arg(m_data->folder));
	QSettings settings;
	if (settings.value(ModuleSettingsKey + "/maximized", true).toBool())
		showMaximized();
	else
	{
		QRect newGeometry = settings.value(ModuleSettingsKey + "/geometry", geometry()).value<QRect>();
		show();
		qobject_cast<QWidget*>(parent())->setGeometry(newGeometry);
	}
	toggleOptimStepChart(ChartCount-1, true);

	if (settings.contains(ModuleSettingsKey + "/state"))
		restoreState(settings.value(ModuleSettingsKey + "/state").toByteArray());

	// SPM needs an active OpenGL Context (it must be visible when setData is called):
	m_spm->setMinimumWidth(200);
	m_spm->showAllPlots(false);
	auto np = m_data->spmData->numParams();
	std::vector<char> v(m_data->spmData->numParams(), false);
	v[np - 7] = v[np - 6] = v[np - 5] = v[np - 4] = v[np - 3] = v[np - 2] = true;
	m_spm->setData(m_data->spmData, v);
	m_spm->setSelectionMode(iAScatterPlot::Rectangle);
	m_spm->showDefaultMaxizimedPlot();
	m_spm->setSelectionColor(SelectionColor);
	m_spm->setPointRadius(2.5);
	m_spm->settings.enableColorSettings = true;
	setSPMColorByResult();
	connect(m_spm, &iAQSplom::selectionModified, this, &iAFiAKErController::selectionSPMChanged);
	connect(m_spm, &iAQSplom::lookupTableChanged, this, &iAFiAKErController::spmLookupTableChanged);
	m_views[SettingsView]->hide();
	m_views[ProtocolView]->hide();
	m_views[SelectionView]->hide();
	m_views[JobView]->hide();
	m_showReferenceWidget->hide();

	emit setupFinished();
}

QString iAFiAKErController::stackedBarColName(int index) const
{
	return index == 0 ? "Fiber Count" : diffName(index-1);
}

void iAFiAKErController::addStackedBar(int index)
{
	QString title = stackedBarColName(index);
	m_stackedBarsHeaders->addBar(title, 1, 1);
	for (size_t resultID=0; resultID<m_resultUIs.size(); ++resultID)
	{
		double value, maxValue;
		if (index == 0)
		{
			value = m_data->result[resultID].fiberCount;
			maxValue = m_data->maxFiberCount;
		}
		else
		{
			value = m_data->result[resultID].avgDifference.size() > 0 ?
						m_data->result[resultID].avgDifference[index-1] : 0;
			maxValue = m_data->maxAvgDifference[index-1];
		}
		m_resultUIs[resultID].stackedBars->addBar(title, value, maxValue);
	}
	m_resultsListLayout->setColumnStretch(StackedBarColumn, m_stackedBarsHeaders->numberOfBars()* m_data->result.size() );
}

void iAFiAKErController::removeStackedBar(int index)
{
	QString title = stackedBarColName(index);
	m_stackedBarsHeaders->removeBar(title);
	for (size_t resultID=0; resultID<m_resultUIs.size(); ++resultID)
		m_resultUIs[resultID].stackedBars->removeBar(title);
	m_resultsListLayout->setColumnStretch(StackedBarColumn, m_stackedBarsHeaders->numberOfBars()*m_data->result.size());
}

void iAFiAKErController::setSPMColorByResult()
{
	iALookupTable lut;
	int numOfResults = m_data->result.size();
	lut.setRange(0, numOfResults - 1);
	lut.allocate(numOfResults);
	for (size_t i = 0; i < numOfResults; i++)
		lut.setColor(i, m_resultColorTheme->color(i));
	m_spm->setLookupTable(lut, m_data->spmData->numParams() - 1);
}

void iAFiAKErController::stackedColSelect()
{
	auto source = qobject_cast<QAction*>(QObject::sender());
	size_t colID = source->property("colID").toULongLong();
	QString title = stackedBarColName(colID);
	if (source->isChecked())
	{
		addInteraction(QString("Added %1 to stacked bar chart").arg(title));
		addStackedBar(colID);
	}
	else
	{
		addInteraction(QString("Removed %1 from stacked bar chart").arg(title));
		removeStackedBar(colID);
	}
}

void iAFiAKErController::switchStackMode(bool stack)
{
	for (size_t resultID=0; resultID<m_resultUIs.size(); ++resultID)
		m_resultUIs[resultID].stackedBars->setDoStack(stack);
}

void iAFiAKErController::distributionChoiceChanged(int index)
{
	addInteraction(QString("Changed histogram distribution source to %1.").arg(m_data->spmData->parameterName(index)));
	changeDistributionSource(index);
}

void iAFiAKErController::histogramBinsChanged(int value)
{
	addInteraction(QString("Changed histogram bins to %1.").arg(value));
	HistogramBins = value;
	changeDistributionSource(m_distributionChoice->currentIndex());
}

void iAFiAKErController::distributionColorThemeChanged(QString const & colorThemeName)
{
	addInteraction(QString("Changed distribution color theme to '%1'.").arg(colorThemeName));
	m_colorByThemeName = colorThemeName;
	changeDistributionSource(m_distributionChoice->currentIndex());
	m_spm->setColorTheme(colorThemeName);
}

bool iAFiAKErController::matchQualityVisActive() const
{
	size_t colorLookupParam = m_distributionChoice->currentIndex();
	return (colorLookupParam >= m_data->spmData->numParams() - 1);
}

void iAFiAKErController::resultColorThemeChanged(QString const & colorThemeName)
{
	addInteraction(QString("Changed result color theme to '%1'.").arg(colorThemeName));
	m_resultColorTheme = iAColorThemeManager::instance().theme(colorThemeName);

	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
		m_resultUIs[resultID].mini3DVis->setColor(getResultColor(resultID));

	// recolor the optimization step plots:
	for (size_t chartID = 0; chartID < m_optimStepChart.size(); ++chartID)
	{
		if (!m_optimStepChart[chartID])
			continue;
		for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
		{
			if (m_resultUIs[resultID].startPlotIdx == NoPlotsIdx)
				continue;
			for (size_t p = 0; p < m_data->result[resultID].fiberCount; ++p)
				m_optimStepChart[chartID]->plots()[m_resultUIs[resultID].startPlotIdx + p]->setColor(getResultColor(resultID));
		}
		m_optimStepChart[chartID]->update();
	}

	updateHistogramColors();
	if (m_spm->colorScheme() == iAQSplom::ByParameter)
		return;

	setSPMColorByResult();
	// main3DVis automatically updated through SPM
}

void iAFiAKErController::stackedBarColorThemeChanged(QString const & colorThemeName)
{
	addInteraction(QString("Changed stacked bar color theme to '%1'.").arg(colorThemeName));
	auto colorTheme = iAColorThemeManager::instance().theme(colorThemeName);
	m_stackedBarsHeaders->setColorTheme(colorTheme);
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
		m_resultUIs[resultID].stackedBars->setColorTheme(colorTheme);
}

void iAFiAKErController::changeDistributionSource(int index)
{
	if (matchQualityVisActive() && m_referenceID == NoResult)
	{
		DEBUG_LOG(QString("You need to set a reference first!"));
		return;
	}
	double range[2];
	if (matchQualityVisActive())
	{
		range[0] = - 1.0;
		range[1] = 1.0;
	}
	else
	{
		range[0] = m_data->spmData->paramRange(index)[0];
		range[1] = m_data->spmData->paramRange(index)[1];
	}
	double yMax = 0;
	for (size_t resultID = 0; resultID<m_data->result.size(); ++resultID)
	{
		auto & d = m_data->result[resultID];
		auto & chart = m_resultUIs[resultID].histoChart;
		chart->clearPlots();
		chart->setXBounds(range[0], range[1]);
		if (matchQualityVisActive() && resultID != m_referenceID)
			continue;
		std::vector<double> fiberData(d.fiberCount);
		for (size_t fiberID = 0; fiberID<d.fiberCount; ++fiberID)
			fiberData[fiberID] = matchQualityVisActive() ? m_data->avgRefFiberMatch[fiberID]
					: d.table->GetValue(fiberID, index).ToDouble();
		auto histogramData = iAHistogramData::create(fiberData, HistogramBins, Continuous, range[0], range[1]);
		QSharedPointer<iAPlot> histogramPlot =
			(m_distributionChartType->currentIndex() == 0) ?
			QSharedPointer<iAPlot>(new iABarGraphPlot(histogramData, m_resultColorTheme->color(resultID)))
			: QSharedPointer<iAPlot>(new iALinePlot(histogramData, m_resultColorTheme->color(resultID)));
		chart->addPlot(histogramPlot);
		if (histogramData->yBounds()[1] > yMax)
			yMax = histogramData->yBounds()[1];
	}
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		m_resultUIs[resultID].histoChart->setYBounds(0, yMax);
	}
	updateRefDistPlots();
	if (m_colorByDistribution->isChecked())
		colorByDistrToggled();
	updateHistogramColors();
}

void iAFiAKErController::updateHistogramColors()
{
	double range[2] = { 0.0, static_cast<double>(HistogramBins) };
	auto lut = m_colorByDistribution->isChecked() ?
		QSharedPointer<iALookupTable>(new iALookupTable(iALUT::Build(range, m_colorByThemeName, 255, 1)))
		: QSharedPointer<iALookupTable>();
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto & chart = m_resultUIs[resultID].histoChart;
		if (chart->plots().size() > 0)
		{
			if (dynamic_cast<iABarGraphPlot*>(chart->plots()[0].data()))
				dynamic_cast<iABarGraphPlot*>(chart->plots()[0].data())->setLookupTable(lut);
			if (!lut)
				chart->plots()[0]->setColor(getResultColor(resultID));
		}
		if (chart->plots().size() > 1)
			chart->plots()[1]->setColor(getResultColor(m_referenceID));
		chart->update();
	}
}

void iAFiAKErController::updateRefDistPlots()
{
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto & chart = m_resultUIs[resultID].histoChart;
		if (chart->plots().size() > 1)
			chart->removePlot(chart->plots()[1]);
		if (m_referenceID != NoResult && resultID != m_referenceID && !matchQualityVisActive() && m_showReferenceInChart->isChecked())
		{
			QColor refColor = m_resultColorTheme->color(m_referenceID);
			refColor.setAlpha(DistributionRefAlpha);
			QSharedPointer<iAPlotData> refPlotData = m_resultUIs[m_referenceID].histoChart->plots()[0]->data();
			QSharedPointer<iAPlot> refPlot =
				(m_distributionChartType->currentIndex() == 0) ?
				QSharedPointer<iAPlot>(new iABarGraphPlot(refPlotData, refColor))
				: QSharedPointer<iAPlot>(new iALinePlot(refPlotData, refColor));
			chart->addPlot(refPlot);
		}
		chart->update();
	}
}

void iAFiAKErController::colorByDistrToggled()
{
	addInteraction(QString("Toggled color by distribution %1.").arg(m_colorByDistribution->isChecked()?"on":"off"));
	if (m_colorByDistribution->isChecked())
	{
		size_t colorLookupParam = m_distributionChoice->currentIndex();
		if (matchQualityVisActive())
		{
			// set all currently shown main visualizations back to their result color
			for (int resultID = 0; resultID < m_resultUIs.size(); ++resultID)
			{
				if (resultID == m_referenceID)
					continue;
				auto mainVis = m_resultUIs[resultID].main3DVis;
				if (mainVis->visible())
					mainVis->setColor(getResultColor(resultID));
			}
			setSPMColorByResult();
			showSpatialOverview();
		}
		else
		{   // this triggers also spmLookupTableChanged (which updates the 3D views)
			m_spm->setColorParam(colorLookupParam);
			m_spm->rangeFromParameter();
		}
	}
	else
	{
		setSPMColorByResult();
	}
	updateHistogramColors();
}

QColor iAFiAKErController::getResultColor(int resultID)
{
	QColor color = m_resultColorTheme->color(resultID);
	color.setAlpha(SelectionOpacity);
	return color;
}

namespace
{
	bool resultSelected(std::vector<iAFiberCharUIData> const & uiCollection, int resultID)
	{
		return (uiCollection[resultID].main3DVis->visible());
	}
	bool noResultSelected(std::vector<iAFiberCharUIData> const & uiCollection)
	{
		for (int i = 0; i < uiCollection.size(); ++i)
			if (resultSelected(uiCollection, i))
				return false;
		return true;
	}
	bool anyOtherResultSelected(std::vector<iAFiberCharUIData> const & uiCollection, int resultID)
	{
		for (int i = 0; i < uiCollection.size(); ++i)
			if (resultSelected(uiCollection, i) && resultID != i)
				return true;
		return false;
	}
}

void iAFiAKErController::toggleOptimStepChart(int chartID, bool visible)
{
	if (!visible)
	{
		if (!m_optimStepChart[chartID])
		{
			DEBUG_LOG(QString("Optim Step chart %1 toggled invisible, but not created yet.").arg(chartID));
			return;
		}
		m_optimStepChart[chartID]->setVisible(false);
		return;
	}
	if (!m_optimStepChart[chartID])
	{
		if (chartID < ChartCount-1 && m_referenceID == NoResult)
		{
			DEBUG_LOG(QString("You need to set a reference first!"));
			return;
		}
		m_optimStepChart[chartID] = new iAChartWidget(nullptr, "Optimization Step", diffName(chartID));
		m_optimStepChart[chartID]->setDrawXAxisAtZero(true);
		size_t plotsBefore = 0, curIdx = 0;
		while (curIdx < chartID)
		{  // TODO: check invisible plots?
			if (m_optimStepChart[curIdx])
				++plotsBefore;
			++curIdx;
		}
		m_optimChartLayout->insertWidget(plotsBefore, m_optimStepChart[chartID]);
		m_optimStepChart[chartID]->setMinimumHeight(100);
		m_optimStepChart[chartID]->setSelectionMode(iAChartWidget::SelectPlot);
		m_optimStepChart[chartID]->addXMarker(m_data->optimStepMax -1, OptimStepMarkerColor);
		for (int resultID=0; resultID<m_data->result.size(); ++resultID)
		{
			auto & d = m_data->result[resultID];
			if (m_resultUIs[resultID].startPlotIdx == NoPlotsIdx)
				continue;
			for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
			{
				QSharedPointer<iAVectorPlotData> plotData;
				if (chartID < ChartCount-1)
					plotData = QSharedPointer<iAVectorPlotData>(new iAVectorPlotData(d.refDiffFiber[fiberID].diff[chartID].timestep));
				else
					plotData = QSharedPointer<iAVectorPlotData>(new iAVectorPlotData(d.projectionError[fiberID]));
				plotData->setXDataType(Discrete);
				m_optimStepChart[chartID]->addPlot(QSharedPointer<iALinePlot>(new iALinePlot(plotData, getResultColor(resultID))));
			}
		}
		connect(m_optimStepChart[chartID], &iAChartWidget::plotsSelected,
				this, &iAFiAKErController::selectionOptimStepChartChanged);
	}
	m_optimStepChart[chartID]->setVisible(true);
	m_optimStepChart[chartID]->clearMarkers();
	m_optimStepChart[chartID]->addXMarker(m_optimStepSlider->value(), OptimStepMarkerColor);

	bool allVisible = noResultSelected(m_resultUIs);
	for (size_t resultID=0; resultID<m_data->result.size(); ++resultID)
	{
		if (m_resultUIs[resultID].startPlotIdx == NoPlotsIdx)
			continue;
		for (size_t p = 0; p < m_data->result[resultID].fiberCount; ++p)
			m_optimStepChart[chartID]->plots()[m_resultUIs[resultID].startPlotIdx+p]
					->setVisible(allVisible || resultSelected(m_resultUIs, resultID));
	}
	m_optimStepChart[chartID]->update();
	showSelectionInPlot(chartID);
}

void iAFiAKErController::addInteraction(QString const & interaction)
{
	m_interactionProtocolModel->invisibleRootItem()->appendRow(new QStandardItem(interaction));
}

void iAFiAKErController::toggleVis(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	addInteraction(QString("Toggle visibility of %1").arg(resultName(resultID)));
	showMainVis(resultID, state);
}

void iAFiAKErController::showMainVis(size_t resultID, int state)
{
	auto & data = m_data->result[resultID];
	auto & ui = m_resultUIs[resultID];
	if (state == Qt::Checked)
	{
		ui.main3DVis->setSelectionOpacity(SelectionOpacity);
		ui.main3DVis->setContextOpacity(ContextOpacity);
		auto vis = dynamic_cast<iA3DCylinderObjectVis*>(ui.main3DVis.data());
		if (vis)
		{
			vis->setDiameterFactor(DiameterFactor);
			vis->setContextDiameterFactor(ContextDiameterFactor);
		}
		if (matchQualityVisActive())
			showSpatialOverview();
		else if (m_spm->colorScheme() == iAQSplom::ByParameter)
		{
			if (vis)
			{
				vis->setLookupTable(m_spm->lookupTable(), m_spm->colorLookupParam());
				vis->updateColorSelectionRendering();
			}
		}
		else
		{
			ui.main3DVis->setColor(getResultColor(resultID));
		}
		if (ui.startPlotIdx != NoPlotsIdx)
		{
			if (!anyOtherResultSelected(m_resultUIs, resultID))
				for (size_t c= 0; c < ChartCount; ++c)
					if (m_optimStepChart[c] && m_optimStepChart[c]->isVisible())
						for (size_t p = 0; p < m_optimStepChart[c]->plots().size(); ++p)
							m_optimStepChart[c]->plots()[p]->setVisible(false);
			for (size_t c= 0; c < ChartCount; ++c)
				if (m_optimStepChart[c] && m_optimStepChart[c]->isVisible())
					for (size_t p = 0; p < data.fiberCount; ++p)
						m_optimStepChart[c]->plots()[ui.startPlotIdx + p]->setVisible(true);
		}

		bool anythingSelected = isAnythingSelected();
		if (anythingSelected)
			ui.main3DVis->setSelection(m_selection[resultID], anythingSelected);
		if (data.timeValues.size() > 0)
		{
			if (m_data->objectType == iACsvConfig::Cylinders)
			{
				auto vis = dynamic_cast<iA3DCylinderObjectVis*>(ui.main3DVis.data());
				vis->updateValues(data.timeValues[
					std::min(data.timeValues.size() - 1, static_cast<size_t>(m_optimStepSlider->value()))]);
			}
		}
		ui.main3DVis->show();
		m_style->addInput( resultID, ui.main3DVis->getPolyData(), ui.main3DVis->getActor() );
		m_spm->addFilter(m_data->spmData->numParams()-1, resultID);
	}
	else
	{
		if (ui.startPlotIdx != NoPlotsIdx)
		{
			if (anyOtherResultSelected(m_resultUIs, resultID))
			{
				for (size_t c=0; c<ChartCount; ++c)
					if (m_optimStepChart[c] && m_optimStepChart[c]->isVisible())
						for (size_t p = 0; p < data.fiberCount; ++p)
							m_optimStepChart[c]->plots()[ui.startPlotIdx + p]->setVisible(false);
			}
			else // nothing selected, show everything
				for (size_t c=0; c<ChartCount; ++c)
					if (m_optimStepChart[c] && m_optimStepChart[c]->isVisible())
						for (size_t p = 0; p < m_optimStepChart[c]->plots().size(); ++p)
							m_optimStepChart[c]->plots()[p]->setVisible(true);
		}
		ui.main3DVis->hide();
		m_style->removeInput(resultID);
		m_spm->removeFilter(m_data->spmData->numParams()-1, resultID);
	}
	for (size_t c=0; c<ChartCount; ++c)
		if (m_optimStepChart[c] && m_optimStepChart[c]->isVisible())
			m_optimStepChart[c]->update();
	changeReferenceDisplay();
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}

void iAFiAKErController::toggleBoundingBox(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	addInteraction(QString("Toggle bounding box of result %1.")
		.arg(resultName(resultID)));
	auto & ui = m_resultUIs[resultID];
	if (state == Qt::Checked)
	{
		const double * b = ui.main3DVis->bounds();
		DEBUG_LOG(QString("Bounding box: (x: %1..%2, y: %3..%4, z: %5..%6)").arg(b[0]).arg(b[1]).arg(b[2]).arg(b[3]).arg(b[4]).arg(b[5]));
		ui.main3DVis->showBoundingBox();
	}
	else
		ui.main3DVis->hideBoundingBox();
}

void iAFiAKErController::getResultFiberIDFromSpmID(size_t spmID, size_t & resultID, size_t & fiberID)
{
	size_t curStart = 0;
	resultID = 0;
	fiberID = 0;
	while (spmID >= curStart + m_data->result[resultID].fiberCount && resultID < m_data->result.size())
	{
		curStart += m_data->result[resultID].fiberCount;
		++resultID;
	}
	if (resultID == m_data->result.size())
	{
		DEBUG_LOG(QString("Invalid index in SPM: %1").arg(spmID));
		return;
	}
	fiberID = spmID - curStart;
}

std::vector<std::vector<size_t> > & iAFiAKErController::selection()
{
	return m_selection;
}

void iAFiAKErController::clearSelection()
{
	for (size_t resultID=0; resultID<m_selection.size(); ++resultID)
		m_selection[resultID].clear();
}

void iAFiAKErController::sortSelection(QString const & source)
{
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
		std::sort(m_selection[resultID].begin(), m_selection[resultID].end());
	newSelection(source);
}

void iAFiAKErController::newSelection(QString const & source)
{
	size_t selSize = selectionSize();
	if (selSize == 0 || (m_selections.size() > 0 && m_selection == m_selections[m_selections.size()-1]))
		return;
	size_t resultCount = 0;
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
		resultCount += (m_selection[resultID].size() > 0) ? 1 : 0;
	m_selections.push_back(m_selection);
	m_selectionListModel->appendRow(new QStandardItem(QString("%1 fibers in %2 results (%3)")
		.arg(selSize).arg(resultCount).arg(source)));
	showSelectionDetail();
}

size_t iAFiAKErController::selectionSize() const
{
	size_t selectionSize = 0;
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
		selectionSize += m_selection[resultID].size();
	return selectionSize;
}

void iAFiAKErController::showSelectionInPlots()
{
	for (size_t chartID=0; chartID<ChartCount; ++chartID)
		showSelectionInPlot(chartID);
}

void iAFiAKErController::showSelectionInPlot(int chartID)
{
	auto chart = m_optimStepChart[chartID];
	if (!chart || !chart->isVisible())
		return;
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		if (m_resultUIs[resultID].startPlotIdx != NoPlotsIdx)
		{
			size_t curSelIdx = 0;
			QColor color(getResultColor(resultID));
			if (isAnythingSelected())
				color.setAlpha(ContextOpacity);
			for (size_t fiberID=0; fiberID < m_data->result[resultID].fiberCount; ++fiberID)
			{
				auto plot = dynamic_cast<iALinePlot*>(chart->plots()[m_resultUIs[resultID].startPlotIdx + fiberID].data());
				if (curSelIdx < m_selection[resultID].size() && fiberID == m_selection[resultID][curSelIdx])
				{
					plot->setLineWidth(2);
					plot->setColor(SelectionColor);
					++curSelIdx;
				}
				else
				{
					plot->setLineWidth(1);
					plot->setColor(color);
				}
			}
		}
	}
	chart->update();
}

bool iAFiAKErController::isAnythingSelected() const
{
	for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
		if (m_selection[resultID].size() > 0)
			return true;
	return false;
}

void iAFiAKErController::showSelectionIn3DViews()
{
	bool anythingSelected = isAnythingSelected();
	m_showReferenceWidget->setVisible(anythingSelected);
	for (size_t resultID = 0; resultID<m_resultUIs.size(); ++resultID)
	{
		auto & ui = m_resultUIs[resultID];
		ui.mini3DVis->setSelection(m_selection[resultID], anythingSelected);
		if (ui.main3DVis->visible())
			ui.main3DVis->setSelection(m_selection[resultID], anythingSelected);
	}
	// TODO: prevent multiple render window / widget updates?
}

void iAFiAKErController::showSelectionInSPM()
{
	std::vector<size_t> spmSelection;
	spmSelection.reserve(selectionSize());
	size_t spmIDStart = 0;
	for (size_t resultID = 0; resultID<m_data->result.size(); ++resultID)
	{
		for (int fiberID = 0; fiberID < m_selection[resultID].size(); ++fiberID)
		{
			size_t spmID = spmIDStart + m_selection[resultID][fiberID];
			spmSelection.push_back(spmID);
		}
		spmIDStart += m_data->result[resultID].fiberCount;
	}
	m_spm->setSelection(spmSelection);
}

void iAFiAKErController::selection3DChanged()
{
	addInteraction(QString("Selected %1 fibers in 3D view.").arg(selectionSize()));
	sortSelection("3D view");
	showSelectionIn3DViews();
	showSelectionInPlots();
	showSelectionInSPM();
	changeReferenceDisplay();
	updateFiberContext();
	if (isAnythingSelected() && !m_views[SelectionView]->isVisible())
		m_views[SelectionView]->show();
}

void iAFiAKErController::selectionSPMChanged(std::vector<size_t> const & selection)
{
	addInteraction(QString("Selected %1 fibers in scatter plot matrix.").arg(selection.size()));
	// map from SPM index to (resultID, fiberID) pairs
	clearSelection();
	size_t resultID, fiberID;
	for (size_t spmID: selection)
	{
		getResultFiberIDFromSpmID(spmID, resultID, fiberID);
		m_selection[resultID].push_back(fiberID);
	}
	sortSelection("SPM");
	showSelectionIn3DViews();
	showSelectionInPlots();
	changeReferenceDisplay();
	updateFiberContext();
	if (isAnythingSelected() && !m_views[SelectionView]->isVisible())
		m_views[SelectionView]->show();
}

void iAFiAKErController::selectionOptimStepChartChanged(std::vector<size_t> const & selection)
{
	addInteraction(QString("Selected %1 fibers in optimization step chart.").arg(selection.size()));
	size_t curSelectionIndex = 0;
	clearSelection();
	// map from plot IDs to (resultID, fiberID) pairs
	for (size_t resultID=0; resultID<m_resultUIs.size() && curSelectionIndex < selection.size(); ++resultID)
	{
		if (m_resultUIs[resultID].startPlotIdx != NoPlotsIdx)
		{
			while (curSelectionIndex < selection.size() &&
				   selection[curSelectionIndex] <
				   (m_resultUIs[resultID].startPlotIdx + m_data->result[resultID].fiberCount) )
			{
				size_t inResultFiberIdx = selection[curSelectionIndex] - m_resultUIs[resultID].startPlotIdx;
				m_selection[resultID].push_back(inResultFiberIdx);
				++curSelectionIndex;
			}
		}
	}
	sortSelection("Chart");
	showSelectionInPlots();
	showSelectionIn3DViews();
	showSelectionInSPM();
	changeReferenceDisplay();
	updateFiberContext();
	if (isAnythingSelected() && !m_views[SelectionView]->isVisible())
		m_views[SelectionView]->show();
}

void iAFiAKErController::miniMouseEvent(QMouseEvent* ev)
{
	if (ev->buttons() == Qt::RightButton && ev->type() == QEvent::MouseButtonPress)
	{
		int resultID = QObject::sender()->property("resultID").toInt();
		addInteraction(QString("Started FiberScout for %1.").arg(resultName(resultID)));
		iAFeatureScoutModuleInterface * featureScout = m_mainWnd->moduleDispatcher().module<iAFeatureScoutModuleInterface>();
		MdiChild* newChild = m_mainWnd->createMdiChild(false);
		iACsvConfig config = getCsvConfig(m_data->result[resultID].fileName, m_configName);
		featureScout->LoadFeatureScout(config, newChild);
		newChild->loadLayout("FeatureScout");
	}
}

void iAFiAKErController::optimStepSliderChanged(int optimStep)
{
	addInteraction(QString("Set optimization step slider to step %1.").arg(optimStep));
	setOptimStep(optimStep);
}

void iAFiAKErController::setOptimStep(int optimStep)
{
	m_currentOptimStepLabel->setText(QString::number(optimStep));
	for (size_t chartID= 0; chartID < ChartCount; ++chartID)
	{
		auto chart = m_optimStepChart[chartID];
		if (!chart || !chart->isVisible())
			continue;
		chart->clearMarkers();
		chart->addXMarker(optimStep, OptimStepMarkerColor);
		chart->update();
		for (int resultID = 0; resultID < m_data->result.size(); ++resultID)
		{
			auto main3DVis = m_resultUIs[resultID].main3DVis;
			auto & timeValues = m_data->result[resultID].timeValues;
			if (main3DVis->visible() && timeValues.size() > 0)
				if (m_data->objectType == iACsvConfig::Cylinders)
				{
					auto vis = dynamic_cast<iA3DCylinderObjectVis*>(main3DVis.data());
					vis->updateValues(timeValues[std::min(static_cast<size_t>(optimStep), timeValues.size() - 1)]);
				}
		}
	}
	changeReferenceDisplay();
}

void iAFiAKErController::mainOpacityChanged(int opacity)
{
	addInteraction(QString("Set main opacity to %1.").arg(opacity));
	m_defaultOpacityLabel->setText(QString::number(opacity, 'f', 2));
	SelectionOpacity = opacity;
	for (int resultID = 0; resultID < m_resultUIs.size(); ++resultID)
	{
		auto & vis = m_resultUIs[resultID];
		vis.mini3DVis->setSelectionOpacity(SelectionOpacity);
		vis.mini3DVis->updateColorSelectionRendering();
		if (vis.main3DVis->visible())
		{
			vis.main3DVis->setSelectionOpacity(SelectionOpacity);
			vis.main3DVis->updateColorSelectionRendering();
		}
	}
}

void iAFiAKErController::contextOpacityChanged(int opacity)
{
	addInteraction(QString("Set context opacity to %1.").arg(opacity));
	m_contextOpacityLabel->setText(QString::number(opacity, 'f', 2));
	ContextOpacity = opacity;
	for (int resultID = 0; resultID < m_resultUIs.size(); ++resultID)
	{
		auto & vis = m_resultUIs[resultID];
		vis.mini3DVis->setContextOpacity(ContextOpacity);
		vis.mini3DVis->updateColorSelectionRendering();
		if (vis.main3DVis->visible())
		{
			vis.main3DVis->setContextOpacity(ContextOpacity);
			vis.main3DVis->updateColorSelectionRendering();
		}
	}
	showSelectionInPlots();
}

void iAFiAKErController::diameterFactorChanged(int diameterFactorInt)
{
	if (m_data->objectType != iACsvConfig::Cylinders)
		return;
	DiameterFactor = m_diameterFactorMapper->dstToSrc(diameterFactorInt);
	addInteraction(QString("Set diameter modification factor to %1.").arg(DiameterFactor));
	m_diameterFactorLabel->setText(QString::number(DiameterFactor, 'f', 2));
	for (int resultID = 0; resultID < m_resultUIs.size(); ++resultID)
	{
		auto & vis = m_resultUIs[resultID];
		(dynamic_cast<iA3DCylinderObjectVis*>(vis.mini3DVis.data()))->setDiameterFactor(DiameterFactor);
		if (vis.main3DVis->visible())
			(dynamic_cast<iA3DCylinderObjectVis*>(vis.main3DVis.data()))->setDiameterFactor(DiameterFactor);
	}
}

void iAFiAKErController::contextDiameterFactorChanged(int contextDiameterFactorInt)
{
	if (m_data->objectType != iACsvConfig::Cylinders)
		return;
	ContextDiameterFactor = m_diameterFactorMapper->dstToSrc(contextDiameterFactorInt);
	addInteraction(QString("Set context diameter modification factor to %1.").arg(ContextDiameterFactor));
	m_contextDiameterFactorLabel->setText(QString::number(ContextDiameterFactor, 'f', 2));
	for (int resultID = 0; resultID < m_resultUIs.size(); ++resultID)
	{
		auto & vis = m_resultUIs[resultID];
		(dynamic_cast<iA3DCylinderObjectVis*>(vis.mini3DVis.data()))->setContextDiameterFactor(ContextDiameterFactor);
		if (vis.main3DVis->visible())
			(dynamic_cast<iA3DCylinderObjectVis*>(vis.main3DVis.data()))->setContextDiameterFactor(ContextDiameterFactor);
	}
}

void iAFiAKErController::showFiberContextChanged(int newState)
{
	m_showFiberContext = (newState == Qt::Checked);
	updateFiberContext();
}

void iAFiAKErController::mergeFiberContextBoxesChanged(int newState)
{
	m_mergeContextBoxes = (newState == Qt::Checked);
	updateFiberContext();
}

void iAFiAKErController::contextSpacingChanged(double value)
{
	m_contextSpacing = value;
	updateFiberContext();
}

namespace
{
	vtkSmartPointer<vtkActor> getCubeActor(iAVec3T<double> const & start, iAVec3T<double> const & end)
	{
		auto cube = vtkSmartPointer<vtkCubeSource>::New();
		cube->SetXLength(abs(start[0] - end[0]));
		cube->SetYLength(abs(start[1] - end[1]));
		cube->SetZLength(abs(start[2] - end[2]));
		auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
		mapper->SetInputConnection(cube->GetOutputPort());
		auto actor = vtkSmartPointer<vtkActor>::New();
		auto pos = (start + end) / 2;
		DEBUG_LOG(QString("Bounding box (x: %1..%2, y: %3..%4, z: %5..%6), center: (%7, %8, %9)")
			.arg(start[0]).arg(end[0])
			.arg(start[1]).arg(end[1])
			.arg(start[2]).arg(end[2])
			.arg(pos[0]).arg(pos[1]).arg(pos[2]));
		actor->SetPosition( pos.data() );
		actor->GetProperty()->SetRepresentationToWireframe();
		actor->SetMapper(mapper);
		return actor;
	}
}

void iAFiAKErController::updateFiberContext()
{
	for (auto actor : m_contextActors)
		m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(actor);
	m_contextActors.clear();
	if (m_showFiberContext)
	{
		iAVec3T<double> minCoord(std::numeric_limits<double>::max()), maxCoord(std::numeric_limits<double>::lowest());
		for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
		{
			auto & d = m_data->result[resultID];
			for (int selectionID = 0; selectionID < m_selection[resultID].size(); ++selectionID)
			{
				size_t fiberID = m_selection[resultID][selectionID];
				double diameter = d.table->GetValue(fiberID, d.mapping->value(iACsvConfig::Diameter)).ToFloat();
				double radius = diameter / 2;
				if (!m_mergeContextBoxes)
				{
					minCoord = std::numeric_limits<double>::max();
					maxCoord = std::numeric_limits<double>::lowest();
				}
				for (int i = 0; i < 3; ++i)
				{
					double startI = d.table->GetValue(fiberID, d.mapping->value(iACsvConfig::StartX + i)).ToFloat();
					double endI = d.table->GetValue(fiberID, d.mapping->value(iACsvConfig::EndX + i)).ToFloat();

					if ((startI - radius - m_contextSpacing) < minCoord[i])
						minCoord[i] = startI - radius - m_contextSpacing;
					if ((endI - radius - m_contextSpacing) < minCoord[i])
						minCoord[i] = endI - radius - m_contextSpacing;

					if ((startI + radius + m_contextSpacing) > maxCoord[i])
						maxCoord[i] = startI + radius + m_contextSpacing;
					if ((endI + radius + m_contextSpacing) > maxCoord[i])
						maxCoord[i] = endI + radius + m_contextSpacing;
				}
				if (!m_mergeContextBoxes)
				{
					auto actor = getCubeActor(minCoord, maxCoord);
					m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(actor);
					m_contextActors.push_back(actor);
				}
			}
		}
		if (m_mergeContextBoxes)
		{
			auto actor = getCubeActor(minCoord, maxCoord);
			m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(actor);
			m_contextActors.push_back(actor);
		}
	}
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}

namespace
{
	void setResultBackground(iAFiberCharUIData & ui, QColor const & color)
	{
		ui.nameActions->setBackgroundColor(color);
		ui.topFiller->setStyleSheet("background-color: " + color.name());
		ui.bottomFiller->setStyleSheet("background-color: " + color.name());
		ui.previewWidget->setBackgroundColor(color);
		ui.vtkWidget->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->SetBackground(
			color.redF(), color.greenF(), color.blueF());
		ui.vtkWidget->update();
		ui.stackedBars->setBackgroundColor(color);
		ui.histoChart->setBackgroundColor(color);
	}
}

void iAFiAKErController::referenceToggled()
{
	if (m_refDistCompute)
	{
		DEBUG_LOG("Another reference computation is currently running, please let that finish first!");
		return;
	}
	size_t referenceID = QObject::sender()->property("resultID").toULongLong();
	setReference(referenceID);
}

void iAFiAKErController::setReference(size_t referenceID)
{
	if (referenceID == m_referenceID)
	{
		DEBUG_LOG(QString("The selected result (%1) is already set as reference!").arg(referenceID));
		return;
	}
	if (m_referenceID != NoResult)
	{
		auto & ui = m_resultUIs[m_referenceID];
		setResultBackground(ui, m_mainRenderer->palette().color(QWidget::backgroundRole()));
		ui.nameLabel->setText(ui.nameLabel->text().left(ui.nameLabel->text().length()-RefMarker.length()));
	}
	addInteraction(QString("Reference set to %1").arg(resultName(referenceID)));
	m_refDistCompute = new iARefDistCompute(m_data, referenceID);
	connect(m_refDistCompute, &QThread::finished, this, &iAFiAKErController::refDistAvailable);
	m_views[JobView]->show();
	m_jobs->addJob("Computing Reference Similarities", m_refDistCompute->progress(), m_refDistCompute);
	m_refDistCompute->start();
}

void iAFiAKErController::refDistAvailable()
{
	size_t startIdx = m_data->spmData->numParams() - (iAFiberCharData::FiberValueCount + iARefDistCompute::SimilarityMeasureCount + iARefDistCompute::EndColumns);
	std::vector<size_t> changedSpmColumns;
	for (size_t paramID = 0; paramID < iAFiberCharData::FiberValueCount + iARefDistCompute::SimilarityMeasureCount; ++paramID)
	{
		size_t columnID = startIdx + paramID;
		changedSpmColumns.push_back(columnID);
	}
	m_referenceID = m_refDistCompute->referenceID();
	m_spnboxReferenceCount->setMaximum(std::min(iARefDistCompute::MaxNumberOfCloseFibers, m_data->result[m_referenceID].fiberCount));
	m_data->spmData->updateRanges(changedSpmColumns);
	m_spm->update();
	delete m_refDistCompute;
	m_refDistCompute = nullptr;

	auto & ui = m_resultUIs[m_referenceID];
	setResultBackground(ui, ReferenceColor);
	ui.nameLabel->setText(ui.nameLabel->text() + RefMarker);

	for (size_t chartID=0; chartID<ChartCount-1; ++chartID)
		m_chartCB[chartID]->setEnabled(true);

	updateRefDistPlots();

	for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount + iARefDistCompute::SimilarityMeasureCount; ++diffID)
	{
		auto diffAvgAction = new QAction(m_data->spmData->parameterName(startIdx+diffID), nullptr);
		diffAvgAction->setProperty("colID", static_cast<unsigned long long>(diffID+1));
		diffAvgAction->setCheckable(true);
		diffAvgAction->setChecked(false);
		connect(diffAvgAction, &QAction::triggered, this, &iAFiAKErController::stackedColSelect);
		m_stackedBarsHeaders->contextMenu()->addAction(diffAvgAction);
	}

	QSignalBlocker cblock(m_distributionChoice);
	m_distributionChoice->setCurrentIndex(m_data->spmData->numParams() - 1);
	QSignalBlocker cbColorByBlock(m_colorByDistribution);
	m_colorByDistribution->setChecked(true);
	showMainVis(m_referenceID, Qt::Checked);
	changeDistributionSource(m_data->spmData->numParams() - 1);

	m_views[JobView]->hide();
}

void iAFiAKErController::showSpatialOverviewButton()
{
	addInteraction("Showing Spatial Overview");
	showSpatialOverview();
}

void iAFiAKErController::selectionModeChanged(int mode)
{
	m_style->setSelectionMode(static_cast<iASelectionInteractorStyle::SelectionMode>(mode));
}

void iAFiAKErController::showSpatialOverview()
{
	if (m_referenceID == NoResult)
		return;
	double range[2] = {-1.0, 1.0};
	QSharedPointer<iALookupTable> lut(new iALookupTable());
	*lut = iALUT::Build(range, m_colorByThemeName, 255, SelectionOpacity);
	auto ref3D = m_resultUIs[m_referenceID].main3DVis;
	QSignalBlocker cbBlock(m_resultUIs[m_referenceID].cbShow);
	m_resultUIs[m_referenceID].cbShow->setChecked(true);
	size_t colID = m_data->result[m_referenceID].table->GetNumberOfColumns()-1;
	ref3D->setLookupTable(lut, colID);
	ref3D->updateColorSelectionRendering();
	ref3D->show();
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}

void iAFiAKErController::spmLookupTableChanged()
{
	QSharedPointer<iALookupTable> lut = m_spm->lookupTable();
	size_t colorLookupParam = m_spm->colorLookupParam();
	// TODO:
	//     - select distribution in combobox?
	/*
	if (colorLookupParam != m_distributionChoice->currentIndex())
	{
		QSignalBlocker
	}
	*/
	//     - update color theme name if changed in SPM settings
	for (size_t resultID = 0; resultID < m_resultUIs.size(); ++resultID)
	{
		if (m_resultUIs[resultID].main3DVis->visible() && (!matchQualityVisActive() || resultID != m_referenceID) )
			m_resultUIs[resultID].main3DVis->setLookupTable(lut, colorLookupParam);
	}
}

void iAFiAKErController::showReferenceToggled()
{
	bool showRef = m_chkboxShowReference->isChecked();
	addInteraction(QString("Show reference fibers toggled to %1").arg(showRef?"on":"off"));
	changeReferenceDisplay();
}

void iAFiAKErController::showReferenceCountChanged(int count)
{
	addInteraction(QString("Reference fibers count changed to %1").arg(count));
	changeReferenceDisplay();
}

void iAFiAKErController::showReferenceMeasureChanged(int index)
{
	addInteraction(QString("Selected reference match measure #%1").arg(index));
	changeReferenceDisplay();
}

void iAFiAKErController::showReferenceLinesToggled()
{
	bool showLines = m_chkboxShowLines->isChecked();
	addInteraction(QString("Show lines to reference fibers toggled to %1").arg(showLines ? "on" : "off"));
	changeReferenceDisplay();
}

void iAFiAKErController::changeReferenceDisplay()
{
	size_t similarityMeasure = clamp(0, iARefDistCompute::SimilarityMeasureCount, m_cmbboxSimilarityMeasure->currentIndex());
	bool showRef = m_chkboxShowReference->isChecked();
	int refCount = std::min(iARefDistCompute::MaxNumberOfCloseFibers, static_cast<size_t>(m_spnboxReferenceCount->value()));

	if (m_nearestReferenceVis)
	{
		m_nearestReferenceVis->hide();
		m_nearestReferenceVis.clear();
	}

	if (m_refLineActor)
		m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(m_refLineActor);
	if (!isAnythingSelected() || !showRef)
	{
		m_mainRenderer->GetRenderWindow()->Render();
		m_mainRenderer->update();
		return;
	}
	if (m_referenceID == NoResult)
	{
		DEBUG_LOG(QString("You need to set a reference first!"));
		return;
	}
	m_refVisTable = vtkSmartPointer<vtkTable>::New();
	m_refVisTable->Initialize();
	// ID column (int):
	vtkSmartPointer<vtkIntArray> arrID = vtkSmartPointer<vtkIntArray>::New();
	arrID->SetName(m_data->result[m_referenceID].table->GetColumnName(0));
	m_refVisTable->AddColumn(arrID);
	// other columns (float):
	for (int col = 1; col < m_data->result[m_referenceID].table->GetNumberOfColumns() - 1; ++col)
	{
		addColumn(m_refVisTable, 0, m_data->result[m_referenceID].table->GetColumnName(col), 0);
	}

	std::vector<iAFiberSimilarity> referenceIDsToShow;

	double range[2];
	range[0] = std::numeric_limits<double>::max();
	range[1] = std::numeric_limits<double>::lowest();
	//DEBUG_LOG("Showing reference fibers:");
	for (size_t resultID=0; resultID < m_selection.size(); ++resultID)
	{
		if (resultID == m_referenceID || !resultSelected(m_resultUIs, resultID))
			continue;
		for (size_t fiberIdx = 0; fiberIdx < m_selection[resultID].size(); ++fiberIdx)
		{
			size_t fiberID = m_selection[resultID][fiberIdx];
			for (int n=0; n<refCount; ++n)
			{
				referenceIDsToShow.push_back(m_data->result[resultID].refDiffFiber[fiberID].dist[similarityMeasure][n]);
			}
		}
	}
	if (referenceIDsToShow.empty())
		return;

	m_refVisTable->SetNumberOfRows(referenceIDsToShow.size());

	auto refTable = m_data->result[m_referenceID].table;
	for (size_t fiberIdx=0; fiberIdx<referenceIDsToShow.size(); ++fiberIdx)
	{
		size_t refFiberID = referenceIDsToShow[fiberIdx].index;
		double similarity = referenceIDsToShow[fiberIdx].similarity;
		for (int colIdx = 0; colIdx < refTable->GetNumberOfColumns(); ++colIdx)
		{
			m_refVisTable->SetValue(fiberIdx, colIdx, refTable->GetValue(refFiberID, colIdx));
		}
		// set projection error value to similarity...
		m_refVisTable->SetValue(fiberIdx, refTable->GetNumberOfColumns()-2, similarity);
	}

	m_nearestReferenceVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(m_ren, m_refVisTable,
		m_data->result[m_referenceID].mapping, QColor(0,0,0), std::map<size_t, std::vector<iAVec3f> >()) );
	/*
	QSharedPointer<iALookupTable> lut(new iALookupTable);
	*lut.data() = iALUT::Build(m_data->spmData->paramRange(m_data->spmData->numParams()-iARefDistCompute::EndColumns-iARefDistCompute::SimilarityMeasureCount+similarityMeasure),
		m_colorByThemeName, 256, SelectionOpacity);
	*/
	m_nearestReferenceVis->show();
	// for now, color by reference result color:
	m_nearestReferenceVis->setColor(getResultColor(m_referenceID));
	// ... and set up color coding by it!
	//m_nearestReferenceVis->setLookupTable(lut, refTable->GetNumberOfColumns()-2);
	// TODO: show similarity color map somewhere!!!

	// Lines from Fiber points to reference:
	if (!m_chkboxShowLines->isChecked())
		return;

	auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetNumberOfComponents(4);
	colors->SetName("Colors");
	QColor ConnectColor(128, 128, 128);
	unsigned char c[4];
	c[0] = ConnectColor.red();
	c[1] = ConnectColor.green();
	c[2] = ConnectColor.blue();
	c[3] = ConnectColor.alpha();
	size_t colorCount = m_refVisTable->GetNumberOfRows() * 4;
	for (size_t row = 0; row < colorCount; ++row)
	{
#if (VTK_MAJOR_VERSION < 7) || (VTK_MAJOR_VERSION==7 && VTK_MINOR_VERSION==0)
		m_colors->InsertNextTupleValue(c);
#else
		colors->InsertNextTypedTuple(c);
#endif
	}
	auto points = vtkSmartPointer<vtkPoints>::New();
	auto linePolyData = vtkSmartPointer<vtkPolyData>::New();
	auto lines = vtkSmartPointer<vtkCellArray>::New();
	size_t curFiber = 0;
	auto & ref = m_data->result[m_referenceID];
	size_t timeStep = static_cast<size_t>(m_optimStepSlider->value());
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
	{
		auto & d = m_data->result[resultID];
		if (resultID == m_referenceID || !resultSelected(m_resultUIs, resultID))
			continue;
		for (size_t fiberIdx = 0; fiberIdx < m_selection[resultID].size(); ++fiberIdx)
		{
			size_t fiberID = m_selection[resultID][fiberIdx];
			for (int n = 0; n < refCount; ++n)
			{
				iAVec3f start1, start2, end1, end2;
				size_t refFiberID = d.refDiffFiber[fiberID].dist[similarityMeasure][n].index;
				for (int i = 0; i < 3; ++i)
				{
					if (d.timeValues.size() > 0)
						start1[i] = d.timeValues[std::min(timeStep, d.timeValues.size()-1)][fiberID][i];
					else
						start1[i] = d.table->GetValue(fiberID, d.mapping->value(iACsvConfig::StartX + i)).ToFloat();
					end1[i] = ref.table->GetValue(refFiberID, ref.mapping->value(iACsvConfig::StartX + i)).ToFloat();
				}
				for (int i = 0; i < 3; ++i)
				{
					if (d.timeValues.size() > 0)
						start2[i] = d.timeValues[std::min(timeStep, d.timeValues.size()-1)][fiberID][3 + i];
					else
						start2[i] = d.table->GetValue(fiberID, d.mapping->value(iACsvConfig::EndX + i)).ToFloat();
					end2[i] = ref.table->GetValue(refFiberID, ref.mapping->value(iACsvConfig::EndX + i)).ToFloat();
				}
				if ((start1 - start2).length() > (start1 - end2).length() && (end1 - end2).length() > (end1 - start2).length())
				{
					iAVec3f tmp = start1;
					start1 = start2;
					start2 = tmp;
				}
				points->InsertNextPoint(start1.data());
				points->InsertNextPoint(end1.data());
				auto line1 = vtkSmartPointer<vtkLine>::New();
				line1->GetPointIds()->SetId(0, 4 * curFiber); // the index of line start point in pts
				line1->GetPointIds()->SetId(1, 4 * curFiber + 1); // the index of line end point in pts
				lines->InsertNextCell(line1);
				points->InsertNextPoint(start2.data());
				points->InsertNextPoint(end2.data());
				auto line2 = vtkSmartPointer<vtkLine>::New();
				line2->GetPointIds()->SetId(0, 4 * curFiber + 2); // the index of line start point in pts
				line2->GetPointIds()->SetId(1, 4 * curFiber + 3); // the index of line end point in pts
				lines->InsertNextCell(line2);
				++curFiber;
			}
		}
	}
	linePolyData->SetPoints(points);
	linePolyData->SetLines(lines);
	linePolyData->GetPointData()->AddArray(colors);

	auto ids = vtkSmartPointer<vtkIdTypeArray>::New();
	ids->SetName("OriginalIds");
	ids->SetNumberOfTuples(points->GetNumberOfPoints());
	for (vtkIdType id = 0; id < points->GetNumberOfPoints(); ++id)
		ids->SetTuple1(id, id);
	linePolyData->GetPointData()->AddArray(ids);

	auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

	mapper->SelectColorArray("Colors");
	mapper->SetScalarModeToUsePointFieldData();
	mapper->ScalarVisibilityOn();
	mapper->SetInputData(linePolyData);

	m_refLineActor = vtkSmartPointer<vtkActor>::New();
	m_refLineActor->SetMapper(mapper);
	m_refLineActor->GetProperty()->SetLineWidth(2);
	m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_refLineActor);
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}

void iAFiAKErController::playPauseOptimSteps()
{
	QPushButton* btn = qobject_cast<QPushButton*>(sender());
	if (m_playTimer->isActive())
	{
		addInteraction(QString("Stopped optimization step animation"));
		m_playTimer->stop();
		btn->setText("Play");
	}
	else
	{
		addInteraction(QString("Started optimization step animation"));
		m_playTimer->start();
		btn->setText("Pause");
	}
}

void iAFiAKErController::playTimer()
{
	QSignalBlocker block(m_optimStepSlider);
	int newStep = (m_optimStepSlider->value() + 1) % (m_optimStepSlider->maximum() + 1);
	m_optimStepSlider->setValue(newStep);
	setOptimStep(newStep);
}

void iAFiAKErController::playDelayChanged(int newInterval)
{
	addInteraction(QString("Changed optimization step animation delay to %1").arg(newInterval));
	m_playTimer->setInterval(newInterval);
}

void iAFiAKErController::visualizeCylinderSamplePoints()
{
	size_t resultID, fiberID = NoPlotsIdx;
	for (resultID = 0; resultID < m_selection.size(); ++resultID)
	{
		if (m_selection[resultID].size() > 0)
		{
			fiberID = m_selection[resultID][0];
			break;
		}
	}
	addInteraction(QString("Visualized cylinder sampling for fiber %1 in %2").arg(fiberID).arg(resultName(resultID)));
	if (fiberID == NoPlotsIdx)
		return;
	hideSamplePointsPrivate();

	auto & d = m_data->result[resultID];
	auto const & mapping = *d.mapping.data();
	std::vector<Vec3D> sampledPoints;
	iAFiberData sampleFiber(d.table, fiberID, mapping);
	samplePoints(sampleFiber, sampledPoints);
	auto sampleData = vtkSmartPointer<vtkPolyData>::New();
	auto points = vtkSmartPointer<vtkPoints>::New();
	for (size_t s = 0; s < sampledPoints.size(); ++s)
	{
		double pt[3];
		for (int i = 0; i < 3; ++i)
		pt[i] = sampledPoints[s][i];
		points->InsertNextPoint(pt);
	}
	sampleData->SetPoints(points);
	auto vertexFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
	vertexFilter->SetInputData(sampleData);
	vertexFilter->Update();

	// For color:
	auto polydata = vtkSmartPointer<vtkPolyData>::New();
	polydata->DeepCopy(vertexFilter->GetOutput());
	auto colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
	colors->SetNumberOfComponents(3);
	colors->SetName ("Colors");
	unsigned char blue[3] = {0, 0, 255};
	for (size_t s = 0; s < sampledPoints.size(); ++s)
#if (VTK_MAJOR_VERSION < 7) || (VTK_MAJOR_VERSION==7 && VTK_MINOR_VERSION==0)
		colors->InsertNextTupleValue(blue);
#else
		colors->InsertNextTypedTuple(blue);
#endif
	polydata->GetPointData()->SetScalars(colors);

	auto sampleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_sampleActor = vtkSmartPointer<vtkActor>::New();
	sampleMapper->SetInputData(polydata);
	m_sampleActor->SetMapper(sampleMapper);
	sampleMapper->Update();
	m_sampleActor->GetProperty()->SetPointSize(2);
	m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_sampleActor);
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}

void iAFiAKErController::hideSamplePoints()
{
	if (!m_sampleActor)
		return;
	addInteraction("Hide cylinder sampling points");
	hideSamplePointsPrivate();
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
	m_sampleActor = nullptr;
}

void iAFiAKErController::hideSamplePointsPrivate()
{
	if (m_sampleActor)
		m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(m_sampleActor);
}

void iAFiAKErController::optimDataToggled(int state)
{
	int chartID = QObject::sender()->property("chartID").toInt();
	addInteraction(QString("Toggled visibility of %1 vs. optimization step chart.").arg(diffName(chartID)));
	toggleOptimStepChart(chartID, state == Qt::Checked);
}

void iAFiAKErController::selectionFromListActivated(QModelIndex const & index)
{
	auto item = m_selectionListModel->itemFromIndex(index);
	int row = item->row();
	addInteraction(QString("Switched to selection %1").arg(row));
	m_selection = m_selections[row];
	showSelectionDetail();
	showSelectionIn3DViews();
	showSelectionInPlots();
	showSelectionInSPM();
	changeReferenceDisplay();
}

void iAFiAKErController::showSelectionDetail()
{
	m_selectionDetailModel->clear();
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
	{
		if (m_selection[resultID].size() == 0)
			continue;
		auto resultItem = new QStandardItem(resultName(resultID));
		resultItem->setData(static_cast<unsigned long long>(resultID), Qt::UserRole);
		m_selectionDetailModel->appendRow(resultItem);
		for (size_t selID = 0; selID < m_selection[resultID].size(); ++selID)
		{
			size_t fiberID = m_selection[resultID][selID];
			auto item = new QStandardItem(QString("%1").arg(fiberID));
			item->setData(static_cast<unsigned long long>(fiberID), Qt::UserRole);
			resultItem->appendRow(item);
		}
	}
}

void iAFiAKErController::selectionDetailsItemClicked(QModelIndex const & index)
{
	auto item = m_selectionDetailModel->itemFromIndex(index);
	if (!item->hasChildren())
	{   // item text can be changed by users, so use internal data!
		size_t resultID = item->parent()->data(Qt::UserRole).toULongLong();
		size_t fiberID  = item->data(Qt::UserRole).toULongLong();
		addInteraction(QString("Focus on fiber %1 in %2").arg(fiberID).arg(resultName(resultID)));
		clearSelection();
		m_selection[resultID].push_back(fiberID);
		showSelectionIn3DViews();
		showSelectionInPlots();
		showSelectionInSPM();
		changeReferenceDisplay();
	}
}

QString iAFiAKErController::diffName(int chartID) const
{
	size_t spmCol = m_data->spmData->numParams() -
		(iAFiberCharData::FiberValueCount + iARefDistCompute::SimilarityMeasureCount + iARefDistCompute::EndColumns) + chartID;
	return m_data->spmData->parameterName(spmCol);
}

QString iAFiAKErController::resultName(size_t resultID) const
{
	return QFileInfo(m_data->result[resultID].fileName).baseName();
}

void iAFiAKErController::saveAnalysisClick()
{
	QString fileName = QFileDialog::getSaveFileName(this, ModuleSettingsKey, m_data->folder, "FIAKER Project file (*.fpf);;");
	if (fileName.isEmpty())
		return;
	addInteraction(QString("Save Analysis as '%1'").arg(fileName));
	QSettings projectFile(fileName, QSettings::IniFormat);
	projectFile.setIniCodec("UTF-8");
	projectFile.setValue(ProjectFileFolder, MakeRelative(QFileInfo(fileName).absolutePath(), m_data->folder));
	projectFile.setValue(ProjectFileFormat, m_configName);
	if (m_referenceID != NoResult)
		projectFile.setValue(ProjectFileReference, static_cast<qulonglong>(m_referenceID));
}

void iAFiAKErController::loadAnalysisClick()
{
	addInteraction(QString("Loading Analysis (in new window!)"));
	loadAnalysis(m_mainWnd, m_data->folder);
}

void iAFiAKErController::loadAnalysis(MainWindow* mainWnd, QString const & folder)
{
	QString fileName = QFileDialog::getOpenFileName(mainWnd, ModuleSettingsKey, folder, "FIAKER Project file (*.fpf);;");
	if (fileName.isEmpty())
		return;
	QSettings projectFile(fileName, QSettings::IniFormat);
	projectFile.setIniCodec("UTF-8");
	auto dataFolder  = MakeAbsolute(QFileInfo(fileName).absolutePath(), projectFile.value(ProjectFileFolder, "").toString());
	auto configName  = projectFile.value(ProjectFileFormat, "").toString();
	auto stepShift   = projectFile.value(ProjectFileStepShift, 0).toDouble();
	auto explorer = new iAFiAKErController(mainWnd);
	explorer->m_projectReferenceID = projectFile.value(ProjectFileReference, static_cast<qulonglong>(NoResult)).toULongLong();
	mainWnd->addSubWindow(explorer);
	if (explorer->m_projectReferenceID != NoResult)
		connect(explorer, &iAFiAKErController::setupFinished, explorer, &iAFiAKErController::setProjectReference);
	explorer->start(dataFolder, configName, stepShift);
}

void iAFiAKErController::setProjectReference()
{
	setReference(m_projectReferenceID);
}

void iAFiAKErController::loadVolume(QString const & fileName)
{
	addInteraction(QString("Loading reference volume '%1'").arg(fileName));
	iAConnector con;
	iAITKIO::ScalarPixelType pixelType;
	iAITKIO::ImagePointer img = iAITKIO::readFile(fileName, pixelType, false);
	con.setImage(img);
	m_refImg = vtkSmartPointer<vtkImageData>::New();
	m_refImg->DeepCopy(con.vtkImage());
	double rng[2]; m_refImg->GetScalarRange(rng);
	m_refCF = defaultColorTF(rng);
	m_refOF = defaultOpacityTF(rng, true);
	iASimpleTransferFunction tf(
		m_refCF.GetPointer(),
		m_refOF.GetPointer()
	);
	m_refRenderer = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(&tf, m_refImg));
	m_refRenderer->addTo(m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer());
}

void iAFiAKErController::showReferenceInChartToggled()
{
	addInteraction(QString("Toggled showing of reference in distribution charts in result list to %1")
		.arg(m_showReferenceInChart->isChecked()?"on":"off"));
	updateRefDistPlots();
}

void iAFiAKErController::distributionChartTypeChanged(int idx)
{
	addInteraction(QString("Distribution chart plot type switched to %1")
		.arg(m_distributionChartType->itemText(idx)));
	changeDistributionSource(m_distributionChoice->currentIndex());
}

void iAFiAKErController::toggleDockWidgetTitleBars()
{
	for (auto w : m_views)
		w->toggleTitleBar();
}

void iAFiAKErController::toggleSettings()
{
	m_views[SettingsView]->setVisible(!m_views[SettingsView]->isVisible());
}
