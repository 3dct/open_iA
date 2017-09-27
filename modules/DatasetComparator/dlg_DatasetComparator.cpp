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
#include "iATransferFunction.h"
#include "iAVolumeRenderer.h"
#include "iATypedCallHelper.h"
#include "iAFunction.h"
#include "iAFunctionalBoxplot.h"

//#include <omp.h>

#include <vtkPoints.h>
#include <vtkCellArray.h>
#include <vtkImageData.h>
#include <vtkLine.h>
#include <vtkPolyData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkAbstractVolumeMapper.h>
#include <vtkVolumeProperty.h>
#include <vtkProperty.h>
#include <vtkCornerAnnotation.h>
#include <vtkTextProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkTextActor.h>

#include <QMap>
#include <QList>
#include <QColor>

//#include <sys/timeb.h>
//#include "iAConsole.h"

const double golden_ratio = 0.618033988749895;

dlg_DatasetComparator::dlg_DatasetComparator( QWidget * parent /*= 0*/, QDir datasetsDir, Qt::WindowFlags f /*= 0 */ )
	: DatasetComparatorConnector( parent, f ), 
	m_mdiChild(static_cast<MdiChild*>(parent)),
	m_datasetsDir(datasetsDir),
	m_customPlot(new QCustomPlot(dockWidgetContents)),
	m_dataPointInfo(new QCPItemText(m_customPlot)),
	m_MultiRendererView(new multi3DRendererView()),
	m_renderWindow(vtkSmartPointer<vtkRenderWindow>::New())
{
	m_renderWindow->SetNumberOfLayers(2);
	m_MultiRendererView->wgtContainer->SetRenderWindow(m_renderWindow);
	auto renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	renderWindowInteractor->SetRenderWindow(m_renderWindow);
	auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	renderWindowInteractor->SetInteractorStyle(style);
	m_mdiChild->tabifyDockWidget(m_mdiChild->r, m_MultiRendererView);
	m_MultiRendererView->show();
	
	QFont m_dataPointInfoFont(font().family(), 11);
	m_dataPointInfoFont.setBold(true);
	m_dataPointInfo->setFont(m_dataPointInfoFont);
	m_dataPointInfo->setLayer("main");
	sl_fbpTransparency->hide();

	mapIntensities();

	m_customPlot->setOpenGl(false);
	m_customPlot->setBackground(Qt::darkGray);
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
	connect(m_customPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));
	connect(m_customPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChangedByUser()));

	connect(pB_Update, SIGNAL(clicked()), this, SLOT(updateDatasetComparator()));
	connect(cb_showFbp, SIGNAL(stateChanged(int )), this, SLOT(showFBPGraphs()));
	connect(cb_fbpView, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFBPView()));
	connect(sl_fbpTransparency, SIGNAL(valueChanged(int)), this, SLOT(setFbpTransparency(int)));
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
	graphPen.setWidth(2);  // For better perfromance setOpenGl(true) on qcustomplot;
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
		m_customPlot->graph()->selectionDecorator()->setPen(p);  // Selection color: orange

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
	auto pts = vtkSmartPointer<vtkPoints>::New();
	unsigned int pathSteps = m_DatasetIntensityMap.at(0).second.size();
	QList<icData>  data = m_DatasetIntensityMap.at(0).second;
	for (unsigned int i = 0; i < pathSteps; ++i)
	{
		double point[3] = { (double) data[i].x, (double) data[i].y, (double)data[i].z };
		pts->InsertNextPoint(point);
	}

	auto linesPolyData = vtkSmartPointer<vtkPolyData>::New();
	linesPolyData->SetPoints(pts);
	auto lines = vtkSmartPointer<vtkCellArray>::New();
	for (unsigned int i = 0; i < pathSteps - 1; ++i)
	{
		auto line = vtkSmartPointer<vtkLine>::New();
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
	m_imgDataList.clear();
	m_DatasetIntensityMap.clear();
	mapIntensities();
}

void dlg_DatasetComparator::mapIntensities()
{
	QThread* thread = new QThread;
	iAIntensityMapper * im = new iAIntensityMapper(m_datasetsDir, PathNameToId[cb_Paths->currentText()], m_DatasetIntensityMap, m_imgDataList);
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

void dlg_DatasetComparator::mouseMove(QMouseEvent* e)
{
	if (cb_showFbp->isChecked() && cb_fbpView->currentText() == "Alone")
		return;

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
		m_dataPointInfo->setLayer("overlay");
		m_dataPointInfo->setVisible(true);
	}
	else
		m_dataPointInfo->setVisible(false);
	m_customPlot->replot();
}

template <typename  T>
void setVoxelIntensity(
	vtkImageData* inputImage, unsigned int x, unsigned int y, 
	unsigned int z, double intensity)
{
	T *v = static_cast< T* >(inputImage->GetScalarPointer(x, y, z));
	*v = intensity;
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
	medianPen.setWidth(5);
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
	maxPen.setWidth(3);
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
	minPen.setWidth(3);
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
			sl_fbpTransparency->show();
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
				m_customPlot->graph(i)->removeFromLegend();
				if (i < m_DatasetIntensityMap.size())
				{
					m_customPlot->graph(i)->setVisible(true);
					m_customPlot->graph(i)->addToLegend();
				}
				else
				{
					m_customPlot->graph(i)->setLayer("background");
					m_customPlot->graph(i)->setVisible(true);
					if (m_customPlot->graph(i)->name() != "Third Quartile")
						m_customPlot->graph(i)->addToLegend();
				}
			}
		}
		else
		{
			sl_fbpTransparency->hide();
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

void dlg_DatasetComparator::updateFBPView()
{
	if (cb_showFbp->isChecked())
		showFBPGraphs();
}

void dlg_DatasetComparator::setFbpTransparency(int value)
{
	double alpha = round(value * 255 / 100.0);
	QPen p; QColor c; QBrush b;
	for (int i = m_DatasetIntensityMap.size(); i < m_customPlot->graphCount(); ++i)
	{
		p = m_customPlot->graph(i)->pen();
		c = m_customPlot->graph(i)->pen().color();
		c.setAlpha(alpha);
		p.setColor(c);
		m_customPlot->graph(i)->setPen(p);
		if (m_customPlot->graph(i)->name() == "Interquartile Range")
		{
			b = m_customPlot->graph(i)->brush();
			c = m_customPlot->graph(i)->brush().color();
			c.setAlpha(alpha);
			b.setColor(c);
			m_customPlot->graph(i)->setBrush(b);
		}
	}
	m_customPlot->replot();
}

void dlg_DatasetComparator::selectionChangedByUser()
{
	// TODO[?]: only process active curve/dataset
	// TODO: change transfer function for "hiden" values should be HistogramRangeMinimum-1 
	
	m_renderWindow->GetRenderers()->RemoveAllItems();

	auto backgroundRenderer = vtkSmartPointer<vtkRenderer>::New();
	backgroundRenderer->SetLayer(0);
	backgroundRenderer->InteractiveOff();
	backgroundRenderer->SetBackground(1.0, 1.0, 1.0);
	m_renderWindow->AddRenderer(backgroundRenderer);

	QList<QCPGraph *> selGraphsList = m_customPlot->selectedGraphs();
	QStringList datasetsList = m_datasetsDir.entryList();

	for (unsigned int i = 0; i < selGraphsList.size(); ++i)
	{
		int idx = datasetsList.indexOf(selGraphsList[i]->name());
		QList<QCPDataRange> selHilberIndices = selGraphsList[i]->selection().dataRanges();
		unsigned int pathSteps = m_DatasetIntensityMap.at(idx).second.size(); 
		QList<icData>  data = m_DatasetIntensityMap.at(idx).second;
		int scalarType = m_imgDataList[idx]->GetScalarType();

		if (selHilberIndices.size() < 1)
		{
			for (unsigned int i = 0; i < pathSteps; ++i)
				VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_imgDataList[idx],
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
						VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_imgDataList[idx],
							data[i].x, data[i].y, data[i].z, data[i].intensity);
						showVoxel = true;
						break;
					}
				}
				if (!showVoxel)
					VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_imgDataList[idx],
						data[i].x, data[i].y, data[i].z, r[0]);
			}
		}
		
		m_imgDataList[idx]->Modified();
		float viewportColumns = selGraphsList.size() < 3.0 ? fmod(selGraphsList.size(), 3.0) : 3.0;
		float viewportRows = ceil(selGraphsList.size() / viewportColumns);
		float fieldLengthX = 1.0 / viewportColumns, fieldLengthY = 1.0 / viewportRows;
		
		auto cornerAnnotation = vtkSmartPointer<vtkCornerAnnotation>::New();
		cornerAnnotation->SetLinearFontScaleFactor(2);
		cornerAnnotation->SetNonlinearFontScaleFactor(1);
		cornerAnnotation->SetMaximumFontSize(14);
		cornerAnnotation->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
		cornerAnnotation->GetTextProperty()->SetFontSize(14);
		cornerAnnotation->SetText(2, selGraphsList[i]->name().toStdString().c_str());
		cornerAnnotation->GetTextProperty()->BoldOn();
		cornerAnnotation->GetTextProperty()->SetColor(
			selGraphsList[i]->pen().color().redF(),
			selGraphsList[i]->pen().color().greenF(),
			selGraphsList[i]->pen().color().blueF());
		
		iASimpleTransferFunction tf(m_mdiChild->getColorTransferFunction(), m_mdiChild->getPiecewiseFunction());
		auto renderer = vtkSmartPointer<vtkRenderer>::New();
		renderer->SetLayer(1);
		renderer->SetActiveCamera(m_mdiChild->getRaycaster()->getCamera());
		renderer->GetActiveCamera()->ParallelProjectionOn();
		renderer->SetViewport(fmod(i, viewportColumns) * fieldLengthX,
			1 - (ceil((i + 1.0) / viewportColumns) / viewportRows),
			fmod(i, viewportColumns) * fieldLengthX + fieldLengthX,
			1 - (ceil((i + 1.0) / viewportColumns) / viewportRows) + fieldLengthY);
		renderer->AddViewProp(cornerAnnotation);
		renderer->ResetCamera();
		
		m_volumeRenderer = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(&tf, m_imgDataList[idx]));
		m_volumeRenderer->ApplySettings(m_mdiChild->GetVolumeSettings());
		m_volumeRenderer->AddTo(renderer);
		m_volumeRenderer->AddBoundingBoxTo(renderer);
		m_renderWindow->AddRenderer(renderer);
	}
	m_renderWindow->Render();
}