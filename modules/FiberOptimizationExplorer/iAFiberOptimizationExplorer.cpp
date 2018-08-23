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

class iAResultData
{
public:
	vtkSmartPointer<vtkTable> m_resultTable;
	QSharedPointer<QMap<uint, uint> > m_outputMapping;
	iAVtkWidgetClass* m_vtkWidget;
	QSharedPointer<iA3DCylinderObjectVis> m_mini3DVis;
	QSharedPointer<iA3DCylinderObjectVis> m_main3DVis;
	QString m_fileName;
	// timestep, fiber, 6 values (center, phi, theta, length)
	std::vector<std::vector<std::vector<double> > > m_timeValues;
	// fiber, timestep, global projection error
	std::vector<QSharedPointer<std::vector<double> > > m_projectionError;
	size_t m_startPlotIdx;
	std::vector<size_t> m_referenceMapping;
	// fiber, errors (shiftx, shifty, shiftz, phi, theta, length, diameter)
	std::vector<std::vector<double> > m_referenceDiff;
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
	const int DefaultMainOpacity = 128;
	const size_t NoPlotsIdx = std::numeric_limits<size_t>::max();
	const QString ModuleSettingsKey("FiberOptimizationExplorer");

	QColor ProjectionErrorDefaultPlotColor(128, 128, 128, 64);
	QColor SelectionColor(255, 0, 0, 255);
}

iAFiberOptimizationExplorer::iAFiberOptimizationExplorer(QString const & path, MainWindow* mainWnd):
	m_colorTheme(iAColorThemeManager::GetInstance().GetTheme("Brewer Accent (max. 8)")),
	m_mainWnd(mainWnd),
	m_timeStepCount(0),
	m_splomData(new iASPLOMData()),
	m_splom(new iAQSplom()),
	m_lastResultID(-1)
{
	setMinimumSize(600, 400);
	this->setCentralWidget(nullptr);
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

	m_renderManager = QSharedPointer<iARendererManager>(new iARendererManager());

	m_mainRenderer = new iAVtkWidgetClass();
	//QSurfaceFormat format = m_mainRenderer->format();
	//format.setSamples(4);
	//m_mainRenderer->setFormat(format);
	// QVTKOpenGLWidget does not seem to work with multi-sampling enabled (window remains empty when I add polydata actor)
	auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	auto ren = vtkSmartPointer<vtkRenderer>::New();
	ren->SetBackground(1.0, 1.0, 1.0);
	renWin->SetAlphaBitPlanes(1);
	ren->SetUseDepthPeeling(true);
	renWin->AddRenderer(ren);
	m_renderManager->addToBundle(ren);
	m_mainRenderer->SetRenderWindow(renWin);
	m_opacitySlider = new QSlider(Qt::Horizontal);
	m_opacitySlider->setMinimum(0);
	m_opacitySlider->setMaximum(255);
	m_opacitySlider->setValue(DefaultMainOpacity);
	connect(m_opacitySlider, &QSlider::valueChanged, this, &iAFiberOptimizationExplorer::mainOpacityChanged);
	m_currentOpacityLabel = new QLabel(QString::number(DefaultMainOpacity));
	QWidget* opacityWidget = new QWidget();
	opacityWidget->setLayout(new QHBoxLayout());
	opacityWidget->layout()->addWidget(m_opacitySlider);
	opacityWidget->layout()->addWidget(m_currentOpacityLabel);
	opacityWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QWidget* mainRendererContainer = new QWidget();
	mainRendererContainer->setLayout(new QVBoxLayout());
	mainRendererContainer->layout()->addWidget(m_mainRenderer);
	mainRendererContainer->layout()->addWidget(opacityWidget);

	m_style = vtkSmartPointer<iASelectionInteractorStyle>::New();
	m_style->assignToRenderWindow(renWin);
	connect(m_style.GetPointer(), &iASelectionInteractorStyle::selectionChanged, this, &iAFiberOptimizationExplorer::selection3DChanged);

	QWidget* optimizationSteps = new QWidget();
	m_timeStepProjectionErrorChart = new iAChartWidget(optimizationSteps, "Time Step", "Projection Error");
	m_timeStepProjectionErrorChart->setMinimumHeight(200);
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
	optimizationSteps->layout()->addWidget(m_timeStepProjectionErrorChart);
	optimizationSteps->layout()->addWidget(timeSteps);

	int resultID = 0;
	m_defaultButtonGroup = new QButtonGroup();

	const int MaxDatasetCount = 24;
	if (csvFileNames.size() > MaxDatasetCount)
	{
		DEBUG_LOG(QString("You tried to open %1 datasets. This tool can only handle a small amount of datasets. Loading only the first %2.").arg(csvFileNames.size()).arg(MaxDatasetCount));
		csvFileNames.erase( csvFileNames.begin() + MaxDatasetCount, csvFileNames.end() );
	}

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

		if (resultID == 0)
		{
			std::vector<QString> paramNames;
			for (QString s: io.getOutputHeaders())
				paramNames.push_back(s);
			paramNames.push_back("XShift");
			paramNames.push_back("YShift");
			paramNames.push_back("ZShift");
			paramNames.push_back("PhiDiff");
			paramNames.push_back("ThetaDiff");
			paramNames.push_back("LengthDiff");
			paramNames.push_back("DiameterDiff");
			paramNames.push_back("ProjectionErrorReduction");
			paramNames.push_back("Result_ID");
			m_splomData->setParameterNames(paramNames);
		}
		// TODO: Check if output mapping is the same (it must be)!
		vtkIdType numColumns = tableCreator.getTable()->GetNumberOfColumns();
		vtkIdType numFibers = tableCreator.getTable()->GetNumberOfRows();
		// TOOD: simplify - load all tables beforehand, then allocate splom data fully and then fill it?
		m_splomData->data()[m_splomData->numParams()-9].resize(m_splomData->data()[m_splomData->numParams()-9].size()+ numFibers, 0);
		m_splomData->data()[m_splomData->numParams()-8].resize(m_splomData->data()[m_splomData->numParams()-8].size()+ numFibers, 0);
		m_splomData->data()[m_splomData->numParams()-7].resize(m_splomData->data()[m_splomData->numParams()-7].size()+ numFibers, 0);
		m_splomData->data()[m_splomData->numParams()-6].resize(m_splomData->data()[m_splomData->numParams()-6].size()+ numFibers, 0);
		m_splomData->data()[m_splomData->numParams()-5].resize(m_splomData->data()[m_splomData->numParams()-5].size()+ numFibers, 0);
		m_splomData->data()[m_splomData->numParams()-4].resize(m_splomData->data()[m_splomData->numParams()-4].size()+ numFibers, 0);
		m_splomData->data()[m_splomData->numParams()-3].resize(m_splomData->data()[m_splomData->numParams()-3].size()+ numFibers, 0);
		m_splomData->data()[m_splomData->numParams()-2].resize(m_splomData->data()[m_splomData->numParams()-2].size()+ numFibers, 0); // proj error red
		for (vtkIdType row = 0; row < numFibers; ++row)
		{
			for (vtkIdType col = 0; col < numColumns; ++col)
			{
				double value = tableCreator.getTable()->GetValue(row, col).ToDouble();
				m_splomData->data()[col].push_back(value);
			}
			m_splomData->data()[m_splomData->numParams()-1].push_back(resultID);
		}
		
		iAResultData resultData;
		resultData.m_vtkWidget  = new iAVtkWidgetClass();
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		auto ren = vtkSmartPointer<vtkRenderer>::New();
		m_renderManager->addToBundle(ren);
		ren->SetBackground(1.0, 1.0, 1.0);
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
				resultData.m_vtkWidget, tableCreator.getTable(), io.getOutputMapping(), m_colorTheme->GetColor(resultID)));
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
				m_resultData[m_resultData.size() - 1].m_startPlotIdx = m_timeStepProjectionErrorChart->plots().size();
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
					QSharedPointer<iAVectorPlotData> plotData(new iAVectorPlotData(values));
					m_timeStepProjectionErrorChart->addPlot(QSharedPointer<iALineFunctionDrawer>(new iALineFunctionDrawer(plotData, ProjectionErrorDefaultPlotColor)));
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

	timeStepSlider->setMaximum(m_timeStepCount - 1);
	timeStepSlider->setValue(m_timeStepCount - 1);
	m_currentTimeStepLabel->setText(QString::number(m_timeStepCount - 1));

	QWidget* resultList = new QWidget();
	resultList->setLayout(resultsListLayout);

	iADockWidgetWrapper* main3DView = new iADockWidgetWrapper(mainRendererContainer, "3D view", "foe3DView");
	iADockWidgetWrapper* resultListDockWidget = new iADockWidgetWrapper(resultList, "Result list", "foeResultList");
	iADockWidgetWrapper* timeSliderWidget = new iADockWidgetWrapper(optimizationSteps, "Time Steps", "foeTimeSteps");
	iADockWidgetWrapper* splomWidget = new iADockWidgetWrapper(m_splom, "Scatter Plot Matrix", "foeSPLOM");

	addDockWidget(Qt::RightDockWidgetArea, resultListDockWidget);
	splitDockWidget(resultListDockWidget, main3DView, Qt::Horizontal);
	splitDockWidget(main3DView, splomWidget, Qt::Horizontal);
	splitDockWidget(resultListDockWidget, timeSliderWidget, Qt::Vertical);
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
	m_splom->setSelectionColor(SelectionColor);
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
	connect(m_splom, &iAQSplom::selectionModified, this, &iAFiberOptimizationExplorer::selectionSPLOMChanged);
}

QColor iAFiberOptimizationExplorer::getMainRendererColor(int resultID)
{
	QColor color = m_colorTheme->GetColor(resultID);
	color.setAlpha(m_opacitySlider->value());
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
				data.m_resultTable, data.m_outputMapping, getMainRendererColor(resultID)));
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

void iAFiberOptimizationExplorer::selection3DChanged(std::vector<size_t> const & selection)
{
	if (!m_lastMain3DVis)
		return;
	m_lastMain3DVis->renderSelection(selection, 0, getMainRendererColor(m_lastResultID), nullptr);
		
	// shift IDs so that they are in the proper range for SPLOM (which has the fibers from all datasets one after another)
	std::vector<size_t> selectionSPLOM(selection);
	if (m_lastResultID > 0)
	{
		size_t startID = 0;
		for (int i = 0; i < m_lastResultID; ++i)
			startID += m_resultData[i].m_resultTable->GetNumberOfRows();
		for (int s = 0; s < selectionSPLOM.size(); ++s)
		{
			selectionSPLOM[s] += startID;
		}
	}
	m_splom->setSelection(selectionSPLOM);
		
	// color the projection error plots of the selected fibers:
	for (auto plot : m_timeStepProjectionErrorChart->plots())
	{
		plot->setColor(ProjectionErrorDefaultPlotColor);
	}
	if (m_resultData[m_lastResultID].m_startPlotIdx != NoPlotsIdx)
	{
		for (size_t idx : selection)
		{
			auto plot = m_timeStepProjectionErrorChart->plots()[m_resultData[m_lastResultID].m_startPlotIdx + idx];
			plot->setColor(SelectionColor);
		}
	}
	m_timeStepProjectionErrorChart->update();
}

void getResultFiberIDFromSPLOMID(size_t splomID, size_t & resultID, size_t & fiberID, std::vector<size_t> const & fiberCounts)
{
	size_t curStart = 0;
	resultID = 0;
	fiberID = 0;
	while (splomID >= curStart + fiberCounts[resultID] && resultID < fiberCounts.size())
	{
		curStart += fiberCounts[resultID];
		++resultID;
	}
	if (resultID == fiberCounts.size())
	{
		DEBUG_LOG(QString("Invalid index in SPLOM: %1").arg(splomID));
		return;
	}
	fiberID = splomID - curStart;
}

void iAFiberOptimizationExplorer::selectionSPLOMChanged(std::vector<size_t> const & selection)
{
	// map from SPLOM index to (resultID, fiberID) pairs
	std::vector<size_t> fiberCounts(m_resultData.size());
	for (int o = 0; o < m_resultData.size(); ++o)
	{
		fiberCounts[o] = m_resultData[o].m_resultTable->GetNumberOfRows();
	}
	std::vector<std::vector<size_t> > selectionsByResult(m_resultData.size());
	size_t resultID, fiberID;
	for (size_t splomID: selection)
	{
		getResultFiberIDFromSPLOMID(splomID, resultID, fiberID, fiberCounts);
		selectionsByResult[resultID].push_back(fiberID);
	}
	for (auto plot : m_timeStepProjectionErrorChart->plots())
	{
		plot->setColor(ProjectionErrorDefaultPlotColor);
	}
	for (size_t resultID=0; resultID<m_resultData.size(); ++resultID)
	{
		// color fibers in each 3D visualization
		std::sort(selectionsByResult[resultID].begin(), selectionsByResult[resultID].end());
		auto result = m_resultData[resultID];
		result.m_mini3DVis->renderSelection(selectionsByResult[resultID], 0, getMainRendererColor(resultID), nullptr);
		if (result.m_main3DVis)
			result.m_main3DVis->renderSelection(selectionsByResult[resultID], 0, getMainRendererColor(resultID), nullptr);
		// color the projection error plots of the selected fibers:
		if (m_resultData[resultID].m_startPlotIdx != NoPlotsIdx)
		{
			for (size_t idx : selectionsByResult[resultID])
			{
				auto plot = m_timeStepProjectionErrorChart->plots()[m_resultData[resultID].m_startPlotIdx + idx];
				plot->setColor(SelectionColor);
			}
		}
		m_timeStepProjectionErrorChart->update();
	}
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
			{
				m_resultData[resultID].m_main3DVis->updateValues(m_resultData[resultID].m_timeValues[timeStep]);
			}
		}
	}
}

void iAFiberOptimizationExplorer::mainOpacityChanged(int opacity)
{
	m_currentOpacityLabel->setText(QString::number(opacity));
	for (int resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		if (m_resultData[resultID].m_main3DVis)
			m_resultData[resultID].m_main3DVis->renderSelection(std::vector<size_t>(), 0, getMainRendererColor(resultID), nullptr);
	}
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

size_t getBestMatch(vtkVariantArray* fiberInfo, QMap<uint, uint> const & mapping, vtkTable* reference,
	std::vector<int> const & colsToInclude, std::vector<double> const & weights, double & minDistance)
{
	size_t refFiberCount = reference->GetNumberOfRows();
	minDistance = std::numeric_limits<double>::max();
	size_t bestMatch = std::numeric_limits<size_t>::max();
	for (size_t fiberID = 0; fiberID < refFiberCount; ++fiberID)
	{
		double curDistance = getDistance(fiberInfo, mapping, reference->GetRow(fiberID), colsToInclude, weights);
		if (curDistance < minDistance)
		{
			bestMatch = fiberID;
			minDistance = curDistance;
		}
	}
	return bestMatch;
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
	std::vector<int> colsToInclude;                  std::vector<double> weights;
	colsToInclude.push_back(iACsvConfig::CenterX);   weights.push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::CenterX])[1]);
	colsToInclude.push_back(iACsvConfig::CenterY);   weights.push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::CenterY])[1]);
	colsToInclude.push_back(iACsvConfig::CenterZ);   weights.push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::CenterZ])[1]);
	colsToInclude.push_back(iACsvConfig::Phi);       weights.push_back(1 / (2 * vtkMath::Pi()));
	colsToInclude.push_back(iACsvConfig::Theta);     weights.push_back(1 / (2 * vtkMath::Pi()));
	colsToInclude.push_back(iACsvConfig::Length);    weights.push_back(1 / m_splomData->paramRange((*m_resultData[m_referenceID].m_outputMapping)[iACsvConfig::Length])[1]);

	// "register" other datasets to reference:
	auto const & mapping = *m_resultData[m_referenceID].m_outputMapping.data();

	for (size_t resultID = 0; resultID < m_resultData.size(); ++resultID)
	{
		if (resultID == m_referenceID)
			continue;

		DEBUG_LOG(QString("Matching result %1 to reference (%2):").arg(resultID).arg(m_referenceID));
		size_t fiberCount = m_resultData[resultID].m_resultTable->GetNumberOfRows();
		m_resultData[resultID].m_referenceMapping.resize(fiberCount);
		m_resultData[resultID].m_referenceDiff.resize(fiberCount);
		for (size_t fiberID = 0; fiberID < fiberCount; ++fiberID)
		{
			double distance;
			// find the best-matching fiber in reference & compute difference:
			size_t referenceFiberID = getBestMatch(m_resultData[resultID].m_resultTable->GetRow(fiberID),
				mapping, m_resultData[m_referenceID].m_resultTable,
				colsToInclude, weights, distance);
			m_resultData[resultID].m_referenceMapping[fiberID] = referenceFiberID;
			DEBUG_LOG(QString("  Fiber %1: Best matches fiber %2 from reference (distance: %3)").arg(fiberID).arg(referenceFiberID).arg(distance));
		}
	}
	colsToInclude.push_back(iACsvConfig::Diameter); // for now, don't include diameter in error calculations. later, move up?
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
			// compute error (=difference - shiftx, shifty, shiftz, phi, theta, length, diameter)
			std::vector<double> refDiff(7);
			for (size_t diffID = 0; diffID < colsToInclude.size(); ++diffID)
			{
				refDiff[diffID] = m_resultData[resultID].m_resultTable->GetValue(fiberID, mapping[colsToInclude[diffID]]).ToDouble()
					- m_resultData[m_referenceID].m_resultTable->GetValue(m_resultData[resultID].m_referenceMapping[fiberID], mapping[colsToInclude[diffID]]).ToDouble();
				m_splomData->data()[m_splomData->numParams()-9 + diffID][splomID] = refDiff[diffID];
				++ splomID;
			}
			DEBUG_LOG(QString("  Fiber %1 -> ref #%2. Shift: center=(%3, %4, %5), phi=%6, theta=%7, length=%8, diameter=%9")
				.arg(fiberID).arg(m_resultData[resultID].m_referenceMapping[fiberID])
				.arg(refDiff[0]).arg(refDiff[1]).arg(refDiff[2])
				.arg(refDiff[3]).arg(refDiff[4]).arg(refDiff[5]).arg(refDiff[6])
			);
			m_resultData[resultID].m_referenceDiff[fiberID].swap(refDiff);
		}
	}
	for (size_t paramID = 0; paramID < colsToInclude.size(); ++paramID)
	{
		m_splomData->updateRange(m_splomData->numParams() - 9 + paramID);
	}

	// TODO: how to visualize?
}
