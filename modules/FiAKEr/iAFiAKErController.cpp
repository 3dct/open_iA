/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "iAFiAKErController.h"

#include "iAFiberCharData.h"
#include "iAFiberData.h"     // for samplePoints
#include "iAJobListView.h"
#include "iARefDistCompute.h"
#include "iAStackedBarChart.h"

// FeatureScout:
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
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "iALookupTable.h"
#include "iALUT.h"
#include "iAModuleDispatcher.h"
#include "iARendererManager.h"
#include "iAStringHelper.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLine.h>
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
	const int HistogramMinWidth = 80;
	const int StackedBarMinWidth = 70;
	const int DefaultPlayDelay = 1000;
	const int HistogramBins = 20;

	int SelectionOpacity = iA3DLineObjectVis::DefaultSelectionOpacity;
	int ContextOpacity = iA3DLineObjectVis::DefaultContextOpacity;
	const size_t NoPlotsIdx = std::numeric_limits<size_t>::max();
	const size_t NoResult = NoPlotsIdx;
	const QString ModuleSettingsKey("FiberOptimizationExplorer");

	const QColor DistributionPlotColor(70, 70, 70, 255);
	const QColor DistributionRefPlotColor(70, 70, 70, 80);
	const QColor OptimStepMarkerColor(192, 0, 0);
	const QColor SelectionColor(0, 0, 0);

}

//! UI elements for each result
class iAFiberCharUIData
{
public:
	iAVtkWidgetClass* vtkWidget;
	QSharedPointer<iA3DCylinderObjectVis> mini3DVis;
	QSharedPointer<iA3DCylinderObjectVis> main3DVis;
	QCheckBox* cbBoundingBox;
	iAChartWidget* histoChart;
	iAStackedBarChart* stackedBars;
	//! index where the plots for this result start
	size_t startPlotIdx;
};

iAFiAKErController::iAFiAKErController(MainWindow* mainWnd) :
	m_colorTheme(iAColorThemeManager::GetInstance().GetTheme("Brewer Accent (max. 8)")),
	m_mainWnd(mainWnd),
	m_spm(new iAQSplom()),
	m_referenceID(NoResult),
	m_playTimer(new QTimer(this)),
	m_refDistCompute(nullptr),
	m_renderManager(new iARendererManager())
{
	setDockOptions(AllowNestedDocks | AllowTabbedDocks);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif
	setMinimumSize(600, 400);
	setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
}

void iAFiAKErController::start(QString const & path, QString const & configName)
{
	m_configName = configName;
	m_jobs = new iAJobListView();
	m_views.resize(DockWidgetCount);
	m_views[JobView] = new iADockWidgetWrapper(m_jobs, "Jobs", "foeJobs");
	addDockWidget(Qt::BottomDockWidgetArea, m_views[JobView]);

	m_data = QSharedPointer<iAFiberResultsCollection>(new iAFiberResultsCollection());
	auto resultsLoader = new iAFiberResultsLoader(m_data, path, configName);
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


	// Main 3D View:

	m_mainRenderer = new iAVtkWidgetClass();
	auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	auto ren = vtkSmartPointer<vtkRenderer>::New();
	ren->SetBackground(1.0, 1.0, 1.0);
	renWin->SetAlphaBitPlanes(1);
	ren->SetUseDepthPeeling(true);
	ren->SetMaximumNumberOfPeels(1000);
	renWin->AddRenderer(ren);
	m_mainRenderer->SetRenderWindow(renWin);
	m_renderManager->addToBundle(ren);
	m_style = vtkSmartPointer<iASelectionInteractorStyle>::New();
	m_style->setSelectionProvider(this);
	m_style->assignToRenderWindow(renWin);
	m_style->setRenderer(ren);
	connect(m_style.GetPointer(), &iASelectionInteractorStyle::selectionChanged, this, &iAFiAKErController::selection3DChanged);

	QWidget* showReferenceWidget = new QWidget();
	m_chkboxShowReference = new QCheckBox("Show ");
	m_spnboxReferenceCount = new QSpinBox();
	m_spnboxReferenceCount->setValue(1);
	m_spnboxReferenceCount->setMinimum(1);
	m_spnboxReferenceCount->setMaximum(iARefDistCompute::MaxNumberOfCloseFibers);
	showReferenceWidget->setLayout(new QHBoxLayout());
	m_cmbboxDistanceMeasure = new QComboBox();
	m_cmbboxDistanceMeasure->addItem("Dist1 (Midpoint, Angles, Length)");
	m_cmbboxDistanceMeasure->addItem("Dist2 (Start-Start/Center-Center/End-End)");
	m_cmbboxDistanceMeasure->addItem("Dist3 (All 9 pairs Start-/Center-/Endpoint)");
	m_cmbboxDistanceMeasure->addItem("Dist4 (Min. Fiber Fragment Distance)");
	m_cmbboxDistanceMeasure->addItem("Dist5 (Overlap % short fiber)");
	m_cmbboxDistanceMeasure->addItem("Dist6 (Overlap % in relation to Volume Ratio, short fiber)");
	m_cmbboxDistanceMeasure->addItem("Dist7 (Overlap % in relation to Volume Ratio,  directional)");
	m_cmbboxDistanceMeasure->setCurrentIndex(iARefDistCompute::BestDistanceMetric);
	m_chkboxShowLines = new QCheckBox("Connect");
	connect(m_chkboxShowReference, &QCheckBox::stateChanged, this, &iAFiAKErController::showReferenceToggled);
	connect(m_spnboxReferenceCount, SIGNAL(valueChanged(int)), this, SLOT(showReferenceCountChanged(int)));
	connect(m_cmbboxDistanceMeasure, SIGNAL(currentIndexChanged(int)), this, SLOT(showReferenceMeasureChanged(int)));
	connect(m_chkboxShowLines, &QCheckBox::stateChanged, this, &iAFiAKErController::showReferenceLinesToggled);
	showReferenceWidget->layout()->addWidget(m_chkboxShowReference);
	showReferenceWidget->layout()->addWidget(m_spnboxReferenceCount);
	showReferenceWidget->layout()->addWidget(new QLabel("nearest ref. fibers, metric:"));
	showReferenceWidget->layout()->addWidget(m_cmbboxDistanceMeasure);
	showReferenceWidget->layout()->addWidget(m_chkboxShowLines);
	showReferenceWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QWidget* mainRendererContainer = new QWidget();
	mainRendererContainer->setLayout(new QVBoxLayout());
	mainRendererContainer->layout()->addWidget(m_mainRenderer);
	mainRendererContainer->layout()->addWidget(showReferenceWidget);


	// Settings View

	m_defaultOpacitySlider = new QSlider(Qt::Horizontal);
	m_defaultOpacitySlider->setMinimum(0);
	m_defaultOpacitySlider->setMaximum(255);
	m_defaultOpacitySlider->setValue(SelectionOpacity);
	connect(m_defaultOpacitySlider, &QSlider::valueChanged, this, &iAFiAKErController::mainOpacityChanged);
	m_defaultOpacityLabel = new QLabel(QString::number(SelectionOpacity));
	QWidget* defaultOpacityWidget = new QWidget();
	defaultOpacityWidget->setLayout(new QHBoxLayout());
	defaultOpacityWidget->layout()->addWidget(new QLabel("Main Opacity"));
	defaultOpacityWidget->layout()->addWidget(m_defaultOpacitySlider);
	defaultOpacityWidget->layout()->addWidget(m_defaultOpacityLabel);
	defaultOpacityWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	m_contextOpacitySlider = new QSlider(Qt::Horizontal);
	m_contextOpacitySlider->setMinimum(0);
	m_contextOpacitySlider->setMaximum(255);
	m_contextOpacitySlider->setValue(ContextOpacity);
	connect(m_contextOpacitySlider, &QSlider::valueChanged, this, &iAFiAKErController::contextOpacityChanged);
	m_contextOpacityLabel = new QLabel(QString::number(ContextOpacity));
	QWidget* contextOpacityWidget = new QWidget();
	contextOpacityWidget->setLayout(new QHBoxLayout());
	contextOpacityWidget->layout()->addWidget(new QLabel("Context Opacity"));
	contextOpacityWidget->layout()->addWidget(m_contextOpacitySlider);
	contextOpacityWidget->layout()->addWidget(m_contextOpacityLabel);
	contextOpacityWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QWidget* moreButtons = new QWidget();
	moreButtons->setLayout(new QHBoxLayout());
	auto showSampledCylinder = new QPushButton("Sample Fiber");
	auto hideSampledCylinder = new QPushButton("Hide Sample Points");
	auto spatialOverviewButton = new QPushButton("Spatial Overview");
	auto selectionModeChoice = new QComboBox();
	selectionModeChoice->addItem("Rubberband Rectangle (Endpoints)");
	selectionModeChoice->addItem("Single Fiber Click");
	connect(showSampledCylinder, &QPushButton::pressed, this, &iAFiAKErController::visualizeCylinderSamplePoints);
	connect(hideSampledCylinder, &QPushButton::pressed, this, &iAFiAKErController::hideSamplePoints);
	connect(spatialOverviewButton, &QPushButton::pressed, this, &iAFiAKErController::showSpatialOverviewButton);
	connect(selectionModeChoice, SIGNAL(currentIndexChanged(int)), this, SLOT(selectionModeChanged(int)));
	moreButtons->layout()->addWidget(showSampledCylinder);
	moreButtons->layout()->addWidget(hideSampledCylinder);
	moreButtons->layout()->addWidget(spatialOverviewButton);
	moreButtons->layout()->addWidget(selectionModeChoice);
	moreButtons->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QGroupBox* main3DViewSettings = new QGroupBox("Main 3D View");
	main3DViewSettings->setLayout(new QVBoxLayout());
	main3DViewSettings->layout()->addWidget(defaultOpacityWidget);
	main3DViewSettings->layout()->addWidget(contextOpacityWidget);
	main3DViewSettings->layout()->addWidget(moreButtons);

	QWidget* playControls = new QWidget();
	playControls->setLayout(new QHBoxLayout());
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
	playControls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	m_optimStepChart.resize(iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + 1);

	auto dataChooser = new QScrollArea();
	dataChooser->setWidgetResizable(true);
	auto comboBoxContainer = new QWidget();
	dataChooser->setWidget(comboBoxContainer);
	dataChooser->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
	comboBoxContainer->setLayout(new QVBoxLayout());
	ChartCount = iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + 1;
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
	optimStepSettings->layout()->addWidget(playControls);
	optimStepSettings->layout()->addWidget(dataChooser);

	QWidget* settingsView = new QWidget();
	settingsView->setLayout(new QVBoxLayout());
	settingsView->layout()->addWidget(main3DViewSettings);
	settingsView->layout()->addWidget(optimStepSettings);


	// Optimization Steps View:

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
	optimStepsView->layout()->addWidget(chartContainer);
	optimStepsView->layout()->addWidget(optimStepsCtrls);


	// Results List View

	size_t commonPrefixLength = 0, commonSuffixLength = 0;
	QString baseName0;
	for (int resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		QString baseName = QFileInfo(m_data->result[resultID].fileName).baseName();
		if (resultID > 0)
		{
			commonPrefixLength = std::min(commonPrefixLength, static_cast<size_t>(GreatestCommonPrefixLength(baseName, baseName0)));
			commonSuffixLength = std::min(commonSuffixLength, static_cast<size_t>(GreatestCommonSuffixLength(baseName, baseName0)));
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
	auto colorTheme = iAColorThemeManager::GetInstance().GetTheme("Material red (max. 10)");
	m_defaultButtonGroup = new QButtonGroup();
	auto resultListScrollArea = new QScrollArea();
	resultListScrollArea->setWidgetResizable(true);
	auto resultList = new QWidget();
	resultListScrollArea->setWidget(resultList);
	auto resultsListLayout = new QGridLayout();
	resultsListLayout->setSpacing(5);
	resultsListLayout->setColumnStretch(3, 1);
	resultsListLayout->setColumnStretch(4, 1);

	m_stackedBarsHeaders = new iAStackedBarChart(colorTheme, true);
	m_stackedBarsHeaders->setMinimumWidth(StackedBarMinWidth);
	auto headerFiberCountAction = new QAction("Fiber Count", nullptr);
	headerFiberCountAction->setProperty("colID", 0);
	headerFiberCountAction->setCheckable(true);
	headerFiberCountAction->setChecked(true);
	connect(headerFiberCountAction, &QAction::triggered, this, &iAFiAKErController::stackedColSelect);
	m_stackedBarsHeaders->contextMenu()->addAction(headerFiberCountAction);
	connect(m_stackedBarsHeaders, &iAStackedBarChart::switchedStackMode, this, &iAFiAKErController::switchStackMode);

	auto nameLabel = new QLabel("Name");
	nameLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	auto actionsLabel = new QLabel("Actions");
	actionsLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	auto previewLabel = new QLabel("Preview");
	previewLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	auto distrCmb = new QComboBox();
	QStringList paramNames;
	for (int curIdx = 0; curIdx < m_data->spmData->numParams() - 1; ++curIdx)
		paramNames.push_back(QString("%1 Distribution").arg(m_data->spmData->parameterName(curIdx)));
	distrCmb->addItems(paramNames);
	connect(distrCmb, SIGNAL(currentIndexChanged(int)), this, SLOT(changeDistributionSource(int)));
	distrCmb->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	resultsListLayout->addWidget(nameLabel, 0, 0);
	resultsListLayout->addWidget(actionsLabel, 0, 1);
	resultsListLayout->addWidget(previewLabel, 0, 2);
	resultsListLayout->addWidget(m_stackedBarsHeaders, 0, 3);
	resultsListLayout->addWidget(distrCmb, 0, 4);

	for (int resultID = 0; resultID < m_data->result.size(); ++resultID)
	{
		auto & d = m_data->result.at(resultID);
		auto & uiData = m_resultUIs[resultID];

		uiData.vtkWidget = new iAVtkWidgetClass();
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

		QCheckBox* toggleMainRender = new QCheckBox("Show");
		toggleMainRender->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		toggleMainRender->setProperty("resultID", resultID);
		uiData.cbBoundingBox = new QCheckBox("Box");
		uiData.cbBoundingBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		uiData.cbBoundingBox->setProperty("resultID", resultID);
		QRadioButton* toggleReference = new QRadioButton("");
		toggleReference->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		toggleReference->setProperty("resultID", resultID);
		m_defaultButtonGroup->addButton(toggleReference);

		QWidget* resultActions = new QWidget();
		resultActions->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
		resultActions->setLayout(new QVBoxLayout());
		resultActions->layout()->addWidget(toggleMainRender);
		resultActions->layout()->addWidget(uiData.cbBoundingBox);
		resultActions->layout()->addWidget(toggleReference);

		uiData.stackedBars = new iAStackedBarChart(colorTheme);
		uiData.stackedBars->setMinimumWidth(StackedBarMinWidth);
		uiData.stackedBars->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		connect(m_stackedBarsHeaders, &iAStackedBarChart::weightsChanged, uiData.stackedBars, &iAStackedBarChart::setWeights);

		uiData.histoChart = new iAChartWidget(resultList, "Fiber Length", "");
		uiData.histoChart->setShowXAxisLabel(false);
		uiData.histoChart->setMinimumWidth(HistogramMinWidth);
		uiData.histoChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

		QString name = QFileInfo(d.fileName).baseName();
		name = name.mid(commonPrefixLength, name.size() - commonPrefixLength - commonSuffixLength);

		resultsListLayout->addWidget(new QLabel(name), resultID + 1, 0);
		resultsListLayout->addWidget(resultActions, resultID + 1, 1);
		resultsListLayout->addWidget(uiData.vtkWidget, resultID + 1, 2);
		resultsListLayout->addWidget(uiData.stackedBars, resultID + 1, 3);
		resultsListLayout->addWidget(uiData.histoChart, resultID + 1, 4);

		uiData.mini3DVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(
			uiData.vtkWidget, d.table, d.mapping, getResultColor(resultID)));
		uiData.main3DVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(
			m_mainRenderer, d.table, d.mapping, getResultColor(resultID)));
		uiData.mini3DVis->setColor(getResultColor(resultID));
		uiData.mini3DVis->show();
		ren->ResetCamera();

		connect(uiData.vtkWidget, &iAVtkWidgetClass::mouseEvent, this, &iAFiAKErController::miniMouseEvent);
		connect(toggleMainRender, &QCheckBox::stateChanged, this, &iAFiAKErController::toggleVis);
		connect(toggleReference, &QRadioButton::toggled, this, &iAFiAKErController::referenceToggled);
		connect(uiData.cbBoundingBox, &QCheckBox::stateChanged, this, &iAFiAKErController::toggleBoundingBox);
	}
	resultList->setLayout(resultsListLayout);
	addStackedBar(0);
	distrCmb->setCurrentIndex((*m_data->result[0].mapping)[iACsvConfig::Length]);

	// Interaction Protocol:

	m_interactionProtocol = new QTreeView();
	m_interactionProtocol->setHeaderHidden(true);
	m_interactionProtocolModel = new QStandardItemModel();
	m_interactionProtocol->setModel(m_interactionProtocolModel);
	QWidget* protocolView = new QWidget();
	protocolView->setLayout(new QHBoxLayout());
	protocolView->layout()->addWidget(m_interactionProtocol);


	// Selection View:

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
	selectionView->layout()->addWidget(selectionListWrapper);
	selectionView->layout()->addWidget(selectionDetailWrapper);

	m_views.resize(DockWidgetCount);
	m_views[ResultListView] = new iADockWidgetWrapper(resultListScrollArea, "Result list", "foeResultList");
	m_views[Main3DView]     = new iADockWidgetWrapper(mainRendererContainer, "3D view", "foe3DView");
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

	restoreState(settings.value(ModuleSettingsKey + "/state", saveState()).toByteArray());

	// SPM needs an active OpenGL Context (it must be visible when setData is called):
	m_spm->setMinimumWidth(200);
	m_spm->showAllPlots(false);
	auto np = m_data->spmData->numParams();
	std::vector<char> v(m_data->spmData->numParams(), false);
	auto & map = *m_data->result[0].mapping.data();
	v[map[iACsvConfig::StartX]] = v[map[iACsvConfig::StartY]] = v[map[iACsvConfig::StartZ]]
		= v[np - 7] = v[np - 6] = v[np - 5] = v[np - 4] = v[np - 3] = v[np - 2] = true;
	m_spm->setData(m_data->spmData, v);
	iALookupTable lut;
	int numOfResults = m_data->result.size();
	lut.setRange(0, numOfResults - 1);
	lut.allocate(numOfResults);
	for (size_t i = 0; i < numOfResults; i++)
		lut.setColor(i, m_colorTheme->GetColor(i));
	m_spm->setLookupTable(lut, m_data->spmData->numParams() - 1);
	m_spm->setSelectionMode(iAScatterPlot::Rectangle);
	m_spm->showDefaultMaxizimedPlot();
	m_spm->setSelectionColor(SelectionColor);
	m_spm->setPointRadius(2.5);
	m_spm->settings.enableColorSettings = true;
	connect(m_spm, &iAQSplom::selectionModified, this, &iAFiAKErController::selectionSPMChanged);
	connect(m_spm, &iAQSplom::lookupTableChanged, this, &iAFiAKErController::spmLookupTableChanged);
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
}

void iAFiAKErController::removeStackedBar(int index)
{
	QString title = stackedBarColName(index);
	m_stackedBarsHeaders->removeBar(title);
	for (size_t resultID=0; resultID<m_resultUIs.size(); ++resultID)
		m_resultUIs[resultID].stackedBars->removeBar(title);
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

void iAFiAKErController::changeDistributionSource(int index)
{
	auto range = m_data->spmData->paramRange(index);
	for (size_t resultID=0; resultID<m_data->result.size(); ++resultID)
	{
		auto & d = m_data->result[resultID];
		auto & chart = m_resultUIs[resultID].histoChart;
		chart->clearPlots();
		chart->setXBounds(range[0], range[1]);
		std::vector<double> fiberData(d.fiberCount);
		for (size_t fiberID=0; fiberID<d.fiberCount; ++fiberID)
			fiberData[fiberID] = d.table->GetValue(fiberID, index).ToDouble();
		auto histogramData = iAHistogramData::Create(fiberData, HistogramBins, Continuous, range[0], range[1]);
		auto histogramPlot = QSharedPointer<iABarGraphPlot>(new iABarGraphPlot(histogramData, DistributionPlotColor));
		chart->addPlot(histogramPlot);
	}
	if (m_referenceID != NoResult)
	{
		QSharedPointer<iAPlotData> refPlotData = m_resultUIs[m_referenceID].histoChart->plots()[0]->data();
		for (size_t resultID = 0; resultID < m_data->result.size(); ++resultID)
		{
			if (resultID == m_referenceID)
				continue;
			QSharedPointer<iABarGraphPlot> refPlot(new iABarGraphPlot(refPlotData, DistributionRefPlotColor));
			m_resultUIs[resultID].histoChart->addPlot(refPlot);
		}
	}
	for (size_t resultID = 0; resultID<m_data->result.size(); ++resultID)
		m_resultUIs[resultID].histoChart->update();
}

QColor iAFiAKErController::getResultColor(int resultID)
{
	QColor color = m_colorTheme->GetColor(resultID);
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
	auto & data = m_data->result[resultID];
	auto & ui = m_resultUIs[resultID];
	if (state == Qt::Checked)
	{
		ui.main3DVis->setSelectionOpacity(SelectionOpacity);
		ui.main3DVis->setContextOpacity(ContextOpacity);
		if (m_spm->colorScheme() == iAQSplom::ByParameter)
		{
			ui.main3DVis->setLookupTable(m_spm->lookupTable(), m_spm->colorLookupParam());
			ui.main3DVis->updateColorSelectionRendering();
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
			ui.main3DVis->updateValues(data.timeValues[
				std::min(data.timeValues.size() - 1, static_cast<size_t>(m_optimStepSlider->value()))]);
		}
		ui.main3DVis->show();
		m_style->addInput( resultID, ui.main3DVis->getLinePolyData() );
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
	addInteraction(QString("Toggle bounding box of %1").arg(resultName(resultID)));
	auto & ui = m_resultUIs[resultID];
	if (state == Qt::Checked)
		ui.main3DVis->showBoundingBox();
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
	for (size_t resultID = 0; resultID<m_resultUIs.size(); ++resultID)
	{
		auto & ui = m_resultUIs[resultID];
		ui.mini3DVis->setSelection(m_selection[resultID], anythingSelected);
		if (ui.main3DVis->visible())
			ui.main3DVis->setSelection(m_selection[resultID], anythingSelected);
	}
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
}

void iAFiAKErController::miniMouseEvent(QMouseEvent* ev)
{
	if (ev->buttons() == Qt::RightButton && ev->type() == QEvent::MouseButtonPress)
	{
		int resultID = QObject::sender()->property("resultID").toInt();
		addInteraction(QString("Started FiberScout for %1.").arg(resultName(resultID)));
		iAFeatureScoutModuleInterface * featureScout = m_mainWnd->getModuleDispatcher().GetModule<iAFeatureScoutModuleInterface>();
		MdiChild* newChild = m_mainWnd->createMdiChild(false);
		iACsvConfig config = getCsvConfig(m_data->result[resultID].fileName, m_configName);
		featureScout->LoadFeatureScout(config, newChild);
		newChild->LoadLayout("FeatureScout");
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
				main3DVis->updateValues(timeValues[std::min(static_cast<size_t>(optimStep), timeValues.size()-1)]);
		}
	}
}

void iAFiAKErController::mainOpacityChanged(int opacity)
{
	addInteraction(QString("Set main opacity to %1.").arg(opacity));
	m_defaultOpacityLabel->setText(QString::number(opacity));
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
	m_contextOpacityLabel->setText(QString::number(opacity));
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

void iAFiAKErController::referenceToggled(bool)
{
	if (m_refDistCompute)
	{
		DEBUG_LOG("Another reference computation is currently running, please let that finish first!");
		return;
	}
	QRadioButton* sender = qobject_cast<QRadioButton*>(QObject::sender());
	sender->setText("reference");
	for (auto button : m_defaultButtonGroup->buttons())
		if (button != sender)
			button->setText("");
	size_t referenceID = sender->property("resultID").toULongLong();
	addInteraction(QString("Reference set to %1").arg(resultName(referenceID)));
	m_refDistCompute = new iARefDistCompute(m_data, referenceID);
	connect(m_refDistCompute, &QThread::finished, this, &iAFiAKErController::refDistAvailable);
	m_jobs->addJob("Computing Reference Distances", m_refDistCompute->progress(), m_refDistCompute);
	m_refDistCompute->start();
}

void iAFiAKErController::refDistAvailable()
{
	size_t startIdx = m_data->spmData->numParams() - (iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + iARefDistCompute::EndColumns);
	std::vector<size_t> changedSpmColumns;
	for (size_t paramID = 0; paramID < iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount; ++paramID)
	{
		size_t columnID = startIdx + paramID;
		changedSpmColumns.push_back(columnID);
	}
	m_referenceID = m_refDistCompute->referenceID();
	m_data->spmData->updateRanges(changedSpmColumns);
	m_spm->update();
	delete m_refDistCompute;
	m_refDistCompute = nullptr;

	for (size_t chartID=0; chartID<ChartCount-1; ++chartID)
		m_chartCB[chartID]->setEnabled(true);


	auto refPlotData = m_resultUIs[m_referenceID].histoChart->plots()[0]->data();

	for (size_t resultID=0; resultID<m_data->result.size(); ++resultID)
	{
		QSharedPointer<iABarGraphPlot> newPlot(new iABarGraphPlot(refPlotData, QColor(70, 70, 70, 80)));
		auto & chart = m_resultUIs[resultID].histoChart;
		if (chart->plots().size() > 1)
			chart->removePlot(chart->plots()[1]);
		m_resultUIs[resultID].histoChart->addPlot(newPlot);
		m_resultUIs[resultID].histoChart->update();
	}


	for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount; ++diffID)
	{
		auto diffAvgAction = new QAction(m_data->spmData->parameterName(startIdx+diffID), nullptr);
		diffAvgAction->setProperty("colID", static_cast<unsigned long long>(diffID+1));
		diffAvgAction->setCheckable(true);
		diffAvgAction->setChecked(false);
		connect(diffAvgAction, &QAction::triggered, this, &iAFiAKErController::stackedColSelect);
		m_stackedBarsHeaders->contextMenu()->addAction(diffAvgAction);
	}

	showSpatialOverview();
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
	*lut = iALUT::Build(range, "Diverging red-gray-blue", 255, SelectionOpacity);
	auto ref3D = m_resultUIs[m_referenceID].main3DVis;
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
	for (size_t resultID = 0; resultID < m_resultUIs.size(); ++resultID)
	{
		m_resultUIs[resultID].mini3DVis->setLookupTable(lut, colorLookupParam);
		if (m_resultUIs[resultID].main3DVis->visible())
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
	size_t distanceMeasure = m_cmbboxDistanceMeasure->currentIndex();
	bool showRef = m_chkboxShowReference->isChecked();
	int refCount = std::min(iARefDistCompute::MaxNumberOfCloseFibers, m_spnboxReferenceCount->value());

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
		DEBUG_LOG("Please select a reference first!");
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

	std::vector<iAFiberDistance> referenceIDsToShow;

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
				referenceIDsToShow.push_back(m_data->result[resultID].refDiffFiber[fiberID].dist[distanceMeasure][n]);
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
		double distance = referenceIDsToShow[fiberIdx].distance;
		for (int colIdx = 0; colIdx < refTable->GetNumberOfColumns(); ++colIdx)
		{
			m_refVisTable->SetValue(fiberIdx, colIdx, refTable->GetValue(refFiberID, colIdx));
		}
		// set projection error value to distance...
		m_refVisTable->SetValue(fiberIdx, refTable->GetNumberOfColumns()-2, distance);
	}

	m_nearestReferenceVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(m_mainRenderer, m_refVisTable,
		m_data->result[m_referenceID].mapping, QColor(0,0,0) ) );
	QSharedPointer<iALookupTable> lut(new iALookupTable);
	*lut.data() = iALUT::Build(m_data->spmData->paramRange(m_data->spmData->numParams()-iARefDistCompute::EndColumns-iARefDistCompute::DistanceMetricCount+distanceMeasure),
		"ColorBrewer single hue 5-class oranges", 256, SelectionOpacity);
	m_nearestReferenceVis->show();
	// ... and set up color coding by it!
	m_nearestReferenceVis->setLookupTable(lut, refTable->GetNumberOfColumns()-2);
	// TODO: show distance color map somewhere!!!

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
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
	{
		if (resultID == m_referenceID || !resultSelected(m_resultUIs, resultID))
			continue;
		for (size_t fiberIdx = 0; fiberIdx < m_selection[resultID].size(); ++fiberIdx)
		{
			size_t fiberID = m_selection[resultID][fiberIdx];
			for (int n = 0; n < refCount; ++n)
			{
				float first[3], end[3];
				size_t refFiberID = m_data->result[resultID].refDiffFiber[fiberID].dist[distanceMeasure][n].index;
				for (int i = 0; i < 3; ++i)
				{
					first[i] = m_data->result[resultID].table->GetValue(fiberID, m_data->result[resultID].mapping->value(iACsvConfig::StartX + i)).ToFloat();
					end[i] = m_data->result[m_referenceID].table->GetValue(refFiberID, m_data->result[m_referenceID].mapping->value(iACsvConfig::StartX + i)).ToFloat();
				}
				points->InsertNextPoint(first);
				points->InsertNextPoint(end);
				auto line1 = vtkSmartPointer<vtkLine>::New();
				line1->GetPointIds()->SetId(0, 4 * curFiber); // the index of line start point in pts
				line1->GetPointIds()->SetId(1, 4 * curFiber + 1); // the index of line end point in pts
				lines->InsertNextCell(line1);
				for (int i = 0; i < 3; ++i)
				{
					first[i] = m_data->result[resultID].table->GetValue(fiberID, m_data->result[resultID].mapping->value(iACsvConfig::EndX + i)).ToFloat();
					end[i] = m_data->result[m_referenceID].table->GetValue(refFiberID, m_data->result[m_referenceID].mapping->value(iACsvConfig::EndX + i)).ToFloat();
				}
				points->InsertNextPoint(first);
				points->InsertNextPoint(end);
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
		(iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + iARefDistCompute::EndColumns) + chartID;
	return m_data->spmData->parameterName(spmCol);
}

QString iAFiAKErController::resultName(size_t resultID) const
{
	return QFileInfo(m_data->result[resultID].fileName).baseName();
}
