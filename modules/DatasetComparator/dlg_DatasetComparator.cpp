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
#include "defines.h"
#include "dlg_DatasetComparator.h"
#include "iAColorTheme.h"
#include "iAHistogramWidget.h"
#include "iAIntensityMapper.h"
#include "iARenderer.h"

#include "qcustomplot.h"

//#include <omp.h>

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkImageData.h>
#include <vtkLine.h>
#include <vtkPolyData.h>

#include <QMap>
#include <QList>
#include <QColor>

//#include <sys/timeb.h>
//#include "iAConsole.h"

#include <itkImageBase.h>
#include <itkImageIOBase.h>
#include "iATypedCallHelper.h"


const double golden_ratio = 0.618033988749895;

dlg_DatasetComparator::dlg_DatasetComparator( QWidget * parent /*= 0*/, QDir datasetsDir, Qt::WindowFlags f /*= 0 */ )
	: DatasetComparatorConnector( parent, f ), 
	m_mdiChild(static_cast<MdiChild*>(parent)),
	m_datasetsDir(datasetsDir)
{
	mapIntensities();

	customPlot = new QCustomPlot(dockWidgetContents);
	customPlot->setOpenGl(false);
	customPlot->legend->setVisible(true);
	customPlot->legend->setFont(QFont("Helvetica", 11));
	customPlot->xAxis->setLabel("Hilbert index");
	customPlot->yAxis->setLabel("Intensity valueis label");
	customPlot->xAxis2->setVisible(true);
	customPlot->xAxis2->setTickLabels(false);
	customPlot->yAxis2->setVisible(true);
	customPlot->yAxis2->setTickLabels(false);
	customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	customPlot->setMultiSelectModifier(Qt::ShiftModifier);
	connect(customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->xAxis2, SLOT(setRange(QCPRange)));
	connect(customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), customPlot->yAxis2, SLOT(setRange(QCPRange)));
	connect(customPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));

	connect(pB_Update, SIGNAL(clicked()), this, SLOT(updateDatasetComparator()));
}

dlg_DatasetComparator::~dlg_DatasetComparator()
{}

void dlg_DatasetComparator::showLinePlots()
{
	customPlot->removeGraph(0);
	QList<QPair<QString, QList<icData>>>::iterator it;
	iAColorTheme const * theme = iAColorThemeManager::GetInstance().GetTheme("Brewer Qualitaive 1 (max. 8)");
	QColor graphPenColor;
	QPen graphPen;
	//graphPen.setWidth(3);	// Make sure qcustomplot uses opengl (or set width to 0), otherwise bad performance
	int datasetIdx = 0;
	for (it = m_DatasetIntensityMap.begin(); it != m_DatasetIntensityMap.end(); ++it)
	{
		if (m_DatasetIntensityMap.size() <= theme->size())
		{
			graphPenColor = theme->GetColor(datasetIdx);
			++datasetIdx;
		}
		else
		{
			// https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/
			float h = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			h += golden_ratio;
			h = fmodf(h, 1.0);
			graphPenColor = QColor::fromHsvF(h, 0.95, 0.95, 1.0);
		}
		customPlot->addGraph();
		connect(customPlot->graph(), SIGNAL(selectionChanged(const QCPDataSelection & )), 
			this, SLOT(selectionChanged(const QCPDataSelection & )));
		customPlot->graph()->setSelectable(QCP::stMultipleDataRanges);
		graphPen.setColor(graphPenColor);
		customPlot->graph()->setPen(graphPen);
		customPlot->graph()->setName(it->first);
		QList<icData> l = it->second;
		QSharedPointer<QCPGraphDataContainer> emaData(new QCPGraphDataContainer);
		for (unsigned int i = 0; i < l.size(); ++i)
			emaData->add(QCPGraphData(double(i), l[i].intensity));
		customPlot->graph()->setData(emaData);
	}
	customPlot->graph(0)->rescaleAxes();
	PlotsContainer_verticalLayout->addWidget(customPlot);
}

void dlg_DatasetComparator::visualizePath()
{
	vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
	QString str = m_DatasetIntensityMap.at(0).first;
	unsigned int pathSteps = m_DatasetIntensityMap.at(0).second.size();
	QList<icData>  data = m_DatasetIntensityMap.at(0).second;
	for (unsigned int i = 0; i < pathSteps; ++i)
	{
		double point[3] = { (double) data[i].x, (double) data[i].y, (double)data[i].z };
		pts->InsertNextPoint(point);
	}

	vtkSmartPointer<vtkPolyData> linesPolyData = vtkSmartPointer<vtkPolyData>::New();
	linesPolyData->SetPoints(pts);
	vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
	for (unsigned int i = 0; i < pathSteps - 1; ++i)
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

void dlg_DatasetComparator::updateDatasetComparator()
{
	mapIntensities();
}

void dlg_DatasetComparator::mapIntensities()
{
	QThread* thread = new QThread;
	iAIntensityMapper * im = new iAIntensityMapper(m_datasetsDir, PathNameToId[cb_Paths->currentText()], m_DatasetIntensityMap);
	im->moveToThread(thread);
	connect(im, SIGNAL(error(QString)), this, SLOT(errorString(QString)));		//TODO: Handle error case
	connect(thread, SIGNAL(started()), im, SLOT(process()));
	connect(im, SIGNAL(finished()), thread, SLOT(quit()));
	connect(im, SIGNAL(finished()), im, SLOT(deleteLater()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	//connect(thread, SIGNAL(finished()), this, SLOT(visualizePath()));
	connect(thread, SIGNAL(finished()), this, SLOT(showLinePlots()));
	thread->start();
}

void dlg_DatasetComparator::mousePress(QMouseEvent* e)
{
	if ((e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier)
		customPlot->setSelectionRectMode(QCP::srmSelect);
	else
		customPlot->setSelectionRectMode(QCP::srmNone);
}

template <typename  T>
void setVoxelIntensity(
	vtkImageData* inputImage, unsigned int x, unsigned int y, 
	unsigned int z, double intensity)
{
	T *v = static_cast< T* >(inputImage->GetScalarPointer(x, y, z));
	*v = intensity;
}

void dlg_DatasetComparator::selectionChanged(const QCPDataSelection & selection)
{
	// TODO: only process active curve/dataset 
	// TODO: change transfer function for "hiden" values should be HistogramRangeMinimum-1 

	QList< 	QCPDataRange> selHilberIndices = selection.dataRanges();
	int* dims = m_mdiChild->getImagePointer()->GetDimensions();
	QString str = m_DatasetIntensityMap.at(0).first;
	unsigned int pathSteps = m_DatasetIntensityMap.at(0).second.size();
	QList<icData>  data = m_DatasetIntensityMap.at(0).second;
	int scalarType = m_mdiChild->getImagePointer()->GetScalarType();

	if (selHilberIndices.size() < 1)
	{
		for (unsigned int i = 0; i < pathSteps; ++i)
			VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_mdiChild->getImagePointer(),
				data[i].x, data[i].y, data[i].z, data[i].intensity);
	}
	else
	{
		double r[2];
		m_mdiChild->getHistogram()->GetDataRange(r);
		for (unsigned int i = 0; i < pathSteps; ++i)
		{
			bool showVoxel = false;
			for (int j = 0; j < selHilberIndices.size(); ++j)
			{
				if (i >= selHilberIndices.at(j).begin() && i <= selHilberIndices.at(j).end())
				{
					VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_mdiChild->getImagePointer(),
						data[i].x, data[i].y, data[i].z, data[i].intensity);
					showVoxel = true;
					break;
				}
			}
			if (!showVoxel)
				VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_mdiChild->getImagePointer(),
					data[i].x, data[i].y, data[i].z, r[0]);
		}
	}
	m_mdiChild->getImagePointer()->Modified();
	m_mdiChild->getRaycaster()->update();
	m_mdiChild->updateSlicers();
}

