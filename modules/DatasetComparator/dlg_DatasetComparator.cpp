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

#include "iAFunction.h"
#include "iAFunctionalBoxplot.h"


const double golden_ratio = 0.618033988749895;

dlg_DatasetComparator::dlg_DatasetComparator( QWidget * parent /*= 0*/, QDir datasetsDir, Qt::WindowFlags f /*= 0 */ )
	: DatasetComparatorConnector( parent, f ), 
	m_mdiChild(static_cast<MdiChild*>(parent)),
	m_datasetsDir(datasetsDir),
	m_customPlot(new QCustomPlot(dockWidgetContents)),
	m_dataPointInfo(new QCPItemText(m_customPlot))
{
	QFont m_dataPointInfoFont(font().family(), 11);
	m_dataPointInfoFont.setBold(true);
	m_dataPointInfo->setFont(m_dataPointInfoFont);
	m_dataPointInfo->setLayer("main");
	
	mapIntensities();
	
	m_customPlot->setOpenGl(false);
	//customPlot->setBackground(Qt::darkGray);
	m_customPlot->setPlottingHints(QCP::phFastPolylines);  // Graph/Curve lines are drawn with a faster method
	m_customPlot->legend->setVisible(true);
	m_customPlot->legend->setFont(QFont("Helvetica", 11));
	m_customPlot->xAxis->setLabel("Hilbert index");
	m_customPlot->yAxis->setLabel("Intensity valueis label");
	m_customPlot->xAxis2->setVisible(true);
	m_customPlot->xAxis2->setTickLabels(false);
	m_customPlot->yAxis2->setVisible(true);
	m_customPlot->yAxis2->setTickLabels(false);
	m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	m_customPlot->setMultiSelectModifier(Qt::ShiftModifier);
	connect(m_customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), m_customPlot->xAxis2, SLOT(setRange(QCPRange)));
	connect(m_customPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), m_customPlot->yAxis2, SLOT(setRange(QCPRange)));
	connect(m_customPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
	connect(m_customPlot, SIGNAL(plottableClick(QCPAbstractPlottable*, int, QMouseEvent*)), this, SLOT(graphClicked(QCPAbstractPlottable*, int, QMouseEvent*)));

	connect(m_customPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));

	connect(pB_Update, SIGNAL(clicked()), this, SLOT(updateDatasetComparator()));
	connect(cb_showFbp, SIGNAL(stateChanged(int )), this, SLOT(showFBPGraphs()));
}

dlg_DatasetComparator::~dlg_DatasetComparator()
{}

void dlg_DatasetComparator::showLinePlots()
{
	m_customPlot->clearGraphs();
	QList<QPair<QString, QList<icData>>>::iterator it;
	iAColorTheme const * theme = iAColorThemeManager::GetInstance().GetTheme("Metro Colors (max. 20)");
	QColor graphPenColor;
	QPen graphPen;
	graphPen.setWidth(2);	// Make sure qcustomplot uses opengl (or set width to 0), otherwise bad performance
	int datasetIdx = 0;
	std::vector<iAFunction<unsigned int, double> *> functions;
	
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

		m_customPlot->addGraph();
		connect(m_customPlot->graph(), SIGNAL(selectionChanged(const QCPDataSelection &)),
			this, SLOT(selectionChanged(const QCPDataSelection &)));
		m_customPlot->graph()->setSelectable(QCP::stMultipleDataRanges);
		graphPen.setColor(graphPenColor);
		m_customPlot->graph()->setPen(graphPen);
		m_customPlot->graph()->setName(it->first);
		QPen p = m_customPlot->graph()->selectionDecorator()->pen();
		p.setColor(QColor(254, 153, 41));
		m_customPlot->graph()->selectionDecorator()->setPen(p);  // orange

		QList<icData> l = it->second;
		QSharedPointer<QCPGraphDataContainer> graphData(new QCPGraphDataContainer);
		iAFunction<unsigned int, double> * funct = new iAFunction<unsigned int, double>(l.size());
		
		for (unsigned int i = 0; i < l.size(); ++i)
		{
			graphData->add(QCPGraphData(double(i), l[i].intensity));
			funct->set(i, l[i].intensity);
		}
		functions.push_back(funct);
		m_customPlot->graph()->setData(graphData);
	}

	ModifiedDepthMeasure<unsigned int, double> measure;
	auto functionalBoxplotData = new iAFunctionalBoxplot<unsigned int, double>(
		functions, 0, m_DatasetIntensityMap[0].second.size() - 1, &measure, 2);
	createFBPGraphs(functionalBoxplotData);

	m_customPlot->graph(0)->rescaleAxes();
	m_customPlot->replot();
	PlotsContainer_verticalLayout->addWidget(m_customPlot);
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
	m_DatasetIntensityMap.clear();
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
		m_customPlot->setSelectionRectMode(QCP::srmSelect);
	else
		m_customPlot->setSelectionRectMode(QCP::srmNone);
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
		m_dataPointInfo->setVisible(false);
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

void dlg_DatasetComparator::createFBPGraphs(iAFunctionalBoxplot<unsigned int, double> * fbpData)
{
	QSharedPointer<QCPGraphDataContainer> fb_075Data(new QCPGraphDataContainer);
	for (unsigned int i = 0; i < fbpData->getMedian().size(); ++i)
		fb_075Data->add(QCPGraphData(double(i), fbpData->getCentralRegion().getMax(i)));
	m_customPlot->addGraph();
	m_customPlot->graph()->setVisible(false);
	m_customPlot->graph()->removeFromLegend();
	m_customPlot->graph()->setData(fb_075Data);
	m_customPlot->graph()->setName("Third Quartile");
	QPen quantilePen;
	quantilePen.setColor(QColor(200, 200, 200, 255));
	m_customPlot->graph()->setPen(quantilePen);
	m_customPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_025Data(new QCPGraphDataContainer);
	for (unsigned int i = 0; i < fbpData->getMedian().size(); ++i)
		fb_025Data->add(QCPGraphData(double(i), fbpData->getCentralRegion().getMin(i)));
	m_customPlot->addGraph();
	m_customPlot->graph()->setVisible(false);
	m_customPlot->graph()->removeFromLegend();
	m_customPlot->graph()->setData(fb_025Data);
	m_customPlot->graph()->setName("Interquartile Range"); 
	m_customPlot->graph()->setPen(quantilePen);
	m_customPlot->graph()->setBrush(QColor(200, 200, 200, 255));
	m_customPlot->graph()->setChannelFillGraph(m_customPlot->graph(m_customPlot->graphCount() - 2));
	m_customPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_medianData(new QCPGraphDataContainer);
	for (unsigned int i = 0; i < fbpData->getMedian().size(); ++i)
		fb_medianData->add(QCPGraphData(double(i), fbpData->getMedian().get(i)));
	m_customPlot->addGraph();
	m_customPlot->graph()->setVisible(false);
	m_customPlot->graph()->removeFromLegend();
	m_customPlot->graph()->setName("Median");
	QPen medianPen;
	medianPen.setColor(QColor(0, 0, 0, 255));
	medianPen.setWidth(7);
	m_customPlot->graph()->setPen(medianPen);
	m_customPlot->graph()->setData(fb_medianData);
	m_customPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_MaxData(new QCPGraphDataContainer);
	for (unsigned int i = 0; i < fbpData->getMedian().size(); ++i)
		fb_MaxData->add(QCPGraphData(double(i), fbpData->getEnvelope().getMax(i)));
	m_customPlot->addGraph();
	m_customPlot->graph()->setVisible(false);
	m_customPlot->graph()->removeFromLegend();
	m_customPlot->graph()->setData(fb_MaxData);
	m_customPlot->graph()->setName("Max");
	QPen maxPen;
	maxPen.setColor(QColor(255, 0, 0, 255));
	maxPen.setWidth(5);
	m_customPlot->graph()->setPen(maxPen);
	m_customPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_MinData(new QCPGraphDataContainer);
	for (unsigned int i = 0; i < fbpData->getMedian().size(); ++i)
		fb_MinData->add(QCPGraphData(double(i), fbpData->getEnvelope().getMin(i)));
	m_customPlot->addGraph();
	m_customPlot->graph()->setVisible(false);
	m_customPlot->graph()->removeFromLegend();
	m_customPlot->graph()->setData(fb_MinData);
	m_customPlot->graph()->setName("Min");
	QPen minPen;
	minPen.setColor(QColor(0, 0, 255, 255));
	minPen.setWidth(5);
	m_customPlot->graph()->setPen(minPen);
	m_customPlot->graph()->setSelectable(QCP::stNone);
}

void dlg_DatasetComparator::showFBPGraphs()
{
	int graphCnt = m_customPlot->graphCount();
	for (int i = 0; i < graphCnt; ++i)
	{
		if (cb_showFbp->isChecked())
		{
			if (cb_fbpView->currentText() == "Alone")
			{
				if (i >= m_DatasetIntensityMap.size())
				{
					m_customPlot->graph(i)->setVisible(true);
					if (m_customPlot->graph(i)->name() != "Third Quartile")
						m_customPlot->graph(i)->addToLegend();
				}
				else
				{
					m_customPlot->graph(i)->setVisible(false);
					m_customPlot->graph(i)->removeFromLegend();
				}
			}
			else
			{
				if (i >= m_DatasetIntensityMap.size())
				{
					m_customPlot->graph(i)->setLayer("background");
					m_customPlot->graph(i)->setVisible(true);
					if(m_customPlot->graph(i)->name() != "Third Quartile")
						m_customPlot->graph(i)->addToLegend();
				}
			}
		}
		else
		{
			if (i >= m_DatasetIntensityMap.size())
			{
				m_customPlot->graph(i)->setVisible(false);
				m_customPlot->graph(i)->removeFromLegend();
			}
			else
			{
				m_customPlot->graph(i)->setVisible(true);
				m_customPlot->graph(i)->addToLegend();
			}
		}
	}
	m_customPlot->replot();
}

void dlg_DatasetComparator::mouseMove(QMouseEvent* e)
{
	QPoint p(e->pos().x(), e->pos().y());
	QList<double> l;
	for (int i = 0; i < m_DatasetIntensityMap.size(); ++i)
		l.push_back(m_customPlot->graph(i)->selectTest(p, true));
	auto minDistance = *std::min_element(std::begin(l), std::end(l));
	if (minDistance >= 0 && minDistance < 2.0)
	{
		auto idx = l.indexOf(minDistance);
		auto x = m_customPlot->xAxis->pixelToCoord(e->pos().x());
		auto y = m_customPlot->yAxis->pixelToCoord(e->pos().y());
		m_dataPointInfo->setText(QString("%1:").arg(m_customPlot->graph(idx)->name()));
		m_dataPointInfo->position->setCoords(QPointF(x, y));
		m_dataPointInfo->setVisible(true);
	}
	else
		m_dataPointInfo->setVisible(false);
	m_customPlot->replot();
}
