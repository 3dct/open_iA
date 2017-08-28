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
#include "dlg_DatasetComparator.h"
#include "iAColorTheme.h"
#include "iAIntensityMapper.h"
#include "defines.h"
#include "qcustomplot.h"
#include "iARenderer.h"

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkPolyData.h>

#include <QMap>
#include <QList>
#include <QColor>

const double golden_ratio = 0.618033988749895;

dlg_DatasetComparator::dlg_DatasetComparator( QWidget * parent /*= 0*/, QDir datasetsDir, Qt::WindowFlags f /*= 0 */ )
	: DatasetComparatorConnector( parent, f ), 
	m_mdiChild(static_cast<MdiChild*>(parent)),
	m_datasetsDir(datasetsDir)
{
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
	connect(thread, SIGNAL(finished()), this, SLOT(visualizeHilbertPath()));
	connect(thread, SIGNAL(finished()), this, SLOT(showHilbertLinePlots()));
	thread->start();
}

dlg_DatasetComparator::~dlg_DatasetComparator()
{}

void dlg_DatasetComparator::showHilbertLinePlots()
{
	QCustomPlot * customPlot = new QCustomPlot(dockWidgetContents);
	customPlot->legend->setVisible(true);
	customPlot->legend->setFont(QFont("Helvetica", 11));
	customPlot->xAxis->setLabel("Hilbert index");
	customPlot->yAxis->setLabel("Intensity valueis label");
	customPlot->xAxis2->setVisible(true);
	customPlot->xAxis2->setTickLabels(false);
	customPlot->yAxis2->setVisible(true);
	customPlot->yAxis2->setTickLabels(false);
	connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
	connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));
	customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables);

	QMap<QString, QList<int>>::iterator it;
	iAColorTheme const * theme = iAColorThemeManager::GetInstance().GetTheme("Brewer Qualitaive 1 (max. 8)");
	QColor graphPenColor;
	QPen graphPen;
	graphPen.setWidth(3);
	int datasetIdx = 0;
	for (it = m_DatasetIntensityMap.begin(); it != m_DatasetIntensityMap.end(); ++it)
	{
		if (m_DatasetIntensityMap.size() <= theme->size())
		{
			graphPenColor = theme->GetColor(datasetIdx);
		}
		else
		{
			//TODO: Use predefined colors, e.g., from color brewer or pantone
			// https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
			float h = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			h += golden_ratio;
			h = fmodf(h, 1.0);
			graphPenColor = QColor::fromHsvF(h, 0.95, 0.95, 1.0);
		}
		customPlot->addGraph();
		graphPen.setColor(graphPenColor);
		customPlot->graph(datasetIdx)->setPen(graphPen);
		customPlot->graph(datasetIdx)->setName(it.key());
		QList<int> l = it.value();
		QVector<double> x(l.size()), y(l.size());
		for (int i = 0; i < l.size(); ++i)
		{
			x[i] = i;
			y[i] = l[i];
		}
		customPlot->graph(datasetIdx)->setData(x, y);
		++datasetIdx;
	}
	customPlot->graph(0)->rescaleAxes();
	PlotsContainer_verticalLayout->addWidget(customPlot);
}

void dlg_DatasetComparator::visualizeHilbertPath()
{
	// TODO: Delete unused polydata! + consider Spacing?
	vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
	QString str = m_DatasetIntensityMap.firstKey();
	unsigned int pathSteps = m_DatasetIntensityMap.values(str).at(0).size();

	for (unsigned int i = 0; i < pathSteps; ++i)
	{
		double point[3] = { (double) m_HPath->EvaluateToIndex(i)[0],
			(double) m_HPath->EvaluateToIndex(i)[1],
			(double) m_HPath->EvaluateToIndex(i)[2] };
		pts->InsertNextPoint(point);
	}

	vtkSmartPointer<vtkPolyData> linesPolyData = vtkSmartPointer<vtkPolyData>::New();
	linesPolyData->SetPoints(pts);
	vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
	for (int i = 0; i < pathSteps - 1; ++i)
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