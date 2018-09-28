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
//#include "iAPerformanceHelper.h"
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
#include <QTimer>
//#include <QWebEngineView>

#include <QtGlobal> // for QT_VERSION

#include <array>

namespace
{
	const int DefaultPlayDelay = 1000;
	QColor TimeMarkerColor(192, 0, 0);

	int SelectionOpacity = iA3DLineObjectVis::DefaultSelectionOpacity;
	int ContextOpacity = iA3DLineObjectVis::DefaultContextOpacity;
	const size_t NoPlotsIdx = std::numeric_limits<size_t>::max();
	const QString ModuleSettingsKey("FiberOptimizationExplorer");

	QColor ProjectionErrorDefaultPlotColor(128, 128, 128, SelectionOpacity);
	QColor SPLOMSelectionColor(255, 0, 0, ContextOpacity);

	int NoResult = -1;
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
	m_refDistCompute(nullptr)
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
//	iATimeGuard perfGUI("Creating GUI");
	
	QGridLayout* resultsListLayout = new QGridLayout();

	m_mainRenderer = new iAVtkWidgetClass();
	auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	auto ren = vtkSmartPointer<vtkRenderer>::New();
	ren->SetBackground(1.0, 1.0, 1.0);
	renWin->SetAlphaBitPlanes(1);
	ren->SetUseDepthPeeling(true);
	ren->SetMaximumNumberOfPeels(1000);
	renWin->AddRenderer(ren);
	m_mainRenderer->SetRenderWindow(renWin);

	m_renderManager = QSharedPointer<iARendererManager>(new iARendererManager());
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
	connect(m_chkboxShowReference, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::changeReferenceDisplay);
	connect(m_spnboxReferenceCount, SIGNAL(valueChanged(int)), this, SLOT(changeReferenceDisplay()));
	connect(m_cmbboxDistanceMeasure, SIGNAL(currentIndexChanged(int)), this, SLOT(changeReferenceDisplay()));
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

	QWidget* mainRendererContainer = new QWidget();
	mainRendererContainer->setLayout(new QVBoxLayout());
	mainRendererContainer->layout()->addWidget(m_mainRenderer);
	mainRendererContainer->layout()->addWidget(showReferenceWidget);
	mainRendererContainer->layout()->addWidget(defaultOpacityWidget);
	mainRendererContainer->layout()->addWidget(contextOpacityWidget);

	m_style = vtkSmartPointer<iASelectionInteractorStyle>::New();
	m_style->setSelectionProvider(this);
	m_style->assignToRenderWindow(renWin);
	connect(m_style.GetPointer(), &iASelectionInteractorStyle::selectionChanged, this, &iAFiberOptimizationExplorer::selection3DChanged);

	QWidget* optimizationSteps = new QWidget();
	m_timeStepChart = new iAChartWidget(optimizationSteps, "Time Step", "Projection Error");
	m_timeStepChart->setMinimumHeight(200);
	m_timeStepChart->setSelectionMode(iAChartWidget::SelectPlot);
	connect(m_timeStepChart, &iAChartWidget::plotsSelected, this, &iAFiberOptimizationExplorer::selectionTimeStepChartChanged);

	QComboBox* dataChooser = new QComboBox();

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
	connect(playPauseButton, &QPushButton::pressed, this, &iAFiberOptimizationExplorer::playPauseTimeSteps);
	connect(stepDelayInput, SIGNAL(valueChanged(int)), this, SLOT(playDelayChanged(int)));
	playControls->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	optimizationSteps->setLayout(new QVBoxLayout());
	QWidget* timeSteps = new QWidget();
	m_timeStepSlider = new QSlider(Qt::Horizontal);
	m_timeStepSlider->setMinimum(0);
	connect(m_timeStepSlider, &QSlider::valueChanged, this, &iAFiberOptimizationExplorer::timeSliderChanged);
	m_currentTimeStepLabel = new QLabel("");
	timeSteps->setLayout(new QHBoxLayout());
	timeSteps->layout()->addWidget(m_timeStepSlider);
	timeSteps->layout()->addWidget(m_currentTimeStepLabel);
	timeSteps->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	optimizationSteps->layout()->addWidget(m_timeStepChart);
	optimizationSteps->layout()->addWidget(timeSteps);
	optimizationSteps->layout()->addWidget(playControls);

//	iAPerformanceHelper perfGUIresult;
//	perfGUIresult.start("Per-result GUI initialization");

	int resultID = 0;
	m_defaultButtonGroup = new QButtonGroup();

	for (resultID=0; resultID<m_results->results.size(); ++resultID)
	{
		auto & d = m_results->results.at(resultID);
		iAFiberCharUIData uiData;

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

		if (!d.projectionError.empty())
		{
			uiData.startPlotIdx = m_timeStepChart->plots().size();

			for (size_t fiberID = 0; fiberID < d.fiberCount; ++fiberID)
			{
				QSharedPointer<iAVectorPlotData> plotData(new iAVectorPlotData(d.projectionError[fiberID]));
				plotData->setXDataType(Discrete);
				m_timeStepChart->addPlot(QSharedPointer<iALinePlot>(new iALinePlot(plotData, getResultColor(resultID))));
			}
		}
		else
			uiData.startPlotIdx = NoPlotsIdx;

		connect(uiData.vtkWidget, &iAVtkWidgetClass::mouseEvent, this, &iAFiberOptimizationExplorer::miniMouseEvent);
		connect(toggleMainRender, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::toggleVis);
		connect(toggleReference, &QRadioButton::toggled, this, &iAFiberOptimizationExplorer::referenceToggled);
		connect(uiData.cbBoundingBox, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::toggleBoundingBox);

		m_resultUIs.push_back(uiData);
	}
	m_selection.resize(resultID);
//	perfGUIresult.stop();

	for (int i = 0; i < iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + 1; ++i)
	{
		dataChooser->addItem(m_results->splomData->parameterName(m_results->splomData->numParams() -
			(iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount + iARefDistCompute::EndColumns) + i));
	}
	dataChooser->setCurrentIndex(iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount);
	connect(dataChooser, SIGNAL(currentIndexChanged(int)), this, SLOT(timeErrorDataChanged(int)));

	m_timeStepSlider->setMaximum(m_results->timeStepMax - 1);
	m_timeStepSlider->setValue(m_results->timeStepMax - 1);
	m_timeStepChart->addXMarker(m_results->timeStepMax-1, TimeMarkerColor);
	m_currentTimeStepLabel->setText(QString::number(m_results->timeStepMax - 1));

	QWidget* resultList = new QWidget();
	resultList->setLayout(resultsListLayout);

	iADockWidgetWrapper* main3DView = new iADockWidgetWrapper(mainRendererContainer, "3D view", "foe3DView");
	iADockWidgetWrapper* resultListDockWidget = new iADockWidgetWrapper(resultList, "Result list", "foeResultList");
	iADockWidgetWrapper* timeSliderWidget = new iADockWidgetWrapper(optimizationSteps, "Time Steps", "foeTimeSteps");
	iADockWidgetWrapper* splomWidget = new iADockWidgetWrapper(m_splom, "Scatter Plot Matrix", "foeSPLOM");

//	m_browser = new QWebEngineView();
//	iADockWidgetWrapper* browserWidget = new iADockWidgetWrapper(m_browser, "LineUp", "foeLineUp");

//	iAPerformanceHelper perfGUIfinal6;
//	perfGUIfinal6.start("Setting up DockWidgets...");
	
	splitDockWidget(m_jobDockWidget, resultListDockWidget, Qt::Vertical);
	splitDockWidget(resultListDockWidget, main3DView, Qt::Horizontal);
	splitDockWidget(resultListDockWidget, timeSliderWidget, Qt::Vertical);
	splitDockWidget(main3DView, splomWidget, Qt::Vertical);
//	splitDockWidget(resultListDockWidget, browserWidget, Qt::Vertical);

//	perfGUIfinal6.stop();

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
//	iAPerformanceHelper perfGUIstate;
//	perfGUIstate.start("Showing and restoring state...");
	QSettings settings;
	if (settings.value(ModuleSettingsKey + "/maximized", true).toBool())
		showMaximized();
	else
	{
		QRect newGeometry = settings.value(ModuleSettingsKey + "/geometry", geometry()).value<QRect>();
		show();
		qobject_cast<QWidget*>(parent())->setGeometry(newGeometry);
	}
	restoreState(settings.value(ModuleSettingsKey + "/state", saveState()).toByteArray());

//	perfGUIstate.stop();
//	iAPerformanceHelper perfGUIsplom;
//	perfGUIsplom.start("Initializing SPLOM...");

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

//	perfGUIsplom.stop();
}

QColor iAFiberOptimizationExplorer::getResultColor(int resultID)
{
	QColor color = m_colorTheme->GetColor(resultID);
	color.setAlpha(SelectionOpacity);
	return color;
}

namespace
{
	bool anythingElseShown(std::vector<iAFiberCharUIData> const & uiCollection, int resultID)
	{
		for (int i = 0; i < uiCollection.size(); ++i)
			if (uiCollection[i].main3DVis->visible() && resultID != i)
				return true;
		return false;
	}
}

void iAFiberOptimizationExplorer::toggleVis(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	iAFiberCharData & data = m_results->results[resultID];
	iAFiberCharUIData & ui = m_resultUIs[resultID];
	if (state == Qt::Checked)
	{
		if (!anythingElseShown(m_resultUIs, resultID))
			for (size_t p = 0; p < m_timeStepChart->plots().size(); ++p)
				m_timeStepChart->plots()[p]->setVisible(false);
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
		for (size_t p = 0; p < data.fiberCount; ++p)
			if (ui.startPlotIdx != NoPlotsIdx)
				m_timeStepChart->plots()[ui.startPlotIdx + p]->setVisible(true);

		bool anythingSelected = isAnythingSelected();
		if (anythingSelected)
			ui.main3DVis->setSelection(m_selection[resultID], anythingSelected);
		if (data.timeValues.size() > 0)
		{
			ui.main3DVis->updateValues(data.timeValues[
				std::min(data.timeValues.size() - 1, static_cast<size_t>(m_timeStepSlider->value()))]);
		}
		ui.main3DVis->show();
		m_style->addInput( resultID, ui.main3DVis->getLinePolyData() );
		m_splom->addFilter( m_results->splomData->numParams()-1, resultID);
	}
	else
	{
		if (anythingElseShown(m_resultUIs, resultID) && ui.startPlotIdx != NoPlotsIdx)
			for (size_t p = 0; p < data.fiberCount; ++p)
				m_timeStepChart->plots()[ui.startPlotIdx + p]->setVisible(false);
		else // nothing selected, show everything
			for (size_t p = 0; p < m_timeStepChart->plots().size(); ++p)
				m_timeStepChart->plots()[p]->setVisible(true);
		ui.main3DVis->hide();
		m_style->removeInput(resultID);
		m_splom->removeFilter( m_results->splomData->numParams()-1, resultID);
	}
	m_timeStepChart->update();
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}

void iAFiberOptimizationExplorer::toggleBoundingBox(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
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
	{
		m_selection[resultID].clear();
	}
}

void iAFiberOptimizationExplorer::sortCurrentSelection()
{
	for (size_t resultID = 0; resultID < m_results->results.size(); ++resultID)
	{
		std::sort(m_selection[resultID].begin(), m_selection[resultID].end());
	}
}

void iAFiberOptimizationExplorer::showCurrentSelectionInPlot()
{
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
				else if (m_selection[resultID].size() > 0)
				{
					color.setAlpha(ContextOpacity);
				}
				auto plot = m_timeStepChart->plots()[m_resultUIs[resultID].startPlotIdx + fiberID];
				plot->setColor(color);
			}
		}
	}
	m_timeStepChart->update();
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
	{
		splomSelectionSize += m_selection[resultID].size();
	}
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
	sortCurrentSelection();
	showCurrentSelectionIn3DViews();
	showCurrentSelectionInPlot();
	showCurrentSelectionInSPLOM();
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::selectionSPLOMChanged(std::vector<size_t> const & selection)
{
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
	showCurrentSelectionInPlot();
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::selectionTimeStepChartChanged(std::vector<size_t> const & selection)
{
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
	showCurrentSelectionInPlot();
	showCurrentSelectionIn3DViews();
	showCurrentSelectionInSPLOM();
	changeReferenceDisplay();
}

void iAFiberOptimizationExplorer::miniMouseEvent(QMouseEvent* ev)
{
	if (ev->buttons() == Qt::RightButton && ev->type() == QEvent::MouseButtonPress)
	{
		int resultID = QObject::sender()->property("resultID").toInt();
		iAFeatureScoutModuleInterface * featureScout = m_mainWnd->getModuleDispatcher().GetModule<iAFeatureScoutModuleInterface>();
		MdiChild* newChild = m_mainWnd->createMdiChild(false);
		iACsvConfig config = getCsvConfig(m_results->results[resultID].fileName, m_configName);
		featureScout->LoadFeatureScout(config, newChild);
		newChild->LoadLayout("FeatureScout");
	}
}

void iAFiberOptimizationExplorer::timeSliderChanged(int timeStep)
{
	m_timeStepChart->clearMarkers();
	m_timeStepChart->addXMarker(timeStep, TimeMarkerColor);
	m_timeStepChart->update();
	m_currentTimeStepLabel->setText(QString::number(timeStep));
	for (int resultID = 0; resultID < m_results->results.size(); ++resultID)
	{
		//m_resultData[resultID].m_mini3DVis->updateValues(m_resultData[resultID].m_timeValues[timeStep]);
		if (m_resultUIs[resultID].main3DVis->visible())
			m_resultUIs[resultID].main3DVis->updateValues(m_results->results[resultID]
				.timeValues[std::min(static_cast<size_t>(timeStep), m_results->results[resultID].timeValues.size()-1)]);
	}
}

void iAFiberOptimizationExplorer::mainOpacityChanged(int opacity)
{
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
	showCurrentSelectionInPlot();
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
	m_referenceID = sender->property("resultID").toULongLong();
	m_refDistCompute = new iARefDistCompute(m_results->results, *m_results->splomData.data(), m_referenceID);
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
	m_results->splomData->updateRanges(changedSplomColumns);
	m_splom->update();
	delete m_refDistCompute;
	m_refDistCompute = nullptr;
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

void iAFiberOptimizationExplorer::playPauseTimeSteps()
{
	QPushButton* btn = qobject_cast<QPushButton*>(sender());
	if (m_playTimer->isActive())
	{
		m_playTimer->stop();
		btn->setText("Play");
	}
	else
	{
		m_playTimer->start();
		btn->setText("Pause");
	}
}

void iAFiberOptimizationExplorer::playTimer()
{
	m_timeStepSlider->setValue((m_timeStepSlider->value() + 1) % (m_timeStepSlider->maximum() + 1));
	// update of 3D vis is automatically done through signal on slider change!
}

void iAFiberOptimizationExplorer::playDelayChanged(int newInterval)
{
	m_playTimer->setInterval(newInterval);
}

void iAFiberOptimizationExplorer::timeErrorDataChanged(int colIndex)
{
	if (m_referenceID == NoResult)
	{
		DEBUG_LOG("Please select a reference first!");
		return;
	}
	for (size_t resultID = 0; resultID < m_results->results.size(); ++resultID)
	{
		auto & d = m_results->results[resultID];
		auto & ui = m_resultUIs[resultID];
		if (ui.startPlotIdx == NoPlotsIdx)
			continue;
		size_t fiberCount = d.table->GetNumberOfRows();
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			auto & timeSteps = d.refDiffFiber[fiberID].timeStep;
			auto plotData = static_cast<iAVectorPlotData*>(m_timeStepChart->plots()[ui.startPlotIdx + fiberID]->data().data());
			size_t timeStepCount = std::min(std::min(timeSteps.size(), d.timeValues.size()), plotData->GetNumBin());
			for (size_t timeStep = 0; timeStep < timeStepCount; ++timeStep)
			{
				if (colIndex >= 0 && colIndex < iAFiberCharData::FiberValueCount + iARefDistCompute::DistanceMetricCount)
				{
					plotData->data()[timeStep] = timeSteps[timeStep].diff[colIndex];
				}
				else if (d.projectionError.size() > 0)
				{
					plotData->data()[timeStep] = d.projectionError[fiberID][timeStep];
				}
			}
			plotData->updateBounds();
		}
	}
	m_timeStepChart->setYCaption(m_results->splomData->parameterName(m_results->splomData->numParams()-
		(iAFiberCharData::FiberValueCount+iARefDistCompute::DistanceMetricCount+iARefDistCompute::EndColumns)
															+colIndex));
	m_timeStepChart->updateYBounds();
	m_timeStepChart->update();
}
