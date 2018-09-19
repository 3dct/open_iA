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

// FeatureScout:
#include "iA3DCylinderObjectVis.h"
#include "iACsvConfig.h"
#include "iACsvIO.h"
#include "iACsvVtkTableCreator.h"
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
#include "iAvec3.h"
#include "io/iAFileUtils.h"
#include "mainwindow.h"
#include "mdichild.h"

#include <vtkFloatArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRendererCollection.h>
#include <vtkRenderer.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>
#include <vtkVersion.h>
#include <vtkVertexGlyphFilter.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QSpinBox>
#include <QTextStream>
#include <QTimer>

#include <QtGlobal> // for QT_VERSION

#include <random>

typedef iAVec3T<double> Vec3D;

class iAFiberDistance
{
public:
	size_t index;
	double distance;
	friend bool operator<(iAFiberDistance const & a, iAFiberDistance const & b);
};
bool operator<(iAFiberDistance const & a, iAFiberDistance const & b)
{
	return a.distance < b.distance;
}

class iAResultData
{
public:
	// data:
	vtkSmartPointer<vtkTable> m_resultTable;
	QSharedPointer<QMap<uint, uint> > m_outputMapping;
	QString m_fileName;
	// timestep, fiber, 6 values (center, phi, theta, length)
	std::vector<std::vector<std::vector<double> > > m_timeValues;
	// fiber, timestep, global projection error
	std::vector<QSharedPointer<std::vector<double> > > m_projectionError;
	size_t m_startPlotIdx;
	// fiber, errors (shiftx, shifty, shiftz, phi, theta, length, diameter)
	// TODO: compute + visualize per timestep!
	std::vector<std::vector<double> > m_referenceDiff;
	// fiber, distance measure, match in reference
	std::vector<std::vector<std::vector<iAFiberDistance> > > m_referenceDist;
	size_t m_fiberCount;

	// UI elements:
	iAVtkWidgetClass* m_vtkWidget;
	QSharedPointer<iA3DCylinderObjectVis> m_mini3DVis;
	QSharedPointer<iA3DCylinderObjectVis> m_main3DVis;
	QCheckBox* m_boundingBox;
};


const QString iAFiberOptimizationExplorer::LegacyFormat("FiberOpt Legacy Format");
const QString iAFiberOptimizationExplorer::SimpleFormat("FiberOpt Simple Format");

namespace
{
	const int DistanceMetricCount = 5;
	int DifferenceCount;
	const int EndColumns = 2;
	const double CoordinateShift = 74.5;
	const int DefaultPlayDelay = 1000;
	QColor TimeMarkerColor(192, 0, 0);

	iACsvConfig getLegacyConfig()
	{
		iACsvConfig config = iACsvConfig::getLegacyFiberFormat("");
		config.skipLinesStart = 0;
		config.containsHeader = false;
		config.visType = iACsvConfig::Cylinders;
		return config;
	}

	iACsvConfig getSimpleConfig()
	{
		iACsvConfig config;
		config.encoding = "System";
		config.skipLinesStart = 0;
		config.skipLinesEnd = 0;
		config.containsHeader = false;
		config.columnSeparator = ",";
		config.decimalSeparator = ".";
		config.addAutoID = false;
		config.objectType = iAFeatureScoutObjectType::Fibers;
		config.computeLength = false;
		config.computeAngles = false;
		config.computeTensors = false;
		config.computeCenter = false;
		config.computeStartEnd = true;
		std::fill(config.offset, config.offset + 3, CoordinateShift);
		config.visType = iACsvConfig::Cylinders;
		config.currentHeaders = QStringList() <<
			"ID" << "CenterX" << "CenterY" << "CenterZ" << "Phi" << "Theta" << "Length";
		config.selectedHeaders = config.currentHeaders;
		config.columnMapping.clear();
		config.columnMapping.insert(iACsvConfig::CenterX, 1);
		config.columnMapping.insert(iACsvConfig::CenterY, 2);
		config.columnMapping.insert(iACsvConfig::CenterZ, 3);
		config.columnMapping.insert(iACsvConfig::Phi, 4);
		config.columnMapping.insert(iACsvConfig::Theta, 5);
		config.columnMapping.insert(iACsvConfig::Length, 6);
		config.visType = iACsvConfig::Cylinders;
		config.isDiameterFixed = true;
		config.fixedDiameterValue = 7;
		return config;
	}

	iACsvConfig getCsvConfig(QString const & csvFile, QString const & formatName)
	{
		iACsvConfig result;
		QSettings settings;
		if (!result.load(settings, formatName))
		{
			if (formatName == iACsvConfig::LegacyFiberFormat)
				result = iACsvConfig::getLegacyFiberFormat(csvFile);
			else if (formatName == iACsvConfig::LegacyVoidFormat)
				result = iACsvConfig::getLegacyPoreFormat(csvFile);
			else if (formatName == iAFiberOptimizationExplorer::LegacyFormat)
				result = getLegacyConfig();
			else if (formatName == iAFiberOptimizationExplorer::SimpleFormat)
				result = getSimpleConfig();
			else
				DEBUG_LOG(QString("Invalid format %1!").arg(formatName));
		}
		result.fileName = csvFile;
		return result;
	}

	int SelectionOpacity = iA3DLineObjectVis::DefaultSelectionOpacity;
	int ContextOpacity = iA3DLineObjectVis::DefaultContextOpacity;
	const size_t NoPlotsIdx = std::numeric_limits<size_t>::max();
	const QString ModuleSettingsKey("FiberOptimizationExplorer");

	QColor ProjectionErrorDefaultPlotColor(128, 128, 128, SelectionOpacity);
	QColor SPLOMSelectionColor(255, 0, 0, ContextOpacity);

	int MaxNumberOfCloseFibers = 25;

	int NoResult = -1;

	void addColumn(vtkSmartPointer<vtkTable> table, float value, char const * columnName, size_t numRows)
	{
		vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
		arrX->SetName(columnName);
		arrX->SetNumberOfValues(numRows);
#if (VTK_MAJOR_VERSION >= 8)
		arrX->Fill(value);
#else
		for (vtkIdType i=0; i<numRows; ++i)
		{
			arrX->SetValue(i, value);
		}
#endif
		table->AddColumn(arrX);
	}
}

iAFiberOptimizationExplorer::iAFiberOptimizationExplorer(MainWindow* mainWnd) :
	m_colorTheme(iAColorThemeManager::GetInstance().GetTheme("Brewer Accent (max. 8)")),
	m_mainWnd(mainWnd),
	m_timeStepMax(1),
	m_splomData(new iASPLOMData()),
	m_splom(new iAQSplom()),
	m_referenceID(NoResult),
	m_playTimer(new QTimer(this))
{
	setDockOptions(AllowNestedDocks | AllowTabbedDocks);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif
	setMinimumSize(600, 400);
	setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
}

bool iAFiberOptimizationExplorer::load(QString const & path, QString const & configName)
{
	//QVBoxLayout* mainLayout = new QVBoxLayout();
	//setLayout(mainLayout);
	//QScrollArea* scrollArea = new QScrollArea();
	//mainLayout->addWidget(scrollArea);
	//scrollArea->setWidgetResizable(true);
	//QWidget* resultsListWidget = new QWidget();
	//scrollArea->setWidget(resultsListWidget);
	m_configName = configName;
	QGridLayout* resultsListLayout = new QGridLayout();

	QStringList filters;
	filters << "*.csv";
	QStringList csvFileNames;
	FindFiles(path, filters, false, csvFileNames, Files);

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
	m_spnboxReferenceCount->setMaximum(MaxNumberOfCloseFibers);
	showReferenceWidget->setLayout(new QHBoxLayout());
	m_cmbboxDistanceMeasure = new QComboBox();
	m_cmbboxDistanceMeasure->addItem("Dist1 (Midpoint, Angles, Length)");
	m_cmbboxDistanceMeasure->addItem("Dist2 (Start-Start/Center-Center/End-End)");
	m_cmbboxDistanceMeasure->addItem("Dist3 (all 9 pairs Start-/Center-/Endpoint)");
	m_cmbboxDistanceMeasure->addItem("Dist4 (Overlap %)");
	m_cmbboxDistanceMeasure->addItem("Dist5 (Overlap % in relation to Volume Ratio)");
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

	int resultID = 0;
	m_defaultButtonGroup = new QButtonGroup();

	const int MaxDatasetCount = 25;
	if (csvFileNames.size() > MaxDatasetCount)
	{
		DEBUG_LOG(QString("The specified folder %1 contains %2 datasets; currently we only support loading up to %3 datasets!")
			.arg(path).arg(csvFileNames.size()).arg(MaxDatasetCount));
		return false;
	}
	for (QString csvFile : csvFileNames)
	{
		iACsvConfig config = getCsvConfig(csvFile, configName);

		iACsvIO io;
		iACsvVtkTableCreator tableCreator;
		if (!io.loadCSV(tableCreator, config))
		{
			DEBUG_LOG(QString("Could not load file '%1' - probably it's in a wrong format; skipping!").arg(csvFile));
			continue;
		}

		vtkIdType numColumns = tableCreator.getTable()->GetNumberOfColumns();
		if (resultID == 0)
		{
			std::vector<QString> paramNames;
			for (QString s: io.getOutputHeaders())
				paramNames.push_back(s);
			paramNames.push_back("StartXShift");
			paramNames.push_back("StartYShift");
			paramNames.push_back("StartZShift");
			paramNames.push_back("EndXShift");
			paramNames.push_back("EndYShift");
			paramNames.push_back("EndZShift");
			paramNames.push_back("XmShift");
			paramNames.push_back("YmShift");
			paramNames.push_back("ZmShift");
			paramNames.push_back("PhiDiff");
			paramNames.push_back("ThetaDiff");
			paramNames.push_back("LengthDiff");
			paramNames.push_back("DiameterDiff");
			DifferenceCount = paramNames.size() - numColumns;
			for (int i=0; i<DistanceMetricCount; ++i)
			{
				paramNames.push_back(QString("MinDist%1").arg(i+1));
			}
			paramNames.push_back("ProjectionErrorReduction");
			paramNames.push_back("Result_ID");
			m_splomData->setParameterNames(paramNames);
		}
		// TODO: Check if output mapping is the same (it must be)!
		vtkIdType numFibers = tableCreator.getTable()->GetNumberOfRows();
		if (numFibers < MaxNumberOfCloseFibers)
			MaxNumberOfCloseFibers = numFibers;
		// TOOD: simplify - load all tables beforehand, then allocate splom data fully and then fill it?
		for (int i = (DistanceMetricCount+DifferenceCount+EndColumns); i >= EndColumns; --i)
		{
			m_splomData->data()[m_splomData->numParams() - i].resize(m_splomData->data()[m_splomData->numParams() - i].size() + numFibers, 0);
		}
		for (vtkIdType row = 0; row < numFibers; ++row)
		{
			for (vtkIdType col = 0; col < numColumns; ++col)
			{
				double value = tableCreator.getTable()->GetValue(row, col).ToDouble();
				m_splomData->data()[col].push_back(value);
			}
			m_splomData->data()[m_splomData->numParams()-1].push_back(resultID);
		}
		// TODO: reuse splomData also for 3d visualization?
		for (int col = 0; col < (DistanceMetricCount+DifferenceCount+EndColumns-1); ++col)
		{
			addColumn(tableCreator.getTable(), 0, m_splomData->parameterName(numColumns+col).toStdString().c_str(), numFibers);
		}
		addColumn(tableCreator.getTable(), resultID, m_splomData->parameterName(m_splomData->numParams()-1).toStdString().c_str(), numFibers);
		
		iAResultData resultData;
		resultData.m_vtkWidget  = new iAVtkWidgetClass();
		resultData.m_fiberCount = numFibers;
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		renWin->SetAlphaBitPlanes(1);
		auto ren = vtkSmartPointer<vtkRenderer>::New();
		m_renderManager->addToBundle(ren);
		ren->SetBackground(1.0, 1.0, 1.0);
		ren->SetUseDepthPeeling(true);
		ren->SetMaximumNumberOfPeels(1000);
		renWin->AddRenderer(ren);
		resultData.m_vtkWidget->SetRenderWindow(renWin);
		resultData.m_vtkWidget->setProperty("resultID", resultID);

		QCheckBox* toggleMainRender = new QCheckBox(QFileInfo(csvFile).baseName());
		toggleMainRender->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		toggleMainRender->setProperty("resultID", resultID);
		resultData.m_boundingBox = new QCheckBox("Box");
		resultData.m_boundingBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		resultData.m_boundingBox->setProperty("resultID", resultID);
		QRadioButton* toggleReference = new QRadioButton("");
		toggleReference->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		toggleReference->setProperty("resultID", resultID);
		m_defaultButtonGroup->addButton(toggleReference);
		resultsListLayout->addWidget(toggleMainRender, resultID, 0);
		resultsListLayout->addWidget(resultData.m_boundingBox, resultID, 1);
		resultsListLayout->addWidget(toggleReference, resultID, 2);
		resultsListLayout->addWidget(resultData.m_vtkWidget, resultID, 3);

		resultData.m_mini3DVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(
				resultData.m_vtkWidget, tableCreator.getTable(), io.getOutputMapping(), getResultColor(resultID)));
		resultData.m_main3DVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(m_mainRenderer,
				tableCreator.getTable(), io.getOutputMapping(), getResultColor(resultID)));
		resultData.m_mini3DVis->setColor(getResultColor(resultID));
		resultData.m_mini3DVis->show();
		ren->ResetCamera();
		resultData.m_resultTable = tableCreator.getTable();
		resultData.m_outputMapping = io.getOutputMapping();
		resultData.m_fileName = csvFile;
		
		m_resultData.push_back(resultData);

		connect(resultData.m_vtkWidget, &iAVtkWidgetClass::mouseEvent, this, &iAFiberOptimizationExplorer::miniMouseEvent);
		connect(toggleMainRender, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::toggleVis);
		connect(toggleReference, &QRadioButton::toggled, this, &iAFiberOptimizationExplorer::referenceToggled);
		connect(resultData.m_boundingBox, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::toggleBoundingBox);

		QFileInfo timeInfo(QFileInfo(csvFile).absolutePath() + "/" + QFileInfo(csvFile).baseName());

		// TODO: in case reading gets inefficient, look at pre-reserving the required amount of fields
		//       and using std::vector::swap to assign the sub-vectors!

		size_t thisResultTimeStepMax = 1;
		if (timeInfo.exists() && timeInfo.isDir())
		{
			// read projection error info:
			QFile projErrorFile(timeInfo.absoluteFilePath() + "/projection_error.csv");
			if (!projErrorFile.open(QIODevice::ReadOnly | QIODevice::Text))
			{
				DEBUG_LOG(QString("Unable to open projection error file: %1").arg(projErrorFile.errorString()));
			}
			else
			{
				resultData.m_projectionError.resize(numFibers);
				QTextStream in(&projErrorFile);
				size_t fiberNr = 0;
				m_resultData[m_resultData.size() - 1].m_startPlotIdx = m_timeStepChart->plots().size();
				while (!in.atEnd())
				{
					QString line = in.readLine();
					QStringList valueStrList = line.split(",");
					if (valueStrList.size() < 2)
						continue;
					if (fiberNr >= numFibers)
					{
						DEBUG_LOG(QString("Discrepancy: More lines in %1 file than there were fibers in the fiber description csv (%2)").arg(projErrorFile.fileName()).arg(numFibers));
						break;
					}
					QSharedPointer<std::vector<double> > values (new std::vector<double>() );
					for (int i = 0; i < valueStrList.size(); ++i)
					{
						if (valueStrList[i] == "nan")
							break;
						values->push_back(valueStrList[i].toDouble());
					}
					for (int i = 0; i < values->size(); ++i)
					{
						(*values)[i] -= values->at(values->size() - 1);
					}
					resultData.m_projectionError[fiberNr] = values;
					double projErrorRed = values->at(0) - values->at(values->size() - 1);
					m_splomData->data()[m_splomData->numParams()-2][fiberNr] = projErrorRed;
					resultData.m_resultTable->SetValue(fiberNr, m_splomData->numParams() - 2, projErrorRed);
					QSharedPointer<iAVectorPlotData> plotData(new iAVectorPlotData(values));
					plotData->setXDataType(Discrete);
					m_timeStepChart->addPlot(QSharedPointer<iALinePlot>(new iALinePlot(plotData, getResultColor(resultID))));
					fiberNr++;
				}
			}

			// fiber, timestep, value
			std::vector<std::vector<std::vector<double> > > fiberTimeValues;
			int curFiber = 0;
			do
			{
				QString fiberTimeCsv = QString("fiber%1_paramlog.csv").arg(curFiber, 3, 10, QChar('0'));
				QFileInfo fiberTimeCsvInfo(timeInfo.absoluteFilePath() + "/" + fiberTimeCsv);
				if (!fiberTimeCsvInfo.exists())
					break;
				std::vector<std::vector<double> > singleFiberValues;
				QFile file(fiberTimeCsvInfo.absoluteFilePath());
				if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				{
					DEBUG_LOG(QString("Unable to open file: %1").arg(file.errorString()));
					break;
				}
				QTextStream in(&file);
				in.readLine(); // skip header line
				size_t lineNr = 1;
				while (!in.atEnd())
				{
					lineNr++;
					QString line = in.readLine();
					QStringList values = line.split(",");
					if (values.size() != 6)
					{
						DEBUG_LOG(QString("Invalid line %1 in file %2, there should be 6 entries but there are %3 (line: %4)")
							.arg(lineNr).arg(fiberTimeCsvInfo.fileName()).arg(values.size()).arg(line));
						continue;
					}
					int valIdx = 0;
					double middlePoint[3];
					for (int i = 0; i < 3; ++i)
						middlePoint[i] = values[i].toDouble() + CoordinateShift; // middle point positions are shifted!
					double theta = values[4].toDouble();
					if (theta < 0)  // theta is encoded in -Pi, Pi instead of 0..Pi as we expect
						theta = 2*vtkMath::Pi() + theta;
					double phi = values[3].toDouble();
					double radius = values[5].toDouble() * 0.5;

					std::vector<double> timeStepValues(6);
					// convert spherical to cartesian coordinates:
					double dir[3];
					dir[0] = radius * std::sin(phi) * std::cos(theta);
					dir[1] = radius * std::sin(phi) * std::sin(theta);
					dir[2] = radius * std::cos(phi);
					for (int i = 0; i<3; ++i)
					{
						timeStepValues[i] = middlePoint[i] + dir[i];
						timeStepValues[i+3] = middlePoint[i] - dir[i];
					}
					/*
					DEBUG_LOG(QString("Fiber %1, step %2: Start (%3, %4, %5) - End (%6, %7, %8)")
						.arg(curFiber)
						.arg(singleFiberValues.size())
						.arg(timeStepValues[0]).arg(timeStepValues[1]).arg(timeStepValues[2])
						.arg(timeStepValues[3]).arg(timeStepValues[4]).arg(timeStepValues[5]));
					*/
					singleFiberValues.push_back(timeStepValues);
				}
				if (singleFiberValues.size() > thisResultTimeStepMax)
				{
					thisResultTimeStepMax = singleFiberValues.size();
				}
				fiberTimeValues.push_back(singleFiberValues);
				++curFiber;
			} while (true);
			int fiberCount = curFiber;

			// transform from [fiber, timestep, value] to [timestep, fiber, value] indexing
			// TODO: make sure all datasets have the same max timestep count!
			m_resultData[m_resultData.size() - 1].m_timeValues.resize(thisResultTimeStepMax);
			for (int t = 0; t < thisResultTimeStepMax; ++t)
			{
				m_resultData[m_resultData.size() - 1].m_timeValues[t].resize(fiberCount);
				for (int f = 0; f < fiberCount; ++f)
				{
					m_resultData[m_resultData.size() - 1].m_timeValues[t][f] = (t<fiberTimeValues[f].size())?fiberTimeValues[f][t] : fiberTimeValues[f][fiberTimeValues[f].size()-1];
				}
			}
		}
		else
		{
			m_resultData[m_resultData.size() - 1].m_startPlotIdx = NoPlotsIdx;
		}
		if (thisResultTimeStepMax > m_timeStepMax)
		{
			if (m_timeStepMax > 1)
			{
				DEBUG_LOG(QString("In result %1, the maximum number of timesteps changes from %2 to %3! This shouldn't be a problem, but support for it is currently untested.")
					.arg(resultID).arg(m_timeStepMax).arg(thisResultTimeStepMax));
			}
			m_timeStepMax = thisResultTimeStepMax;
		}
		++resultID;
	}
	if (m_resultData.size() == 0)
	{
		DEBUG_LOG(QString("The specified folder %1 does not contain any valid csv files!").arg(path));
		return false;
	}
	m_splomData->updateRanges();
	m_currentSelection.resize(resultID);

	m_timeStepSlider->setMaximum(m_timeStepMax - 1);
	m_timeStepSlider->setValue(m_timeStepMax - 1);
	m_timeStepChart->addXMarker(m_timeStepMax-1, TimeMarkerColor);
	m_currentTimeStepLabel->setText(QString::number(m_timeStepMax - 1));

	QWidget* resultList = new QWidget();
	resultList->setLayout(resultsListLayout);

	iADockWidgetWrapper* main3DView = new iADockWidgetWrapper(mainRendererContainer, "3D view", "foe3DView");
	iADockWidgetWrapper* resultListDockWidget = new iADockWidgetWrapper(resultList, "Result list", "foeResultList");
	iADockWidgetWrapper* timeSliderWidget = new iADockWidgetWrapper(optimizationSteps, "Time Steps", "foeTimeSteps");
	iADockWidgetWrapper* splomWidget = new iADockWidgetWrapper(m_splom, "Scatter Plot Matrix", "foeSPLOM");

	addDockWidget(Qt::BottomDockWidgetArea, resultListDockWidget);
	splitDockWidget(resultListDockWidget, main3DView, Qt::Horizontal);
	splitDockWidget(resultListDockWidget, timeSliderWidget, Qt::Vertical);
	splitDockWidget(main3DView, splomWidget, Qt::Vertical);
	return true;
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

	// splom needs an active OpenGL Context (it must be visible when setData is called):
	m_splom->setMinimumWidth(200);
	m_splom->setSelectionColor(SPLOMSelectionColor);
	m_splom->setData(m_splomData);
	iALookupTable lut;
	int numOfResults = m_resultData.size();
	lut.setRange(0, numOfResults - 1);
	lut.allocate(numOfResults);
	for (size_t i = 0; i < numOfResults; i++)
		lut.setColor(i, m_colorTheme->GetColor(i));
	m_splom->setLookupTable(lut, m_splomData->numParams() - 1);
	m_splom->setSelectionMode(iAScatterPlot::Rectangle);
	std::vector<bool> paramVisib(m_splomData->numParams(), false);
	paramVisib[7] = paramVisib[9] = paramVisib[14] = paramVisib[15] = paramVisib[16] = paramVisib[17] = paramVisib[18] = paramVisib[m_splomData->numParams()-2] = true;
	m_splom->setParameterVisibility(paramVisib);
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
	bool anythingElseShown(std::vector<iAResultData> const & resultData, int resultID)
	{
		for (int i = 0; i < resultData.size(); ++i)
			if (resultData[i].m_main3DVis->visible() && resultID != i)
				return true;
		return false;
	}
}

void iAFiberOptimizationExplorer::toggleVis(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	iAResultData & data = m_resultData[resultID];
	if (state == Qt::Checked)
	{
		if (!anythingElseShown(m_resultData, resultID))
			for (size_t p = 0; p < m_timeStepChart->plots().size(); ++p)
				m_timeStepChart->plots()[p]->setVisible(false);
		data.m_main3DVis->setSelectionOpacity(SelectionOpacity);
		data.m_main3DVis->setContextOpacity(ContextOpacity);
		if (m_splom->colorScheme() == iAQSplom::ByParameter)
		{
			data.m_main3DVis->setLookupTable(m_splom->lookupTable(), m_splom->colorLookupParam());
			data.m_main3DVis->updateColorSelectionRendering();
		}
		else
		{
			data.m_main3DVis->setColor(getResultColor(resultID));
		}
		for (size_t p = 0; p < data.m_fiberCount; ++p)
			m_timeStepChart->plots()[data.m_startPlotIdx + p]->setVisible(true);

		bool anythingSelected = isAnythingSelected();
		if (anythingSelected)
			data.m_main3DVis->setSelection(m_currentSelection[resultID], anythingSelected);
		m_resultData[resultID].m_main3DVis->updateValues(m_resultData[resultID].m_timeValues[
			std::min(m_resultData[resultID].m_timeValues.size()-1, static_cast<size_t>(m_timeStepSlider->value()))]);
		data.m_main3DVis->show();
		m_style->addInput( resultID, data.m_main3DVis->getLinePolyData() );
		m_splom->addFilter( m_splomData->numParams()-1, resultID);
	}
	else
	{
		if (anythingElseShown(m_resultData, resultID))
			for (size_t p = 0; p < data.m_fiberCount; ++p)
				m_timeStepChart->plots()[data.m_startPlotIdx + p]->setVisible(false);
		else // nothing selected, show everything
			for (size_t p = 0; p < m_timeStepChart->plots().size(); ++p)
				m_timeStepChart->plots()[p]->setVisible(true);
		data.m_main3DVis->hide();
		m_style->removeInput(resultID);
		m_splom->removeFilter( m_splomData->numParams()-1, resultID);
	}
	m_timeStepChart->update();
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}

void iAFiberOptimizationExplorer::toggleBoundingBox(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	iAResultData & data = m_resultData[resultID];
	if (state == Qt::Checked)
		data.m_main3DVis->showBoundingBox();
	else
		data.m_main3DVis->hideBoundingBox();
}

void iAFiberOptimizationExplorer::getResultFiberIDFromSplomID(size_t splomID, size_t & resultID, size_t & fiberID)
{
	size_t curStart = 0;
	resultID = 0;
	fiberID = 0;
	while (splomID >= curStart + m_resultData[resultID].m_fiberCount && resultID < m_resultData.size())
	{
		curStart += m_resultData[resultID].m_fiberCount;
		++resultID;
	}
	if (resultID == m_resultData.size())
	{
		DEBUG_LOG(QString("Invalid index in SPLOM: %1").arg(splomID));
		return;
	}
	fiberID = splomID - curStart;
}

std::vector<std::vector<size_t> > & iAFiberOptimizationExplorer::selection()
{
	return m_currentSelection;
}

void iAFiberOptimizationExplorer::clearSelection()
{
	for (size_t resultID=0; resultID<m_currentSelection.size(); ++resultID)
	{
		m_currentSelection[resultID].clear();
	}
}

void iAFiberOptimizationExplorer::sortCurrentSelection()
{
	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		std::sort(m_currentSelection[resultID].begin(), m_currentSelection[resultID].end());
	}
}

void iAFiberOptimizationExplorer::showCurrentSelectionInPlot()
{
	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		if (m_resultData[resultID].m_startPlotIdx != NoPlotsIdx)
		{
			size_t curSelIdx = 0;
			QColor color(getResultColor(resultID));
			for (size_t fiberID=0; fiberID < m_resultData[resultID].m_fiberCount; ++fiberID)
			{
				if (curSelIdx < m_currentSelection[resultID].size() && fiberID == m_currentSelection[resultID][curSelIdx])
				{
					color.setAlpha(SelectionOpacity);
					++curSelIdx;
				}
				else if (m_currentSelection[resultID].size() > 0)
				{
					color.setAlpha(ContextOpacity);
				}
				auto plot = m_timeStepChart->plots()[m_resultData[resultID].m_startPlotIdx + fiberID];
				plot->setColor(color);
			}
		}
	}
	m_timeStepChart->update();
}

bool iAFiberOptimizationExplorer::isAnythingSelected() const
{
	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
		if (m_currentSelection[resultID].size() > 0)
			return true;
	return false;
}

void iAFiberOptimizationExplorer::showCurrentSelectionIn3DViews()
{
	bool anythingSelected = isAnythingSelected();
	for (size_t resultID = 0; resultID<m_resultData.size(); ++resultID)
	{
		auto result = m_resultData[resultID];
		result.m_mini3DVis->setSelection(m_currentSelection[resultID], anythingSelected);
		if (result.m_main3DVis->visible())
			result.m_main3DVis->setSelection(m_currentSelection[resultID], anythingSelected);

	}
}

void iAFiberOptimizationExplorer::showCurrentSelectionInSPLOM()
{
	size_t splomSelectionSize = 0;
	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		splomSelectionSize += m_currentSelection[resultID].size();
	}
	std::vector<size_t> splomSelection;
	splomSelection.reserve(splomSelectionSize);
	size_t splomIDStart = 0;
	for (size_t resultID = 0; resultID<m_resultData.size(); ++resultID)
	{
		for (int fiberID = 0; fiberID < m_currentSelection[resultID].size(); ++fiberID)
		{
			size_t splomID = splomIDStart + m_currentSelection[resultID][fiberID];
			splomSelection.push_back(splomID);
		}
		splomIDStart += m_resultData[resultID].m_fiberCount;
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
		m_currentSelection[resultID].push_back(fiberID);
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
	for (size_t resultID=0; resultID<m_resultData.size() && curSelectionIndex < selection.size(); ++resultID)
	{
		if (m_resultData[resultID].m_startPlotIdx != NoPlotsIdx)
		{
			while (curSelectionIndex < selection.size() &&
				   selection[curSelectionIndex] <
				   (m_resultData[resultID].m_startPlotIdx + m_resultData[resultID].m_resultTable->GetNumberOfRows()) )
			{
				size_t inResultFiberIdx = selection[curSelectionIndex] - m_resultData[resultID].m_startPlotIdx;
				m_currentSelection[resultID].push_back(inResultFiberIdx);
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
		iACsvConfig config = getCsvConfig(m_resultData[resultID].m_fileName, m_configName);
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
	for (int resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		//m_resultData[resultID].m_mini3DVis->updateValues(m_resultData[resultID].m_timeValues[timeStep]);
		if (m_resultData[resultID].m_main3DVis->visible())
			m_resultData[resultID].m_main3DVis->updateValues(m_resultData[resultID]
				.m_timeValues[std::min(static_cast<size_t>(timeStep), m_resultData[resultID].m_timeValues.size()-1)]);
	}
}

void iAFiberOptimizationExplorer::mainOpacityChanged(int opacity)
{
	m_defaultOpacityLabel->setText(QString::number(opacity));
	SelectionOpacity = opacity;
	for (int resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		m_resultData[resultID].m_mini3DVis->setSelectionOpacity(SelectionOpacity);
		m_resultData[resultID].m_mini3DVis->updateColorSelectionRendering();
		if (m_resultData[resultID].m_main3DVis->visible())
		{
			m_resultData[resultID].m_main3DVis->setSelectionOpacity(SelectionOpacity);
			m_resultData[resultID].m_main3DVis->updateColorSelectionRendering();
		}
	}
}

void iAFiberOptimizationExplorer::contextOpacityChanged(int opacity)
{
	m_contextOpacityLabel->setText(QString::number(opacity));
	ContextOpacity = opacity;
	for (int resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		m_resultData[resultID].m_mini3DVis->setContextOpacity(ContextOpacity);
		m_resultData[resultID].m_mini3DVis->updateColorSelectionRendering();
		if (m_resultData[resultID].m_main3DVis->visible())
		{
			m_resultData[resultID].m_main3DVis->setContextOpacity(ContextOpacity);
			m_resultData[resultID].m_main3DVis->updateColorSelectionRendering();
		}
	}
	showCurrentSelectionInPlot();
}

namespace
{
	double l2dist(double const * const pt1, double const * const pt2, int count)
	{
		double sqDistSum = 0;
		for (int i = 0; i < count; ++i)
			sqDistSum += std::pow(pt1[i] - pt2[i], 2);
		return std::sqrt(sqDistSum);
	}

	void setPoints(vtkVariantArray* fiber, QMap<uint, uint> const & mapping, double points[3][3])
	{
		points[0][0] = fiber->GetValue(mapping[iACsvConfig::StartX]).ToDouble();
		points[0][1] = fiber->GetValue(mapping[iACsvConfig::StartY]).ToDouble();
		points[0][2] = fiber->GetValue(mapping[iACsvConfig::StartZ]).ToDouble();
		points[1][0] = fiber->GetValue(mapping[iACsvConfig::CenterX]).ToDouble();
		points[1][1] = fiber->GetValue(mapping[iACsvConfig::CenterY]).ToDouble();
		points[1][2] = fiber->GetValue(mapping[iACsvConfig::CenterZ]).ToDouble();
		points[2][0] = fiber->GetValue(mapping[iACsvConfig::EndX]).ToDouble();
		points[2][1] = fiber->GetValue(mapping[iACsvConfig::EndY]).ToDouble();
		points[2][2] = fiber->GetValue(mapping[iACsvConfig::EndZ]).ToDouble();
	}

	// great points about floating point equals: https://stackoverflow.com/a/41405501/671366
	template<typename T1, typename T2>
	static bool isApproxEqual(T1 a, T2 b, T1 tolerance = std::numeric_limits<T1>::epsilon())
	{
		T1 diff = std::fabs(a - b);
		if (diff <= tolerance)
			return true;

		if (diff < std::fmax(std::fabs(a), std::fabs(b)) * tolerance)
			return true;

		return false;
	}

	Vec3D perpendicularVector(Vec3D const & vectorIn)
	{
		if (!isApproxEqual(vectorIn[0], 0.0) && !isApproxEqual(-vectorIn[0], vectorIn[1]))
			return Vec3D(vectorIn[2], vectorIn[2], -vectorIn[0] - vectorIn[1]);
		else
			return Vec3D(-vectorIn[1] - vectorIn[2], vectorIn[0], vectorIn[0]);
	}

	Vec3D fromSpherical(double phi, double theta, double radius)
	{
		return Vec3D(
			radius * std::sin(phi) * std::cos(theta),
			radius * std::sin(phi) * std::sin(theta),
			radius * std::cos(phi));
	}

	const int CylinderSamplePoints = 200;

	void samplePoints(vtkVariantArray* fiberInfo, QMap<uint, uint> const & mapping, std::vector<Vec3D > & result, size_t numSamples)
	{
		std::default_random_engine generator; // deterministic, will always produce the same "random" numbers; might be exchanged for another generator to check the spread we still get
		std::uniform_real_distribution<double> radiusRnd(0, 1);
		std::uniform_real_distribution<double> posRnd(0, 1);

		Vec3D fiberStart(fiberInfo->GetValue(mapping[iACsvConfig::StartX]).ToDouble(),
			fiberInfo->GetValue(mapping[iACsvConfig::StartY]).ToDouble(),
			fiberInfo->GetValue(mapping[iACsvConfig::StartZ]).ToDouble());
		Vec3D fiberEnd(fiberInfo->GetValue(mapping[iACsvConfig::EndX]).ToDouble(),
			fiberInfo->GetValue(mapping[iACsvConfig::EndY]).ToDouble(),
			fiberInfo->GetValue(mapping[iACsvConfig::EndZ]).ToDouble());
		Vec3D fiberDir = fiberEnd - fiberStart;
		double fiberRadius = fiberInfo->GetValue(mapping[iACsvConfig::Diameter]).ToDouble() / 2;
		/*
		DEBUG_LOG(QString("Sampling fiber (%1, %2, %3) - (%4, %5, %6), radius = %7")
			.arg(fiberStart[0]).arg(fiberStart[1]).arg(fiberStart[2])
			.arg(fiberEnd[0]).arg(fiberEnd[1]).arg(fiberEnd[2]).arg(fiberRadius));
		*/

		Vec3D perpDir = perpendicularVector(fiberDir).normalized();
		Vec3D perpDir2 = crossProduct(fiberDir, perpDir).normalized();
		std::vector<Vec3D> perpDirs;
		perpDirs.push_back(Vec3D(perpDir));
		perpDirs.push_back(Vec3D(perpDir2));
		perpDirs.push_back(-Vec3D(perpDir));
		perpDirs.push_back(-Vec3D(perpDir2));
		for (size_t a = 0; a < 4; ++a)
		{
			perpDirs.push_back((perpDirs[a] + perpDirs[(a + 1) % 4]).normalized());
		}

		std::uniform_int_distribution<int> angleRnd(0, perpDirs.size() - 1);
		/*
		DEBUG_LOG(QString("Normal Vectors: (%1, %2, %3), (%4, %5, %6)")
			.arg(perpDir[0]).arg(perpDir[1]).arg(perpDir[2])
			.arg(perpDir2[0]).arg(perpDir2[1]).arg(perpDir2[2]));
		*/
			result.resize(numSamples);

		for (int i = 0; i < numSamples; ++i)
		{
			int angleIdx = angleRnd(generator);
			double radius = fiberRadius * std::sqrt(radiusRnd(generator));
			double t = posRnd(generator);
			result[i] = fiberStart + fiberDir * t + perpDirs[angleIdx] * radius;
			//DEBUG_LOG(QString("    Sampled point: (%1, %2, %3)").arg(result[i][0]).arg(result[i][1]).arg(result[i][2]));
		}
	}

	//linePnt - point the line passes through
	//lineDir - unit vector in direction of line, either direction works
	//pnt - the point to find nearest on line for
	Vec3D nearestPointOnLine(Vec3D const & linePoint, Vec3D const & lineDir, Vec3D const & point, double & dist)
	{
		auto normLineDir = lineDir.normalized();
		auto vecToPoint = point - linePoint;
		dist = dotProduct(vecToPoint, normLineDir);
		return linePoint + normLineDir * dist;
	}

	bool pointContainedInFiber(Vec3D const & point, vtkVariantArray* fiber, QMap<uint, uint> const & mapping)
	{
		Vec3D start(fiber->GetValue(mapping[iACsvConfig::StartX]).ToDouble(),
			fiber->GetValue(mapping[iACsvConfig::StartY]).ToDouble(),
			fiber->GetValue(mapping[iACsvConfig::StartZ]).ToDouble());
		Vec3D end(fiber->GetValue(mapping[iACsvConfig::EndX]).ToDouble(),
			fiber->GetValue(mapping[iACsvConfig::EndY]).ToDouble(),
			fiber->GetValue(mapping[iACsvConfig::EndZ]).ToDouble());
		Vec3D dir = end - start;
		double dist;
		Vec3D ptOnLine = nearestPointOnLine(start, dir, point, dist);
		if (dist > 0 && dist < dir.length())  // check whether point is between start and end
		{
			double radius = fiber->GetValue(mapping[iACsvConfig::Diameter]).ToDouble() / 2.0;
			double distance = (ptOnLine - point).length();
			return distance < radius;
		}
		return false;
	}

	double getOverlap(vtkVariantArray* fiber1, QMap<uint, uint> const & mapping, vtkVariantArray* fiber2, bool normalizeByVolumeRatio)
	{
		// leave out pi in volume, as we only need relation of the volumes!
		double fiber1Vol = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble() + std::pow(fiber1->GetValue(mapping[iACsvConfig::Diameter]).ToDouble() / 2, 2);
		double fiber2Vol = fiber2->GetValue(mapping[iACsvConfig::Length]).ToDouble() + std::pow(fiber2->GetValue(mapping[iACsvConfig::Diameter]).ToDouble() / 2, 2);
		// TODO: also map fiber volume (currently not mapped!
		vtkVariantArray* shorterFiber = (fiber1Vol < fiber2Vol) ? fiber1 : fiber2;
		vtkVariantArray* longerFiber  = (fiber1Vol > fiber2Vol) ? fiber1 : fiber2;
		std::vector<Vec3D > sampledPoints;
		samplePoints(shorterFiber, mapping, sampledPoints, CylinderSamplePoints);
		size_t containedPoints = 0;
		for (Vec3D pt : sampledPoints)
		{
			if (pointContainedInFiber(pt, longerFiber, mapping))
				++containedPoints;
		}
		double distance = static_cast<double>(containedPoints) / CylinderSamplePoints;
		if (normalizeByVolumeRatio)
			distance *= (fiber1Vol < fiber2Vol) ? fiber1Vol / fiber2Vol : fiber2Vol / fiber1Vol;
		return distance;
	}

	// currently: L2 norm (euclidean distance). other measures?
	double getDistance(vtkVariantArray* fiber1, QMap<uint, uint> const & mapping, vtkVariantArray* fiber2, int distanceMeasure)
	{
		double distance = 0;
		switch(distanceMeasure)
		{
		default:
		case 0: // mid-point, angle, length
		{
			const int Dist1ValueCount = 6;
			double val1[Dist1ValueCount], val2[Dist1ValueCount];
			val1[0] = fiber1->GetValue(mapping[iACsvConfig::Phi])    .ToDouble();
			val1[1] = fiber1->GetValue(mapping[iACsvConfig::Theta])  .ToDouble();
			val1[2] = fiber1->GetValue(mapping[iACsvConfig::CenterX]).ToDouble();
			val1[3] = fiber1->GetValue(mapping[iACsvConfig::CenterY]).ToDouble();
			val1[4] = fiber1->GetValue(mapping[iACsvConfig::CenterZ]).ToDouble();
			val1[5] = fiber1->GetValue(mapping[iACsvConfig::Length]) .ToDouble();

			val2[0] = fiber2->GetValue(mapping[iACsvConfig::Phi])    .ToDouble();
			val2[1] = fiber2->GetValue(mapping[iACsvConfig::Theta])  .ToDouble();
			val2[2] = fiber2->GetValue(mapping[iACsvConfig::CenterX]).ToDouble();
			val2[3] = fiber2->GetValue(mapping[iACsvConfig::CenterY]).ToDouble();
			val2[4] = fiber2->GetValue(mapping[iACsvConfig::CenterZ]).ToDouble();
			val2[5] = fiber2->GetValue(mapping[iACsvConfig::Length]) .ToDouble();

			// TODO: opposite direction treatment! -> phi/theta reversed?
			double radius = 1; //< radius doesn't matter here, we're only interested in the direction
			Vec3D dir1 = fromSpherical(val1[0], val1[1], radius);
			Vec3D dir2 = fromSpherical(val2[0], val2[1], radius);
			double fiberAngle = angle(dir1, dir2);
			if (fiberAngle > vtkMath::Pi() / 2) // if angle larger than 90°
			{
				val1[0] += (val1[0] < vtkMath::Pi()) ? vtkMath::Pi() : -vtkMath::Pi();
				val1[1] += (val1[1] < 0) ? vtkMath::Pi()/2 : -vtkMath::Pi()/2;
				// just to check if now angles are ok...
				dir1 = fromSpherical(val1[0], val1[1], radius);
				dir2 = fromSpherical(val2[0], val2[1], radius);
				fiberAngle = angle(dir1, dir2);
				if (fiberAngle > vtkMath::Pi() / 2 ) // still larger than 90° ? Then my calculations are wrong!
				{
					DEBUG_LOG(QString("Wrong angle computation: phi1=%1, theta1=%2, phi2=%3, theta2=%4")
							  .arg(val1[0]).arg(val1[1]).arg(val2[0]).arg(val2[1]))
				}
			}

			distance = l2dist(val1, val2, Dist1ValueCount);
			break;
		}
		case 1: // start/end/center
		{
			double points1[3][3];
			double points2[3][3];
			setPoints(fiber1, mapping, points1);
			setPoints(fiber2, mapping, points2);
			double fiber1Len = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();
			double fiber2Len = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();

			double dist1StartTo2Start = l2dist(points1[0], points2[0], 3);
			double dist1StartTo2End = l2dist(points1[0], points2[2], 3);
			double dist1EndTo2Start = l2dist(points1[2], points2[0], 3);
			double dist1EndTo2End = l2dist(points1[2], points2[2], 3);

			distance = l2dist(points1[1], points2[1], 3);
			// switch start and end of second fiber if distance from start of first to end of second is smaller:
			if (dist1StartTo2Start > dist1StartTo2End && dist1EndTo2End > dist1EndTo2Start)
				distance += dist1StartTo2End + dist1EndTo2Start;
			else
				distance += dist1StartTo2Start + dist1EndTo2End;
			distance /= 3;

			break;
		}
		case 2: // distances between all 9 pairs of the 3 points of each fiber:
		{

			double points1[3][3];
			double points2[3][3];
			setPoints(fiber1, mapping, points1);
			setPoints(fiber2, mapping, points2);
			double fiber1Len = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();
			double fiber2Len = fiber1->GetValue(mapping[iACsvConfig::Length]).ToDouble();

			for (int i=0; i<3; ++i)
				for (int j=0; j<3; ++j)
					distance += l2dist(points1[i], points2[j], 3);
			distance /= fiber1Len;
			break;
		}
		case 3: // overlap between the cylinder volumes, sampled through CylinderSamplePoints from the shorter fiber
		{
			// 1. sample points on the cylinder
			//    -> regular?
			//        - n places along the fiber axis (e.g. split length into 5 equal segments)
			//        - at each place, take m points along the fiber surface (i.e. split 360° by m = x, one point for each segment of angle width x)
			//        - at surface? -> at distance r from current middle point, in direction of angle x
			//    -> random? -> probably simplest, something like https://stackoverflow.com/a/9266704/671366:
			//        - one random variable for point along fiber axis (0..1, where 0=start point, 1=end point)
			//        - one random variable for direction from axis (0..360°)
			//            -> for now: only 4 positions (0, 90, 180, 270)°, which makes it much easier to handle (no rotation around custom axis, just cross product/inversion of direction!
			//        - one random variable for distance from center (0.. fiber radius); make sure to use sqrt of random variable to avoid clustering points in center (http://mathworld.wolfram.com/DiskPointPicking.html)
			//    - pseudorandom?
			//        --> no idea at the moment
			distance = 1 - getOverlap(fiber1, mapping, fiber2, false);
			break;
		}
		case 4:
		{
			distance = 1 - getOverlap(fiber1, mapping, fiber2, true);
			break;
		}
		}
		return distance;
	}

	void getBestMatches(vtkVariantArray* fiberInfo, QMap<uint, uint> const & mapping, vtkTable* reference,
			std::vector<std::vector<iAFiberDistance> > & bestMatches)
	{
		size_t refFiberCount = reference->GetNumberOfRows();
		bestMatches.resize(DistanceMetricCount);
		for (int d=0; d<DistanceMetricCount; ++d)
		{
			std::vector<iAFiberDistance> distances(refFiberCount);
			for (size_t fiberID = 0; fiberID < refFiberCount; ++fiberID)
			{
				distances[fiberID].index = fiberID;
				double curDistance = getDistance(fiberInfo, mapping, reference->GetRow(fiberID), d);
				distances[fiberID].distance = curDistance;
			}
			std::sort(distances.begin(), distances.end());
			std::copy(distances.begin(), distances.begin() + MaxNumberOfCloseFibers, std::back_inserter(bestMatches[d]));
		}
	}

}

void iAFiberOptimizationExplorer::referenceToggled(bool)
{
	QRadioButton* sender = qobject_cast<QRadioButton*>(QObject::sender());
	sender->setText("reference");
	for (auto button: m_defaultButtonGroup->buttons())
		if (button != sender)
			button->setText("");

	m_referenceID = sender->property("resultID").toULongLong();

	// "register" other datasets to reference:
	auto const & mapping = *m_resultData[m_referenceID].m_outputMapping.data();

	/*
	std::vector<std::vector<double> > sampledPoints;
	samplePoints(m_resultData[0].m_resultTable->GetRow(0), mapping, sampledPoints);
	m_sampleData = vtkSmartPointer<vtkPolyData>::New();
	auto points = vtkSmartPointer<vtkPoints>::New();
	for (size_t s = 0; s < sampledPoints.size(); ++s)
	{
		double pt[3];
		for (int i = 0; i < 3; ++i)
			pt[i] = sampledPoints[s][i];
		points->InsertNextPoint(pt);
	}
	m_sampleData->SetPoints(points);
	auto vertexFilter = vtkSmartPointer<vtkVertexGlyphFilter>::New();
	vertexFilter->SetInputData(m_sampleData);
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

	m_sampleMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
	m_sampleActor = vtkSmartPointer<vtkActor>::New();
	m_sampleMapper->SetInputData(polydata);
	m_sampleActor->SetMapper(m_sampleMapper);
	m_sampleMapper->Update();
	m_sampleActor->GetProperty()->SetPointSize(2);
	m_mainRenderer->GetRenderWindow()->GetRenderers()->GetFirstRenderer()->AddActor(m_sampleActor);
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();

	return;
	*/

	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		if (resultID == m_referenceID)
			continue;

		//DEBUG_LOG(QString("Matching result %1 to reference (%2):").arg(resultID).arg(m_referenceID));
		size_t fiberCount = m_resultData[resultID].m_resultTable->GetNumberOfRows();
		m_resultData[resultID].m_referenceDist.resize(fiberCount);
		m_resultData[resultID].m_referenceDiff.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			// find the best-matching fibers in reference & compute difference:
			getBestMatches(m_resultData[resultID].m_resultTable->GetRow(fiberID),
				mapping, m_resultData[m_referenceID].m_resultTable,
				m_resultData[resultID].m_referenceDist[fiberID]);
			/*
			DEBUG_LOG(QString("  Fiber %1: Best match: distmetric1: fiber %2 (distance: %3), distmetric2: %4 (distance: %5)")
				.arg(fiberID)
				.arg(m_resultData[resultID].m_referenceDist[fiberID][0][0].index)
				.arg(m_resultData[resultID].m_referenceDist[fiberID][0][0].distance)
				.arg(m_resultData[resultID].m_referenceDist[fiberID][1][0].index)
				.arg(m_resultData[resultID].m_referenceDist[fiberID][1][0].distance));
			*/
		}
	}
	
	std::vector<size_t> diffCols;
	diffCols.push_back(mapping[iACsvConfig::StartX]);
	diffCols.push_back(mapping[iACsvConfig::StartY]);
	diffCols.push_back(mapping[iACsvConfig::StartZ]);
	diffCols.push_back(mapping[iACsvConfig::EndX]);
	diffCols.push_back(mapping[iACsvConfig::EndY]);
	diffCols.push_back(mapping[iACsvConfig::EndZ]);
	diffCols.push_back(mapping[iACsvConfig::CenterX]);
	diffCols.push_back(mapping[iACsvConfig::CenterY]);
	diffCols.push_back(mapping[iACsvConfig::CenterZ]);
	diffCols.push_back(mapping[iACsvConfig::Phi]);
	diffCols.push_back(mapping[iACsvConfig::Theta]);
	diffCols.push_back(mapping[iACsvConfig::Length]);
	diffCols.push_back(mapping[iACsvConfig::Diameter]);
	size_t splomID = 0;
	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		if (resultID == m_referenceID)
		{
			splomID += m_resultData[resultID].m_resultTable->GetNumberOfRows();
			continue;
		}
		//DEBUG_LOG(QString("Differences of result %1 to reference (%2):").arg(resultID).arg(m_referenceID));
		size_t fiberCount = m_resultData[resultID].m_resultTable->GetNumberOfRows();
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			// compute error (=difference - startx, starty, startz, endx, endy, endz, shiftx, shifty, shiftz, phi, theta, length, diameter)
			std::vector<double> refDiff(DifferenceCount);
			for (size_t diffID = 0; diffID < diffCols.size(); ++diffID)
			{
				refDiff[diffID] = m_resultData[resultID].m_resultTable->GetValue(fiberID, diffCols[diffID]).ToDouble()
					- m_resultData[m_referenceID].m_resultTable->GetValue(m_resultData[resultID].m_referenceDist[fiberID][0][0].index, diffCols[diffID]).ToDouble();
				size_t tableColumnID = m_splomData->numParams() - (DifferenceCount + DistanceMetricCount + EndColumns) + diffID;
				m_splomData->data()[tableColumnID][splomID] = refDiff[diffID];
				m_resultData[resultID].m_resultTable->SetValue(fiberID, tableColumnID, refDiff[diffID]);
			}
			for (size_t distID = 0; distID < DistanceMetricCount; ++distID)
			{
				double dist = m_resultData[resultID].m_referenceDist[fiberID][distID][0].distance;
				size_t tableColumnID = m_splomData->numParams() - (DistanceMetricCount + EndColumns) + distID;
				m_splomData->data()[tableColumnID][splomID] = dist;
				m_resultData[resultID].m_resultTable->SetValue(fiberID, tableColumnID, dist);
			}
			/*
			DEBUG_LOG(QString("  Fiber %1 -> ref #%2. Shift: startx=(%3, %4, %5), endx=(%6, %7, %8), center=(%9, %10, %11), phi=%12, theta=%13, length=%14, diameter=%15")
				.arg(fiberID).arg(m_resultData[resultID].m_referenceDist[fiberID][0][0].index)
				.arg(refDiff[0]).arg(refDiff[1]).arg(refDiff[2])
				.arg(refDiff[3]).arg(refDiff[4]).arg(refDiff[5])
				.arg(refDiff[6]).arg(refDiff[7]).arg(refDiff[8])
				.arg(refDiff[9]).arg(refDiff[10]).arg(refDiff[11]).arg(refDiff[12])
			);
			*/
			m_resultData[resultID].m_referenceDiff[fiberID].swap(refDiff);
			++splomID;
		}
	}
	std::vector<size_t> changedSplomColumns;
	for (size_t paramID = 0; paramID < diffCols.size()+DistanceMetricCount; ++paramID)
	{
		size_t columnID = m_splomData->numParams() - (DifferenceCount + DistanceMetricCount + EndColumns) + paramID;
		changedSplomColumns.push_back(columnID);
	}
	m_splomData->updateRanges(changedSplomColumns);
	// TODO: how to visualize?
}

void iAFiberOptimizationExplorer::splomLookupTableChanged()
{
	QSharedPointer<iALookupTable> lut = m_splom->lookupTable();
	size_t colorLookupParam = m_splom->colorLookupParam();
	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		m_resultData[resultID].m_mini3DVis->setLookupTable(lut, colorLookupParam);
		if (m_resultData[resultID].m_main3DVis->visible())
			m_resultData[resultID].m_main3DVis->setLookupTable(lut, colorLookupParam);
	}
}
void iAFiberOptimizationExplorer::changeReferenceDisplay()
{
	size_t distanceMeasure = m_cmbboxDistanceMeasure->currentIndex();
	bool showRef = m_chkboxShowReference->isChecked();
	int refCount = std::min(MaxNumberOfCloseFibers, m_spnboxReferenceCount->value());

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
	arrID->SetName(m_resultData[m_referenceID].m_resultTable->GetColumnName(0));
	m_refVisTable->AddColumn(arrID);
	// other columns (float):
	for (int col = 1; col < m_resultData[m_referenceID].m_resultTable->GetNumberOfColumns() - 1; ++col)
	{
		addColumn(m_refVisTable, 0, m_resultData[m_referenceID].m_resultTable->GetColumnName(col), 0);
	}

	std::vector<iAFiberDistance> referenceIDsToShow;

	double range[2];
	range[0] = std::numeric_limits<double>::max();
	range[1] = std::numeric_limits<double>::lowest();
	//DEBUG_LOG("Showing reference fibers:");
	for (size_t resultID=0; resultID < m_resultData.size(); ++resultID)
	{
		if (resultID == m_referenceID)
			continue;
		//DEBUG_LOG(QString("  In Result %1").arg(resultID));
		for (size_t fiberIdx = 0; fiberIdx < m_currentSelection[resultID].size(); ++fiberIdx)
		{
			size_t fiberID = m_currentSelection[resultID][fiberIdx];
			//DEBUG_LOG(QString("    For Fiber %1").arg(fiberID));
			for (int n=0; n<refCount; ++n)
			{
				/*
				DEBUG_LOG(QString("      Ref. Fiber %1, distance=%2")
						  .arg(m_resultData[resultID].m_referenceDist[fiberID][distanceMeasure][n].index)
						  .arg(m_resultData[resultID].m_referenceDist[fiberID][distanceMeasure][n].distance));
				*/
				referenceIDsToShow.push_back(m_resultData[resultID].m_referenceDist[fiberID][distanceMeasure][n]);
			}
		}
	}

	m_refVisTable->SetNumberOfRows(referenceIDsToShow.size());

	auto refTable = m_resultData[m_referenceID].m_resultTable;
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
							m_resultData[m_referenceID].m_outputMapping, QColor(0,0,0) ) );
	QSharedPointer<iALookupTable> lut(new iALookupTable);
	*lut.data() = iALUT::Build(m_splomData->paramRange(m_splomData->numParams()-EndColumns-DistanceMetricCount+distanceMeasure), "ColorBrewer single hue 5-class oranges", 256, SelectionOpacity);
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
