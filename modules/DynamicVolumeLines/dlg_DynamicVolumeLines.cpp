/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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
#include "dlg_DynamicVolumeLines.h"
#include "iAIntensityMapper.h"
#include "iALinearColorGradientBar.h"
#include "iANonLinearAxisTicker.h"
#include "iAOrientationWidget.h"
#include "iASegmentTree.h"

#include "charts/iADiagramFctWidget.h"
#include "iAColorTheme.h"
#include "iAFunction.h"
#include "iAFunctionalBoxplot.h"
#include "iALUT.h"
#include "iARenderer.h"
#include "iATransferFunction.h"
#include "iATypedCallHelper.h"
#include "iAVolumeRenderer.h"
#include "iAVtkWidget.h"

#include <vtkAbstractVolumeMapper.h>
#include <vtkActor.h>
#include <vtkActor2DCollection.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellArray.h>
#include <vtkColorTransferFunction.h>
#include <vtkCornerAnnotation.h>
#include <vtkImageData.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkLine.h>
#include <vtkLookupTable.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkScalarBarActor.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h> 
#include <vtkVolumeProperty.h>

const double impInitValue = 0.025;
const double offsetY = 1000;
const QString plotColor = "DVL-Metro Colors (max. 17)";	// Brewer Qualitaive 1 (max. 8) // DVL-Metro Colors (max. 17)

void winModCallback(vtkObject *caller, long unsigned int vtkNotUsed(eventId),
	void* vtkNotUsed(client), void* vtkNotUsed(callData))
{
	auto *r = static_cast<vtkRenderer*>(caller);
	if (!r->GetActors2D()->GetLastActor2D())
		return;
	auto r_centerX = r->GetCenter()[0];
	auto r_centerY = r->GetCenter()[1];
	r->GetActors2D()->GetLastActor2D()->SetPosition(r_centerX, r_centerY);
}

dlg_DynamicVolumeLines::dlg_DynamicVolumeLines(QWidget *parent /*= 0*/, QDir datasetsDir, Qt::WindowFlags f /*= 0 */) :
	DynamicVolumeLinesConnector(parent, f),
	m_mdiChild(static_cast<MdiChild*>(parent)),
	m_datasetsDir(datasetsDir),
	m_MultiRendererView(new multi3DRendererView()),
	m_mrvBGRen(vtkSmartPointer<vtkRenderer>::New()),
	m_mrvTxtAct(vtkSmartPointer<vtkTextActor>::New()),
	m_scalingWidget(0),
	m_compLvlLUT(vtkSmartPointer<vtkLookupTable>::New()),
	m_histLUT(vtkSmartPointer<vtkLookupTable>::New()),
	m_subHistBinCntChanged(false),
	m_histVisMode(true),
	m_nonlinearScaledPlot(new QCustomPlot(dockWidgetContents)),
	m_linearScaledPlot(new QCustomPlot(dockWidgetContents))
{
	m_mdiChild->getRenderer()->setAreaPicker();
	
	m_nonlinearScaledPlot->setObjectName("nonlinear");
	m_linearScaledPlot->setObjectName("linear");

	setupScaledPlot(m_nonlinearScaledPlot);
	setupScalingWidget();
	setupScaledPlot(m_linearScaledPlot);
	//setupDebugPlot();		// Debug
	setupPlotConnections(m_nonlinearScaledPlot);
	setupPlotConnections(m_linearScaledPlot);
	setupGUIElements();
	setupGUIConnections();
	setupMultiRendererView();
	generateHilbertIdx();
}

dlg_DynamicVolumeLines::~dlg_DynamicVolumeLines()
{}

void dlg_DynamicVolumeLines::setupScaledPlot(QCustomPlot *qcp)
{
	qcp->axisRect()->setAutoMargins(QCP::msNone);
	qcp->axisRect()->setMargins(QMargins(58, 1, 3, 32));
	qcp->setCursor(QCursor(Qt::CrossCursor));
	qcp->installEventFilter(this);  // To catch key press event
	qcp->setOpenGl(true);
	qcp->setPlottingHints(QCP::phFastPolylines);  // Graph/Curve lines are drawn with a faster method
	qcp->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	qcp->setMultiSelectModifier(Qt::ShiftModifier);
	qcp->legend->setVisible(false);
	qcp->legend->setFont(QFont("Helvetica", 9));
	qcp->legend->setRowSpacing(0);
	qcp->xAxis->ticker()->setTickCount(40);
	qcp->xAxis->setLabel("Hilbert index");
	qcp->xAxis->grid()->setVisible(false);
	qcp->xAxis->grid()->setSubGridVisible(false);
	qcp->xAxis->setTickLabels(true);
	qcp->xAxis->setSubTicks(false);	 // set to 'false if too many ticks
	qcp->xAxis->setNumberFormat("f");
	qcp->xAxis->setNumberPrecision(0);
	qcp->yAxis->setLabel("Intensities");
	qcp->yAxis->grid()->setSubGridVisible(false);
	qcp->yAxis->grid()->setVisible(false);
	qcp->xAxis2->setVisible(true);
	qcp->xAxis2->setSubTicks(false);	// set to 'false if too many ticks
	qcp->xAxis2->setTicks(false);
	qcp->yAxis2->setVisible(true);
	qcp->yAxis2->setTickLabels(false);
	qcp->yAxis2->setTicks(false);
	qcp->yAxis2->setSubTicks(false);

	qcp->addLayer("cursor", qcp->layer("legend"), QCustomPlot::LayerInsertMode::limBelow);
	qcp->layer("cursor")->setMode(QCPLayer::lmBuffered);

	QToolButton *tb_MinMaxPlot = new QToolButton();
	tb_MinMaxPlot->setObjectName(QString(qcp->objectName()).append("MinMaxTB"));
	tb_MinMaxPlot->setStyleSheet("border: 1px solid; margin-left: 3px;");
	tb_MinMaxPlot->setMinimumSize(QSize(0, 0));
	tb_MinMaxPlot->setMaximumSize(QSize(13, 10));
	tb_MinMaxPlot->setIcon(QIcon(":/images/minus.png"));
	tb_MinMaxPlot->setIconSize(QSize(10, 10));
	connect(tb_MinMaxPlot, SIGNAL(clicked()), this, SLOT(changePlotVisibility()));

	PlotsContainer_verticalLayout->addWidget(tb_MinMaxPlot);
	PlotsContainer_verticalLayout->addWidget(qcp);
}

bool dlg_DynamicVolumeLines::eventFilter(QObject *o, QEvent *e)
{
	if (e->type() == QEvent::KeyPress)
	{
		QKeyEvent *k = (QKeyEvent*)e;
		QString keyStr = QKeySequence(k->key()).toString();
		QCustomPlot *plot = qobject_cast<QCustomPlot*>(o);
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

void dlg_DynamicVolumeLines::setupScalingWidget()
{
	m_scalingWidget = new iAScalingWidget(this);
	PlotsContainer_verticalLayout->addWidget(m_scalingWidget);
}

void dlg_DynamicVolumeLines::setupPlotConnections(QCustomPlot *qcp)
{
	if (qcp->objectName().contains("nonlinear"))
	{
		connect(qcp->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(syncLinearXAxis(QCPRange)));
		connect(qcp->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(syncYAxis(QCPRange)));
		connect(qcp, SIGNAL(afterReplot()), m_linearScaledPlot, SLOT(replot()));
	}
	else
	{
		connect(qcp->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(syncNonlinearXAxis(QCPRange)));
		connect(qcp->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(syncYAxis(QCPRange)));
		connect(qcp, SIGNAL(afterReplot()), m_nonlinearScaledPlot, SLOT(replot()));
	}
	connect(qcp, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
	connect(qcp, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));
	connect(qcp, SIGNAL(mouseWheel(QWheelEvent*)), this, SLOT(mouseWheel(QWheelEvent*)));
	connect(qcp, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChangedByUser()));
	connect(qcp, SIGNAL(legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)),
		this, SLOT(legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)));
}

void dlg_DynamicVolumeLines::setupGUIElements()
{
	sl_FBPTransparency->hide();
	cb_showFBP->setEnabled(false);
	cb_FBPView->setEnabled(false);

	iALinearColorGradientBar *compLvl_colorBar = new iALinearColorGradientBar(this,
		"ColorBrewer single hue 5-class grays", false);
	m_compLvlLUT = compLvl_colorBar->getLut();
	connect(this, SIGNAL(compLevelRangeChanged(QVector<double>)),
		compLvl_colorBar, SLOT(compLevelRangeChanged(QVector<double>)));
	QVBoxLayout *compLvl_lutLayoutHB = new QVBoxLayout(this);
	compLvl_lutLayoutHB->setMargin(0);
	compLvl_lutLayoutHB->addWidget(compLvl_colorBar);
	compLvl_lutLayoutHB->update();
	scalarBarWidget->setLayout(compLvl_lutLayoutHB);

	iALinearColorGradientBar *hist_colorBar = new iALinearColorGradientBar(this,
		"Extended Black Body", true);
	m_histLUT = hist_colorBar->getLut();
	connect(hist_colorBar, SIGNAL(colorMapChanged(vtkSmartPointer<vtkLookupTable>)),
		this, SLOT(updateHistColorMap(vtkSmartPointer<vtkLookupTable>)));
	QVBoxLayout *hist_lutLayoutHB = new QVBoxLayout(this);
	hist_lutLayoutHB->setMargin(0);
	hist_lutLayoutHB->addWidget(hist_colorBar);
	hist_lutLayoutHB->update();
	histBarWidget->setLayout(hist_lutLayoutHB);

	m_orientationWidget = new iAOrientationWidget(this);
	orientationWidgetLayout->addWidget(m_orientationWidget);
	m_orientationWidget->update(m_linearScaledPlot, 0, m_nonlinearMappingVec.size() - 1,
		m_minEnsembleIntensity - offsetY, m_maxEnsembleIntensity + offsetY);
}

void dlg_DynamicVolumeLines::updateHistColorMap(vtkSmartPointer<vtkLookupTable> lut)
{
	m_histLUT = lut;
	visualize();
}

void dlg_DynamicVolumeLines::setupGUIConnections()
{
	connect(pB_Update, SIGNAL(clicked()), this, SLOT(updateDynamicVolumeLines()));
	connect(cb_showFBP, SIGNAL(stateChanged(int)), this, SLOT(showFBPGraphs()));
	connect(cb_FBPView, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFBPView()));
	connect(sl_FBPTransparency, SIGNAL(valueChanged(int)), this, SLOT(setFBPTransparency(int)));
	connect(sb_BkgrdThr, SIGNAL(valueChanged(double)), this, SLOT(visualize()));
	connect(cb_BkgrdThrLine, SIGNAL(stateChanged(int)), this, SLOT(showBkgrdThrLine()));
	connect(sb_nonlinearScalingFactor, SIGNAL(valueChanged(double)), this, SLOT(visualize()));
	connect(pB_selectCompLevel, SIGNAL(clicked()), this, SLOT(selectCompLevel()));
	connect(m_linearScaledPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), m_orientationWidget, SLOT(update()));
	connect(m_linearScaledPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), m_orientationWidget, SLOT(update()));
	connect(sb_histBinWidth, SIGNAL(valueChanged(int)), this, SLOT(visualize()));
	connect(sb_subHistBinCnt, SIGNAL(valueChanged(int)), this, SLOT(setSubHistBinCntFlag()));
	connect(sb_UpperCompLevelThr, SIGNAL(valueChanged(double)), this, SLOT(compLevelRangeChanged()));
	connect(sb_LowerCompLevelThr, SIGNAL(valueChanged(double)), this, SLOT(compLevelRangeChanged()));
	connect(m_mdiChild->getRenderer(), SIGNAL(cellsSelected(vtkPoints*)),
		this, SLOT(setSelectionForPlots(vtkPoints*)));
	connect(m_mdiChild->getRenderer(), SIGNAL(noCellsSelected()),
		this, SLOT(setNoSelectionForPlots()));
}

void dlg_DynamicVolumeLines::changePlotVisibility()
{
	QToolButton *tb = qobject_cast<QToolButton*>(QObject::sender());
	tb->objectName().contains("nonlinear") ?
		setPlotVisibility(tb, m_nonlinearScaledPlot) : 
		setPlotVisibility(tb, m_linearScaledPlot);
}

void dlg_DynamicVolumeLines::setupMultiRendererView()
{
	m_mrvTxtAct->SetInput("No Hilbert index selected");
	m_mrvTxtAct->GetTextProperty()->SetFontSize(24);
	m_mrvTxtAct->GetTextProperty()->SetColor(0.0, 0.0, 0.0);
	m_mrvTxtAct->GetTextProperty()->SetJustificationToCentered();
	m_mrvTxtAct->GetTextProperty()->SetVerticalJustificationToCentered();
	
	auto mrvWinModCallback = vtkSmartPointer<vtkCallbackCommand>::New();
	mrvWinModCallback->SetCallback(winModCallback);
	m_mrvBGRen->AddObserver(vtkCommand::ModifiedEvent, mrvWinModCallback);
	m_mrvBGRen->SetLayer(0);
	m_mrvBGRen->InteractiveOff();
	m_mrvBGRen->SetBackground(1.0, 1.0, 1.0);
	m_mrvBGRen->AddActor2D(m_mrvTxtAct);
	
	CREATE_OLDVTKWIDGET(wgtContainer);
	auto mrvRenWin = wgtContainer->GetRenderWindow();
	mrvRenWin->SetNumberOfLayers(2);
	mrvRenWin->AddRenderer(m_mrvBGRen);
	mrvRenWin->Render();
	m_mdiChild->tabifyDockWidget(m_mdiChild->renderer, m_MultiRendererView);
	m_MultiRendererView->verticalLayout->addWidget(wgtContainer);
	m_MultiRendererView->show();
}

void dlg_DynamicVolumeLines::generateHilbertIdx()
{
	QThread *thread = new QThread;
	iAIntensityMapper *im = new iAIntensityMapper(m_datasetsDir, PathNameToId[cb_Paths->currentText()],
		m_DatasetIntensityMap, m_imgDataList, m_minEnsembleIntensity, m_maxEnsembleIntensity);
	im->moveToThread(thread);
	connect(thread, SIGNAL(started()), im, SLOT(process()));
	connect(im, SIGNAL(finished()), thread, SLOT(quit()));
	connect(im, SIGNAL(finished()), im, SLOT(deleteLater()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	//connect(thread, SIGNAL(finished()), this, SLOT(visualizePath()));		// Debug
	connect(thread, SIGNAL(finished()), this, SLOT(visualize()));
	thread->start();
}

void dlg_DynamicVolumeLines::visualizePath()
{
	double *spacing = m_imgDataList[0]->GetSpacing();
	auto pts = vtkSmartPointer<vtkPoints>::New();
	auto pathSteps = m_DatasetIntensityMap.at(0).second.size();
	QList<icData>  data = m_DatasetIntensityMap.at(0).second;
	double point[3];
	for (unsigned int i = 0; i < pathSteps; ++i)
	{
		point[0] = data[i].x * spacing[0];
		point[1] = data[i].y * spacing[1];
		point[2] = data[i].z * spacing[2];
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
	m_mdiChild->getRenderer()->setPolyData(linesPolyData);
	m_mdiChild->getRenderer()->GetPolyActor()->GetProperty()->SetInterpolationToFlat();
	m_mdiChild->getRenderer()->GetPolyActor()->GetProperty()->SetOpacity(1.0);
	m_mdiChild->getRenderer()->GetPolyActor()->GetProperty()->SetColor(1.0, 0.0, 0.0);
	m_mdiChild->getRenderer()->GetPolyActor()->GetProperty()->SetLineWidth(4);
	m_mdiChild->getRenderer()->GetPolyActor()->GetProperty()->SetRenderLinesAsTubes(true);
	m_mdiChild->getRenderer()->update();
}

void dlg_DynamicVolumeLines::visualize()
{
	//TODO: refactor!?
	m_nonlinearScaledPlot->clearGraphs();
	m_nonlinearScaledPlot->clearItems();
	m_linearScaledPlot->clearItems();
	//m_debugPlot->clearGraphs();	// Debug
	
	calcNonLinearMapping();
	generateSegmentTree();
	//showDebugPlot();		// Debug
	
	if (m_linearScaledPlot->graphCount() < 1)
	{
		std::vector<iAFunction<double, double> *> linearFCPFunctions;
		for (auto it = m_DatasetIntensityMap.begin(); it != m_DatasetIntensityMap.end(); ++it)
		{
			m_linearScaledPlot->addGraph();
			m_linearScaledPlot->graph()->setVisible(false);
			m_linearScaledPlot->graph()->setSelectable(QCP::stMultipleDataRanges);
			m_linearScaledPlot->graph()->setPen(getDatasetPen(it - m_DatasetIntensityMap.begin(),
				m_DatasetIntensityMap.size(), 2, plotColor));
			m_linearScaledPlot->graph()->setName(it->first);
			QCPScatterStyle scatter;
			scatter.setShape(QCPScatterStyle::ssNone);	 // Check ssDisc/ssDot to show single selected points
			scatter.setSize(3.0);
			m_linearScaledPlot->graph()->setScatterStyle(scatter);
			QPen p = m_linearScaledPlot->graph()->selectionDecorator()->pen();
			p.setWidth(5);
			p.setColor(QColor(255, 0, 0));  // Selection color: red
			m_linearScaledPlot->graph()->selectionDecorator()->setPen(p);
			QSharedPointer<QCPGraphDataContainer> linearScaledPlotData(new QCPGraphDataContainer);
			auto * funct = new iAFunction<double, double>();
			for (unsigned int i = 0; i < it->second.size(); ++i)
			{
				linearScaledPlotData->add(QCPGraphData(double(i), it->second[i].intensity));
				funct->insert(std::make_pair(i, it->second[i].intensity));
			}
			linearFCPFunctions.push_back(funct);
			m_linearScaledPlot->graph()->setData(linearScaledPlotData);
		}
		ModifiedDepthMeasure<double, double> l_measure;
		auto l_functionalBoxplotData = new iAFunctionalBoxplot<double, double>(linearFCPFunctions, &l_measure, 2);
		setupFBPGraphs(m_linearScaledPlot, l_functionalBoxplotData);
	}

	m_linearDataPointInfo = new QCPItemText(m_linearScaledPlot);
	m_linearDataPointInfo->setFont(QFont("Helvetica", 8, QFont::Bold));
	m_linearDataPointInfo->setLayer("cursor");
	m_linearDataPointInfo->setVisible(true);
	m_linearDataPointInfo->setColor(QColor(0, 0, 0));
	m_linearDataPointInfo->setPositionAlignment(Qt::AlignLeft);
	m_linearDataPointInfo->setTextAlignment(Qt::AlignLeft);
	m_linearDataPointInfo->setBrush(QColor(255, 255, 255, 180));
	m_linearDataPointInfo->setPadding(QMargins(3, 1, 1, 1));

	m_linearIdxLine = new QCPItemStraightLine(m_linearScaledPlot);
	m_linearIdxLine->setVisible(true);
	m_linearIdxLine->setAntialiased(false);
	m_linearIdxLine->setLayer("cursor");
	m_linearIdxLine->setPen(QPen(QColor(255, 128, 0), 1.5, Qt::SolidLine));
	m_linearIdxLine->point1->setTypeX(QCPItemPosition::ptPlotCoords);
	m_linearIdxLine->point1->setTypeY(QCPItemPosition::ptAxisRectRatio);
	m_linearIdxLine->point1->setAxes(m_linearScaledPlot->xAxis, m_linearScaledPlot->yAxis);
	m_linearIdxLine->point1->setAxisRect(m_linearScaledPlot->axisRect());
	m_linearIdxLine->point2->setTypeX(QCPItemPosition::ptPlotCoords);
	m_linearIdxLine->point2->setTypeY(QCPItemPosition::ptAxisRectRatio);
	m_linearIdxLine->point2->setAxes(m_linearScaledPlot->xAxis, m_linearScaledPlot->yAxis);
	m_linearIdxLine->point2->setAxisRect(m_linearScaledPlot->axisRect());
	m_linearIdxLine->point1->setCoords(0.0, 0.0);
	m_linearIdxLine->point2->setCoords(0.0, 0.0);
	m_linearIdxLine->setClipToAxisRect(true);

	std::vector<iAFunction<double, double> *> nonlinearFCPFunctions;
	for (auto it = m_DatasetIntensityMap.begin(); it != m_DatasetIntensityMap.end(); ++it)
	{
		m_nonlinearScaledPlot->addGraph();
		m_nonlinearScaledPlot->graph()->setVisible(false);
		m_nonlinearScaledPlot->graph()->setSelectable(QCP::stMultipleDataRanges);
		m_nonlinearScaledPlot->graph()->setPen(getDatasetPen(it - m_DatasetIntensityMap.begin(),
			m_DatasetIntensityMap.size(), 2, plotColor));
		m_nonlinearScaledPlot->graph()->setName(it->first);
		QCPScatterStyle scatter;
		scatter.setShape(QCPScatterStyle::ssNone);	 // Check ssDisc/ssDot to show single selected points
		scatter.setSize(3.0);
		m_nonlinearScaledPlot->graph()->setScatterStyle(scatter);
		QPen p = m_nonlinearScaledPlot->graph()->selectionDecorator()->pen();
		p.setWidth(5);
		p.setColor(QColor(255, 0, 0));  // Selection color: red
		m_nonlinearScaledPlot->graph()->selectionDecorator()->setPen(p);  
		QSharedPointer<QCPGraphDataContainer> nonlinearScaledPlotData(new QCPGraphDataContainer);
		auto * funct = new iAFunction<double, double>();
		for (int i = 0; i < m_nonlinearMappingVec.size(); ++i)
		{
			nonlinearScaledPlotData->add(QCPGraphData(m_nonlinearMappingVec[i], it->second[i].intensity));
			funct->insert(std::make_pair(m_nonlinearMappingVec[i], it->second[i].intensity));
		}
		nonlinearFCPFunctions.push_back(funct);
		m_nonlinearScaledPlot->graph()->setData(nonlinearScaledPlotData);
	}

	ModifiedDepthMeasure<double, double> nl_measure;
	auto nonlinearFBPData = new iAFunctionalBoxplot<double, double>(nonlinearFCPFunctions, &nl_measure, 2);
	setupFBPGraphs(m_nonlinearScaledPlot, nonlinearFBPData);
	
	m_nonlinearTicker = QSharedPointer<iANonLinearAxisTicker>(new iANonLinearAxisTicker);
	m_nonlinearTicker->setTickData(m_nonlinearMappingVec);
	m_nonlinearTicker->setAxis(m_nonlinearScaledPlot->xAxis);
	m_nonlinearScaledPlot->xAxis->setTicker(m_nonlinearTicker);
	m_nonlinearScaledPlot->xAxis2->setTicker(m_nonlinearTicker);

	m_nonlinearDataPointInfo = new QCPItemText(m_nonlinearScaledPlot);
	m_nonlinearDataPointInfo->setFont(QFont("Helvetica", 8, QFont::Bold));
	m_nonlinearDataPointInfo->setLayer("cursor");
	m_nonlinearDataPointInfo->setVisible(true);
	m_nonlinearDataPointInfo->setColor(QColor(0, 0, 0));
	m_nonlinearDataPointInfo->setPositionAlignment(Qt::AlignLeft);
	m_nonlinearDataPointInfo->setTextAlignment(Qt::AlignLeft);
	m_nonlinearDataPointInfo->setBrush(QColor(255, 255, 255, 180));
	m_nonlinearDataPointInfo->setPadding(QMargins(3, 1, 1, 1));

	m_nonlinearIdxLine = new QCPItemStraightLine(m_nonlinearScaledPlot);
	m_nonlinearIdxLine->setVisible(true);
	m_nonlinearIdxLine->setAntialiased(false);
	m_nonlinearIdxLine->setLayer("cursor");
	m_nonlinearIdxLine->setPen(QPen(QColor(255, 128, 0), 1.5, Qt::SolidLine));
	m_nonlinearIdxLine->point1->setTypeX(QCPItemPosition::ptPlotCoords);
	m_nonlinearIdxLine->point1->setTypeY(QCPItemPosition::ptAxisRectRatio);
	m_nonlinearIdxLine->point1->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
	m_nonlinearIdxLine->point1->setAxisRect(m_nonlinearScaledPlot->axisRect());
	m_nonlinearIdxLine->point2->setTypeX(QCPItemPosition::ptPlotCoords);
	m_nonlinearIdxLine->point2->setTypeY(QCPItemPosition::ptAxisRectRatio);
	m_nonlinearIdxLine->point2->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
	m_nonlinearIdxLine->point2->setAxisRect(m_nonlinearScaledPlot->axisRect());
	m_nonlinearIdxLine->point1->setCoords(0.0, 0.0);
	m_nonlinearIdxLine->point2->setCoords(0.0, 0.0);
	m_nonlinearIdxLine->setClipToAxisRect(true);

	showBkgrdThrRanges(m_nonlinearScaledPlot);
	showBkgrdThrRanges(m_linearScaledPlot);

	m_selGraphList.clear();
	for (int i = 0; i < m_DatasetIntensityMap.size(); ++i)
	{
		m_selGraphList.append(m_nonlinearScaledPlot->graph(i));
		m_selGraphList.append(m_linearScaledPlot->graph(i));
	}
	//showCompressionLevel();

	//m_debugPlot->rescaleAxes();	// Debug
	//m_debugPlot->replot();	// Debug
	m_linearScaledPlot->rescaleAxes();
	m_linearScaledPlot->replot();
	m_nonlinearScaledPlot->rescaleAxes();
	m_nonlinearScaledPlot->replot();

	if (!m_histVisMode)
	{
		m_scalingWidget->setRange(0, m_linearScaledPlot->xAxis->range().upper - 1, 0, 0, 0, 0);
	}
	else
	{
		double lowerBinBoarder = m_nonlinearScaledPlot->xAxis->range().lower / m_stepSize;
		double lowerVisibleBinBoarder = ceil(lowerBinBoarder);
		double lowerVisibleRest = lowerVisibleBinBoarder - lowerBinBoarder;
		double upperBinBoarder = (m_nonlinearScaledPlot->xAxis->range().upper - m_nonlinearScaledPlot->xAxis->range().lower) / m_stepSize;	//TODO: check
		double upperVisibleBinBoarder = floor(upperBinBoarder);
		double upperVisibleRest = upperBinBoarder - upperVisibleBinBoarder;
		double lowerLinearVisibleRest = m_linearHistBinBoarderVec[lowerVisibleBinBoarder - 1] - m_linearScaledPlot->xAxis->range().lower;
		double upperLinearVisibleRest = m_linearScaledPlot->xAxis->range().upper - m_linearHistBinBoarderVec[upperVisibleBinBoarder - 1];
		m_scalingWidget->setOverviewRange(lowerVisibleBinBoarder,
			upperVisibleBinBoarder, lowerVisibleRest, upperVisibleRest, lowerLinearVisibleRest,
			upperLinearVisibleRest, m_histBinImpFunctAvgVec, m_linearHistBinBoarderVec);
	}

	m_scalingWidget->setAxes(m_nonlinearScaledPlot->xAxis, m_linearScaledPlot->xAxis);
	m_scalingWidget->setNonlinearScalingVec(m_nonlinearMappingVec, m_impFunctVec);
	m_scalingWidget->update();

	m_orientationWidget->update(m_linearScaledPlot, 0, m_nonlinearMappingVec.size() - 1,
		m_minEnsembleIntensity - offsetY, m_maxEnsembleIntensity + offsetY);
}

void dlg_DynamicVolumeLines::calcNonLinearMapping()
{
	QList<double> innerEnsembleDistList;
	double maxInnerEnsableDist = 0.0;

	for (int i = 0; i < m_DatasetIntensityMap[0].second.size(); ++i)
	{
		double innerEnsembleDist = -1.0;
		double thr = sb_BkgrdThr->value();
		if (m_DatasetIntensityMap[0].second[i].intensity >= thr)
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
	m_nonlinearMappingVec.clear();
	m_bkgrdThrRangeList.clear();
	m_impFunctVec.clear();

	for (int i = 0; i < innerEnsembleDistList.size(); ++i)
	{
		// TODO: set imp start value automatically
		double imp = impInitValue;
		if (innerEnsembleDistList[i] >= 0.0)
		{
			imp = innerEnsembleDistList[i];
			imp /= maxInnerEnsableDist;
		}
		imp = pow(imp * 1, sb_nonlinearScalingFactor->value()); // //imp = pow(imp*2,-0.9);
		m_impFunctVec.append(imp);
		m_nonlinearMappingVec.append(i == 0 ? imp : m_nonlinearMappingVec[i - 1] + imp);

		//TODO: Better way to calc bkgrdThrRanges
		if (innerEnsembleDistList[i] >= 0.0 && sectionStart >= 0.0 &&
			i!= innerEnsembleDistList.size())
		{
			m_bkgrdThrRangeList.append(QCPRange(sectionStart, m_nonlinearMappingVec[i-1]));
			sectionStart = -1.0;
		}
		else if(innerEnsembleDistList[i] == -1.0 &&
			sectionStart == -1.0)
		{
			sectionStart = m_nonlinearMappingVec[i];
		}
		else if (innerEnsembleDistList[i] == -1.0 && 
			sectionStart >= 0.0 && i == innerEnsembleDistList.size()-1)
		{
			m_bkgrdThrRangeList.append(QCPRange(sectionStart, m_nonlinearMappingVec[i]));
			sectionStart = -1.0;
		}
	}
}

void dlg_DynamicVolumeLines::generateSegmentTree()
{
	// TODO: draw after BkgdRanges + draw only the histograms without the BkgdRanes 
	int subhistBinCnt = sb_subHistBinCnt->value(), lowerBnd = 0, upperBnd = 65535,
		plotBinWidth = sb_histBinWidth->value(),
		plotWidth = m_linearScaledPlot->axisRect()->rect().width(),
		plotBinCnt = ceil(plotWidth / (double)plotBinWidth);
	double rgb[3]; QColor c;
	m_stepSize = (m_nonlinearMappingVec.last() - m_nonlinearMappingVec.first()) / (plotWidth / (double)plotBinWidth);
	m_histBinImpFunctAvgVec.clear();
	m_linearHistBinBoarderVec.clear();

	if (m_segmTreeList.isEmpty() | m_subHistBinCntChanged)
	{
		m_segmTreeList.clear();
		m_subHistBinCntChanged = false; 
		for (int datsetNumber = 0; datsetNumber < m_DatasetIntensityMap.size(); ++datsetNumber)
		{
			std::vector<int> intensityVec;
			for (int hIdx = 0; hIdx < m_DatasetIntensityMap[datsetNumber].second.size(); ++hIdx)
				intensityVec.push_back(m_DatasetIntensityMap[datsetNumber].second[hIdx].intensity);
			iASegmentTree *segmentTree = new iASegmentTree(intensityVec, subhistBinCnt, lowerBnd, upperBnd);
			m_segmTreeList.append(segmentTree);
		}
	}

	for (int xBinNumber = 1; xBinNumber <= plotBinCnt; ++xBinNumber)
	{
		//TODO: refactoring -> move to helper class (also see sync functions above)
		auto lower = qLowerBound(m_nonlinearMappingVec.begin(),
			m_nonlinearMappingVec.end(), (xBinNumber - 1)*m_stepSize);
		int nonlinearLowerIdx = lower - m_nonlinearMappingVec.begin() - 1;
		if (nonlinearLowerIdx < 0) nonlinearLowerIdx = 0.0;
		double lowerDistToNextPoint = m_nonlinearMappingVec[nonlinearLowerIdx + 1] -
			m_nonlinearMappingVec[nonlinearLowerIdx];
		double lowerDistToCurrPoint = (xBinNumber - 1)*m_stepSize -
			m_nonlinearMappingVec[nonlinearLowerIdx];
		if (lowerDistToCurrPoint < 0) lowerDistToCurrPoint = 0.0;
		auto upper = qLowerBound(m_nonlinearMappingVec.begin(),
			m_nonlinearMappingVec.end(), xBinNumber*m_stepSize);
		int nonlinearUpperIdx = upper - m_nonlinearMappingVec.begin();
		double upperDistToNextPoint = 0.0, upperDistToCurrPoint = 0.0;
		if (nonlinearUpperIdx < m_nonlinearMappingVec.size())
		{
			upperDistToNextPoint = m_nonlinearMappingVec[nonlinearUpperIdx] - m_nonlinearMappingVec[nonlinearUpperIdx - 1];
			upperDistToCurrPoint = xBinNumber*m_stepSize - m_nonlinearMappingVec[nonlinearUpperIdx - 1];
		}
		else
		{
			nonlinearUpperIdx = m_nonlinearMappingVec.size() - 1;
			upperDistToNextPoint = 1.0;
			upperDistToCurrPoint = 1.0;
		}
		
		double linearLowerDbl = nonlinearLowerIdx + lowerDistToCurrPoint / lowerDistToNextPoint;
		double linearUpperDbl = nonlinearUpperIdx - 1 + upperDistToCurrPoint / upperDistToNextPoint;
		int linearLowerIdx = floor(linearLowerDbl);
		int linearUpperIdx = floor(linearUpperDbl);

		double sum = 0.0, avg = 0.0;
		for (int i = nonlinearLowerIdx; i <= nonlinearUpperIdx; ++i)
			sum += m_impFunctVec[i];
		avg = sum / (nonlinearUpperIdx - nonlinearLowerIdx + 1);
		m_histBinImpFunctAvgVec.append(avg);

		m_linearHistBinBoarderVec.append(linearUpperDbl);

		for (int yBinNumber = 0; yBinNumber < subhistBinCnt; ++yBinNumber)
		{
			unsigned int nonlinear_sum = 0, linear_sum = 0;
			for (int treeNumber = 0; treeNumber < m_segmTreeList.size(); ++treeNumber)
			{
				nonlinear_sum += m_segmTreeList[treeNumber]->hist_query(nonlinearLowerIdx, nonlinearUpperIdx)[yBinNumber];
				linear_sum += m_segmTreeList[treeNumber]->hist_query(linearLowerIdx, linearUpperIdx)[yBinNumber];
			}

			QCPItemRect *nonlin_histRectItem = new QCPItemRect(m_nonlinearScaledPlot);
			nonlin_histRectItem->setObjectName("histRect");
			nonlin_histRectItem->setAntialiased(false);
			nonlin_histRectItem->setLayer("background");
			nonlin_histRectItem->setPen(QPen(Qt::NoPen));
			//nonlin_histRectItem->setPen(QPen(QColor(Qt::yellow)));	// Debug
			nonlin_histRectItem->setClipToAxisRect(true);
			// TODO: problem with 256 subhistBinCnt color gradient badly visible 
			// -> scale to other max value (local histogram maximum)
			m_histLUT->GetColor((double)nonlinear_sum / (nonlinearUpperIdx - nonlinearLowerIdx + 1), rgb);
			c.setRgbF(rgb[0], rgb[1], rgb[2]);
			nonlin_histRectItem->setBrush(QBrush(c));
			nonlin_histRectItem->topLeft->setCoords((xBinNumber - 1)*m_stepSize, ((double)yBinNumber / subhistBinCnt) * upperBnd);
			nonlin_histRectItem->bottomRight->setCoords(xBinNumber*m_stepSize, ((double)(yBinNumber + 1) / subhistBinCnt)*upperBnd);

			QCPItemRect *linear_histRectItem = new QCPItemRect(m_linearScaledPlot);
			linear_histRectItem->setObjectName("histRect");
			linear_histRectItem->setAntialiased(false);
			linear_histRectItem->setLayer("background");
			linear_histRectItem->setPen(QPen(Qt::NoPen));
			//linear_histRectItem->setPen(QPen(QColor(Qt::yellow)));	// Debug
			linear_histRectItem->setClipToAxisRect(true);
			m_histLUT->GetColor((double)linear_sum / (linearUpperIdx - linearLowerIdx + 1), rgb);
			c.setRgbF(rgb[0], rgb[1], rgb[2]);
			linear_histRectItem->setBrush(c);
			linear_histRectItem->topLeft->setCoords(linearLowerDbl, ((double)yBinNumber / subhistBinCnt) * upperBnd);
			linear_histRectItem->bottomRight->setCoords(linearUpperDbl, ((double)(yBinNumber + 1) / subhistBinCnt)*upperBnd);
		}
	}
}

void dlg_DynamicVolumeLines::setupFBPGraphs(QCustomPlot* qcp, iAFunctionalBoxplot<double, double>* FBPData)
{
	QSharedPointer<QCPGraphDataContainer> FBP075Data(new QCPGraphDataContainer);
	for (auto it = FBPData->getMedian().begin(); it != FBPData->getMedian().end(); ++it)
		FBP075Data->add(QCPGraphData(it->first, FBPData->getCentralRegion().getMax(it->first)));
	qcp->addGraph();
	qcp->graph()->setVisible(false);
	qcp->graph()->removeFromLegend();
	qcp->graph()->setData(FBP075Data);
	qcp->graph()->setName("Third Quartile");
	qcp->graph()->setPen(QPen(Qt::NoPen));
	qcp->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> FBP025Data(new QCPGraphDataContainer);
	for (auto it = FBPData->getMedian().begin(); it != FBPData->getMedian().end(); ++it)
		FBP025Data->add(QCPGraphData(it->first, FBPData->getCentralRegion().getMin(it->first)));
	qcp->addGraph();
	qcp->graph()->setVisible(false);
	qcp->graph()->removeFromLegend();
	qcp->graph()->setData(FBP025Data);
	qcp->graph()->setName("Interquartile Range");
	qcp->graph()->setPen(QPen(Qt::NoPen));
	qcp->graph()->setBrush(QColor(200, 200, 200, 255));
	qcp->graph()->setChannelFillGraph(qcp->graph(qcp->graphCount() - 2));
	qcp->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> medianData(new QCPGraphDataContainer);
	for (auto it = FBPData->getMedian().begin(); it != FBPData->getMedian().end(); ++it)
		medianData->add(QCPGraphData(it->first, it->second));
	qcp->addGraph();
	qcp->graph()->setVisible(false);
	qcp->graph()->removeFromLegend();
	qcp->graph()->setName("Median");
	qcp->graph()->setPen(QPen(QColor(0, 0, 0, 255), 5));
	qcp->graph()->setData(medianData);
	qcp->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> MaxData(new QCPGraphDataContainer);
	for (auto it = FBPData->getMedian().begin(); it != FBPData->getMedian().end(); ++it)
		MaxData->add(QCPGraphData(it->first, FBPData->getEnvelope().getMax(it->first)));
	qcp->addGraph();
	qcp->graph()->setVisible(false);
	qcp->graph()->removeFromLegend();
	qcp->graph()->setData(MaxData);
	qcp->graph()->setName("Upper Whisker");
	qcp->graph()->setPen(QPen(QColor(255, 0, 0, 255), 3));
	qcp->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> MinData(new QCPGraphDataContainer);
	for (auto it = FBPData->getMedian().begin(); it != FBPData->getMedian().end(); ++it)
		MinData->add(QCPGraphData(it->first, FBPData->getEnvelope().getMin(it->first)));
	qcp->addGraph();
	qcp->graph()->setVisible(false);
	qcp->graph()->removeFromLegend();
	qcp->graph()->setData(MinData);
	qcp->graph()->setName("Lower Whisker");
	qcp->graph()->setPen(QPen(QColor(0, 0, 255, 255), 3));
	qcp->graph()->setSelectable(QCP::stNone);
}

void dlg_DynamicVolumeLines::showBkgrdThrRanges(QCustomPlot* qcp)
{
	QCPItemStraightLine *thrLine = new QCPItemStraightLine(qcp);
	thrLine->setObjectName("BkgrdThrLine");
	thrLine->setVisible(cb_BkgrdThrLine->isChecked());
	thrLine->setAntialiased(false);
	thrLine->setLayer("background");
	thrLine->setPen(QPen(Qt::darkGray, 0, Qt::DotLine));
	thrLine->point1->setTypeX(QCPItemPosition::ptAxisRectRatio);
	thrLine->point1->setTypeY(QCPItemPosition::ptPlotCoords);
	thrLine->point1->setAxes(qcp->xAxis, qcp->yAxis);
	thrLine->point1->setAxisRect(qcp->axisRect());
	thrLine->point1->setCoords(0.0, sb_BkgrdThr->value());
	thrLine->point2->setTypeX(QCPItemPosition::ptAxisRectRatio);
	thrLine->point2->setTypeY(QCPItemPosition::ptPlotCoords);
	thrLine->point2->setAxes(qcp->xAxis, qcp->yAxis);
	thrLine->point2->setAxisRect(qcp->axisRect());
	thrLine->point2->setCoords(1.0, sb_BkgrdThr->value());
	thrLine->setClipToAxisRect(true);

	for (auto it = m_bkgrdThrRangeList.begin(); it != m_bkgrdThrRangeList.end(); ++it)
	{
		QCPItemRect *bkgrdRect = new QCPItemRect(qcp);
		bkgrdRect->setAntialiased(false);
		bkgrdRect->setLayer("background");
		bkgrdRect->setPen(QPen(Qt::NoPen));
		bkgrdRect->setBrush(QBrush(QColor(255, 235, 215)));
		bkgrdRect->topLeft->setTypeX(QCPItemPosition::ptPlotCoords);
		bkgrdRect->topLeft->setTypeY(QCPItemPosition::ptAxisRectRatio);
		bkgrdRect->topLeft->setAxes(qcp->xAxis, qcp->yAxis);
		bkgrdRect->topLeft->setAxisRect(qcp->axisRect());
		bkgrdRect->bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
		bkgrdRect->bottomRight->setTypeY(QCPItemPosition::ptAxisRectRatio);
		bkgrdRect->bottomRight->setAxes(qcp->xAxis, qcp->yAxis);
		bkgrdRect->bottomRight->setAxisRect(qcp->axisRect());
		bkgrdRect->setClipToAxisRect(true);

		double tlCoordX = 0.0, brCoordX = 1.0;
		if (qcp->objectName().contains("nonlinear"))
		{
			tlCoordX = it->lower;
			brCoordX = it->upper;
		}
		else
		{
			auto start = qLowerBound(m_nonlinearMappingVec.begin(), m_nonlinearMappingVec.end(), it->lower);
			tlCoordX = start - m_nonlinearMappingVec.begin();
			auto end = qLowerBound(m_nonlinearMappingVec.begin(), m_nonlinearMappingVec.end(), it->upper);
			brCoordX = end - m_nonlinearMappingVec.begin();
		}

		bkgrdRect->topLeft->setCoords(tlCoordX, 0.0);
		bkgrdRect->bottomRight->setCoords(brCoordX, 1.0);
	}
}

void  dlg_DynamicVolumeLines::checkHistVisMode(int lowerIdx, int upperIdx)
{
	if ((upperIdx - lowerIdx) <= sb_RngSwtVal->value() && m_histVisMode)	// TODO: remove magic number; better strategy
	{
		m_histVisMode = false;
		switchLevelOfDetail(m_histVisMode, cb_showFBP, cb_FBPView, sl_FBPTransparency,
			m_nonlinearScaledPlot, m_linearScaledPlot, m_scalingWidget);
		if (cb_showFBP->isChecked())
		{
			switchFBPMode(cb_FBPView->currentText(), m_nonlinearScaledPlot, m_linearScaledPlot,
				m_DatasetIntensityMap.size(), sl_FBPTransparency);
		}
		else
		{
			sl_FBPTransparency->hide();
			for (int i = 0; i < m_selGraphList.size(); ++i)
				m_selGraphList[i]->setVisible(true);
		}
	}
	else if ((upperIdx - lowerIdx) > sb_RngSwtVal->value() && !m_histVisMode)
	{
		m_histVisMode = true;
		switchLevelOfDetail(m_histVisMode, cb_showFBP, cb_FBPView, sl_FBPTransparency,
			m_nonlinearScaledPlot, m_linearScaledPlot, m_scalingWidget);
		for (int i = 0; i < m_nonlinearScaledPlot->graphCount(); ++i)
		{
			m_nonlinearScaledPlot->graph(i)->setVisible(false);
			m_linearScaledPlot->graph(i)->setVisible(false);
		}
	}
}

void dlg_DynamicVolumeLines::syncLinearXAxis(QCPRange nonlinearXRange)
{
	QCPRange boundedRange;
	if (nonlinearXRange.lower < m_nonlinearMappingVec.first() &&
		(nonlinearXRange.upper > m_nonlinearMappingVec.last()))
	{
		boundedRange.lower = m_nonlinearMappingVec.first();
		boundedRange.upper = m_nonlinearMappingVec.last();
		m_nonlinearScaledPlot->xAxis->setRange(boundedRange);
		return;
	}
	if (nonlinearXRange.lower < m_nonlinearMappingVec.first())
	{
		boundedRange.lower = m_nonlinearMappingVec.first();
		boundedRange.upper = m_nonlinearMappingVec.first() + nonlinearXRange.size();
		if (boundedRange.upper > m_nonlinearMappingVec.last())
			boundedRange.upper = m_nonlinearMappingVec.last();
		nonlinearXRange = boundedRange;
		m_nonlinearScaledPlot->xAxis->setRange(boundedRange);
	}
	else if (nonlinearXRange.upper > m_nonlinearMappingVec.last())
	{
		boundedRange.lower = m_nonlinearMappingVec.last() - nonlinearXRange.size();
		boundedRange.upper = m_nonlinearMappingVec.last();
		if (boundedRange.lower < m_nonlinearMappingVec.first())
			boundedRange.lower = m_nonlinearMappingVec.first();
		nonlinearXRange = boundedRange;
		m_nonlinearScaledPlot->xAxis->setRange(boundedRange);
	}

	auto lower = qLowerBound(m_nonlinearMappingVec.begin(),
		m_nonlinearMappingVec.end(), nonlinearXRange.lower);
	int lowerIdx = lower - m_nonlinearMappingVec.begin() - 1;
	if (lowerIdx < 0) lowerIdx = 0;
	double lowerDistToNextPoint = m_nonlinearMappingVec[lowerIdx + 1] -
		m_nonlinearMappingVec[lowerIdx];
	double lowerDistToCurrPoint = nonlinearXRange.lower -
		m_nonlinearMappingVec[lowerIdx];
	if (lowerDistToCurrPoint < 0) lowerDistToCurrPoint = 0;

	auto upper = qLowerBound(m_nonlinearMappingVec.begin(),
		m_nonlinearMappingVec.end(), nonlinearXRange.upper);
	int upperIdx = upper - m_nonlinearMappingVec.begin();
	double upperDistToNextPoint = 0.0, upperDistToCurrPoint = 0.0;
	if (upperIdx < m_nonlinearMappingVec.size())
	{
		upperDistToNextPoint = m_nonlinearMappingVec[upperIdx] - m_nonlinearMappingVec[upperIdx - 1];
		upperDistToCurrPoint = nonlinearXRange.upper - m_nonlinearMappingVec[upperIdx - 1];
	}
	else
	{
		upperIdx = m_nonlinearMappingVec.size() - 1;
		upperDistToNextPoint = 1.0;
		upperDistToCurrPoint = 1.0;
		nonlinearXRange.upper = m_nonlinearMappingVec.last();
	}

	double linearLowerBinBoarder = lowerIdx + lowerDistToCurrPoint / lowerDistToNextPoint;
	double linearUpperBinBoarder = upperIdx - 1 + upperDistToCurrPoint / upperDistToNextPoint;

	m_linearScaledPlot->xAxis->blockSignals(true);
	m_linearScaledPlot->xAxis->setRange(linearLowerBinBoarder, linearUpperBinBoarder);
	m_linearScaledPlot->xAxis->blockSignals(false);

	checkHistVisMode(lowerIdx, upperIdx);

	if (!m_histVisMode)
	{
		m_scalingWidget->setRange(lowerIdx, upperIdx, nonlinearXRange.lower - m_nonlinearMappingVec[lowerIdx],
			m_nonlinearMappingVec[upperIdx] - nonlinearXRange.upper, lowerDistToCurrPoint / lowerDistToNextPoint,
			upperDistToCurrPoint / upperDistToNextPoint);
	}
	else
	{
		double lowerBinBoarder = nonlinearXRange.lower / m_stepSize;
		double lowerVisibleBinBoarder = ceil(lowerBinBoarder);
		double lowerVisibleRest = lowerVisibleBinBoarder - lowerBinBoarder;
		double upperBinBoarder = (nonlinearXRange.upper /*- nonlinearXRange.lower*/) / m_stepSize;
		double upperVisibleBinBoarder = floor(upperBinBoarder);
		double upperVisibleRest = upperBinBoarder - upperVisibleBinBoarder;
		double lowerLinearVisibleRest = m_linearHistBinBoarderVec[lowerVisibleBinBoarder - 1] - linearLowerBinBoarder;
		double upperLinearVisibleRest = linearUpperBinBoarder - m_linearHistBinBoarderVec[upperVisibleBinBoarder - 1];
		m_scalingWidget->setOverviewRange(lowerVisibleBinBoarder,
			upperVisibleBinBoarder, lowerVisibleRest, upperVisibleRest, lowerLinearVisibleRest,
			upperLinearVisibleRest, m_histBinImpFunctAvgVec, m_linearHistBinBoarderVec);
	}

	m_scalingWidget->setCursorPos(m_linearIdxLine->positions()[0]->pixelPosition().x(),
		m_nonlinearIdxLine->positions()[0]->pixelPosition().x());

	m_scalingWidget->update();
}

void dlg_DynamicVolumeLines::syncNonlinearXAxis(QCPRange linearXRange)
{
	QCPRange boundedRange = linearXRange;
	if (linearXRange.lower < 0 &&
		(linearXRange.upper > m_nonlinearMappingVec.size() - 1))
	{
		boundedRange.lower = 0;
		boundedRange.upper = m_nonlinearMappingVec.size() - 1;
		m_linearScaledPlot->xAxis->setRange(boundedRange);
		return;
	}
	if (linearXRange.lower < 0)
	{
		boundedRange.lower = 0;
		boundedRange.upper = linearXRange.size();
		if (boundedRange.upper > m_nonlinearMappingVec.size() - 1)
			boundedRange.upper = m_nonlinearMappingVec.size() - 1;
		linearXRange = boundedRange;
		m_linearScaledPlot->xAxis->setRange(boundedRange);
	}
	else if (linearXRange.upper > m_nonlinearMappingVec.size() - 1)
	{
		boundedRange.lower = m_nonlinearMappingVec.size() - 1 - linearXRange.size();
		boundedRange.upper = m_nonlinearMappingVec.size() - 1;
		if (boundedRange.lower < 0) boundedRange.lower = 0;
		linearXRange = boundedRange;
		m_linearScaledPlot->xAxis->setRange(boundedRange);
	}

	double lowerDistToNextPoint = m_nonlinearMappingVec[ceil(linearXRange.lower)] -
		m_nonlinearMappingVec[floor(linearXRange.lower)];
	double lowerDistToCurrPoint = linearXRange.lower - floor(linearXRange.lower);
	if (lowerDistToCurrPoint < 0) lowerDistToCurrPoint = 0;

	double upperDistToNextPoint = 1.0, upperDistToCurrPoint = 0.0;
	if (ceil(linearXRange.upper) < m_nonlinearMappingVec.size())
	{
		upperDistToNextPoint =
			m_nonlinearMappingVec[ceil(linearXRange.upper)] -
			m_nonlinearMappingVec[floor(linearXRange.upper)];
		upperDistToCurrPoint =
			linearXRange.upper - floor(linearXRange.upper);
	}

	double newLower = m_nonlinearMappingVec[floor(linearXRange.lower)] +
		lowerDistToCurrPoint * lowerDistToNextPoint;
	double newUpper = m_nonlinearMappingVec[floor(linearXRange.upper)] +
		upperDistToCurrPoint * upperDistToNextPoint;

	m_nonlinearScaledPlot->xAxis->blockSignals(true);
	m_nonlinearScaledPlot->xAxis->setRange(newLower, newUpper);
	m_nonlinearScaledPlot->xAxis->blockSignals(false);

	checkHistVisMode(linearXRange.lower, linearXRange.upper);

	if (!m_histVisMode)
	{
		m_scalingWidget->setRange(
			floor(linearXRange.lower),
			ceil(linearXRange.upper),
			lowerDistToCurrPoint * lowerDistToNextPoint,
			(ceil(linearXRange.upper) - linearXRange.upper) * upperDistToNextPoint,
			lowerDistToCurrPoint,
			upperDistToCurrPoint);
	}
	else
	{
		double lowerBinBoarder = newLower / m_stepSize;
		double lowerVisibleBinBoarder = ceil(lowerBinBoarder);
		double lowerVisibleRest = lowerVisibleBinBoarder - lowerBinBoarder;
		double upperBinBoarder = (newUpper /*- nonlinearXRange.lower*/) / m_stepSize;
		double upperVisibleBinBoarder = floor(upperBinBoarder);
		double upperVisibleRest = upperBinBoarder - upperVisibleBinBoarder;
		double lowerLinearVisibleRest = m_linearHistBinBoarderVec[lowerVisibleBinBoarder - 1] - linearXRange.lower;
		double upperLinearVisibleRest = linearXRange.upper - m_linearHistBinBoarderVec[upperVisibleBinBoarder - 1];
		m_scalingWidget->setOverviewRange(lowerVisibleBinBoarder,
			upperVisibleBinBoarder, lowerVisibleRest, upperVisibleRest, lowerLinearVisibleRest,
			upperLinearVisibleRest, m_histBinImpFunctAvgVec, m_linearHistBinBoarderVec);
	}
	m_scalingWidget->update();
}

void dlg_DynamicVolumeLines::syncYAxis(QCPRange linearYRange)
{
	QCPAxis *axis = qobject_cast<QCPAxis *>(QObject::sender());
	QCustomPlot *plotU = qobject_cast<QCustomPlot *>(axis->parentPlot());
	QCustomPlot *plotP;
	plotU == m_linearScaledPlot ?
		plotP = m_nonlinearScaledPlot :
		plotP = m_linearScaledPlot;
	QCPRange boundedRange = linearYRange;
	double lowerLimit = m_minEnsembleIntensity - offsetY;
	double upperLimit = m_maxEnsembleIntensity + offsetY;
	if (linearYRange.lower <  lowerLimit &&
		linearYRange.upper > upperLimit)
	{
		boundedRange.lower = lowerLimit;
		boundedRange.upper = upperLimit;
		plotU->yAxis->setRange(boundedRange);
		return;
	}
	if (linearYRange.lower < lowerLimit)
	{
		boundedRange.lower = lowerLimit;
		boundedRange.upper = lowerLimit + linearYRange.size();
		if (boundedRange.upper > upperLimit)
			boundedRange.upper = upperLimit;
		plotU->yAxis->setRange(boundedRange);
	}
	else if (linearYRange.upper > upperLimit)
	{
		boundedRange.lower = upperLimit - linearYRange.size();
		boundedRange.upper = upperLimit;
		if (boundedRange.lower < lowerLimit)
			boundedRange.lower = lowerLimit;
		plotU->yAxis->setRange(boundedRange);
	}
	plotP->yAxis->setRange(boundedRange);
}

void dlg_DynamicVolumeLines::mousePress(QMouseEvent* e)
{
	QCustomPlot *plot = qobject_cast<QCustomPlot*>(QObject::sender());
	if (e->modifiers() == Qt::ControlModifier)
		plot->setSelectionRectMode(QCP::srmSelect);
	else
		plot->setSelectionRectMode(QCP::srmNone);
}

void dlg_DynamicVolumeLines::mouseMove(QMouseEvent* e)
{
	QCustomPlot *plot = qobject_cast<QCustomPlot*>(QObject::sender());

	if (plot->graphCount() < 1)
		return;

	if (e->pos().x() < plot->axisRect()->left() ||
		e->pos().x() > plot->axisRect()->right())
		return;

	QVector<double> distList;
	for (int i = 0; i < m_DatasetIntensityMap.size(); ++i)
		distList.append(plot->graph(i)->selectTest(
			QPoint(e->pos().x(), e->pos().y()), true));
	auto minDist = std::min_element(distList.begin(), distList.end());
	auto idx = minDist - distList.begin();
	auto x = plot->xAxis->pixelToCoord(e->pos().x());
	auto y = plot->yAxis->pixelToCoord(e->pos().y());

	if (plot == m_nonlinearScaledPlot)
	{
		m_nonlinearIdxLine->point1->setCoords(x, 0.0);
		m_nonlinearIdxLine->point2->setCoords(x, 1.0);
		m_nonlinearDataPointInfo->position->setPixelPosition(
			QPoint(e->pos().x() + 5, e->pos().y() - 15));

		auto v = qLowerBound(m_nonlinearMappingVec.begin(), m_nonlinearMappingVec.end(), x);
		int hilbertIdx = v - m_nonlinearMappingVec.begin() - 1;
		if (v - m_nonlinearMappingVec.begin() == 0) hilbertIdx = 0;

		if (*minDist >= 0 && *minDist < 2.0 && plot->graph(idx)->visible())
		{
			m_nonlinearDataPointInfo->setText(QString("%1\nHilbertIdx: %2\nIntensity: %3")
				.arg(m_nonlinearScaledPlot->graph(idx)->name())
				.arg(hilbertIdx).arg((int)y));
		}
		else
		{
			m_nonlinearDataPointInfo->setText(QString("HilbertIdx: %1\nIntensity: %2")
				.arg(hilbertIdx).arg((int)y));
		}

		double nonlinearVecPosDist = 1.0, currNonlinearDist = 0.0;
		if (v - m_nonlinearMappingVec.begin() < m_nonlinearMappingVec.size())
		{
			nonlinearVecPosDist = m_nonlinearMappingVec[hilbertIdx + 1] - m_nonlinearMappingVec[hilbertIdx];
			currNonlinearDist = x - m_nonlinearMappingVec[hilbertIdx];
		}

		m_linearIdxLine->point1->setCoords(hilbertIdx + (currNonlinearDist / nonlinearVecPosDist), 0.0);
		m_linearIdxLine->point2->setCoords(hilbertIdx + (currNonlinearDist / nonlinearVecPosDist), 1.0);
		m_linearDataPointInfo->position->setPixelPosition(QPointF(m_linearScaledPlot->xAxis->coordToPixel(
			hilbertIdx + (currNonlinearDist / nonlinearVecPosDist)) + 5,
			m_linearScaledPlot->yAxis->coordToPixel(y) - 15));
		m_linearDataPointInfo->setText(QString("HilbertIdx: %1\nIntensity: %2").arg(hilbertIdx).arg((int)y));

		m_scalingWidget->setCursorPos(m_linearScaledPlot->xAxis->coordToPixel(
			hilbertIdx + (currNonlinearDist / nonlinearVecPosDist)), e->pos().x());
	}
	else
	{
		m_linearIdxLine->point1->setCoords(x, 0.0);
		m_linearIdxLine->point2->setCoords(x, 1.0);
		m_linearDataPointInfo->position->setPixelPosition(QPointF(e->pos().x() + 5, e->pos().y() - 15));
		m_linearDataPointInfo->setText(QString("HilbertIdx: %1\nIntensity: %2").arg(int(x)).arg((int)y));

		if (*minDist >= 0 && *minDist < 2.0 && plot->graph(idx)->visible())
		{
			m_linearDataPointInfo->setText(QString("%1\nHilbertIdx: %2\nIntensity: %3")
				.arg(m_linearScaledPlot->graph(idx)->name())
				.arg(int(x)).arg((int)y));
		}
		else
		{
			m_nonlinearDataPointInfo->setText(QString("HilbertIdx: %1\nIntensity: %2")
				.arg(int(x)).arg((int)y));
		}

		double nonlinearDistToNextPoint = m_nonlinearMappingVec[ceil(x)] - m_nonlinearMappingVec[floor(x)];
		double distToCurrPoint = x - floor(x);
		double nonlinearXCoord = m_nonlinearMappingVec[floor(x)] + distToCurrPoint * nonlinearDistToNextPoint;

		m_nonlinearIdxLine->point1->setCoords(nonlinearXCoord, 0.0);
		m_nonlinearIdxLine->point2->setCoords(nonlinearXCoord, 1.0);
		m_nonlinearDataPointInfo->position->setCoords(
			m_nonlinearScaledPlot->xAxis->pixelToCoord(
				m_nonlinearScaledPlot->xAxis->coordToPixel(nonlinearXCoord) + 5),
			m_nonlinearScaledPlot->yAxis->pixelToCoord(e->pos().y() - 15));
		m_nonlinearDataPointInfo->setText(QString("HilbertIdx: %1\nIntensity: %2").arg(int(x)).arg((int)y));

		m_scalingWidget->setCursorPos(e->pos().x(), 
			m_nonlinearScaledPlot->xAxis->coordToPixel(nonlinearXCoord));
	}
	m_nonlinearScaledPlot->layer("cursor")->replot();
	m_linearScaledPlot->layer("cursor")->replot();
}

void dlg_DynamicVolumeLines::mouseWheel(QWheelEvent* e)
{
	QCustomPlot *plot = qobject_cast<QCustomPlot*>(QObject::sender());
	switch (e->modifiers())
	{
		case Qt::AltModifier:
			plot->axisRect()->setRangeZoom(Qt::Vertical);
			break;
		case Qt::ControlModifier:
			plot->axisRect()->setRangeZoom(Qt::Horizontal);
			break;
		default:
			plot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
			break;
	}
}

void dlg_DynamicVolumeLines::selectionChangedByUser()
{
	// TODO: change transfer function for "hidden" values; should be HistogramRangeMinimum-1 
	wgtContainer->GetRenderWindow()->GetRenderers()->RemoveAllItems();
	wgtContainer->GetRenderWindow()->AddRenderer(m_mrvBGRen);

	QCustomPlot *plotU = qobject_cast<QCustomPlot*>(QObject::sender());
	QCustomPlot *plotP;
	plotU == m_nonlinearScaledPlot ?
		plotP = m_linearScaledPlot :
		plotP = m_nonlinearScaledPlot;

	auto selGraphsList = plotU->selectedGraphs();
	QList<QCPGraph *> selVisibleGraphsList;
	for (auto graph : selGraphsList)
		if (graph->visible())
			selVisibleGraphsList.append(graph);

	QCPDataSelection sel;
	if (!selVisibleGraphsList.isEmpty())
	{
		m_mrvTxtAct->VisibilityOff();
		for (auto graph : selVisibleGraphsList)
		{
			for (int i = 0; i < m_DatasetIntensityMap.size(); ++i)
			{
				if (m_DatasetIntensityMap[i].first == graph->name())
				{
					plotP->graph(i)->setSelection(graph->selection());
					break;
				}
			}
			for (auto range : graph->selection().dataRanges())
				sel.addDataRange(range, false);
		}
		sel.simplify();
	}
	else
	{
		m_mrvTxtAct->VisibilityOn();
		iARenderer * ren = m_mdiChild->getRenderer();
		vtkRenderWindow * renWin = ren->GetRenderWindow();
		renWin->GetRenderers()->GetFirstRenderer()->RemoveActor(ren->selectedActor);
		renWin->Render();
		for (int i = 0; i < m_DatasetIntensityMap.size(); ++i)
			plotP->graph(i)->setSelection(plotU->graph(i)->selection());
	}

	setSelectionForRenderer(selVisibleGraphsList);
	m_scalingWidget->setSel(sel);
	m_scalingWidget->update();
	m_nonlinearScaledPlot->replot();
	m_linearScaledPlot->replot();
}

void dlg_DynamicVolumeLines::legendClick(QCPLegend* legendU,
	QCPAbstractLegendItem* legendUItem, QMouseEvent* e)
{
	QCustomPlot *plotU = qobject_cast<QCustomPlot *>(QObject::sender());
	QCustomPlot *plotP = plotU == m_linearScaledPlot ?
		plotP = m_nonlinearScaledPlot : plotP = m_linearScaledPlot;

	int legendPItemIdx = 0;
	for (int i = 0; i < legendU->itemCount(); ++i)
	{
		if (legendU->item(i) == legendUItem)
		{
			legendPItemIdx = i;
			break;
		}
	}

	if ((e->button() == Qt::LeftButton) && !cb_showFBP->isChecked() && legendUItem)
	{
		QCPPlottableLegendItem *ptliU = qobject_cast<QCPPlottableLegendItem*>(
			legendUItem);
		QCPPlottableLegendItem *ptliP = qobject_cast<QCPPlottableLegendItem*>(
			plotP->legend->item(legendPItemIdx));

		if (!m_selGraphList.contains(qobject_cast<QCPGraph *>(ptliU->plottable())))
		{
			updateLegendAndGraphVisibility(ptliU, plotP, legendPItemIdx, 1.0, true);
			m_selGraphList.append(qobject_cast<QCPGraph *>(ptliU->plottable()));
			m_selGraphList.append(qobject_cast<QCPGraph *>(ptliP->plottable()));
		}
		else if (m_selGraphList.contains(qobject_cast<QCPGraph *>(ptliU->plottable())) && m_selGraphList.size() > 2)
		{
			m_selGraphList.removeOne(qobject_cast<QCPGraph *>(ptliU->plottable()));
			m_selGraphList.removeOne(qobject_cast<QCPGraph *>(ptliP->plottable()));
		}

		for (int i = 0; i < legendU->itemCount(); ++i)
		{
			QCPPlottableLegendItem *ptli = qobject_cast<QCPPlottableLegendItem*>(
				legendU->item(i));
			if (!m_selGraphList.contains(qobject_cast<QCPGraph *>(ptli->plottable())))
				updateLegendAndGraphVisibility(ptli, plotP, i, 0.3, false);
		}
	}
	else if ((e->button() == Qt::RightButton) &&
		!cb_showFBP->isChecked() && m_selGraphList.size() > 0)
	{
		m_selGraphList.clear();

		for (int i = 0; i < legendU->itemCount(); ++i)
		{
			QCPPlottableLegendItem *ptliU = qobject_cast<QCPPlottableLegendItem*>(
				legendU->item(i));
			QCPPlottableLegendItem *ptliP = qobject_cast<QCPPlottableLegendItem*>(
				plotP->legend->item(i));
			updateLegendAndGraphVisibility(ptliU, plotP, i, 1.0, true);
			m_selGraphList.append(qobject_cast<QCPGraph *>(ptliU->plottable()));
			m_selGraphList.append(qobject_cast<QCPGraph *>(ptliP->plottable()));
		}
	}
	m_nonlinearScaledPlot->replot();
}

void dlg_DynamicVolumeLines::updateDynamicVolumeLines()
{
	m_imgDataList.clear();
	m_DatasetIntensityMap.clear();
	generateHilbertIdx();
}

void dlg_DynamicVolumeLines::showFBPGraphs()
{
	if (cb_showFBP->isChecked())
	{
		switchFBPMode(cb_FBPView->currentText(), m_nonlinearScaledPlot, m_linearScaledPlot,
			m_DatasetIntensityMap.size(), sl_FBPTransparency);
	}
	else
	{
		sl_FBPTransparency->hide();
		for (int i = 0; i < m_nonlinearScaledPlot->graphCount(); ++i)
		{
			if (i >= m_DatasetIntensityMap.size())
			{
				hideGraphandRemoveFromLegend(m_nonlinearScaledPlot, m_linearScaledPlot, i);
			}
			else if (m_selGraphList.contains(m_nonlinearScaledPlot->graph(i)))
			{
				showGraphandAddToLegend(m_nonlinearScaledPlot, m_linearScaledPlot, i);
				
			}
			else
			{
				m_nonlinearScaledPlot->graph(i)->addToLegend();
				m_linearScaledPlot->graph(i)->addToLegend();
				QColor c = m_nonlinearScaledPlot->legend->itemWithPlottable(
					qobject_cast<QCPGraph *>(m_nonlinearScaledPlot->graph(i)))->textColor();
				c.setAlphaF(0.3);
				m_nonlinearScaledPlot->legend->itemWithPlottable(
					qobject_cast<QCPGraph *>(m_nonlinearScaledPlot->graph(i)))->setTextColor(c);
				m_linearScaledPlot->legend->itemWithPlottable(
					qobject_cast<QCPGraph *>(m_linearScaledPlot->graph(i)))->setTextColor(c);
			}
		}
	}
	m_nonlinearScaledPlot->replot();
	m_linearScaledPlot->replot();
}

void dlg_DynamicVolumeLines::showBkgrdThrLine()
{
	m_nonlinearScaledPlot->findChild<QCPItemStraightLine*>("BkgrdThrLine")->setVisible(cb_BkgrdThrLine->isChecked());
	m_linearScaledPlot->findChild<QCPItemStraightLine*>("BkgrdThrLine")->setVisible(cb_BkgrdThrLine->isChecked());
	m_nonlinearScaledPlot->replot();
	m_linearScaledPlot->replot();
}

void dlg_DynamicVolumeLines::updateFBPView()
{
	if (cb_showFBP->isChecked())
		showFBPGraphs();
}

void dlg_DynamicVolumeLines::setFBPTransparency(int value)
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
		m_linearScaledPlot->graph(i)->setPen(p);
		if (m_nonlinearScaledPlot->graph(i)->name() == "Interquartile Range")
		{
			b = m_nonlinearScaledPlot->graph(i)->brush();
			c = m_nonlinearScaledPlot->graph(i)->brush().color();
			c.setAlpha(alpha);
			b.setColor(c);
			m_nonlinearScaledPlot->graph(i)->setBrush(b);
			m_linearScaledPlot->graph(i)->setBrush(b);
		}
	}
	m_nonlinearScaledPlot->replot();
	m_linearScaledPlot->replot();
}

void dlg_DynamicVolumeLines::setSubHistBinCntFlag()
{
	m_subHistBinCntChanged = true;
	visualize();
}

void dlg_DynamicVolumeLines::selectCompLevel()
{
	if ((sb_LowerCompLevelThr->value() > sb_UpperCompLevelThr->value()) ||
		(sb_UpperCompLevelThr->value() < sb_LowerCompLevelThr->value()))
	{
		QMessageBox msgBox;
		msgBox.setText("Lower/upper ranges are flipped.");
		msgBox.setInformativeText("Should Dynamic Volume Lines automatically correct "
			"the ranges and proceed?");
		msgBox.setWindowTitle("Dynamic Volume Lines");
		msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
		double tmp;
		switch (msgBox.exec())
		{
		case QMessageBox::Ok:
			tmp = sb_LowerCompLevelThr->value();
			sb_LowerCompLevelThr->setValue(sb_UpperCompLevelThr->value());
			sb_UpperCompLevelThr->setValue(tmp);
			break;
		case QMessageBox::Cancel:
			return;
		}
	}

	QCPDataSelection selCompLvlRanges;
	double sectionStart = -1.0;
	for (int i = 0; i < m_impFunctVec.size(); ++i)
	{
		if (((m_impFunctVec[i]) < sb_LowerCompLevelThr->value() ||
			(m_impFunctVec[i]) > sb_UpperCompLevelThr->value()) &&
			sectionStart >= 0.0)
		{
			selCompLvlRanges.addDataRange(QCPDataRange(sectionStart, i));
			sectionStart = -1.0;
		}
		else if ((m_impFunctVec[i]) >= sb_LowerCompLevelThr->value() &&
			(m_impFunctVec[i]) <= sb_UpperCompLevelThr->value() &&
			sectionStart == -1.0)
		{
			sectionStart = i-1;
			if (sectionStart < 0)
				sectionStart = 0;
		}
		else if (((m_impFunctVec[i]) >= sb_LowerCompLevelThr->value() ||
			(m_impFunctVec[i]) <= sb_LowerCompLevelThr->value()) &&
			sectionStart >= 0.0 && i == m_impFunctVec.size()-1)
		{
			selCompLvlRanges.addDataRange(QCPDataRange(sectionStart, i+1));
		}
	}

	wgtContainer->GetRenderWindow()->GetRenderers()->RemoveAllItems();
	wgtContainer->GetRenderWindow()->AddRenderer(m_mrvBGRen);
	m_mrvTxtAct->SetVisibility(selCompLvlRanges.dataRanges().empty());

	QList<QCPGraph *> visibleGraphsList;
	for (int i = 0; i < m_linearScaledPlot->graphCount(); ++i)
	{
		if (m_linearScaledPlot->graph(i)->visible())
		{
			m_linearScaledPlot->graph(i)->setSelection(selCompLvlRanges);
			m_nonlinearScaledPlot->graph(i)->setSelection(selCompLvlRanges);
			if (!selCompLvlRanges.dataRanges().empty())
				visibleGraphsList.append(m_linearScaledPlot->graph(i));
		}
	}

	setSelectionForRenderer(visibleGraphsList);
	m_scalingWidget->setSel(selCompLvlRanges);
	m_scalingWidget->update();
	m_nonlinearScaledPlot->replot();
	m_linearScaledPlot->replot();
}

template <typename T>
void setVoxelIntensity(
	vtkImageData* inputImage, unsigned int x, unsigned int y,
	unsigned int z, double intensity)
{
	T *v = static_cast< T* >(inputImage->GetScalarPointer(x, y, z));
	*v = intensity;
}

void dlg_DynamicVolumeLines::setSelectionForRenderer(QList<QCPGraph *> visSelGraphList)
{
	auto datasetsList = m_datasetsDir.entryList();
	for (unsigned int i = 0; i < visSelGraphList.size(); ++i)
	{
		int datasetIdx = datasetsList.indexOf(visSelGraphList[i]->name());
		auto selHilbertIndices = visSelGraphList[i]->selection().dataRanges();
		auto pathSteps = m_DatasetIntensityMap[datasetIdx].second.size();
		auto data = m_DatasetIntensityMap[datasetIdx].second;
		int scalarType = m_imgDataList[datasetIdx]->GetScalarType();

		if (selHilbertIndices.size() < 1)
		{
			for (unsigned int hIdx = 0; hIdx < pathSteps; ++hIdx)
				VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_imgDataList[datasetIdx],
					data[hIdx].x, data[hIdx].y, data[hIdx].z, data[hIdx].intensity);
			m_nonlinearDataPointInfo->setVisible(false);
		}
		else
		{
			double const *r = m_mdiChild->getHistogram()->xBounds();
			for (unsigned int hIdx = 0; hIdx < pathSteps; ++hIdx)
			{
				bool showVoxel = false;
				for (unsigned int j = 0; j < selHilbertIndices.size(); ++j)
				{
					if (hIdx >= selHilbertIndices.at(j).begin() && hIdx < selHilbertIndices.at(j).end())
					{
						VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_imgDataList[datasetIdx],
							data[hIdx].x, data[hIdx].y, data[hIdx].z, data[hIdx].intensity);
						//qDebug() << "M3DRV shows voxel at Pos: " << data[hIdx].x << data[hIdx].y << data[hIdx].z << " Hidx: " << hIdx;
						showVoxel = true;
						break;
					}
				}
				if (!showVoxel)
					VTK_TYPED_CALL(setVoxelIntensity, scalarType, m_imgDataList[datasetIdx],
						data[hIdx].x, data[hIdx].y, data[hIdx].z, r[0]);
			}
		}

		m_imgDataList[datasetIdx]->Modified();
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

		vtkSmartPointer<vtkColorTransferFunction> cTF = vtkSmartPointer<vtkColorTransferFunction>::New();
		cTF->ShallowCopy(m_mdiChild->getColorTransferFunction());
		int index = cTF->GetSize() - 1;
		double val[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
		cTF->GetNodeValue(index, val);
		val[1] = 1.0;	val[2] = 0.0;	val[3] = 0.0;
		cTF->SetNodeValue(index, val);
		vtkSmartPointer<vtkPiecewiseFunction> oTF = vtkSmartPointer<vtkPiecewiseFunction>::New();
		oTF->ShallowCopy(m_mdiChild->getPiecewiseFunction());

		iASimpleTransferFunction tf(cTF, oTF);
		//iASimpleTransferFunction tf(m_mdiChild->getColorTransferFunction(), m_mdiChild->getPiecewiseFunction());
		auto ren = vtkSmartPointer<vtkRenderer>::New();
		ren->SetLayer(1);
		ren->SetActiveCamera(m_mdiChild->getRenderer()->getCamera());
		ren->GetActiveCamera()->ParallelProjectionOn();
		ren->SetViewport(fmod(i, viewportCols) * fieldLengthX,
			1 - (ceil((i + 1.0) / viewportCols) / viewportRows),
			fmod(i, viewportCols) * fieldLengthX + fieldLengthX,
			1 - (ceil((i + 1.0) / viewportCols) / viewportRows) + fieldLengthY);
		ren->AddViewProp(cornerAnnotation);
		ren->ResetCamera();
		m_volRen = QSharedPointer<iAVolumeRenderer>(new iAVolumeRenderer(&tf, m_imgDataList[datasetIdx]));
		m_volRen->ApplySettings(m_mdiChild->GetVolumeSettings());
		m_volRen->AddTo(ren);
		m_volRen->AddBoundingBoxTo(ren);
		wgtContainer->GetRenderWindow()->AddRenderer(ren);
	}
	wgtContainer->GetRenderWindow()->Render();
}

void dlg_DynamicVolumeLines::setNoSelectionForPlots()
{
	QCPDataSelection noSelection;
	for (int i = 0; i < m_nonlinearScaledPlot->graphCount(); ++i)
	{
		m_nonlinearScaledPlot->graph(i)->setSelection(noSelection);
		m_linearScaledPlot->graph(i)->setSelection(noSelection);
	}

	wgtContainer->GetRenderWindow()->GetRenderers()->RemoveAllItems();
	m_mrvTxtAct->VisibilityOn();
	wgtContainer->GetRenderWindow()->AddRenderer(m_mrvBGRen);
	wgtContainer->GetRenderWindow()->Render();

	m_scalingWidget->setSel(noSelection);
	m_scalingWidget->update();

	m_nonlinearScaledPlot->replot();
	m_linearScaledPlot->replot(); 
}

void dlg_DynamicVolumeLines::setSelectionForPlots(vtkPoints *selCellPoints)
{
	// TODO: a single index idx is presented as a line (from idx to idx+1) in the plots,
	// just paint a point in the plots for single indices (performance?)
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
				//qDebug() << j << ".selCellPoints :" << pt[0] << pt[1] << pt[2] << "at Hidx: " << i;
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
	

	QList<QString> visibleGraphsNameList;
	for (int i = 0; i < m_DatasetIntensityMap.size(); ++i)
		if (m_nonlinearScaledPlot->graph(i)->visible())
			visibleGraphsNameList.append(m_nonlinearScaledPlot->graph(i)->name());

	QCPDataSelection sel;
	QList<QCPGraph *> selVisibleGraphsList;
	if (!visibleGraphsNameList.isEmpty())
	{
		m_mrvTxtAct->VisibilityOff();
		for (int i = 0; i < m_nonlinearScaledPlot->graphCount(); ++i)
		{
			if (visibleGraphsNameList.contains(m_nonlinearScaledPlot->graph(i)->name()))
			{
				m_nonlinearScaledPlot->graph(i)->setSelection(selection);
				m_linearScaledPlot->graph(i)->setSelection(selection);
				selVisibleGraphsList.append(m_nonlinearScaledPlot->graph(i));
			}
		}
		for (auto graph : selVisibleGraphsList)
		{
			graph->setSelection(selection);
			for (auto range : graph->selection().dataRanges())
				sel.addDataRange(range, false);
		}
		sel.simplify();
	}
	else
	{
		m_mrvTxtAct->VisibilityOn();
	}

	setSelectionForRenderer(selVisibleGraphsList);
	m_scalingWidget->setSel(sel);
	m_scalingWidget->update();
	m_nonlinearScaledPlot->replot();
	m_linearScaledPlot->replot();
}

void dlg_DynamicVolumeLines::compLevelRangeChanged()
{
	QVector<double> range(2);
	if (sb_UpperCompLevelThr->value() < sb_LowerCompLevelThr->value())
	{
		range[0] = sb_UpperCompLevelThr->value();
		range[1] = sb_LowerCompLevelThr->value();
	}
	else
	{
		range[0] = sb_LowerCompLevelThr->value();
		range[1] = sb_UpperCompLevelThr->value();
	}
	emit compLevelRangeChanged(range);
}

void dlg_DynamicVolumeLines::setupDebugPlot()
{
	m_debugPlot = new QCustomPlot(dockWidgetContents);
	m_debugPlot->setCursor(QCursor(Qt::CrossCursor));
	m_debugPlot->installEventFilter(this);
	m_debugPlot->plotLayout()->insertRow(0);
	auto *helperPlotTitle = new QCPTextElement(m_debugPlot, "Debug Plot", QFont("sans", 14));
	m_debugPlot->plotLayout()->addElement(0, 0, helperPlotTitle);
	m_debugPlot->setOpenGl(true);
	m_debugPlot->setPlottingHints(QCP::phFastPolylines);  // Graph/Curve lines are drawn with a faster method
	m_debugPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
	m_debugPlot->legend->setVisible(true);
	m_debugPlot->legend->setFont(QFont("Helvetica", 11));
	m_debugPlot->xAxis->setLabel("x");
	m_debugPlot->xAxis->setLabel("HilbertIdx");
	m_debugPlot->yAxis->setLabel("y");
	m_debugPlot->yAxis->setLabel("Importance/Cummulative Importance");
	m_debugPlot->xAxis->grid()->setVisible(false);
	m_debugPlot->xAxis->grid()->setSubGridVisible(false);
	m_debugPlot->yAxis->grid()->setVisible(false);
	m_debugPlot->yAxis->grid()->setSubGridVisible(false);
	m_debugPlot->xAxis2->setVisible(true);
	m_debugPlot->xAxis2->setSubTicks(false);	// set to 'false if too many ticks
	m_debugPlot->xAxis2->setTicks(false);
	m_debugPlot->yAxis2->setLabel("Cummulative Importance");
	m_debugPlot->yAxis2->setVisible(true);
	//m_debugPlot->yAxis2->setSubTicks(false);	// Debug
	//m_debugPlot->yAxis2->setTicks(false);		// Debug
	PlotsContainer_verticalLayout->addWidget(m_debugPlot);
}

void dlg_DynamicVolumeLines::showDebugPlot()
{
	m_impFuncPlotData = QSharedPointer<QCPGraphDataContainer>(new QCPGraphDataContainer);
	m_integralImpFuncPlotData = QSharedPointer<QCPGraphDataContainer>(new QCPGraphDataContainer);
	for (int i = 0; i < m_nonlinearMappingVec.size(); ++i)
	{
		m_impFuncPlotData->add(QCPGraphData(double(i), m_impFunctVec[i]));
		m_integralImpFuncPlotData->add(QCPGraphData(double(i), i == 0 ? m_impFunctVec[i] :
			m_nonlinearMappingVec[i - 1] + m_impFunctVec[i]));
	}
	m_debugPlot->addGraph();
	m_debugPlot->graph()->setPen(QPen(Qt::blue));
	m_debugPlot->graph()->setName("Importance Function");
	m_debugPlot->graph()->setData(m_impFuncPlotData);
	m_debugPlot->addGraph(m_debugPlot->xAxis2, m_debugPlot->yAxis2);
	m_debugPlot->graph()->setPen(QPen(Qt::red));
	m_debugPlot->graph()->setName("Cummulative Importance Function");
	m_debugPlot->graph()->setData(m_integralImpFuncPlotData);
}

void dlg_DynamicVolumeLines::showCompressionLevel()
{
	double rgb[3]; QColor c;
	for (int hIdx = 1; hIdx < m_nonlinearMappingVec.size(); ++hIdx)
	{
		m_compLvlLUT->GetColor(m_impFunctVec[hIdx], rgb);
		c.setRgbF(rgb[0], rgb[1], rgb[2]);
		QCPItemRect *nlRect = new QCPItemRect(m_nonlinearScaledPlot);
		nlRect->setAntialiased(false);
		nlRect->setLayer("background");
		nlRect->setPen(QPen(Qt::NoPen));
		nlRect->setBrush(QBrush(c));
		nlRect->topLeft->setTypeX(QCPItemPosition::ptPlotCoords);
		nlRect->topLeft->setTypeY(QCPItemPosition::ptAxisRectRatio);
		nlRect->topLeft->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
		nlRect->topLeft->setAxisRect(m_nonlinearScaledPlot->axisRect());
		nlRect->topLeft->setCoords(m_nonlinearMappingVec[hIdx - 1], 0.96);
		nlRect->bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
		nlRect->bottomRight->setTypeY(QCPItemPosition::ptAxisRectRatio);
		nlRect->bottomRight->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
		nlRect->bottomRight->setAxisRect(m_nonlinearScaledPlot->axisRect());
		nlRect->bottomRight->setCoords(m_nonlinearMappingVec[hIdx], 1.0);
		nlRect->setClipToAxisRect(true);

		QCPItemRect *lRect = new QCPItemRect(m_linearScaledPlot);
		lRect->setAntialiased(false);
		lRect->setLayer("background");
		lRect->setPen(QPen(Qt::NoPen));
		lRect->setBrush(QBrush(c));
		lRect->topLeft->setTypeX(QCPItemPosition::ptPlotCoords);
		lRect->topLeft->setTypeY(QCPItemPosition::ptAxisRectRatio);
		lRect->topLeft->setAxes(m_linearScaledPlot->xAxis, m_linearScaledPlot->yAxis);
		lRect->topLeft->setAxisRect(m_linearScaledPlot->axisRect());
		lRect->topLeft->setCoords(hIdx - 1, 0.96);
		lRect->bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
		lRect->bottomRight->setTypeY(QCPItemPosition::ptAxisRectRatio);
		lRect->bottomRight->setAxes(m_linearScaledPlot->xAxis, m_linearScaledPlot->yAxis);
		lRect->bottomRight->setAxisRect(m_linearScaledPlot->axisRect());
		lRect->bottomRight->setCoords(hIdx, 1.0);
		lRect->setClipToAxisRect(true);
	}
}
