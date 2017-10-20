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
#include "iAFunction.h"
#include "iAFunctionalBoxplot.h"
#include "iAHistogramWidget.h"
#include "iAIntensityMapper.h"
#include "iANonLinearAxisTicker.h"
#include "iARenderer.h"
#include "iATransferFunction.h"
#include "iATypedCallHelper.h"
#include "iAVolumeRenderer.h"

#include <vtkAbstractVolumeMapper.h>
#include <vtkActor.h>
#include <vtkActor2DCollection.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkCornerAnnotation.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLine.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>
#include <vtkVolumeProperty.h>

//#include <omp.h>
//#include <sys/timeb.h>
//#include "iAConsole.h"


void winModCallback(vtkObject* caller, long unsigned int vtkNotUsed(eventId),
	void* vtkNotUsed(client), void* vtkNotUsed(callData))
{
	auto *r = static_cast<vtkRenderer*>(caller);
	if (!r->GetActors2D()->GetLastActor2D())
		return;
	auto r_centerX = r->GetCenter()[0];
	auto r_centerY = r->GetCenter()[1];
	r->GetActors2D()->GetLastActor2D()->SetPosition(r_centerX, r_centerY);
}

dlg_DatasetComparator::dlg_DatasetComparator( QWidget * parent /*= 0*/, QDir datasetsDir, Qt::WindowFlags f /*= 0 */ )
	: DatasetComparatorConnector( parent, f ), 
	m_mdiChild(static_cast<MdiChild*>(parent)),
	m_datasetsDir(datasetsDir),
	m_nonlinearScaledPlot(new QCustomPlot(dockWidgetContents)),
	m_dataPointInfo(new QCPItemText(m_nonlinearScaledPlot)),
	m_MultiRendererView(new multi3DRendererView()),
	m_mrvRenWin(vtkSmartPointer<vtkRenderWindow>::New()),
	m_mrvBGRen(vtkSmartPointer<vtkRenderer>::New()),
	m_mrvTxtAct(vtkSmartPointer<vtkTextActor>::New())
{
	generateHilbertIdx();
	setupQCustomPlot();
	setupGUIConnections();
	setupMultiRendererView();
}

dlg_DatasetComparator::~dlg_DatasetComparator()
{}

void dlg_DatasetComparator::setupQCustomPlot()
{
	m_nonlinearScaledPlot->installEventFilter(this);  // To catche key press event
	m_nonlinearScaledPlot->setOpenGl(true);
	m_nonlinearScaledPlot->setNoAntialiasingOnDrag(true);
	m_nonlinearScaledPlot->setNotAntialiasedElements(QCP::aeAll);
	//m_nonlinearScaledPlot->setBackground(Qt::darkGray);
	m_nonlinearScaledPlot->setPlottingHints(QCP::phFastPolylines);  // Graph/Curve lines are drawn with a faster method
	m_nonlinearScaledPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	m_nonlinearScaledPlot->setMultiSelectModifier(Qt::ShiftModifier);

	m_nonlinearScaledPlot->legend->setVisible(true);
	m_nonlinearScaledPlot->legend->setFont(QFont("Helvetica", 11));
	m_nonlinearScaledPlot->xAxis->setLabel("Hilbert index");
	m_nonlinearScaledPlot->xAxis->grid()->setVisible(false);
	m_nonlinearScaledPlot->xAxis->grid()->setSubGridVisible(false);
	m_nonlinearScaledPlot->xAxis->setTickLabels(true);
	m_nonlinearScaledPlot->xAxis->setSubTicks(true);	  // set to 'false if too many ticks
	m_nonlinearScaledPlot->xAxis->setNumberFormat("fb");  // Instead of 'f' try 'e' or 'g'
	m_nonlinearScaledPlot->xAxis->setNumberPrecision(0);
	m_nonlinearScaledPlot->yAxis->setLabel("Gray Value Intensity");
	m_nonlinearScaledPlot->yAxis->grid()->setSubGridVisible(false);
	m_nonlinearScaledPlot->yAxis->grid()->setVisible(false);
	m_nonlinearScaledPlot->xAxis2->setVisible(true);
	m_nonlinearScaledPlot->xAxis2->setTickLabels(false);
	m_nonlinearScaledPlot->xAxis2->setSubTicks(true);
	m_nonlinearScaledPlot->yAxis2->setVisible(true);
	m_nonlinearScaledPlot->yAxis2->setTickLabels(false);
	
	connect(m_nonlinearScaledPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), m_nonlinearScaledPlot->xAxis2, SLOT(setRange(QCPRange)));
	connect(m_nonlinearScaledPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), m_nonlinearScaledPlot->yAxis2, SLOT(setRange(QCPRange)));
	connect(m_nonlinearScaledPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
	connect(m_nonlinearScaledPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));
	connect(m_nonlinearScaledPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChangedByUser()));
	connect(m_nonlinearScaledPlot, SIGNAL(legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)), 
		this, SLOT(legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)));
	connect(m_mdiChild->getRaycaster(), SIGNAL(cellsSelected(vtkPoints*)),
		this, SLOT(setSelectionFromRenderer(vtkPoints*)));
}

void dlg_DatasetComparator::setupGUIConnections()
{
	connect(pB_Update, SIGNAL(clicked()), this, SLOT(updateDatasetComparator()));
	connect(cb_showFbp, SIGNAL(stateChanged(int)), this, SLOT(showFBPGraphs()));
	connect(cb_fbpView, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFBPView()));
	connect(sl_fbpTransparency, SIGNAL(valueChanged(int)), this, SLOT(setFbpTransparency(int)));
}

void dlg_DatasetComparator::setupMultiRendererView()
{
	auto mrvWinModCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	mrvWinModCallback->SetCallback(winModCallback);
	m_mrvBGRen->AddObserver(vtkCommand::ModifiedEvent, mrvWinModCallback);
	m_mrvBGRen->SetLayer(0);
	m_mrvBGRen->InteractiveOff();
	m_mrvBGRen->SetBackground(1.0, 1.0, 1.0);
	m_mrvBGRen->AddActor2D(m_mrvTxtAct);

	m_mrvTxtAct->SetInput("No Hilbert index selected");
	m_mrvTxtAct->GetTextProperty()->SetFontSize(24);
	m_mrvTxtAct->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
	m_mrvTxtAct->GetTextProperty()->SetJustificationToCentered();
	m_mrvTxtAct->GetTextProperty()->SetVerticalJustificationToCentered();
	
	m_mrvRenWin->SetNumberOfLayers(2);
	m_MultiRendererView->wgtContainer->SetRenderWindow(m_mrvRenWin);
	auto renWinInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	renWinInteractor->SetRenderWindow(m_mrvRenWin);
	auto style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
	renWinInteractor->SetInteractorStyle(style);
	m_mrvRenWin->AddRenderer(m_mrvBGRen);
	m_mrvRenWin->Render();

	m_mdiChild->tabifyDockWidget(m_mdiChild->r, m_MultiRendererView);
	m_MultiRendererView->show();
}

void dlg_DatasetComparator::showLinePlots()
{
	m_nonlinearScaledPlot->clearGraphs();
	showDebugPlot(calcNonLinearMapping(true));
	QSharedPointer<iANonLinearAxisTicker> nonLinearTicker(new iANonLinearAxisTicker);
	nonLinearTicker->setTickData(m_integralValList.toVector());
	nonLinearTicker->setAxis(m_nonlinearScaledPlot->xAxis);
	m_nonlinearScaledPlot->xAxis->setTicker(nonLinearTicker);
	m_nonlinearScaledPlot->xAxis2->setTicker(nonLinearTicker);
	std::vector<iAFunction<double, double> *> functions;
	for (auto it = m_DatasetIntensityMap.begin(); it != m_DatasetIntensityMap.end(); ++it)
	{
		m_nonlinearScaledPlot->addGraph();
		m_nonlinearScaledPlot->graph()->setSelectable(QCP::stMultipleDataRanges);
		m_nonlinearScaledPlot->graph()->setPen(getDatasetPen(it - m_DatasetIntensityMap.begin(),
			m_DatasetIntensityMap.size(), 2, "Metro Colors (max. 20)"));
		m_nonlinearScaledPlot->graph()->setName(it->first);
		QCPScatterStyle myScatter;
		myScatter.setShape(QCPScatterStyle::ssDot);	 // Check ssDisc to show single selected points
		myScatter.setSize(3.0);
		m_nonlinearScaledPlot->graph()->setScatterStyle(myScatter);
		QPen p = m_nonlinearScaledPlot->graph()->selectionDecorator()->pen();
		p.setWidth(5);
		p.setColor(QColor(255, 0, 0));  // Selection color: red
		m_nonlinearScaledPlot->graph()->selectionDecorator()->setPen(p);  

		QSharedPointer<QCPGraphDataContainer> nonlinearScaledPlotData(new QCPGraphDataContainer);
		auto * funct = new iAFunction<double, double>();
		for (int i = 0; i < m_integralValList.size(); ++i)
		{
			nonlinearScaledPlotData->add(QCPGraphData(m_integralValList[i], it->second[i].intensity));
			funct->insert(std::make_pair(m_integralValList[i], it->second[i].intensity));
		}
		functions.push_back(funct);
		m_nonlinearScaledPlot->graph()->setData(nonlinearScaledPlotData);
	}

	//TODO: check DoubleSpinBox + code below in separated function 
	for (auto it = m_bkgrdRangeList.begin(); it != m_bkgrdRangeList.end(); ++it)
	{
		QCPItemRect *xRectItem = new QCPItemRect(m_nonlinearScaledPlot);
		xRectItem->setAntialiased(false);
		xRectItem->setLayer("background");
		xRectItem->setPen(QPen(Qt::NoPen));
		xRectItem->setBrush(QBrush(Qt::lightGray));
		xRectItem->topLeft->setTypeX(QCPItemPosition::ptPlotCoords);
		xRectItem->topLeft->setTypeY(QCPItemPosition::ptAxisRectRatio);
		xRectItem->topLeft->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
		xRectItem->topLeft->setAxisRect(m_nonlinearScaledPlot->axisRect());
		xRectItem->topLeft->setCoords(it->lower, 0.0);
		xRectItem->bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
		xRectItem->bottomRight->setTypeY(QCPItemPosition::ptAxisRectRatio);
		xRectItem->bottomRight->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
		xRectItem->bottomRight->setAxisRect(m_nonlinearScaledPlot->axisRect());
		xRectItem->bottomRight->setCoords(it->upper , 1.0);
		xRectItem->setClipToAxisRect(true);  
	}

	ModifiedDepthMeasure<double, double> measure;
	auto functionalBoxplotData = new iAFunctionalBoxplot<double, double>(functions, &measure, 2);
	setupFBPGraphs(functionalBoxplotData);

	PlotsContainer_verticalLayout->addWidget(m_nonlinearScaledPlot);
	m_nonlinearScaledPlot->graph(0)->rescaleAxes();
	m_nonlinearScaledPlot->replot();
}

void dlg_DatasetComparator::showDebugPlot(bool show)
{
	//TODO: Add linear scaled Hilbert plot
	
	if (!show) return;

	m_helperPlot = new QCustomPlot(dockWidgetContents);
	m_helperPlot->installEventFilter(this);
	m_helperPlot->plotLayout()->insertRow(0);
	auto *title = new QCPTextElement(m_helperPlot, "DebugPlot", QFont("sans", 14));
	m_helperPlot->plotLayout()->addElement(0, 0, title);
	m_helperPlot->setOpenGl(true);
	m_helperPlot->setNoAntialiasingOnDrag(true);
	m_helperPlot->setBackground(Qt::darkGray);
	m_helperPlot->setPlottingHints(QCP::phFastPolylines);  // Graph/Curve lines are drawn with a faster method
	m_helperPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	m_helperPlot->setMultiSelectModifier(Qt::ShiftModifier);
	connect(m_helperPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), m_helperPlot->xAxis2, SLOT(setRange(QCPRange)));
	connect(m_helperPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), m_helperPlot->yAxis2, SLOT(setRange(QCPRange)));
	PlotsContainer_verticalLayout->addWidget(m_helperPlot);

	m_helperPlot->legend->setVisible(true);
	m_helperPlot->legend->setFont(QFont("Helvetica", 11));
	m_helperPlot->xAxis->setLabel("x");
	m_helperPlot->xAxis->setLabel("HilbertIdx");
	m_helperPlot->yAxis->setLabel("y");
	m_helperPlot->yAxis->setLabel("Importance");
	m_helperPlot->xAxis2->setVisible(true);
	m_helperPlot->xAxis2->setLabel("HilbertIdx");
	m_helperPlot->yAxis2->setVisible(true);
	m_helperPlot->yAxis2->setLabel("cummulative Importance");
	
	m_helperPlot->addGraph();
	m_helperPlot->graph()->setPen(QPen(Qt::blue));
	m_helperPlot->graph()->setName("Importance Function");
	m_helperPlot->graph()->setData(m_impFuncPlotData);
	m_helperPlot->addGraph(m_helperPlot->xAxis2, m_helperPlot->yAxis2);
	m_helperPlot->graph()->setPen(QPen(Qt::red));
	m_helperPlot->graph()->setName("Cummulative Importance Function");
	m_helperPlot->graph()->setData(m_integralImpFuncPlotData);
	m_helperPlot->graph(0)->rescaleAxes();
	m_helperPlot->replot();
}

bool dlg_DatasetComparator::calcNonLinearMapping(bool showDebugPlot)
{
	if (showDebugPlot)
	{
		m_impFuncPlotData = QSharedPointer<QCPGraphDataContainer>(new QCPGraphDataContainer);
		m_integralImpFuncPlotData = QSharedPointer<QCPGraphDataContainer>(new QCPGraphDataContainer);
	}

	QList<double> innerEnsembleDistList;
	double maxInnerEnsableDist = 0.0;
	double thr = 10000;

	for (int i = 0; i < m_DatasetIntensityMap[0].second.size(); ++i)
	{
		double innerEnsembleDist = -1.0;
		if (m_DatasetIntensityMap[0].second[i].intensity > thr)
		{
			QList<double> localIntValList;
			for (int j = 0; j < m_DatasetIntensityMap.size(); ++j)
				localIntValList.append(m_DatasetIntensityMap[j].second[i].intensity);
			auto minLocalVal = *std::min_element(std::begin(localIntValList), std::end(localIntValList));
			auto maxLocalVal = *std::max_element(std::begin(localIntValList), std::end(localIntValList));
			innerEnsembleDist = maxLocalVal - minLocalVal;
		}
		innerEnsembleDistList.append(innerEnsembleDist);
		if (maxInnerEnsableDist < innerEnsembleDist)
			maxInnerEnsableDist = innerEnsembleDist;
	}
	
	double sectionStart = -1.0;

	for (int i = 0; i < innerEnsembleDistList.size(); ++i)
	{
		double imp = 0.005;
		if (innerEnsembleDistList[i] >= 0.0)
		{
			imp = innerEnsembleDistList[i];
			imp /= maxInnerEnsableDist;
		}

		imp = pow(imp * 2, 1.4); // //imp = pow(imp*2,-0.9);
		m_integralValList.append(i == 0 ? imp : m_integralValList[i - 1] + imp);

		if ((innerEnsembleDistList[i] >= 0.0 || i == innerEnsembleDistList.size()-1) 
			&& sectionStart >= 0.0)
		{
			m_bkgrdRangeList.append(QCPRange(sectionStart, m_integralValList[i - 1]));
			sectionStart = -1.0;
		}
		else if(innerEnsembleDistList[i] == -1.0 && sectionStart == -1.0)
		{
			sectionStart = (i == 0 ? 0.0 : m_integralValList[i]);
		}

		if (showDebugPlot)
		{
			m_impFuncPlotData->add(QCPGraphData(double(i), imp));
			m_integralImpFuncPlotData->add(QCPGraphData(double(i),
				i == 0 ? imp : m_integralValList[i - 1] + imp));
		}
	}

	return showDebugPlot;
}

void dlg_DatasetComparator::visualizePath()
{
	auto pts = vtkSmartPointer<vtkPoints>::New();
	auto pathSteps = m_DatasetIntensityMap.at(0).second.size();
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
	m_integralValList.clear();
	m_imgDataList.clear();
	m_DatasetIntensityMap.clear();
	generateHilbertIdx();
}

void dlg_DatasetComparator::generateHilbertIdx()
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

bool dlg_DatasetComparator::eventFilter(QObject * o, QEvent *e)
{
	if (e->type() == QEvent::KeyPress)
	{
		QKeyEvent *k = (QKeyEvent *)e;
		QString keyStr = QKeySequence(k->key()).toString();
		QCustomPlot* plot = static_cast<QCustomPlot*>(o);
		if (keyStr == "r" || keyStr == "R")
		{
			plot->rescaleAxes();
			plot->replot();
		}
		if ((keyStr == "h" || keyStr == "H") &&
			plot->legend->itemCount())
		{
			plot->legend->setVisible(!plot->legend->visible());
			plot->replot();
		}
		return true;
	}
	return false;
}

void dlg_DatasetComparator::mousePress(QMouseEvent *e)
{
	if ((e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier)
		m_nonlinearScaledPlot->setSelectionRectMode(QCP::srmSelect);
	else
		m_nonlinearScaledPlot->setSelectionRectMode(QCP::srmNone);
}

void dlg_DatasetComparator::mouseMove(QMouseEvent *e)
{
	if (cb_showFbp->isChecked() && cb_fbpView->currentText() == "Alone")
		return;

	QPoint p(e->pos().x(), e->pos().y());
	QList<double> distList;
	for (int i = 0; i < m_DatasetIntensityMap.size(); ++i)
		distList.push_back(m_nonlinearScaledPlot->graph(i)->selectTest(p, true));
	auto minDistance = *std::min_element(std::begin(distList), std::end(distList));
	auto idx = distList.indexOf(minDistance);
	if (minDistance >= 0 && minDistance < 2.0 
		&& m_nonlinearScaledPlot->graph(idx)->visible())
	{
		auto x = m_nonlinearScaledPlot->xAxis->pixelToCoord(e->pos().x());
		auto y = m_nonlinearScaledPlot->yAxis->pixelToCoord(e->pos().y());
		m_dataPointInfo->setText(QString("%1:").arg(m_nonlinearScaledPlot->graph(idx)->name()));
		m_dataPointInfo->position->setCoords(QPointF(x, y));
		m_dataPointInfo->setLayer("overlay");
		m_dataPointInfo->setVisible(true);
	}
	else
		m_dataPointInfo->setVisible(false);
	m_nonlinearScaledPlot->replot();
}

template <typename  T>
void setVoxelIntensity(
	vtkImageData* inputImage, unsigned int x, unsigned int y, 
	unsigned int z, double intensity)
{
	T *v = static_cast< T* >(inputImage->GetScalarPointer(x, y, z));
	*v = intensity;
}

void dlg_DatasetComparator::setupFBPGraphs(iAFunctionalBoxplot<double, double> *fbpData)
{
	QSharedPointer<QCPGraphDataContainer> fb_075Data(new QCPGraphDataContainer);
	for (auto it = fbpData->getMedian().begin(); it != fbpData->getMedian().end(); ++it)
		fb_075Data->add(QCPGraphData(it->first, fbpData->getCentralRegion().getMax(it->first)));
	m_nonlinearScaledPlot->addGraph();
	m_nonlinearScaledPlot->graph()->setVisible(false);
	m_nonlinearScaledPlot->graph()->removeFromLegend();
	m_nonlinearScaledPlot->graph()->setData(fb_075Data);
	m_nonlinearScaledPlot->graph()->setName("Third Quartile");
	QPen quantilePen;
	quantilePen.setColor(QColor(200, 200, 200, 255));
	m_nonlinearScaledPlot->graph()->setPen(quantilePen);
	m_nonlinearScaledPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_025Data(new QCPGraphDataContainer);
	for (auto it = fbpData->getMedian().begin(); it != fbpData->getMedian().end(); ++it)
		fb_025Data->add(QCPGraphData(it->first, fbpData->getCentralRegion().getMin(it->first)));
	m_nonlinearScaledPlot->addGraph();
	m_nonlinearScaledPlot->graph()->setVisible(false);
	m_nonlinearScaledPlot->graph()->removeFromLegend();
	m_nonlinearScaledPlot->graph()->setData(fb_025Data);
	m_nonlinearScaledPlot->graph()->setName("Interquartile Range"); 
	m_nonlinearScaledPlot->graph()->setPen(quantilePen);
	m_nonlinearScaledPlot->graph()->setBrush(QColor(200, 200, 200, 255));
	m_nonlinearScaledPlot->graph()->setChannelFillGraph(m_nonlinearScaledPlot->graph(m_nonlinearScaledPlot->graphCount() - 2));
	m_nonlinearScaledPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_medianData(new QCPGraphDataContainer);
	for (auto it = fbpData->getMedian().begin(); it != fbpData->getMedian().end(); ++it)
		fb_medianData->add(QCPGraphData(it->first, it->second));
	m_nonlinearScaledPlot->addGraph();
	m_nonlinearScaledPlot->graph()->setVisible(false);
	m_nonlinearScaledPlot->graph()->removeFromLegend();
	m_nonlinearScaledPlot->graph()->setName("Median");
	QPen medianPen;
	medianPen.setColor(QColor(0, 0, 0, 255));
	medianPen.setWidth(7);
	m_nonlinearScaledPlot->graph()->setPen(medianPen);
	m_nonlinearScaledPlot->graph()->setData(fb_medianData);
	m_nonlinearScaledPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_MaxData(new QCPGraphDataContainer);
	for (auto it = fbpData->getMedian().begin(); it != fbpData->getMedian().end(); ++it)
		fb_MaxData->add(QCPGraphData(it->first, fbpData->getEnvelope().getMax(it->first)));
	m_nonlinearScaledPlot->addGraph();
	m_nonlinearScaledPlot->graph()->setVisible(false);
	m_nonlinearScaledPlot->graph()->removeFromLegend();
	m_nonlinearScaledPlot->graph()->setData(fb_MaxData);
	m_nonlinearScaledPlot->graph()->setName("Max");
	QPen maxPen;
	maxPen.setColor(QColor(255, 0, 0, 255));
	maxPen.setWidth(7);
	m_nonlinearScaledPlot->graph()->setPen(maxPen);
	m_nonlinearScaledPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_MinData(new QCPGraphDataContainer);
	for (auto it = fbpData->getMedian().begin(); it != fbpData->getMedian().end(); ++it)
		fb_MinData->add(QCPGraphData(it->first, fbpData->getEnvelope().getMin(it->first)));
	m_nonlinearScaledPlot->addGraph();
	m_nonlinearScaledPlot->graph()->setVisible(false);
	m_nonlinearScaledPlot->graph()->removeFromLegend();
	m_nonlinearScaledPlot->graph()->setData(fb_MinData);
	m_nonlinearScaledPlot->graph()->setName("Min");
	QPen minPen;
	minPen.setColor(QColor(0, 0, 255, 255));
	minPen.setWidth(7);
	m_nonlinearScaledPlot->graph()->setPen(minPen);
	m_nonlinearScaledPlot->graph()->setSelectable(QCP::stNone);
}

void dlg_DatasetComparator::showFBPGraphs()
{
	int graphCnt = m_nonlinearScaledPlot->graphCount();
	for (int i = 0; i < graphCnt; ++i)
	{
		if (cb_showFbp->isChecked())
		{
			sl_fbpTransparency->show();
			if (cb_fbpView->currentText() == "Alone")
			{
				if (i >= m_DatasetIntensityMap.size())
				{
					m_nonlinearScaledPlot->graph(i)->setVisible(true);
					if (m_nonlinearScaledPlot->graph(i)->name() != "Third Quartile")
						m_nonlinearScaledPlot->graph(i)->addToLegend();
				}
				else
				{
					m_nonlinearScaledPlot->graph(i)->setVisible(false);
					m_nonlinearScaledPlot->graph(i)->removeFromLegend();
				}
			}
			else
			{
				m_nonlinearScaledPlot->graph(i)->removeFromLegend();
				if (i < m_DatasetIntensityMap.size())
				{
					m_nonlinearScaledPlot->graph(i)->setVisible(true);
					m_nonlinearScaledPlot->graph(i)->addToLegend();
				}
				else
				{
					m_nonlinearScaledPlot->graph(i)->setLayer("background");
					m_nonlinearScaledPlot->graph(i)->setVisible(true);
					if (m_nonlinearScaledPlot->graph(i)->name() != "Third Quartile")
						m_nonlinearScaledPlot->graph(i)->addToLegend();
				}
			}
		}
		else
		{
			sl_fbpTransparency->hide();
			if (i >= m_DatasetIntensityMap.size())
			{
				m_nonlinearScaledPlot->graph(i)->setVisible(false);
				m_nonlinearScaledPlot->graph(i)->removeFromLegend();
			}
			else
			{
				m_nonlinearScaledPlot->graph(i)->setVisible(true);
				m_nonlinearScaledPlot->graph(i)->addToLegend();
			}
		}
	}
	m_nonlinearScaledPlot->replot();
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
	for (int i = m_DatasetIntensityMap.size(); i < m_nonlinearScaledPlot->graphCount(); ++i)
	{
		p = m_nonlinearScaledPlot->graph(i)->pen();
		c = m_nonlinearScaledPlot->graph(i)->pen().color();
		c.setAlpha(alpha);
		p.setColor(c);
		m_nonlinearScaledPlot->graph(i)->setPen(p);
		if (m_nonlinearScaledPlot->graph(i)->name() == "Interquartile Range")
		{
			b = m_nonlinearScaledPlot->graph(i)->brush();
			c = m_nonlinearScaledPlot->graph(i)->brush().color();
			c.setAlpha(alpha);
			b.setColor(c);
			m_nonlinearScaledPlot->graph(i)->setBrush(b);
		}
	}
	m_nonlinearScaledPlot->replot();
}

void dlg_DatasetComparator::selectionChangedByUser()
{
	// TODO: change transfer function for "hiden" values should be HistogramRangeMinimum-1 
	m_mrvRenWin->GetRenderers()->RemoveAllItems();
	m_mrvBGRen->RemoveActor2D(m_mrvBGRen->GetActors2D()->GetLastActor2D());
	m_mrvRenWin->AddRenderer(m_mrvBGRen);

	auto datasetsList = m_datasetsDir.entryList();
	auto selGraphsList = m_nonlinearScaledPlot->selectedGraphs();
	QList<QCPGraph *> visSelGraphList;
	for (auto selGraph : selGraphsList)
		if (selGraph->visible())
			visSelGraphList.append(selGraph);

	if (!visSelGraphList.size())
		m_mrvBGRen->AddActor2D(m_mrvTxtAct);

	for (unsigned int i = 0; i < visSelGraphList.size(); ++i)
	{
		int idx = datasetsList.indexOf(visSelGraphList[i]->name());
		auto selHilberIndices = visSelGraphList[i]->selection().dataRanges();
		auto pathSteps = m_DatasetIntensityMap[idx].second.size(); 
		auto  data = m_DatasetIntensityMap[idx].second;
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
		float viewportCols = visSelGraphList.size() < 3.0 ? fmod(visSelGraphList.size(), 3.0) : 3.0;
		float viewportRows = ceil(visSelGraphList.size() / viewportCols);
		float fieldLengthX = 1.0 / viewportCols, fieldLengthY = 1.0 / viewportRows;
		
		auto cornerAnnotation = vtkSmartPointer<vtkCornerAnnotation>::New();
		cornerAnnotation->SetLinearFontScaleFactor(2);
		cornerAnnotation->SetNonlinearFontScaleFactor(1);
		cornerAnnotation->SetMaximumFontSize(14);
		cornerAnnotation->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
		cornerAnnotation->GetTextProperty()->SetFontSize(14);
		cornerAnnotation->SetText(2, visSelGraphList[i]->name().toStdString().c_str());
		cornerAnnotation->GetTextProperty()->BoldOn();
		cornerAnnotation->GetTextProperty()->SetColor(
			visSelGraphList[i]->pen().color().redF(),
			visSelGraphList[i]->pen().color().greenF(),
			visSelGraphList[i]->pen().color().blueF());
		
		iASimpleTransferFunction tf(m_mdiChild->getColorTransferFunction(), m_mdiChild->getPiecewiseFunction());
		auto ren = vtkSmartPointer<vtkRenderer>::New();
		ren->SetLayer(1);
		ren->SetActiveCamera(m_mdiChild->getRaycaster()->getCamera());
		ren->GetActiveCamera()->ParallelProjectionOn();
		ren->SetViewport(fmod(i, viewportCols) * fieldLengthX,
			1 - (ceil((i + 1.0) / viewportCols) / viewportRows),
			fmod(i, viewportCols) * fieldLengthX + fieldLengthX,
			1 - (ceil((i + 1.0) / viewportCols) / viewportRows) + fieldLengthY);
		ren->AddViewProp(cornerAnnotation);
		ren->ResetCamera();
		
		m_volRen = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(&tf, m_imgDataList[idx]));
		m_volRen->ApplySettings(m_mdiChild->GetVolumeSettings());
		m_volRen->AddTo(ren);
		m_volRen->AddBoundingBoxTo(ren);
		m_mrvRenWin->AddRenderer(ren);
	}
	m_mrvRenWin->Render();
}

void dlg_DatasetComparator::legendClick(QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent* e)
{
	if ((e->button() == Qt::LeftButton) && !cb_showFbp->isChecked() && item)
	{
		QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
		if (!m_selLegendItemList.contains(plItem))
		{
			updateLegendAndGraphVisibility(plItem, 1.0, true);
			m_selLegendItemList.append(plItem);
		}
		else
			m_selLegendItemList.removeOne(plItem);

		for (int i = 0; i < legend->itemCount(); ++i)
		{
			QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(legend->item(i));
			if (!m_selLegendItemList.contains(plItem))
			{
				if ((m_selLegendItemList.size() > 0))
					updateLegendAndGraphVisibility(plItem, 0.3, false);
				else
					updateLegendAndGraphVisibility(plItem, 1.0, true);
			}
		}
		m_nonlinearScaledPlot->replot();
	}
	else if ((e->button() == Qt::RightButton) && !cb_showFbp->isChecked() && m_selLegendItemList.size() > 0)
	{
		m_selLegendItemList.clear();
		for (int i = 0; i < legend->itemCount(); ++i)
		{
			QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(legend->item(i));
			updateLegendAndGraphVisibility(plItem, 1.0, true);
		}
		m_nonlinearScaledPlot->replot();
	}
}

void dlg_DatasetComparator::setSelectionFromRenderer(vtkPoints *selCellPoints)
{
	auto pathSteps = m_DatasetIntensityMap.at(0).second.size();
	auto data = m_DatasetIntensityMap.at(0).second;
	QList<bool> selHilbertIdxList;
	for (int i = 0; i < pathSteps; ++i)
	{
		bool found = false;
		for (int j = 0; j < selCellPoints->GetNumberOfPoints(); ++j)
		{
			double *pt = selCellPoints->GetPoint(j);
			if (data[i].x == pt[0] && data[i].y == pt[1] && data[i].z == pt[2])
			{
				found = true;
				break;
			}
		}
		selHilbertIdxList.append(found);
	}

	QCPDataSelection selection;
	QCPDataRange selRange;
	selRange.setBegin(-1);
	selRange.setEnd(-1);
	for (int i = 0; i < selHilbertIdxList.size(); ++i)
	{
		if (selRange.begin() > -1)
		{
			if (selHilbertIdxList[i])
				continue;
			else
			{
				selRange.setEnd(i);
				selection.addDataRange(selRange,false);
				selRange.setBegin(-1);
				selRange.setEnd(-1);
			}
		}
		else
		{
			if (selHilbertIdxList[i])
				selRange.setBegin(i);
		}
	}
	m_nonlinearScaledPlot->graph(0)->setSelection(selection);
	m_nonlinearScaledPlot->replot();
}