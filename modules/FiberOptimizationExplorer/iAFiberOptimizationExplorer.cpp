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
#include "iAFiberOptimizationExplorer.h"

#include "iAFiberCharData.h"
#include "iAFiberData.h"     // for samplePoints
#include "iAJobListView.h"
#include "iARefDistCompute.h"

// FeatureScout:
#include "iACsvConfig.h"
#include "iAFeatureScoutModuleInterface.h"
#include "iAVectorPlotData.h"

// Core:
#include "charts/iAChartWidget.h"
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
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkGenericOpenGLRenderWindow.h>
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
#include <QHBoxLayout>
#include <QMessageBox>
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
	const int DefaultPlayDelay = 1000;
	QColor OptimStepMarkerColor(192, 0, 0);

	int SelectionOpacity = iA3DLineObjectVis::DefaultSelectionOpacity;
	int ContextOpacity = iA3DLineObjectVis::DefaultContextOpacity;
	const size_t NoPlotsIdx = std::numeric_limits<size_t>::max();
	const QString ModuleSettingsKey("FiberOptimizationExplorer");

	QColor ProjectionErrorDefaultPlotColor(128, 128, 128, SelectionOpacity);
	QColor SPLOMSelectionColor(255, 0, 0, ContextOpacity);

	int NoResult = NoPlotsIdx;
}

//! UI elements for each result
class iAFiberCharUIData
{
public:
	iAVtkWidgetClass* vtkWidget;
	QSharedPointer<iA3DCylinderObjectVis> mini3DVis;
	QSharedPointer<iA3DCylinderObjectVis> main3DVis;
	QCheckBox* cbBoundingBox;
	//! index where the plots for this result start
	size_t startPlotIdx;
};

iAFiberOptimizationExplorer::iAFiberOptimizationExplorer(MainWindow* mainWnd) :
	m_colorTheme(iAColorThemeManager::GetInstance().GetTheme("Brewer Accent (max. 8)")),
	m_mainWnd(mainWnd),
	m_splom(new iAQSplom()),
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

void iAFiberOptimizationExplorer::start(QString const & path, QString const & configName)
{
	m_configName = configName;
	m_jobs = new iAJobListView();
	m_jobDockWidget = new iADockWidgetWrapper(m_jobs, "Jobs", "foeJobs");
	addDockWidget(Qt::BottomDockWidgetArea, m_jobDockWidget);

	m_results = QSharedPointer<iAFiberResultsCollection>(new iAFiberResultsCollection());
	auto resultsLoader = new iAFiberResultsLoader(m_results, path, configName);
	connect(resultsLoader, SIGNAL(finished()), this, SLOT(resultsLoaded()));
	connect(resultsLoader, SIGNAL(failed(QString const &)), this, SLOT(resultsLoadFailed(QString const &)));
	m_jobs->addJob("Loading results...", resultsLoader->progress(), resultsLoader);
	resultsLoader->start();
}

void iAFiberOptimizationExplorer::resultsLoadFailed(QString const & path)
{
	QMessageBox::warning(m_mainWnd, "Fiber Analytics",
		QString("Could not load data in folder '%1'. Make sure it is in the right format!").arg(path));
	delete parent(); // deletes QMdiSubWindow which this widget is child of
	return;
}

void iAFiberOptimizationExplorer::resultsLoaded()
{
	m_resultUIs.resize(m_results->results.size());
	m_selection.resize(m_results->results.size());


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
	m_cmbboxDistanceMeasure->addItem("Dist3 (all 9 pairs Start-/Center-/Endpoint)");
	m_cmbboxDistanceMeasure->addItem("Dist4 (Overlap % in relation to Volume Ratio)");
	m_cmbboxDistanceMeasure->setCurrentIndex(1);
	connect(m_chkboxShowReference, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::showReferenceToggled);
	connect(m_spnboxReferenceCount, SIGNAL(valueChanged(int)), this, SLOT(showReferenceCountChanged(int)));
	connect(m_cmbboxDistanceMeasure, SIGNAL(currentIndexChanged(int)), this, SLOT(showReferenceMeasureChanged(int)));
	showReferenceWidget->layout()->addWidget(m_chkboxShowReference);
	showReferenceWidget->layout()->addWidget(m_spnboxReferenceCount);
	showReferenceWidget->layout()->addWidget(new QLabel("nearest ref. fibers, distance metric:"));
	showReferenceWidget->layout()->addWidget(m_cmbboxDistanceMeasure);
	showReferenceWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	m_defaultOpacitySlider = new QSlider(Qt::Horizontal);
	m_defaultOpacitySlider->setMinimum(0);
	m_defaultOpacitySlider->setMaximum(255);
	m_defaultOpacitySlider->setValue(SelectionOpacity);
	connect(m_defaultOpacitySlider, &QSlider::valueChanged, this, &iAFiberOptimizationExplorer::mainOpacityChanged);
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
	connect(m_contextOpacitySlider, &QSlider::valueChanged, this, &iAFiberOptimizationExplorer::contextOpacityChanged);
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
	connect(showSampledCylinder, &QPushButton::pressed, this, &iAFiberOptimizationExplorer::visualizeCylinderSamplePoints);
	moreButtons->layout()->addWidget(showSampledCylinder);
	moreButtons->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QWidget* mainRendererContainer = new QWidget();
	mainRendererContainer->setLayout(new QVBoxLayout());
	mainRendererContainer->layout()->addWidget(m_mainRenderer);
	mainRendererContainer->layout()->addWidget(showReferenceWidget);
	mainRendererContainer->layout()->addWidget(defaultOpacityWidget);
	mainRendererContainer->layout()->addWidget(contextOpacityWidget);
	mainRendererContainer->layout()->addWidget(moreButtons);

	m_style = vtkSmartPointer<iASelectionInteractorStyle>::New();
	m_style->setSelectionProvider(this);
	m_style->assignToRenderWindow(renWin);
	connect(m_style.GetPointer(), &iASelectionInteractorStyle::selectionChanged, this, &iAFiberOptimizationExplorer::selection3DChanged);


	// Optimization Steps View:

	m_optimStepChart.resize(iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + 1);

	auto dataChooser = new QWidget();
	dataChooser->setLayout(new QVBoxLayout());
	ChartCount = iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + 1;
	m_chartCB.resize(ChartCount);
	for (int chartID = 0; chartID < ChartCount; ++chartID)
	{
		m_chartCB[chartID] = new QCheckBox(diffName(chartID));
		m_chartCB[chartID]->setChecked(chartID == ChartCount-1);
		m_chartCB[chartID]->setEnabled(chartID == ChartCount-1);
		m_chartCB[chartID]->setProperty("chartID", chartID);
		connect(m_chartCB[chartID], &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::optimDataToggled);
		dataChooser->layout()->addWidget(m_chartCB[chartID]);
	}
	size_t curPlotStart = 0;
	for (int resultID=0; resultID<m_results->results.size(); ++resultID)
	{
		auto & d = m_results->results[resultID];
		if (!d.projectionError.empty())
		{
			m_resultUIs[resultID].startPlotIdx = curPlotStart;
			curPlotStart += d.fiberCount;
		}
		else
			m_resultUIs[resultID].startPlotIdx = NoPlotsIdx;
	}

	QWidget* playControls = new QWidget();
	playControls->setLayout(new QHBoxLayout());
	QPushButton* playPauseButton = new QPushButton("Play");
	QSpinBox* stepDelayInput = new QSpinBox();
	stepDelayInput->setMinimum(100);
	stepDelayInput->setMaximum(10000);
	stepDelayInput->setSingleStep(100);
	stepDelayInput->setValue(DefaultPlayDelay);
	playControls->layout()->addWidget(new QLabel("Delay (ms)"));
	playControls->layout()->addWidget(stepDelayInput);
	playControls->layout()->addWidget(playPauseButton);
	playControls->layout()->addWidget(dataChooser);
	m_playTimer->setInterval(DefaultPlayDelay);
	connect(m_playTimer, &QTimer::timeout, this, &iAFiberOptimizationExplorer::playTimer);
	connect(playPauseButton, &QPushButton::pressed, this, &iAFiberOptimizationExplorer::playPauseOptimSteps);
	connect(stepDelayInput, SIGNAL(valueChanged(int)), this, SLOT(playDelayChanged(int)));
	playControls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	auto chartContainer = new QWidget();
	m_optimChartLayout = new QVBoxLayout();
	chartContainer->setLayout(m_optimChartLayout);
	chartContainer->setSizeIncrement(QSizePolicy::Expanding, QSizePolicy::Expanding);
	auto plotPlusControls = new QWidget();
	plotPlusControls->setLayout(new QVBoxLayout());
	m_optimStepSlider = new QSlider(Qt::Horizontal);
	m_optimStepSlider->setMinimum(0);
	m_optimStepSlider->setMaximum(m_results->optimStepMax - 1);
	m_optimStepSlider->setValue(m_results->optimStepMax - 1);
	m_currentOptimStepLabel = new QLabel("");
	m_currentOptimStepLabel->setText(QString::number(m_results->optimStepMax - 1));
	connect(m_optimStepSlider, &QSlider::valueChanged, this, &iAFiberOptimizationExplorer::optimStepSliderChanged);
	QWidget* optimStepsCtrls = new QWidget();
	optimStepsCtrls->setLayout(new QHBoxLayout());
	optimStepsCtrls->layout()->addWidget(m_optimStepSlider);
	optimStepsCtrls->layout()->addWidget(m_currentOptimStepLabel);
	optimStepsCtrls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	plotPlusControls->layout()->addWidget(chartContainer);
	plotPlusControls->layout()->addWidget(optimStepsCtrls);
	plotPlusControls->layout()->addWidget(playControls);

	QWidget* optimStepsView = new QWidget();
	optimStepsView->setLayout(new QHBoxLayout());
	optimStepsView->layout()->addWidget(plotPlusControls);
	optimStepsView->layout()->addWidget(dataChooser);

	// Results List View
	m_defaultButtonGroup = new QButtonGroup();
	QGridLayout* resultsListLayout = new QGridLayout();
	for (int resultID=0; resultID<m_results->results.size(); ++resultID)
	{
		auto & d = m_results->results.at(resultID);
		auto & uiData = m_resultUIs[resultID];

		uiData.vtkWidget  = new iAVtkWidgetClass();
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		renWin->SetAlphaBitPlanes(1);
		auto ren = vtkSmartPointer<vtkRenderer>::New();
		m_renderManager->addToBundle(ren);
		ren->SetBackground(1.0, 1.0, 1.0);
		ren->SetUseDepthPeeling(true);
		ren->SetMaximumNumberOfPeels(1000);
		renWin->AddRenderer(ren);
		uiData.vtkWidget->SetRenderWindow(renWin);
		uiData.vtkWidget->setProperty("resultID", resultID);

		QCheckBox* toggleMainRender = new QCheckBox(QFileInfo(d.fileName).baseName());
		toggleMainRender->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		toggleMainRender->setProperty("resultID", resultID);
		uiData.cbBoundingBox = new QCheckBox("Box");
		uiData.cbBoundingBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		uiData.cbBoundingBox->setProperty("resultID", resultID);
		QRadioButton* toggleReference = new QRadioButton("");
		toggleReference->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		toggleReference->setProperty("resultID", resultID);
		m_defaultButtonGroup->addButton(toggleReference);
		resultsListLayout->addWidget(toggleMainRender, resultID, 0);
		resultsListLayout->addWidget(uiData.cbBoundingBox, resultID, 1);
		resultsListLayout->addWidget(toggleReference, resultID, 2);
		resultsListLayout->addWidget(uiData.vtkWidget, resultID, 3);

		uiData.mini3DVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(
			uiData.vtkWidget, d.table, d.mapping, getResultColor(resultID)));
		uiData.main3DVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(
			m_mainRenderer, d.table, d.mapping, getResultColor(resultID)));
		uiData.mini3DVis->setColor(getResultColor(resultID));
		uiData.mini3DVis->show();
		ren->ResetCamera();

		connect(uiData.vtkWidget, &iAVtkWidgetClass::mouseEvent, this, &iAFiberOptimizationExplorer::miniMouseEvent);
		connect(toggleMainRender, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::toggleVis);
		connect(toggleReference, &QRadioButton::toggled, this, &iAFiberOptimizationExplorer::referenceToggled);
		connect(uiData.cbBoundingBox, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::toggleBoundingBox);
	}
	QWidget* resultList = new QWidget();
	resultList->setLayout(resultsListLayout);

	// Interaction Protocol:
	//QWidget* protocolWidget = new QWidget();
	m_interactionProtocolView = new QTreeView();
	m_interactionProtocolModel = new QStandardItemModel();
	//protocolWidget->setLayout(new QHBoxLayout());
	m_interactionProtocolView->setModel(m_interactionProtocolModel);

	// List Overview / LineUp View:
//	m_browser = new QWebEngineView();
//	iADockWidgetWrapper* browserWidget = new iADockWidgetWrapper(m_browser, "LineUp", "foeLineUp");

	iADockWidgetWrapper* resultListDockWidget = new iADockWidgetWrapper(resultList, "Result list", "foeResultList");
	iADockWidgetWrapper* main3DView = new iADockWidgetWrapper(mainRendererContainer, "3D view", "foe3DView");
	iADockWidgetWrapper* optimStepWidget = new iADockWidgetWrapper(optimStepsView, "Optimization Steps", "foeTimeSteps");
	iADockWidgetWrapper* splomWidget = new iADockWidgetWrapper(m_splom, "Scatter Plot Matrix", "foeSPLOM");
	iADockWidgetWrapper* interactionProtocolWidget = new iADockWidgetWrapper(m_interactionProtocolView, "Interaction Protocol", "foeInteractions");

	splitDockWidget(m_jobDockWidget, resultListDockWidget, Qt::Vertical);
	splitDockWidget(resultListDockWidget, main3DView, Qt::Horizontal);
	splitDockWidget(resultListDockWidget, optimStepWidget, Qt::Vertical);
	splitDockWidget(main3DView, splomWidget, Qt::Vertical);
//	splitDockWidget(resultListDockWidget, browserWidget, Qt::Vertical);
	splitDockWidget(resultListDockWidget, interactionProtocolWidget, Qt::Vertical);

	loadStateAndShow();
}

iAFiberOptimizationExplorer::~iAFiberOptimizationExplorer()
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

void iAFiberOptimizationExplorer::loadStateAndShow()
{
	addInteraction(QString("Loaded %1 results in folder %2").arg(m_results->results.size()).arg(m_results->folder));
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

	// splom needs an active OpenGL Context (it must be visible when setData is called):
	m_splom->setMinimumWidth(200);
	m_splom->setSelectionColor(SPLOMSelectionColor);
	m_splom->showAllPlots(false);
	auto np = m_results->splomData->numParams();
	std::vector<char> v(m_results->splomData->numParams(), false);
	auto & map = *m_results->results[0].mapping.data();
	v[map[iACsvConfig::StartX]] = v[map[iACsvConfig::StartY]] = v[map[iACsvConfig::StartZ]]
		= v[np - 7] = v[np - 6] = v[np - 5] = v[np - 4] = v[np - 3] = v[np - 2] = true;
	m_splom->setData(m_results->splomData, v);
	iALookupTable lut;
	int numOfResults = m_results->results.size();
	lut.setRange(0, numOfResults - 1);
	lut.allocate(numOfResults);
	for (size_t i = 0; i < numOfResults; i++)
		lut.setColor(i, m_colorTheme->GetColor(i));
	m_splom->setLookupTable(lut, m_results->splomData->numParams() - 1);
	m_splom->setSelectionMode(iAScatterPlot::Rectangle);
	m_splom->showDefaultMaxizimedPlot();
	m_splom->setSelectionColor("black");
	m_splom->setPointRadius(2.5);
	m_splom->settings.enableColorSettings = true;
	connect(m_splom, &iAQSplom::selectionModified, this, &iAFiberOptimizationExplorer::selectionSPLOMChanged);
	connect(m_splom, &iAQSplom::lookupTableChanged, this, &iAFiberOptimizationExplorer::splomLookupTableChanged);
}

QColor iAFiberOptimizationExplorer::getResultColor(int resultID)
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

void iAFiberOptimizationExplorer::toggleOptimStepChart(int chartID, bool visible)
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
		m_optimStepChart[chartID]->addXMarker(m_results->optimStepMax -1, OptimStepMarkerColor);
		for (int resultID=0; resultID<m_results->results.size(); ++resultID)
		{
			auto & d = m_results->results[resultID];
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
				this, &iAFiberOptimizationExplorer::selectionOptimStepChartChanged);
	}
	m_optimStepChart[chartID]->setVisible(true);
	m_optimStepChart[chartID]->clearMarkers();
	m_optimStepChart[chartID]->addXMarker(m_optimStepSlider->value(), OptimStepMarkerColor);

	bool allVisible = noResultSelected(m_resultUIs);
	for (size_t resultID=0; resultID<m_results->results.size(); ++resultID)
	{
		if (m_resultUIs[resultID].startPlotIdx == NoPlotsIdx)
			continue;
		for (size_t p = 0; p < m_results->results[resultID].fiberCount; ++p)
			m_optimStepChart[chartID]->plots()[m_resultUIs[resultID].startPlotIdx+p]
					->setVisible(allVisible || resultSelected(m_resultUIs, resultID));
	}
	m_optimStepChart[chartID]->update();
	showCurrentSelectionInPlot(chartID);
}

void iAFiberOptimizationExplorer::addInteraction(QString const & interaction)
{
	m_interactionProtocolModel->invisibleRootItem()->appendRow(new QStandardItem(interaction));
}

void iAFiberOptimizationExplorer::toggleVis(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	addInteraction(QString("Toggle visibility of result %1").arg(resultID));
	iAFiberCharData & data = m_results->results[resultID];
	iAFiberCharUIData & ui = m_resultUIs[resultID];
	if (state == Qt::Checked)
	{
		ui.main3DVis->setSelectionOpacity(SelectionOpacity);
		ui.main3DVis->setContextOpacity(ContextOpacity);
		if (m_splom->colorScheme() == iAQSplom::ByParameter)
		{
			ui.main3DVis->setLookupTable(m_splom->lookupTable(), m_splom->colorLookupParam());
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
		m_splom->addFilter( m_results->splomData->numParams()-1, resultID);
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
		m_splom->removeFilter( m_results->splomData->numParams()-1, resultID);
	}
	for (size_t c=0; c<ChartCount; ++c)
		if (m_optimStepChart[c] && m_optimStepChart[c]->isVisible())
			m_optimStepChart[c]->update();
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}

void iAFiberOptimizationExplorer::toggleBoundingBox(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	addInteraction(QString("Toggle bounding box of result %1").arg(resultID));
	auto & ui = m_resultUIs[resultID];
	if (state == Qt::Checked)
		ui.main3DVis->showBoundingBox();
	else
		ui.main3DVis->hideBoundingBox();
}

void iAFiberOptimizationExplorer::getResultFiberIDFromSplomID(size_t splomID, size_t & resultID, size_t & fiberID)
{
	size_t curStart = 0;
	resultID = 0;
	fiberID = 0;
	while (splomID >= curStart + m_results->results[resultID].fiberCount && resultID < m_results->results.size())
	{
		curStart += m_results->results[resultID].fiberCount;
		++resultID;
	}
	if (resultID == m_results->results.size())
	{
		DEBUG_LOG(QString("Invalid index in SPLOM: %1").arg(splomID));
		return;
	}
	fiberID = splomID - curStart;
}

std::vector<std::vector<size_t> > & iAFiberOptimizationExplorer::selection()
{
	return m_selection;
}

void iAFiberOptimizationExplorer::clearSelection()
{
	for (size_t resultID=0; resultID<m_selection.size(); ++resultID)
		m_selection[resultID].clear();
}

void iAFiberOptimizationExplorer::sortCurrentSelection()
{
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
		std::sort(m_selection[resultID].begin(), m_selection[resultID].end());
}

void iAFiberOptimizationExplorer::showCurrentSelectionInPlots()
{
	for (size_t chartID=0; chartID<ChartCount; ++chartID)
		showCurrentSelectionInPlot(chartID);
}

void iAFiberOptimizationExplorer::showCurrentSelectionInPlot(int chartID)
{
	bool anythingSelected = isAnythingSelected();
	auto chart = m_optimStepChart[chartID];
	if (!chart || !chart->isVisible())
		return;
	for (size_t resultID = 0; resultID < m_results->results.size(); ++resultID)
	{
		if (m_resultUIs[resultID].startPlotIdx != NoPlotsIdx)
		{
			size_t curSelIdx = 0;
			QColor color(getResultColor(resultID));
			for (size_t fiberID=0; fiberID < m_results->results[resultID].fiberCount; ++fiberID)
			{
				if (curSelIdx < m_selection[resultID].size() && fiberID == m_selection[resultID][curSelIdx])
				{
					color.setAlpha(SelectionOpacity);
					++curSelIdx;
				}
				else if (anythingSelected)
				{
					color.setAlpha(ContextOpacity);
				}
				auto plot = chart->plots()[m_resultUIs[resultID].startPlotIdx + fiberID];
				plot->setColor(color);
			}
		}
	}
	chart->update();
}

bool iAFiberOptimizationExplorer::isAnythingSelected() const
{
	for (size_t resultID = 0; resultID < m_results->results.size(); ++resultID)
		if (m_selection[resultID].size() > 0)
			return true;
	return false;
}

void iAFiberOptimizationExplorer::showCurrentSelectionIn3DViews()
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

void iAFiberOptimizationExplorer::showCurrentSelectionInSPLOM()
{
	size_t splomSelectionSize = 0;
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
		splomSelectionSize += m_selection[resultID].size();
	std::vector<size_t> splomSelection;
	splomSelection.reserve(splomSelectionSize);
	size_t splomIDStart = 0;
	for (size_t resultID = 0; resultID<m_results->results.size(); ++resultID)
	{
		for (int fiberID = 0; fiberID < m_selection[resultID].size(); ++fiberID)
		{
			size_t splomID = splomIDStart + m_selection[resultID][fiberID];
			splomSelection.push_back(splomID);
		}
		splomIDStart += m_results->results[resultID].fiberCount;
	}
	m_splom->setSelection(splomSelection);
}

void iAFiberOptimizationExplorer::selection3DChanged()
{
	size_t selSize = 0;
	for (size_t resultID = 0; resultID < m_selection.size(); ++resultID)
		selSize += m_selection[resultID].size();
	addInteraction(QString("Selected %1 fibers in 3D view.").arg(selSize));
	sortCurrentSelection();
	showCurrentSelectionIn3DViews();
	showCurrentSelectionInPlots();
	showCurrentSelectionInSPLOM();
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::selectionSPLOMChanged(std::vector<size_t> const & selection)
{
	addInteraction(QString("Selected %1 fibers in scatter plot matrix.").arg(selection.size()));
	// map from SPLOM index to (resultID, fiberID) pairs
	clearSelection();
	size_t resultID, fiberID;
	for (size_t splomID: selection)
	{
		getResultFiberIDFromSplomID(splomID, resultID, fiberID);
		m_selection[resultID].push_back(fiberID);
	}
	sortCurrentSelection();
	showCurrentSelectionIn3DViews();
	showCurrentSelectionInPlots();
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::selectionOptimStepChartChanged(std::vector<size_t> const & selection)
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
				   (m_resultUIs[resultID].startPlotIdx + m_results->results[resultID].fiberCount) )
			{
				size_t inResultFiberIdx = selection[curSelectionIndex] - m_resultUIs[resultID].startPlotIdx;
				m_selection[resultID].push_back(inResultFiberIdx);
				++curSelectionIndex;
			}
		}
	}
	sortCurrentSelection();
	showCurrentSelectionInPlots();
	showCurrentSelectionIn3DViews();
	showCurrentSelectionInSPLOM();
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::miniMouseEvent(QMouseEvent* ev)
{
	if (ev->buttons() == Qt::RightButton && ev->type() == QEvent::MouseButtonPress)
	{
		int resultID = QObject::sender()->property("resultID").toInt();
		addInteraction(QString("Started FiberScout for result %1.").arg(resultID));
		iAFeatureScoutModuleInterface * featureScout = m_mainWnd->getModuleDispatcher().GetModule<iAFeatureScoutModuleInterface>();
		MdiChild* newChild = m_mainWnd->createMdiChild(false);
		iACsvConfig config = getCsvConfig(m_results->results[resultID].fileName, m_configName);
		featureScout->LoadFeatureScout(config, newChild);
		newChild->LoadLayout("FeatureScout");
	}
}

void iAFiberOptimizationExplorer::optimStepSliderChanged(int optimStep)
{
	addInteraction(QString("Set optimization step slider to step %1.").arg(optimStep));
	setOptimStep(optimStep);
}

void iAFiberOptimizationExplorer::setOptimStep(int optimStep)
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
		for (int resultID = 0; resultID < m_results->results.size(); ++resultID)
		{
			//m_resultData[resultID].m_mini3DVis->updateValues(m_resultData[resultID].m_timeValues[timeStep]);
			if (m_resultUIs[resultID].main3DVis->visible())
				m_resultUIs[resultID].main3DVis->updateValues(m_results->results[resultID]
					.timeValues[std::min(static_cast<size_t>(optimStep), m_results->results[resultID].timeValues.size()-1)]);
		}
	}
}

void iAFiberOptimizationExplorer::mainOpacityChanged(int opacity)
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

void iAFiberOptimizationExplorer::contextOpacityChanged(int opacity)
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
	showCurrentSelectionInPlots();
}

void iAFiberOptimizationExplorer::referenceToggled(bool)
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
	addInteraction(QString("Reference set to ID %1").arg(referenceID));
	m_refDistCompute = new iARefDistCompute(m_results->results, *m_results->splomData.data(), referenceID);
	connect(m_refDistCompute, &QThread::finished, this, &iAFiberOptimizationExplorer::refDistAvailable);
	m_jobs->addJob("Computing Reference Distances", m_refDistCompute->progress(), m_refDistCompute);
	m_refDistCompute->start();
}

void iAFiberOptimizationExplorer::refDistAvailable()
{
	size_t endOfs = iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + iARefDistCompute::EndColumns;
	std::vector<size_t> changedSplomColumns;
	for (size_t paramID = 0; paramID < iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount; ++paramID)
	{
		size_t columnID = m_results->splomData->numParams() - endOfs + paramID;
		changedSplomColumns.push_back(columnID);
	}
	m_referenceID = m_refDistCompute->referenceID();
	m_results->splomData->updateRanges(changedSplomColumns);
	m_splom->update();
	delete m_refDistCompute;
	m_refDistCompute = nullptr;

	for (size_t chartID=0; chartID<ChartCount-1; ++chartID)
		m_chartCB[chartID]->setEnabled(true);

/*
	 // include lineup following https://github.com/Caleydo/lineupjs
	m_html = "<!DOCTYPE html>\n"
	         "<html lang=\"en\">\n"
	         "  <head>\n"
	         "    <title>LineUp</title>\n"
	         "    <meta charset=\"utf-8\">\n"
	         "    <link href=\"https://unpkg.com/lineupjs/build/LineUpJS.css\" rel=\"stylesheet\">\n"
	         "    <script src=\"https://unpkg.com/lineupjs/build/LineUpJS.js\"></script>\n"
	         "    <script>\n"
		     "      const arr = [];\n";
	for (size_t resultID = 0; resultID < m_results->results.size(); ++resultID)
	{
		if (resultID == m_referenceID)
			continue;
		std::array<double, iAFiberCharData::FiberValueCount> avgError = { 0.0 };
		std::array<double, iAFiberCharData::FiberValueCount> avgDist = { 0.0 };
		auto& d = m_results->results[resultID];
		for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
		{
			size_t lastTimeStepID = d.refDiffFiber[fiberID].timeStep.size() - 1;
			for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			{
				avgError[diffID] += d.refDiffFiber[fiberID].timeStep[lastTimeStepID].diff[diffID];
			}
			for (size_t distID = 0; distID < iARefDistCompute::DistanceMetricCount; ++distID)
			{
				avgDist[distID] += d.refDiffFiber[fiberID].dist[distID][0].distance;
			}
		}
		for (size_t diffID = 0; diffID < iAFiberCharData::FiberValueCount; ++diffID)
			avgError[diffID] /= d.fiberCount;
		for (size_t distID = 0; distID < iARefDistCompute::DistanceMetricCount; ++distID)
			avgDist[distID] /= d.fiberCount;
		m_html += "      arr.push({ "
			"csv: '" + QFileInfo(d.fileName).baseName() + "', " +
			"FiberCount: " + QString::number(d.fiberCount) + ", " +
			"AvgLengthError:" + QString::number(avgError[11]) + ", " +
			"AvgPhiError:" + QString::number(avgError[9]) + ", " +
			"AvgThetaError:" + QString::number(avgError[10]) + ", ";
		for (size_t distID = 0; distID < iARefDistCompute::DistanceMetricCount; ++distID)
		{
			m_html += QString("AvgDist%1:").arg(distID) + QString::number(avgDist[distID]);
			if (distID < iARefDistCompute::DistanceMetricCount - 1)
				m_html += ", ";
		}
		m_html += "});\n";
	}
	m_html +=
		"    </script>\n"
		"  </head>\n"
		"  <body>\n"
		"    <script>\n"
		"      const lineup = LineUpJS.asLineUp(document.body, arr);\n"
		"    </script>\n"
		"  </body>\n"
		"</html>";
	DEBUG_LOG(QString("HTML: %1").arg(m_html));
	m_browser->setHtml(m_html);
	m_browser->show();
*/
}

void iAFiberOptimizationExplorer::splomLookupTableChanged()
{
	QSharedPointer<iALookupTable> lut = m_splom->lookupTable();
	size_t colorLookupParam = m_splom->colorLookupParam();
	for (size_t resultID = 0; resultID < m_resultUIs.size(); ++resultID)
	{
		m_resultUIs[resultID].mini3DVis->setLookupTable(lut, colorLookupParam);
		if (m_resultUIs[resultID].main3DVis->visible())
			m_resultUIs[resultID].main3DVis->setLookupTable(lut, colorLookupParam);
	}
}

void iAFiberOptimizationExplorer::showReferenceToggled()
{
	bool showRef = m_chkboxShowReference->isChecked();
	addInteraction(QString("Show reference fibers toggled to %1").arg(showRef?"on":"off"));
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::showReferenceCountChanged(int count)
{
	addInteraction(QString("Reference fibers count changed to %1").arg(count));
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::showReferenceMeasureChanged(int index)
{
	addInteraction(QString("Selected reference match measure #%1").arg(index));
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::changeReferenceDisplay()
{
	size_t distanceMeasure = m_cmbboxDistanceMeasure->currentIndex();
	bool showRef = m_chkboxShowReference->isChecked();
	int refCount = std::min(iARefDistCompute::MaxNumberOfCloseFibers, m_spnboxReferenceCount->value());

	if (m_nearestReferenceVis)
	{
		m_nearestReferenceVis->hide();
		m_nearestReferenceVis.clear();
	}
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
	arrID->SetName(m_results->results[m_referenceID].table->GetColumnName(0));
	m_refVisTable->AddColumn(arrID);
	// other columns (float):
	for (int col = 1; col < m_results->results[m_referenceID].table->GetNumberOfColumns() - 1; ++col)
	{
		addColumn(m_refVisTable, 0, m_results->results[m_referenceID].table->GetColumnName(col), 0);
	}

	std::vector<iAFiberDistance> referenceIDsToShow;

	double range[2];
	range[0] = std::numeric_limits<double>::max();
	range[1] = std::numeric_limits<double>::lowest();
	//DEBUG_LOG("Showing reference fibers:");
	for (size_t resultID=0; resultID < m_results->results.size(); ++resultID)
	{
		if (resultID == m_referenceID)
			continue;
		for (size_t fiberIdx = 0; fiberIdx < m_selection[resultID].size(); ++fiberIdx)
		{
			size_t fiberID = m_selection[resultID][fiberIdx];
			for (int n=0; n<refCount; ++n)
			{
				referenceIDsToShow.push_back(m_results->results[resultID].refDiffFiber[fiberID].dist[distanceMeasure][n]);
			}
		}
	}

	m_refVisTable->SetNumberOfRows(referenceIDsToShow.size());

	auto refTable = m_results->results[m_referenceID].table;
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
							m_results->results[m_referenceID].mapping, QColor(0,0,0) ) );
	QSharedPointer<iALookupTable> lut(new iALookupTable);
	*lut.data() = iALUT::Build(m_results->splomData->paramRange(m_results->splomData->numParams()-iARefDistCompute::EndColumns-iARefDistCompute::DistanceMetricCount+distanceMeasure),
		"ColorBrewer single hue 5-class oranges", 256, SelectionOpacity);
	m_nearestReferenceVis->show();
	// ... and set up color coding by it!
	m_nearestReferenceVis->setLookupTable(lut, refTable->GetNumberOfColumns()-2);
	// TODO: show distance color map somewhere!!!
}

void iAFiberOptimizationExplorer::playPauseOptimSteps()
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

void iAFiberOptimizationExplorer::playTimer()
{
	QSignalBlocker block(m_optimStepSlider);
	int newStep = (m_optimStepSlider->value() + 1) % (m_optimStepSlider->maximum() + 1);
	m_optimStepSlider->setValue(newStep);
	setOptimStep(newStep);
}

void iAFiberOptimizationExplorer::playDelayChanged(int newInterval)
{
	addInteraction(QString("Changed optimization step animation delay to %1").arg(newInterval));
	m_playTimer->setInterval(newInterval);
}

void iAFiberOptimizationExplorer::visualizeCylinderSamplePoints()
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
	addInteraction(QString("Visualized cylinder sampling for fiber %1 in result %2").arg(fiberID).arg(resultID));
	if (fiberID == NoPlotsIdx)
		return;
	if (m_sampleActor)
		m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->RemoveActor(m_sampleActor);

	auto & d = m_results->results[resultID];
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

QString iAFiberOptimizationExplorer::diffName(int chartID) const
{
	size_t splomCol = m_results->splomData->numParams() -
		(iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + iARefDistCompute::EndColumns) + chartID;
	return m_results->splomData->parameterName(splomCol);
}

void iAFiberOptimizationExplorer::optimDataToggled(int state)
{
	int chartID = QObject::sender()->property("chartID").toInt();
	addInteraction(QString("Toggled visibility of %1 vs. optimization step chart.").arg(diffName(chartID)));
	toggleOptimStepChart(chartID, state == Qt::Checked);
}
