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

#include "iAConsole.h"
#include "io/iAFileUtils.h"
#include "iARendererManager.h"

#include <QVTKOpenGLWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
//#include <QVBoxLayout>

iAFiberOptimizationExplorer::iAFiberOptimizationExplorer(QString const & path)
{
	//QVBoxLayout* mainLayout = new QVBoxLayout();
	//setLayout(mainLayout);
	//QScrollArea* scrollArea = new QScrollArea();
	//mainLayout->addWidget(scrollArea);
	//scrollArea->setWidgetResizable(true);
	//QWidget* resultsListWidget = new QWidget();
	//scrollArea->setWidget(resultsListWidget);

	QGridLayout* resultsListLayout = new QGridLayout();
	//resultsListWidget->setLayout(resultsListLayout);

	QStringList filters;
	filters << "*.csv";
	QStringList csvFileNames;
	FindFiles(path, filters, false, csvFileNames, Files);

	int curLine = 0;
	m_renderManager = QSharedPointer<iARendererManager>(new iARendererManager());
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

		resultsListLayout->addWidget(new QLabel(QFileInfo(csvFile).fileName()), curLine, 0);
		resultsListLayout->addWidget(vtkWidget, curLine, 1);

		QSharedPointer<iA3DCylinderObjectVis> vis(new iA3DCylinderObjectVis(vtkWidget, tableCreator.getTable(), io.getOutputMapping(), QColor(128, 128, 128)));
		vis->show();
		m_vtkWidgets.push_back(vtkWidget);
		m_cylinderVis.push_back(vis);
		
		++curLine;
	}
	setLayout(resultsListLayout);
}