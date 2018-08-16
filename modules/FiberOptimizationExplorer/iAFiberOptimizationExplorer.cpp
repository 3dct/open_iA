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

#include "iAColorTheme.h"
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "io/iAFileUtils.h"
#include "iARendererManager.h"

#include <QVTKOpenGLWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

#include <QFileInfo>
#include <QGridLayout>
#include <QCheckBox>
#include <QScrollArea>
//#include <QVBoxLayout>

iAFiberOptimizationExplorer::iAFiberOptimizationExplorer(QString const & path):
	m_colorTheme(iAColorThemeManager::GetInstance().GetTheme("Brewer Accent (max. 8)"))
{
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

	m_mainRenderer = new QVTKOpenGLWidget();
	auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	auto ren = vtkSmartPointer<vtkRenderer>::New();
	ren->SetBackground(1.0, 1.0, 1.0);
	renWin->SetAlphaBitPlanes(1);
	ren->SetUseDepthPeeling(true);
	renWin->AddRenderer(ren);
	m_renderManager->addToBundle(ren);
	m_mainRenderer->SetRenderWindow(renWin);

	int curLine = 0;
	for (QString csvFile : csvFileNames)
	{
		iACsvConfig config = iACsvConfig::getLegacyFiberFormat(csvFile);
		config.skipLinesStart = 0;
		config.containsHeader = false;
		config.visType = iACsvConfig::Cylinders;

		iACsvIO io;
		iACsvVtkTableCreator tableCreator;
		if (!io.loadCSV(tableCreator, config))
		{
			DEBUG_LOG(QString("Could not load file '%1', skipping!").arg(csvFile));
			continue;
		}
		
		QVTKOpenGLWidget* vtkWidget = new QVTKOpenGLWidget();
		auto renWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
		auto ren = vtkSmartPointer<vtkRenderer>::New();
		m_renderManager->addToBundle(ren);
		ren->SetBackground(1.0, 1.0, 1.0);
		renWin->AddRenderer(ren);
		vtkWidget->SetRenderWindow(renWin);

		QCheckBox* toggleMainRender = new QCheckBox(QFileInfo(csvFile).fileName());
		toggleMainRender->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
		toggleMainRender->setProperty("resultID", curLine);
		connect(toggleMainRender, &QCheckBox::stateChanged, this, &iAFiberOptimizationExplorer::toggleVis);
		resultsListLayout->addWidget(toggleMainRender, curLine, 0);
		resultsListLayout->addWidget(vtkWidget, curLine, 1);

		QSharedPointer<iA3DCylinderObjectVis> vis(new iA3DCylinderObjectVis(vtkWidget, tableCreator.getTable(), io.getOutputMapping(), m_colorTheme->GetColor(curLine)));
		vis->show();
		m_vtkWidgets.push_back(vtkWidget);
		m_mini3DVis.push_back(vis);
		m_resultTables.push_back(tableCreator.getTable());
		m_outputMappings.push_back(io.getOutputMapping());
		
		++curLine;
	}
	QWidget* resultList = new QWidget();
	resultList->setLayout(resultsListLayout);

	iADockWidgetWrapper* main3DView = new iADockWidgetWrapper(m_mainRenderer, "3D view", "foe3DView");
	addDockWidget(Qt::RightDockWidgetArea, main3DView);

	iADockWidgetWrapper* resultListDockWidget = new iADockWidgetWrapper(resultList, "Result list", "foeResultList");
	addDockWidget(Qt::LeftDockWidgetArea, resultListDockWidget);
}

void iAFiberOptimizationExplorer::toggleVis(int state)
{
	int resultID = QObject::sender()->property("resultID").toInt();
	DEBUG_LOG(QString("TogleVis: Result %1 - state=%2").arg(resultID).arg(state));
	if (state == Qt::Checked)
	{
		if (m_main3DVis.contains(resultID))
		{
			DEBUG_LOG("Visualization already exists!");
			return;
		}
		DEBUG_LOG("Showing Vis.");
		QColor color = m_colorTheme->GetColor(resultID);
		color.setAlpha(128);
		QSharedPointer<iA3DCylinderObjectVis> vis(new iA3DCylinderObjectVis(m_mainRenderer,
				m_resultTables[resultID], m_outputMappings[resultID], color));
		vis->show();
		m_main3DVis.insert(resultID, vis);
	}
	else
	{
		if (!m_main3DVis.contains(resultID))
		{
			DEBUG_LOG("Visualization not found!");
			return;
		}
		DEBUG_LOG("Hiding Vis.");
		m_main3DVis[resultID]->hide();
		m_main3DVis.remove(resultID);
	}
	m_mainRenderer->GetRenderWindow()->Render();
	m_mainRenderer->update();
}
