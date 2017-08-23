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
#include "pch.h"
#include "iADatasetComparatorModuleInterface.h"

#include "mainwindow.h"
#include "mdichild.h"
#include "iAIntensityMapper.h"
#include "iARenderer.h"

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkPolyData.h>

#include <QFileDialog>
#include <QSettings>
#include <QThread>

void iADatasetComparatorModuleInterface::Initialize()
{
	QMenu * toolsMenu = m_mainWnd->getToolsMenu();
	QMenu * menuDatasetComparator = getMenuWithTitle(toolsMenu, QString("DatasetComparator"));
	QAction * actionDatasetComparator = new QAction(QApplication::translate("MainWindow", "DatasetComparator", 0), m_mainWnd);
	menuDatasetComparator->addAction(actionDatasetComparator);
	connect(actionDatasetComparator, SIGNAL(triggered()), this, SLOT(DatasetComparator()));
}

void iADatasetComparatorModuleInterface::DatasetComparator()
{
	PrepareActiveChild();

	QSettings settings;
	QString datasetsFolder = settings.value("Tools/DatasetComparator/DatasetsFolder").toString();

	QString datasetsPath = QFileDialog::getExistingDirectory(m_mdiChild, tr("Datasets Directory"),
		datasetsFolder,
		QFileDialog::ShowDirsOnly
		| QFileDialog::DontResolveSymlinks);

	settings.setValue("Tools/DatasetComparator/DatasetsFolder", datasetsPath);

	if (!datasetsPath.isEmpty())
	{
		m_datasetsDir = QDir(datasetsPath);
		m_datasetsDir.setNameFilters(QStringList("*.mhd"));

		m_HPath = PathType::New();
		m_HPath->SetHilbertOrder(3);	// for [8]^3 images
		m_HPath->Initialize();

		QThread* thread = new QThread;
		iAIntensityMapper * im = new iAIntensityMapper(this);
		im->moveToThread(thread);
		connect(im, SIGNAL(error(QString)), this, SLOT(errorString(QString)));		//TODO: Handle error case
		connect(thread, SIGNAL(started()), im, SLOT(process()));
		connect(im, SIGNAL(finished()), thread, SLOT(quit()));
		connect(im, SIGNAL(finished()), im, SLOT(deleteLater()));
		connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
		connect(thread, SIGNAL(finished()), this, SLOT(visualizeHilbertPath()));	//TODO: better solution to visulaize Hilbert path
		thread->start();
	}
}

void iADatasetComparatorModuleInterface::visualizeHilbertPath()
{
	// TODO: consider Spacing
	vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
	QString str = m_DatasetIntensityMap.firstKey();
	unsigned int pathSteps = m_DatasetIntensityMap.values(str).at(0).size();

	for (unsigned int i = 0; i < pathSteps; ++i)
	{
		
		double point[3] = {	m_HPath->EvaluateToIndex(i)[0],
			m_HPath->EvaluateToIndex(i)[1],
			m_HPath->EvaluateToIndex(i)[2] };
		pts->InsertNextPoint(point);
	}

	vtkSmartPointer<vtkPolyData> linesPolyData = vtkSmartPointer<vtkPolyData>::New();
	linesPolyData->SetPoints(pts);
	vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
	for (int i = 0; i < pathSteps; ++i)
	{
		vtkSmartPointer<vtkLine> line = vtkSmartPointer<vtkLine>::New();
		line->GetPointIds()->SetId(0, i); 
		line->GetPointIds()->SetId(1, i + 1); 
		lines->InsertNextCell(line);
	}

	linesPolyData->SetLines(lines);
	m_mdiChild->getRaycaster()->setPolyData(linesPolyData);
	m_mdiChild->getRaycaster()->update();
}