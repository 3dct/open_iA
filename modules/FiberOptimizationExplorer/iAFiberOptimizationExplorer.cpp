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

#include "charts/iAChartWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAScatterPlot.h" // for selection mode: iAScatterPlot::Rectangle
#include "charts/iAQSplom.h"
#include "charts/iASPLOMData.h"
#include "iAColorTheme.h"
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "iALookupTable.h"
#include "iAModuleDispatcher.h"
#include "iARendererManager.h"
#include "iASelectionInteractorStyle.h"
#include "io/iAFileUtils.h"
#include "mainwindow.h"
#include "mdichild.h"

#if (VTK_MAJOR_VERSION >= 8 && defined(VTK_OPENGL2_BACKEND) )
#include <QVTKOpenGLWidget.h>
#else
#include <QVTKWidget2.h>
#endif
#include <vtkFloatArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkTable.h>
#include <vtkVariantArray.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QTextStream>

#include <QtGlobal> // for QT_VERSION

class iAFiberDistance
{
public:
	size_t index;
	std::vector<double> distance;
	friend bool operator<(iAFiberDistance const & a, iAFiberDistance const & b);
};
bool operator<(iAFiberDistance const & a, iAFiberDistance const & b)
{
	return a.distance[0] < b.distance[0];
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
	std::vector<std::vector<double> > m_referenceDiff;
	std::vector<std::vector<iAFiberDistance> > m_referenceDist;
	size_t m_fiberCount;

	// UI elements:
	iAVtkWidgetClass* m_vtkWidget;
	QSharedPointer<iA3DCylinderObjectVis> m_mini3DVis;
	QSharedPointer<iA3DCylinderObjectVis> m_main3DVis;
};

namespace
{
	iACsvConfig getCsvConfig(QString const & csvFile)
	{
		iACsvConfig config = iACsvConfig::getLegacyFiberFormat(csvFile);
		config.skipLinesStart = 0;
		config.containsHeader = false;
		config.visType = iACsvConfig::Cylinders;
		return config;
	}

	const double MiddlePointShift = 74.5;
	int SelectionOpacity = iA3DLineObjectVis::DefaultSelectionOpacity;
	int ContextOpacity = iA3DLineObjectVis::DefaultContextOpacity;
	const size_t NoPlotsIdx = std::numeric_limits<size_t>::max();
	const QString ModuleSettingsKey("FiberOptimizationExplorer");

	QColor ProjectionErrorDefaultPlotColor(128, 128, 128, SelectionOpacity);
	QColor SPLOMSelectionColor(255, 0, 0, ContextOpacity);

	int NumberOfCloseFibers = 25;
}

iAFiberOptimizationExplorer::iAFiberOptimizationExplorer(QString const & path, MainWindow* mainWnd):
	m_colorTheme(iAColorThemeManager::GetInstance().GetTheme("Brewer Accent (max. 8)")),
	m_mainWnd(mainWnd),
	m_timeStepCount(0),
	m_splomData(new iASPLOMData()),
	m_splom(new iAQSplom()),
	m_lastResultID(-1)
{
	setDockOptions(AllowNestedDocks | AllowTabbedDocks );
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif
	setMinimumSize(600, 400);
	setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

	//QVBoxLayout* mainLayout = new QVBoxLayout();
	//setLayout(mainLayout);
	//QScrollArea* scrollArea = new QScrollArea();
	//mainLayout->addWidget(scrollArea);
	//scrollArea->setWidgetResizable(true);
	//QWidget* resultsListWidget = new QWidget();
	//scrollArea->setWidget(resultsListWidget);

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
	mainRendererContainer->layout()->addWidget(defaultOpacityWidget);
	mainRendererContainer->layout()->addWidget(contextOpacityWidget);

	m_style = vtkSmartPointer<iASelectionInteractorStyle>::New();
	m_style->assignToRenderWindow(renWin);
	connect(m_style.GetPointer(), &iASelectionInteractorStyle::selectionChanged, this, &iAFiberOptimizationExplorer::selection3DChanged);

	QWidget* optimizationSteps = new QWidget();
	m_timeStepChart = new iAChartWidget(optimizationSteps, "Time Step", "Projection Error");
	m_timeStepChart->setMinimumHeight(200);
	m_timeStepChart->setSelectionMode(iAChartWidget::SelectPlot);
	connect(m_timeStepChart, &iAChartWidget::plotsSelected, this, &iAFiberOptimizationExplorer::selectionTimeStepChartChanged);

	optimizationSteps->setLayout(new QVBoxLayout());
	QWidget* timeSteps = new QWidget();
	QSlider* timeStepSlider = new QSlider(Qt::Horizontal);
	timeStepSlider->setMinimum(0);
	connect(timeStepSlider, &QSlider::valueChanged, this, &iAFiberOptimizationExplorer::timeSliderChanged);
	m_currentTimeStepLabel = new QLabel("");
	timeSteps->setLayout(new QHBoxLayout());
	timeSteps->layout()->addWidget(timeStepSlider);
	timeSteps->layout()->addWidget(m_currentTimeStepLabel);
	timeSteps->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	optimizationSteps->layout()->addWidget(m_timeStepChart);
	optimizationSteps->layout()->addWidget(timeSteps);

	int resultID = 0;
	m_defaultButtonGroup = new QButtonGroup();

	const int MaxDatasetCount = 24;
	if (csvFileNames.size() > MaxDatasetCount)
	{
		DEBUG_LOG(QString("You tried to open %1 datasets. This tool can only handle a small amount of datasets. Loading only the first %2.").arg(csvFileNames.size()).arg(MaxDatasetCount));
		csvFileNames.erase( csvFileNames.begin() + MaxDatasetCount, csvFileNames.end() );
	}
	size_t additionalColumns = 0;
	for (QString csvFile : csvFileNames)
	{
		iACsvConfig config = getCsvConfig(csvFile);

		iACsvIO io;
		iACsvVtkTableCreator tableCreator;
		if (!io.loadCSV(tableCreator, config))
		{
			DEBUG_LOG(QString("Could not load file '%1', skipping!").arg(csvFile));
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
			paramNames.push_back("ProjectionErrorReduction");
			paramNames.push_back("Result_ID");
			m_splomData->setParameterNames(paramNames);
			additionalColumns = paramNames.size() - numColumns;
		}
		// TODO: Check if output mapping is the same (it must be)!
		vtkIdType numFibers = tableCreator.getTable()->GetNumberOfRows();
		if (numFibers < NumberOfCloseFibers)
			NumberOfCloseFibers = numFibers;
		// TOOD: simplify - load all tables beforehand, then allocate splom data fully and then fill it?
		for (int i = 15; i >= 2; --i)
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
		for (int col = 0; col < additionalColumns; ++col)
		{
			vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
			arrX->SetName(m_splomData->parameterName(numColumns+col).toStdString().c_str());
			arrX->SetNumberOfValues(numFibers);
			tableCreator.getTable()->AddColumn(arrX);
		}
		
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

		QCheckBox* toggleMainRender = new QCheckBox(QFileInfo(csvFile).fileName());
		toggleMainRender->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		toggleMainRender->setProperty("resultID", resultID);
		QRadioButton* toggleReference = new QRadioButton("");
		toggleReference->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		toggleReference->setProperty("resultID", resultID);
		m_defaultButtonGroup->addButton(toggleReference);
		resultsListLayout->addWidget(toggleMainRender, resultID, 0);
		resultsListLayout->addWidget(toggleReference, resultID, 1);
		resultsListLayout->addWidget(resultData.m_vtkWidget, resultID, 2);

		resultData.m_mini3DVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(
				resultData.m_vtkWidget, tableCreator.getTable(), io.getOutputMapping(), getResultColor(resultID)));
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

		QFileInfo timeInfo(QFileInfo(csvFile).absolutePath() + "/" + QFileInfo(csvFile).baseName());

		// TODO: in case reading gets inefficient, look at pre-reserving the required amount of fields
		//       and using std::vector::swap to assign the sub-vectors!
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
					m_timeStepChart->addPlot(QSharedPointer<iALineFunctionDrawer>(new iALineFunctionDrawer(plotData, getResultColor(resultID))));
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
						middlePoint[i] = values[i].toDouble() + MiddlePointShift; // middle point positions are shifted!
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
				if (singleFiberValues.size() > m_timeStepCount)
				{
					if (m_timeStepCount != 0)
					{
						DEBUG_LOG(QString("Different time step counts! Encountered %1 before, now %2").arg(m_timeStepCount).arg(singleFiberValues.size()));
					}
					m_timeStepCount = singleFiberValues.size();
				}
				fiberTimeValues.push_back(singleFiberValues);
				++curFiber;
			} while (true);
			int fiberCount = curFiber;

			// transform from [fiber, timestep, value] to [timestep, fiber, value] indexing
			m_resultData[m_resultData.size() - 1].m_timeValues.resize(m_timeStepCount);
			for (int t = 0; t < m_timeStepCount; ++t)
			{
				m_resultData[m_resultData.size() - 1].m_timeValues[t].resize(fiberCount);
				for (int f = 0; f < fiberCount; ++f)
				{
					m_resultData[m_resultData.size() - 1].m_timeValues[t][f] = fiberTimeValues[f][t];
				}
			}
		}
		else
		{
			m_resultData[m_resultData.size() - 1].m_startPlotIdx = NoPlotsIdx;
		}
		++resultID;
	}
	m_splomData->updateRanges();
	m_currentSelection.resize(resultID);

	timeStepSlider->setMaximum(m_timeStepCount - 1);
	timeStepSlider->setValue(m_timeStepCount - 1);
	m_currentTimeStepLabel->setText(QString::number(m_timeStepCount - 1));

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
}

iAFiberOptimizationExplorer::~iAFiberOptimizationExplorer()
{
	QSettings settings;
	settings.setValue(ModuleSettingsKey + "/maximized", isMaximized());
	if (!isMaximized())
		settings.setValue(ModuleSettingsKey + "/geometry", qobject_cast<QWidget*>(parent())->geometry());
	settings.setValue(ModuleSettingsKey+"/state", saveState());
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

void iAFiberOptimizationExplorer::toggleVis(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	iAResultData & data = m_resultData[resultID];
	if (state == Qt::Checked)
	{
		if (data.m_main3DVis)
		{
			DEBUG_LOG("Visualization already exists!");
			return;
		}
		data.m_main3DVis = QSharedPointer<iA3DCylinderObjectVis>(new iA3DCylinderObjectVis(m_mainRenderer,
				data.m_resultTable, data.m_outputMapping, getResultColor(resultID)));
		data.m_main3DVis->setColor(getResultColor(resultID));
		data.m_main3DVis->setSelectionOpacity(SelectionOpacity);
		data.m_main3DVis->setContextOpacity(ContextOpacity);
		if (m_splom->colorScheme() == iAQSplom::DivergingPerceptuallyUniform)
		{
			data.m_main3DVis->setLookupTable(m_splom->lookupTable(), m_splom->colorLookupParam());
			data.m_main3DVis->updateColorSelectionRendering();
		}
		bool anythingSelected = isAnythingSelected();
		if (anythingSelected)
			data.m_main3DVis->setSelection(m_currentSelection[resultID], anythingSelected);
		data.m_main3DVis->show();
		m_style->setInput( data.m_main3DVis->getLinePolyData() );
		m_lastMain3DVis = data.m_main3DVis;
		m_lastResultID = resultID;
	}
	else
	{
		if (!data.m_main3DVis)
		{
			DEBUG_LOG("Visualization not found!");
			return;
		}
		data.m_main3DVis->hide();
		data.m_main3DVis.reset();
	}
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
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
		if (result.m_main3DVis)
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

void iAFiberOptimizationExplorer::selection3DChanged(std::vector<size_t> const & selection)
{
	if (!m_lastMain3DVis)
		return;
	clearSelection();
	m_currentSelection[m_lastResultID].assign(selection.begin(), selection.end());
	sortCurrentSelection();
	showCurrentSelectionIn3DViews();
	showCurrentSelectionInPlot();
	showCurrentSelectionInSPLOM();
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
}

void iAFiberOptimizationExplorer::miniMouseEvent(QMouseEvent* ev)
{
	if (ev->buttons() == Qt::RightButton && ev->type() == QEvent::MouseButtonPress)
	{
		int resultID = QObject::sender()->property("resultID").toInt();
		iAFeatureScoutModuleInterface * featureScout = m_mainWnd->getModuleDispatcher().GetModule<iAFeatureScoutModuleInterface>();
		MdiChild* newChild = m_mainWnd->createMdiChild(false);
		iACsvConfig config = getCsvConfig(m_resultData[resultID].m_fileName);
		featureScout->LoadFeatureScout(config, newChild);
		newChild->LoadLayout("FeatureScout");
	}
}

void iAFiberOptimizationExplorer::timeSliderChanged(int timeStep)
{
	m_currentTimeStepLabel->setText(QString::number(timeStep));
	for (int resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		if (m_resultData[resultID].m_timeValues.size() > timeStep)
		{
			m_resultData[resultID].m_mini3DVis->updateValues(m_resultData[resultID].m_timeValues[timeStep]);
			if (m_resultData[resultID].m_main3DVis)
				m_resultData[resultID].m_main3DVis->updateValues(m_resultData[resultID].m_timeValues[timeStep]);
		}
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
		if (m_resultData[resultID].m_main3DVis)
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
		if (m_resultData[resultID].m_main3DVis)
		{
			m_resultData[resultID].m_main3DVis->setContextOpacity(ContextOpacity);
			m_resultData[resultID].m_main3DVis->updateColorSelectionRendering();
		}
	}
	showCurrentSelectionInPlot();
}

// currently: L2 norm (euclidean distance). other measures?
double getDistance(vtkVariantArray* fiber1, QMap<uint, uint> const & mapping, vtkVariantArray* fiber2,
	std::vector<int> const & colsToInclude, std::vector<double> const & weights)
{
	double distance = 0;
	for (int colIdx=0; colIdx < colsToInclude.size(); ++colIdx)
		distance += weights[colIdx] * std::pow(fiber1->GetValue(mapping[colsToInclude[colIdx]]).ToDouble() - fiber2->GetValue(mapping[colsToInclude[colIdx]]).ToDouble(), 2);
	return std::sqrt(distance);
}

void getBestMatches(vtkVariantArray* fiberInfo, QMap<uint, uint> const & mapping, vtkTable* reference,
	std::vector<std::vector<int> > const & colsToInclude,
	std::vector<std::vector<double> > const & weights, std::vector<iAFiberDistance> & bestMatches)
{
	size_t refFiberCount = reference->GetNumberOfRows();
	//minDistance = std::numeric_limits<double>::max();
	//size_t bestMatch = std::numeric_limits<size_t>::max();
	std::vector<iAFiberDistance> distances(refFiberCount);
	for (size_t fiberID = 0; fiberID < refFiberCount; ++fiberID)
	{
		distances[fiberID].index = fiberID;
		for (int d=0; d<colsToInclude.size(); ++d)
		{
			double curDistance = getDistance(fiberInfo, mapping, reference->GetRow(fiberID), colsToInclude[d], weights[d]);
			distances[fiberID].distance.push_back(curDistance);
		}
	}
	std::sort(distances.begin(), distances.end());
	std::copy(distances.begin(), distances.begin() + NumberOfCloseFibers, std::back_inserter(bestMatches));
}

void iAFiberOptimizationExplorer::referenceToggled(bool)
{
	QRadioButton* sender = qobject_cast<QRadioButton*>(QObject::sender());
	sender->setText("reference");
	for (auto button: m_defaultButtonGroup->buttons())
		if (button != sender)
			button->setText("");

	m_referenceID = sender->property("resultID").toULongLong();
	
	// determine which columns to use and their normalization factors:
	std::vector<std::vector<int> > colsToInclude(2);  std::vector<std::vector<double> > weights(2);
	colsToInclude[0].push_back(iACsvConfig::CenterX);   weights[0].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::CenterX])[1]);
	colsToInclude[0].push_back(iACsvConfig::CenterY);   weights[0].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::CenterY])[1]);
	colsToInclude[0].push_back(iACsvConfig::CenterZ);   weights[0].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::CenterZ])[1]);
	colsToInclude[0].push_back(iACsvConfig::Phi);       weights[0].push_back(1 / (2 * vtkMath::Pi()));
	colsToInclude[0].push_back(iACsvConfig::Theta);     weights[0].push_back(1 / (2 * vtkMath::Pi()));
	colsToInclude[0].push_back(iACsvConfig::Length);    weights[0].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::Length])[1]);

	colsToInclude[1].push_back(iACsvConfig::StartX);   weights[1].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::StartX])[1]);
	colsToInclude[1].push_back(iACsvConfig::StartY);   weights[1].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::StartY])[1]);
	colsToInclude[1].push_back(iACsvConfig::StartZ);   weights[1].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::StartZ])[1]);
	colsToInclude[1].push_back(iACsvConfig::EndX);     weights[1].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::EndX])[1]);
	colsToInclude[1].push_back(iACsvConfig::EndY);     weights[1].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::EndY])[1]);
	colsToInclude[1].push_back(iACsvConfig::EndZ);     weights[1].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::EndZ])[1]);
	colsToInclude[1].push_back(iACsvConfig::Length);   weights[1].push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::Length])[1]);

	// "register" other datasets to reference:
	auto const & mapping = *m_resultData[m_referenceID].m_outputMapping.data();

	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		if (resultID == m_referenceID)
			continue;

		DEBUG_LOG(QString("Matching result %1 to reference (%2):").arg(resultID).arg(m_referenceID));
		size_t fiberCount = m_resultData[resultID].m_resultTable->GetNumberOfRows();
		m_resultData[resultID].m_referenceDist.resize(fiberCount);
		m_resultData[resultID].m_referenceDiff.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			// find the best-matching fibers in reference & compute difference:
			getBestMatches(m_resultData[resultID].m_resultTable->GetRow(fiberID),
				mapping, m_resultData[m_referenceID].m_resultTable,
				colsToInclude, weights, m_resultData[resultID].m_referenceDist[fiberID]);
			DEBUG_LOG(QString("  Fiber %1: Best matches fiber %2 from reference (distance: %3/%4), second best: %5 (distance: %6/%7)")
				.arg(fiberID)
				.arg(m_resultData[resultID].m_referenceDist[fiberID][0].index)
				.arg(m_resultData[resultID].m_referenceDist[fiberID][0].distance[0])
				.arg(m_resultData[resultID].m_referenceDist[fiberID][0].distance[1])
				.arg(m_resultData[resultID].m_referenceDist[fiberID][1].index)
				.arg(m_resultData[resultID].m_referenceDist[fiberID][1].distance[0])
				.arg(m_resultData[resultID].m_referenceDist[fiberID][1].distance[1]));
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
		DEBUG_LOG(QString("Differences of result %1 to reference (%2):").arg(resultID).arg(m_referenceID));
		size_t fiberCount = m_resultData[resultID].m_resultTable->GetNumberOfRows();
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			// compute error (=difference - startx, starty, startz, endx, endy, endz, shiftx, shifty, shiftz, phi, theta, length, diameter)
			std::vector<double> refDiff(13);
			for (size_t diffID = 0; diffID < diffCols.size(); ++diffID)
			{
				refDiff[diffID] = m_resultData[resultID].m_resultTable->GetValue(fiberID, diffCols[diffID]).ToDouble()
					- m_resultData[m_referenceID].m_resultTable->GetValue(m_resultData[resultID].m_referenceDist[fiberID][0].index, diffCols[diffID]).ToDouble();
				size_t tableColumnID = m_splomData->numParams() - 15 + diffID;
				m_splomData->data()[tableColumnID][splomID] = refDiff[diffID];
				m_resultData[resultID].m_resultTable->SetValue(fiberID, tableColumnID, refDiff[diffID]);
			}
			DEBUG_LOG(QString("  Fiber %1 -> ref #%2. Shift: startx=(%3, %4, %5), endx=(%6, %7, %8), center=(%9, %10, %11), phi=%12, theta=%13, length=%14, diameter=%15")
				.arg(fiberID).arg(m_resultData[resultID].m_referenceDist[fiberID][0].index)
				.arg(refDiff[0]).arg(refDiff[1]).arg(refDiff[2])
				.arg(refDiff[3]).arg(refDiff[4]).arg(refDiff[5])
				.arg(refDiff[6]).arg(refDiff[7]).arg(refDiff[8])
				.arg(refDiff[9]).arg(refDiff[10]).arg(refDiff[11]).arg(refDiff[12])
			);
			m_resultData[resultID].m_referenceDiff[fiberID].swap(refDiff);
			++splomID;
		}
	}
	std::vector<size_t> changedSplomColumns;
	for (size_t paramID = 0; paramID < diffCols.size(); ++paramID)
		changedSplomColumns.push_back(m_splomData->numParams() - 15 + paramID);
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
		if (m_resultData[resultID].m_main3DVis)
			m_resultData[resultID].m_main3DVis->setLookupTable(lut, colorLookupParam);
	}
}
