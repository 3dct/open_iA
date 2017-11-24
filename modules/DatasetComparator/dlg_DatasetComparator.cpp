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
#include "iAScalingWidget.h"
#include "iAPerceptuallyUniformLUT.h"
#include "iALinearColorGradientBar.h"

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

#include <vtkLookupTable.h>
#include <vtkScalarBarActor.h>
#include <QVTKWidget.h>

//#include <omp.h>
//#include <sys/timeb.h>
#include "iAConsole.h"


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
	m_MultiRendererView(new multi3DRendererView()),
	m_mrvRenWin(vtkSmartPointer<vtkRenderWindow>::New()),
	m_mrvBGRen(vtkSmartPointer<vtkRenderer>::New()),
	m_mrvTxtAct(vtkSmartPointer<vtkTextActor>::New()),
	m_scalingWidget(0),
	m_lut(vtkSmartPointer<vtkLookupTable>::New())
{
	setupGUIElements();
	setupNonlinearScaledPlot();
	setupScalingWidget();
	setupLinearScaledPlot();
	//setupDebugPlot();
	setupGUIConnections();
	setupMultiRendererView();
	generateHilbertIdx();
}

dlg_DatasetComparator::~dlg_DatasetComparator()
{}

void dlg_DatasetComparator::setupGUIElements()
{
	l_opacity->hide();
	sl_fbpTransparency->hide();

	QMap<double, QColor> colormap;
	iAPerceptuallyUniformLUT::BuildLinearLUT(m_lut, 0.0, 1.0, 256);
	for (double i = 0.0; i <= 1.0; i += 0.25)
	{
		double c[3];
		m_lut->GetColor(i, c);
		QColor color;
		color.setRgbF(c[0], c[1], c[2]);
		colormap.insert(i, color);
	}
	iALinearColorGradientBar *colorBar = new iALinearColorGradientBar(this, colormap);
	QVBoxLayout *lutLayoutHB = new QVBoxLayout(this);
	lutLayoutHB->setMargin(0);
	lutLayoutHB->addWidget(colorBar);
	lutLayoutHB->update();
	scalarBarWidget->setLayout(lutLayoutHB);
}

void dlg_DatasetComparator::setupScalingWidget()
{
	m_scalingWidget = new iAScalingWidget(this);
	m_scalingWidget->setNonlinearAxis(m_nonlinearScaledPlot->xAxis);
	m_scalingWidget->setNonlinearScalingVector(m_nonlinearMappingVec, m_impFunctVec);
	PlotsContainer_verticalLayout->addWidget(m_scalingWidget);
}

void dlg_DatasetComparator::syncLinearXAxis(QCPRange nonlinearXRange)
{
	QCPRange boundedRange;
	m_nonlinearScaledPlot->xAxis->blockSignals(true);
	if (nonlinearXRange.lower < m_nonlinearMappingVec.first() &&
		(nonlinearXRange.upper > m_nonlinearMappingVec.last()))
	{
		boundedRange.lower = m_nonlinearMappingVec.first();
		boundedRange.upper = m_nonlinearMappingVec.last();
		m_nonlinearScaledPlot->xAxis->setRange(boundedRange);
		m_nonlinearScaledPlot->xAxis->blockSignals(false);
		return;
	}
	if (nonlinearXRange.lower < m_nonlinearMappingVec.first())
	{  
		boundedRange.lower = m_nonlinearMappingVec.first();
		boundedRange.upper = m_nonlinearMappingVec.first() + nonlinearXRange.size();
		nonlinearXRange = boundedRange;
		m_nonlinearScaledPlot->xAxis->setRange(boundedRange);
	}
	else if (nonlinearXRange.upper > m_nonlinearMappingVec.last())
	{
		boundedRange.lower = m_nonlinearMappingVec.last() - nonlinearXRange.size();
		boundedRange.upper = m_nonlinearMappingVec.last();
		nonlinearXRange = boundedRange;
		m_nonlinearScaledPlot->xAxis->setRange(boundedRange);
	}
	m_nonlinearScaledPlot->xAxis->blockSignals(false);

	auto lower = qLowerBound(m_nonlinearMappingVec.begin(),
		m_nonlinearMappingVec.end(), nonlinearXRange.lower);
	int lowerIdx = lower - m_nonlinearMappingVec.begin() - 1;
	if (lowerIdx < 0) lowerIdx = 0;
	double lowerDistToNextPoint = m_nonlinearMappingVec[lowerIdx+1] -
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
		upperDistToNextPoint = m_nonlinearMappingVec[upperIdx] - m_nonlinearMappingVec[upperIdx-1];
		upperDistToCurrPoint = nonlinearXRange.upper - m_nonlinearMappingVec[upperIdx-1];
	}
	else
	{
			upperIdx = m_nonlinearMappingVec.size() - 1;
			upperDistToNextPoint = 1.0;
			upperDistToCurrPoint = 1.0;
			nonlinearXRange.upper = m_nonlinearMappingVec.last();
	}

	double newLower = lowerIdx + lowerDistToCurrPoint / lowerDistToNextPoint,
		newUpper = upperIdx - 1 + upperDistToCurrPoint / upperDistToNextPoint;
	m_linearScaledPlot->xAxis->blockSignals(true);
	m_linearScaledPlot->xAxis->setRange(newLower, newUpper);
	m_linearScaledPlot->xAxis->blockSignals(false);
	m_linearScaledPlot->replot();	

	m_scalingWidget->setRange(
		lowerIdx,
		upperIdx,
		nonlinearXRange.lower - m_nonlinearMappingVec[lowerIdx],
		m_nonlinearMappingVec[upperIdx] - nonlinearXRange.upper,
		lowerDistToCurrPoint / lowerDistToNextPoint,
		upperDistToCurrPoint / upperDistToNextPoint);
	m_scalingWidget->setCursorPositions(
		m_linearIdxLine->positions()[0]->pixelPosition().x(), 
		m_nonlinearIdxLine->positions()[0]->pixelPosition().x());
	m_scalingWidget->update();
}

void dlg_DatasetComparator::syncLinearYAxis(QCPRange nonlinearYRange)
{
	QCPRange boundedRange = nonlinearYRange;
	m_nonlinearScaledPlot->yAxis->blockSignals(true);
	if (nonlinearYRange.lower < m_minEnsembleIntensity && 
		nonlinearYRange.upper > m_maxEnsembleIntensity)
	{
		boundedRange.lower = m_minEnsembleIntensity;
		boundedRange.upper = m_maxEnsembleIntensity;
		m_nonlinearScaledPlot->yAxis->setRange(boundedRange);
		m_nonlinearScaledPlot->yAxis->blockSignals(false);
		return;
	}
	if (nonlinearYRange.lower < m_minEnsembleIntensity)
	{
		boundedRange.lower = m_minEnsembleIntensity;
		boundedRange.upper = m_minEnsembleIntensity + nonlinearYRange.size();
		m_nonlinearScaledPlot->yAxis->setRange(boundedRange);
	}
	else if (nonlinearYRange.upper > m_maxEnsembleIntensity)
	{
		boundedRange.lower = m_maxEnsembleIntensity - nonlinearYRange.size();
		boundedRange.upper = m_maxEnsembleIntensity;
		m_nonlinearScaledPlot->yAxis->setRange(boundedRange);
	}
	m_nonlinearScaledPlot->yAxis->blockSignals(false);

	m_linearScaledPlot->yAxis->blockSignals(true);
	m_linearScaledPlot->yAxis->setRange(boundedRange);
	m_linearScaledPlot->yAxis->blockSignals(false);
	m_linearScaledPlot->replot();
}

void dlg_DatasetComparator::syncNonlinearXAxis(QCPRange linearXRange)
{
	QCPRange boundedRange = linearXRange;
	m_linearScaledPlot->xAxis->blockSignals(true);
	if (linearXRange.lower < 0 &&
		(linearXRange.upper > m_nonlinearMappingVec.size() - 1))
	{
		boundedRange.lower = 0;
		boundedRange.upper = m_nonlinearMappingVec.size() - 1;
		m_linearScaledPlot->xAxis->setRange(boundedRange);
		m_linearScaledPlot->xAxis->blockSignals(false);
		return;
	}
	if (linearXRange.lower < 0)
	{
		boundedRange.lower = 0;
		boundedRange.upper = linearXRange.size();
		if (boundedRange.upper > m_nonlinearMappingVec.size() - 1)
			boundedRange.lower = m_nonlinearMappingVec.size() - 1;
		linearXRange = boundedRange;
		m_linearScaledPlot->xAxis->setRange(boundedRange);
	}
	else if (linearXRange.upper > m_nonlinearMappingVec.size() - 1)
	{
		boundedRange.lower = m_nonlinearMappingVec.size() - 1 - linearXRange.size();
		if (boundedRange.lower < 0) boundedRange.lower = 0;
		boundedRange.upper = m_nonlinearMappingVec.size() - 1;
		linearXRange = boundedRange;
		m_linearScaledPlot->xAxis->setRange(boundedRange);
	}
	m_linearScaledPlot->xAxis->blockSignals(false);
	
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
	m_nonlinearScaledPlot->replot();

	m_scalingWidget->setRange(
		floor(linearXRange.lower), 
		ceil(linearXRange.upper),
		lowerDistToCurrPoint * lowerDistToNextPoint,
		(ceil(linearXRange.upper) - linearXRange.upper) * upperDistToNextPoint,
		lowerDistToCurrPoint,
		upperDistToCurrPoint);
	m_scalingWidget->update();
}

void dlg_DatasetComparator::syncNonlinearYAxis(QCPRange linearYRange)
{
	QCPRange boundedRange = linearYRange;
	m_linearScaledPlot->yAxis->blockSignals(true);
	if (linearYRange.lower < m_minEnsembleIntensity &&
		linearYRange.upper > m_maxEnsembleIntensity)
	{
		boundedRange.lower = m_minEnsembleIntensity;
		boundedRange.upper = m_maxEnsembleIntensity;
		m_linearScaledPlot->yAxis->setRange(boundedRange);
		m_linearScaledPlot->yAxis->blockSignals(false);
		return;
	}
	if (linearYRange.lower < m_minEnsembleIntensity)
	{
		boundedRange.lower = m_minEnsembleIntensity;
		boundedRange.upper = m_minEnsembleIntensity + linearYRange.size();
		m_linearScaledPlot->yAxis->setRange(boundedRange);
	}
	else if (linearYRange.upper > m_maxEnsembleIntensity)
	{
		boundedRange.lower = m_maxEnsembleIntensity - linearYRange.size();
		boundedRange.upper = m_maxEnsembleIntensity;
		m_linearScaledPlot->yAxis->setRange(boundedRange);
	}
	m_linearScaledPlot->yAxis->blockSignals(false);

	m_nonlinearScaledPlot->yAxis->blockSignals(true);
	m_nonlinearScaledPlot->yAxis->setRange(boundedRange);
	m_nonlinearScaledPlot->yAxis->blockSignals(false);
	m_nonlinearScaledPlot->replot();
}

void dlg_DatasetComparator::setupLinearScaledPlot()
{
	m_linearScaledPlot = new QCustomPlot(dockWidgetContents);
	m_linearScaledPlot->axisRect()->setAutoMargins(QCP::msNone);
	m_linearScaledPlot->axisRect()->setMargins(QMargins(58, 1, 3, 32));
	m_linearScaledPlot->setCursor(QCursor(Qt::CrossCursor));
	m_linearScaledPlot->installEventFilter(this);  // To catche key press event
	/*m_linearScaledPlot->plotLayout()->insertRow(0);
	auto *linearScaledPlotTitle = new QCPTextElement(m_linearScaledPlot,
		"Normal Scaled Hilbert Plot", QFont("Helvetica", 11, QFont::Bold));
	m_linearScaledPlot->plotLayout()->addElement(0, 0, linearScaledPlotTitle);*/
	m_linearScaledPlot->setOpenGl(true);
	//m_linearScaledPlot->setNoAntialiasingOnDrag(true);
	//m_linearScaledPlot->setNotAntialiasedElements(QCP::aeAll);
	m_linearScaledPlot->setPlottingHints(QCP::phFastPolylines);  // Graph/Curve lines are drawn with a faster method
	m_linearScaledPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
	m_linearScaledPlot->legend->setVisible(true);
	m_linearScaledPlot->legend->setFont(QFont("Helvetica", 9));
	m_linearScaledPlot->xAxis->setLabel("Hilbert index");
	m_linearScaledPlot->xAxis->grid()->setVisible(false);
	m_linearScaledPlot->xAxis->grid()->setSubGridVisible(false);
	m_linearScaledPlot->xAxis->setTickLabels(true);
	m_linearScaledPlot->xAxis->setSubTicks(false);	 
	m_linearScaledPlot->xAxis->setNumberFormat("f");
	m_linearScaledPlot->xAxis->setNumberPrecision(0);
	m_linearScaledPlot->yAxis->setLabel("Gray Value Intensity");
	m_linearScaledPlot->yAxis->grid()->setSubGridVisible(false);
	m_linearScaledPlot->yAxis->grid()->setVisible(false);
	m_linearScaledPlot->xAxis2->setVisible(true);
	m_linearScaledPlot->xAxis2->setSubTicks(false);	// set to 'false if too many ticks
	m_linearScaledPlot->xAxis2->setTicks(false);
	m_linearScaledPlot->yAxis2->setVisible(true);
	m_linearScaledPlot->yAxis2->setTickLabels(false);
	m_linearScaledPlot->yAxis2->setTicks(false);
	m_linearScaledPlot->yAxis2->setSubTicks(false);
	connect(m_linearScaledPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(syncNonlinearXAxis(QCPRange)));
	connect(m_linearScaledPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(syncNonlinearYAxis(QCPRange)));
	connect(m_linearScaledPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));
	connect(m_linearScaledPlot, SIGNAL(legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)),
		this, SLOT(legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)));

	m_lVisibilityButton = new QToolButton(this);
	m_lVisibilityButton->setStyleSheet("border: 1px solid; margin-left: 3px;");
	m_lVisibilityButton->setMinimumSize(QSize(0, 0));
	m_lVisibilityButton->setMaximumSize(QSize(13, 10));
	m_lVisibilityButton->setIcon(QIcon(":/images/minus.png"));
	m_lVisibilityButton->setIconSize(QSize(10, 10));
	connect(m_lVisibilityButton, SIGNAL(clicked()), this, SLOT(changePlotVisibility()));
	
	PlotsContainer_verticalLayout->addWidget(m_lVisibilityButton);
	PlotsContainer_verticalLayout->addWidget(m_linearScaledPlot);
}

void dlg_DatasetComparator::setupNonlinearScaledPlot()
{
	m_nonlinearScaledPlot = new QCustomPlot(dockWidgetContents);
	m_nonlinearScaledPlot->axisRect()->setAutoMargins(QCP::msNone);
	m_nonlinearScaledPlot->axisRect()->setMargins(QMargins(58, 1, 3, 32));
	m_nonlinearScaledPlot->setCursor(QCursor(Qt::CrossCursor));
	m_nonlinearScaledPlot->installEventFilter(this);  // To catch key press event
	//m_nonlinearScaledPlot->plotLayout()->insertRow(0);
	//auto *nonlinearScaledPlotTitle = new QCPTextElement(m_nonlinearScaledPlot,
	//	"Nonlinear Scaled Hilbert Plot", QFont("Helvetica", 11, QFont::Bold));
	//m_nonlinearScaledPlot->plotLayout()->addElement(0, 0, nonlinearScaledPlotTitle);
	m_nonlinearScaledPlot->setOpenGl(true);
	//m_nonlinearScaledPlot->setNoAntialiasingOnDrag(true);
	//m_nonlinearScaledPlot->setNotAntialiasedElements(QCP::aeAll);
	//m_nonlinearScaledPlot->setBackground(Qt::darkGray);
	m_nonlinearScaledPlot->setPlottingHints(QCP::phFastPolylines);  // Graph/Curve lines are drawn with a faster method
	m_nonlinearScaledPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	m_nonlinearScaledPlot->setMultiSelectModifier(Qt::ShiftModifier);
	m_nonlinearScaledPlot->legend->setVisible(true);
	m_nonlinearScaledPlot->legend->setFont(QFont("Helvetica", 9));
	m_nonlinearScaledPlot->xAxis->setLabel("Hilbert index");
	m_nonlinearScaledPlot->xAxis->grid()->setVisible(false);
	m_nonlinearScaledPlot->xAxis->grid()->setSubGridVisible(false);
	m_nonlinearScaledPlot->xAxis->setTickLabels(true);
	m_nonlinearScaledPlot->xAxis->setSubTicks(true);	  // set to 'false if too many ticks
	m_nonlinearScaledPlot->xAxis->setNumberFormat("f"); 
	m_nonlinearScaledPlot->xAxis->setNumberPrecision(0);
	m_nonlinearScaledPlot->yAxis->setLabel("Gray Value Intensity");
	m_nonlinearScaledPlot->yAxis->grid()->setSubGridVisible(false);
	m_nonlinearScaledPlot->yAxis->grid()->setVisible(false);
	m_nonlinearScaledPlot->xAxis2->setVisible(true);
	m_nonlinearScaledPlot->xAxis2->setTickLabels(false);
	m_nonlinearScaledPlot->xAxis2->setSubTicks(false);	// set to 'false if too many ticks
	m_nonlinearScaledPlot->xAxis2->setTicks(false);
	m_nonlinearScaledPlot->yAxis2->setVisible(true);
	m_nonlinearScaledPlot->yAxis2->setTickLabels(false);
	m_nonlinearScaledPlot->yAxis2->setTicks(false);
	m_nonlinearScaledPlot->yAxis2->setSubTicks(false);
	connect(m_nonlinearScaledPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(syncLinearXAxis(QCPRange)));
	connect(m_nonlinearScaledPlot->yAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(syncLinearYAxis(QCPRange)));
	connect(m_nonlinearScaledPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(mousePress(QMouseEvent*)));
	connect(m_nonlinearScaledPlot, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(mouseMove(QMouseEvent*)));
	connect(m_nonlinearScaledPlot, SIGNAL(selectionChangedByUser()), this, SLOT(selectionChangedByUser()));
	connect(m_nonlinearScaledPlot, SIGNAL(legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)), 
		this, SLOT(legendClick(QCPLegend*, QCPAbstractLegendItem*, QMouseEvent*)));
	connect(m_mdiChild->getRaycaster(), SIGNAL(cellsSelected(vtkPoints*)),
		this, SLOT(setSelectionFromRenderer(vtkPoints*)));

	m_nlVisibilityButton = new QToolButton(this);
	m_nlVisibilityButton->setStyleSheet("border: 1px solid; margin-left: 3px;");
	m_nlVisibilityButton->setMinimumSize(QSize(0, 0));
	m_nlVisibilityButton->setMaximumSize(QSize(13, 10));
	m_nlVisibilityButton->setIcon(QIcon(":/images/minus.png"));
	m_nlVisibilityButton->setIconSize(QSize(10, 10));
	connect(m_nlVisibilityButton, SIGNAL(clicked()), this, SLOT(changePlotVisibility()));

	PlotsContainer_verticalLayout->addWidget(m_nlVisibilityButton);
	PlotsContainer_verticalLayout->addWidget(m_nonlinearScaledPlot);
}

void dlg_DatasetComparator::setupDebugPlot()
{
	m_debugPlot = new QCustomPlot(dockWidgetContents);
	m_debugPlot->setCursor(QCursor(Qt::CrossCursor));
	m_debugPlot->installEventFilter(this);
	m_debugPlot->plotLayout()->insertRow(0);
	auto *helperPlotTitle = new QCPTextElement(m_debugPlot, "Debug Plot", QFont("sans", 14));
	m_debugPlot->plotLayout()->addElement(0, 0, helperPlotTitle);
	m_debugPlot->setOpenGl(true);
	//m_debugPlot->setNoAntialiasingOnDrag(true);
	//m_debugPlot->setBackground(Qt::darkGray);
	m_debugPlot->setPlottingHints(QCP::phFastPolylines);  // Graph/Curve lines are drawn with a faster method
	m_debugPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectPlottables | QCP::iMultiSelect);
	m_debugPlot->setMultiSelectModifier(Qt::ShiftModifier);
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
	//m_debugPlot->yAxis2->setSubTicks(false);	// set to 'false if too many ticks
	//m_debugPlot->yAxis2->setTicks(false);
	PlotsContainer_verticalLayout->addWidget(m_debugPlot);
}

void dlg_DatasetComparator::setupGUIConnections()
{
	connect(pB_Update, SIGNAL(clicked()), this, SLOT(updateDatasetComparator()));
	connect(cb_showFbp, SIGNAL(stateChanged(int)), this, SLOT(showFBPGraphs()));
	connect(cb_fbpView, SIGNAL(currentIndexChanged(int)), this, SLOT(updateFBPView()));
	connect(sl_fbpTransparency, SIGNAL(valueChanged(int)), this, SLOT(setFbpTransparency(int)));
	connect(sb_BkgrdThr, SIGNAL(valueChanged(double)), this, SLOT(visualize()));
	connect(cb_BkgrdThrLine, SIGNAL(stateChanged(int)), this, SLOT(showBkgrdThrLine()));
	connect(sb_nonlinearScalingFactor, SIGNAL(valueChanged(double)), this, SLOT(visualize()));
}

void dlg_DatasetComparator::changePlotVisibility()
{
	QToolButton *tb = qobject_cast<QToolButton*>(QObject::sender());
	if (tb == m_nlVisibilityButton)
	{
		m_nonlinearScaledPlot->isVisible() ? 
			tb->setIcon(QIcon(":/images/add.png")) :
			tb->setIcon(QIcon(":/images/minus.png"));
		m_nonlinearScaledPlot->setVisible(!m_nonlinearScaledPlot->isVisible());
		m_nonlinearScaledPlot->update();
	}
	else
	{
		m_linearScaledPlot->isVisible() ?
			tb->setIcon(QIcon(":/images/add.png")) :
			tb->setIcon(QIcon(":/images/minus.png"));
		m_linearScaledPlot->setVisible(!m_linearScaledPlot->isVisible());
		m_linearScaledPlot->update();
	}
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

void dlg_DatasetComparator::visualize()
{
	m_nonlinearScaledPlot->clearGraphs();
	m_nonlinearScaledPlot->clearItems();
	m_linearScaledPlot->clearItems();
	//m_debugPlot->clearGraphs();
	
	calcNonLinearMapping();
	//showDebugPlot();
	
	if (m_linearScaledPlot->graphCount() < 1)
	{
		for (auto it = m_DatasetIntensityMap.begin(); it != m_DatasetIntensityMap.end(); ++it)
		{
			m_linearScaledPlot->addGraph();
			m_linearScaledPlot->graph()->setPen(getDatasetPen(it - m_DatasetIntensityMap.begin(),
				m_DatasetIntensityMap.size(), 2, "Metro Colors (max. 20)"));
			m_linearScaledPlot->graph()->setName(it->first);
			QCPScatterStyle scatter;
			scatter.setShape(QCPScatterStyle::ssNone);	 // Check ssDisc/ssDot to show single selected points
			scatter.setSize(3.0);
			m_linearScaledPlot->graph()->setScatterStyle(scatter);
			QSharedPointer<QCPGraphDataContainer> graphData(new QCPGraphDataContainer);
			for (unsigned int i = 0; i < it->second.size(); ++i)
				graphData->add(QCPGraphData(double(i), it->second[i].intensity));
			m_linearScaledPlot->graph()->setData(graphData);
		}
	}

	m_linearDataPointInfo = new QCPItemText(m_linearScaledPlot);
	m_linearDataPointInfo->setFont(QFont("Helvetica", 8, QFont::Bold));
	m_linearDataPointInfo->setLayer("overlay");
	m_linearDataPointInfo->setVisible(true);
	m_linearDataPointInfo->setColor(Qt::red);

	m_linearIdxLine = new QCPItemStraightLine(m_linearScaledPlot);
	m_linearIdxLine->setVisible(true);
	m_linearIdxLine->setAntialiased(false);
	m_linearIdxLine->setLayer("overlay");
	m_linearIdxLine->setPen(QPen(Qt::red, 0, Qt::SolidLine));
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

	std::vector<iAFunction<double, double> *> functions;
	for (auto it = m_DatasetIntensityMap.begin(); it != m_DatasetIntensityMap.end(); ++it)
	{
		m_nonlinearScaledPlot->addGraph();
		m_nonlinearScaledPlot->graph()->setSelectable(QCP::stMultipleDataRanges);
		m_nonlinearScaledPlot->graph()->setPen(getDatasetPen(it - m_DatasetIntensityMap.begin(),
			m_DatasetIntensityMap.size(), 2, "Metro Colors (max. 20)"));
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
		functions.push_back(funct);
		m_nonlinearScaledPlot->graph()->setData(nonlinearScaledPlotData);
	}

	ModifiedDepthMeasure<double, double> measure;
	auto functionalBoxplotData = new iAFunctionalBoxplot<double, double>(functions, &measure, 2);
	setupFBPGraphs(functionalBoxplotData);
	
	m_nonlinearTicker = QSharedPointer<iANonLinearAxisTicker>(new iANonLinearAxisTicker);
	m_nonlinearTicker->setTickData(m_nonlinearMappingVec);
	m_nonlinearTicker->setAxis(m_nonlinearScaledPlot->xAxis);
	m_nonlinearScaledPlot->xAxis->setTicker(m_nonlinearTicker);
	m_nonlinearScaledPlot->xAxis2->setTicker(m_nonlinearTicker);

	m_nonlinearDataPointInfo = new QCPItemText(m_nonlinearScaledPlot);
	m_nonlinearDataPointInfo->setFont(QFont("Helvetica", 8, QFont::Bold));
	m_nonlinearDataPointInfo->setLayer("overlay");
	m_nonlinearDataPointInfo->setVisible(true);
	m_nonlinearDataPointInfo->setColor(Qt::red);

	m_nonlinearIdxLine = new QCPItemStraightLine(m_nonlinearScaledPlot);
	m_nonlinearIdxLine->setVisible(true);
	m_nonlinearIdxLine->setAntialiased(false);
	m_nonlinearIdxLine->setLayer("overlay");
	m_nonlinearIdxLine->setPen(QPen(Qt::red, 0, Qt::SolidLine));
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

	showBkgrdThrRanges();
	//showCompressionLevel();

	/*m_debugPlot->rescaleAxes();
	m_debugPlot->replot();*/
	m_linearScaledPlot->rescaleAxes();
	m_linearScaledPlot->replot();
	m_nonlinearScaledPlot->rescaleAxes();
	m_nonlinearScaledPlot->replot();

	m_scalingWidget->setRange(0, m_linearScaledPlot->xAxis->range().upper-1, 0, 0, 0 , 0);
	m_scalingWidget->setNonlinearAxis(m_nonlinearScaledPlot->xAxis);
	m_scalingWidget->setNonlinearScalingVector(m_nonlinearMappingVec, m_impFunctVec);
	m_scalingWidget->update();
}

void dlg_DatasetComparator::showDebugPlot()
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

void dlg_DatasetComparator::calcNonLinearMapping()
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
		double imp = 0.005;
		if (innerEnsembleDistList[i] >= 0.0)
		{
			imp = innerEnsembleDistList[i];
			imp /= maxInnerEnsableDist;
		}
		imp = pow(imp * 1, sb_nonlinearScalingFactor->value()); // //imp = pow(imp*2,-0.9);
		m_impFunctVec.append(imp);
		m_nonlinearMappingVec.append(i == 0 ? imp : m_nonlinearMappingVec[i - 1] + imp);

		if (innerEnsembleDistList[i] >= 0.0 && sectionStart >= 0.0 && i!= innerEnsembleDistList.size())
		{
			m_bkgrdThrRangeList.append(QCPRange(sectionStart, m_nonlinearMappingVec[i-1]));
			sectionStart = -1.0;
		}
		else if(innerEnsembleDistList[i] == -1.0 && sectionStart == -1.0)
		{
			sectionStart = m_nonlinearMappingVec[i];
		}
		else if (innerEnsembleDistList[i] == -1.0 && sectionStart >= 0.0 && i == innerEnsembleDistList.size()-1)
		{
			m_bkgrdThrRangeList.append(QCPRange(sectionStart, m_nonlinearMappingVec[i]));
			sectionStart = -1.0;
		}
	}
}

void dlg_DatasetComparator::showBkgrdThrRanges()
{
	QCPItemStraightLine *thrLine = new QCPItemStraightLine(m_nonlinearScaledPlot);
	thrLine->setVisible(cb_BkgrdThrLine->isChecked());
	thrLine->setAntialiased(false);
	thrLine->setLayer("background");
	thrLine->setPen(QPen(Qt::darkGray, 0, Qt::DotLine));
	thrLine->point1->setTypeX(QCPItemPosition::ptAxisRectRatio);
	thrLine->point1->setTypeY(QCPItemPosition::ptPlotCoords);
	thrLine->point1->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
	thrLine->point1->setAxisRect(m_nonlinearScaledPlot->axisRect());
	thrLine->point1->setCoords(0.0, sb_BkgrdThr->value());
	thrLine->point2->setTypeX(QCPItemPosition::ptAxisRectRatio);
	thrLine->point2->setTypeY(QCPItemPosition::ptPlotCoords);
	thrLine->point2->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
	thrLine->point2->setAxisRect(m_nonlinearScaledPlot->axisRect());
	thrLine->point2->setCoords(1.0, sb_BkgrdThr->value());
	thrLine->setClipToAxisRect(true);

	for (auto it = m_bkgrdThrRangeList.begin(); it != m_bkgrdThrRangeList.end(); ++it)
	{
		QCPItemRect *nonlinearBkgrdRect = new QCPItemRect(m_nonlinearScaledPlot);
		nonlinearBkgrdRect->setAntialiased(false);
		nonlinearBkgrdRect->setLayer("background");
		nonlinearBkgrdRect->setPen(QPen(Qt::NoPen));
		nonlinearBkgrdRect->setBrush(QBrush(QColor(255, 235, 215)));
		nonlinearBkgrdRect->topLeft->setTypeX(QCPItemPosition::ptPlotCoords);
		nonlinearBkgrdRect->topLeft->setTypeY(QCPItemPosition::ptAxisRectRatio);
		nonlinearBkgrdRect->topLeft->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
		nonlinearBkgrdRect->topLeft->setAxisRect(m_nonlinearScaledPlot->axisRect());
		nonlinearBkgrdRect->topLeft->setCoords(it->lower, 0.0);
		nonlinearBkgrdRect->bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
		nonlinearBkgrdRect->bottomRight->setTypeY(QCPItemPosition::ptAxisRectRatio);
		nonlinearBkgrdRect->bottomRight->setAxes(m_nonlinearScaledPlot->xAxis, m_nonlinearScaledPlot->yAxis);
		nonlinearBkgrdRect->bottomRight->setAxisRect(m_nonlinearScaledPlot->axisRect());
		nonlinearBkgrdRect->bottomRight->setCoords(it->upper, 1.0);
		nonlinearBkgrdRect->setClipToAxisRect(true);

		QCPItemRect *linearBkgrdRect = new QCPItemRect(m_linearScaledPlot);
		linearBkgrdRect->setAntialiased(false);
		linearBkgrdRect->setLayer("background");
		linearBkgrdRect->setPen(QPen(Qt::NoPen));
		linearBkgrdRect->setBrush(QBrush(QColor(255, 235, 215)));
		linearBkgrdRect->topLeft->setTypeX(QCPItemPosition::ptPlotCoords);
		linearBkgrdRect->topLeft->setTypeY(QCPItemPosition::ptAxisRectRatio);
		linearBkgrdRect->topLeft->setAxes(m_linearScaledPlot->xAxis, m_linearScaledPlot->yAxis);
		linearBkgrdRect->topLeft->setAxisRect(m_linearScaledPlot->axisRect());
		linearBkgrdRect->topLeft->setCoords(m_nonlinearMappingVec.indexOf(it->lower), 0.0);
		linearBkgrdRect->bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
		linearBkgrdRect->bottomRight->setTypeY(QCPItemPosition::ptAxisRectRatio);
		linearBkgrdRect->bottomRight->setAxes(m_linearScaledPlot->xAxis, m_linearScaledPlot->yAxis);
		linearBkgrdRect->bottomRight->setAxisRect(m_linearScaledPlot->axisRect());
		linearBkgrdRect->bottomRight->setCoords(m_nonlinearMappingVec.indexOf(it->upper), 1.0);
		linearBkgrdRect->setClipToAxisRect(true);
	}
}

void dlg_DatasetComparator::showCompressionLevel()
{
	double rgb[3];QColor c;
	for (int hIdx = 1; hIdx < m_nonlinearMappingVec.size(); ++hIdx)
	{
		m_lut->GetColor(m_impFunctVec[hIdx], rgb);
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
		nlRect->topLeft->setCoords(m_nonlinearMappingVec[hIdx-1], 0.96);
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
		lRect->topLeft->setCoords(hIdx-1, 0.96);
		lRect->bottomRight->setTypeX(QCPItemPosition::ptPlotCoords);
		lRect->bottomRight->setTypeY(QCPItemPosition::ptAxisRectRatio);
		lRect->bottomRight->setAxes(m_linearScaledPlot->xAxis, m_linearScaledPlot->yAxis);
		lRect->bottomRight->setAxisRect(m_linearScaledPlot->axisRect());
		lRect->bottomRight->setCoords(hIdx, 1.0);
		lRect->setClipToAxisRect(true);
	}
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
	m_imgDataList.clear();
	m_DatasetIntensityMap.clear();
	generateHilbertIdx();
}

void dlg_DatasetComparator::generateHilbertIdx()
{
	QThread* thread = new QThread;
	iAIntensityMapper * im = new iAIntensityMapper(m_datasetsDir, PathNameToId[cb_Paths->currentText()], 
		m_DatasetIntensityMap, m_imgDataList, m_minEnsembleIntensity, m_maxEnsembleIntensity);
	im->moveToThread(thread);
	connect(im, SIGNAL(error(QString)), this, SLOT(errorString(QString)));		//TODO: Handle error case
	connect(thread, SIGNAL(started()), im, SLOT(process()));
	connect(im, SIGNAL(finished()), thread, SLOT(quit()));
	connect(im, SIGNAL(finished()), im, SLOT(deleteLater()));
	connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
	//connect(thread, SIGNAL(finished()), this, SLOT(visualizePath()));
	connect(thread, SIGNAL(finished()), this, SLOT(visualize()));
	thread->start();
}

bool dlg_DatasetComparator::eventFilter(QObject* o, QEvent* e)
{
	if (e->type() == QEvent::KeyPress)
	{
		QKeyEvent *k = (QKeyEvent *)e;
		QString keyStr = QKeySequence(k->key()).toString();
		QCustomPlot* plot = qobject_cast<QCustomPlot*>(o);
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

void dlg_DatasetComparator::mousePress(QMouseEvent* e)
{
	if ((e->modifiers() & Qt::ControlModifier) == Qt::ControlModifier)
		m_nonlinearScaledPlot->setSelectionRectMode(QCP::srmSelect);
	else
		m_nonlinearScaledPlot->setSelectionRectMode(QCP::srmNone);
}

void dlg_DatasetComparator::mouseMove(QMouseEvent* e)
{
	//TODO: MouseMove also for linearScaledPlot
	if (m_nonlinearScaledPlot->graphCount() < 1)
		return;

	if (cb_showFbp->isChecked() && cb_fbpView->currentText() == "Alone")
		return;

	if (e->pos().x() < m_nonlinearScaledPlot->axisRect()->left() ||
		e->pos().x() > m_nonlinearScaledPlot->axisRect()->right())
		return;

	QVector<double> distList;
	for (int i = 0; i < m_DatasetIntensityMap.size(); ++i)
		distList.append(m_nonlinearScaledPlot->graph(i)->selectTest(
			QPoint(e->pos().x(), e->pos().y()), true));
	auto minDist = std::min_element(distList.begin(), distList.end());
	auto idx = minDist - distList.begin();
	auto x = m_nonlinearScaledPlot->xAxis->pixelToCoord(e->pos().x());
	auto y = m_nonlinearScaledPlot->yAxis->pixelToCoord(e->pos().y());
	m_nonlinearIdxLine->point1->setCoords(x, 0.0);
	m_nonlinearIdxLine->point2->setCoords(x, 1.0);
	m_nonlinearDataPointInfo->position->setPixelPosition(
		QPoint(e->pos().x() + 40, e->pos().y() - 15));
	auto v = qLowerBound(m_nonlinearMappingVec.begin(), m_nonlinearMappingVec.end(), x);
	int hilbertIdx = v - m_nonlinearMappingVec.begin() - 1;
	if (v - m_nonlinearMappingVec.begin() == 0) hilbertIdx = 0;

	if (*minDist >= 0 && *minDist < 2.0 && m_nonlinearScaledPlot->graph(idx)->visible())
	{
		m_nonlinearDataPointInfo->setText(QString("%1\n%2, %3")
			.arg(m_nonlinearScaledPlot->graph(idx)->name())
			.arg(hilbertIdx)
			.arg((int)y));
	}
	else
	{
		m_nonlinearDataPointInfo->setText(QString("%1, %2").arg(hilbertIdx).arg((int)y));
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
		hilbertIdx + (currNonlinearDist / nonlinearVecPosDist)) + 40,
		m_linearScaledPlot->yAxis->coordToPixel(y) - 15));
	m_linearDataPointInfo->setText(QString("%1, %2").arg(hilbertIdx).arg((int)y));
	
	m_nonlinearScaledPlot->layer("overlay")->replot();
	m_linearScaledPlot->layer("overlay")->replot();

	m_scalingWidget->setCursorPositions(m_linearScaledPlot->xAxis->coordToPixel(
		hilbertIdx + (currNonlinearDist / nonlinearVecPosDist)), e->pos().x());
}

template <typename T>
void setVoxelIntensity(
	vtkImageData* inputImage, unsigned int x, unsigned int y, 
	unsigned int z, double intensity)
{
	T *v = static_cast< T* >(inputImage->GetScalarPointer(x, y, z));
	*v = intensity;
}

void dlg_DatasetComparator::setupFBPGraphs(iAFunctionalBoxplot<double, double>* fbpData)
{
	QSharedPointer<QCPGraphDataContainer> fb_075Data(new QCPGraphDataContainer);
	for (auto it = fbpData->getMedian().begin(); it != fbpData->getMedian().end(); ++it)
		fb_075Data->add(QCPGraphData(it->first, fbpData->getCentralRegion().getMax(it->first)));
	m_nonlinearScaledPlot->addGraph();
	m_nonlinearScaledPlot->graph()->setVisible(false);
	m_nonlinearScaledPlot->graph()->removeFromLegend();
	m_nonlinearScaledPlot->graph()->setData(fb_075Data);
	m_nonlinearScaledPlot->graph()->setName("Third Quartile");
	m_nonlinearScaledPlot->graph()->setPen(QPen(Qt::NoPen));
	m_nonlinearScaledPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_025Data(new QCPGraphDataContainer);
	for (auto it = fbpData->getMedian().begin(); it != fbpData->getMedian().end(); ++it)
		fb_025Data->add(QCPGraphData(it->first, fbpData->getCentralRegion().getMin(it->first)));
	m_nonlinearScaledPlot->addGraph();
	m_nonlinearScaledPlot->graph()->setVisible(false);
	m_nonlinearScaledPlot->graph()->removeFromLegend();
	m_nonlinearScaledPlot->graph()->setData(fb_025Data);
	m_nonlinearScaledPlot->graph()->setName("Interquartile Range"); 
	m_nonlinearScaledPlot->graph()->setPen(QPen(Qt::NoPen));
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
	m_nonlinearScaledPlot->graph()->setPen(QPen(QColor(0, 0, 0, 255), 7));
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
	m_nonlinearScaledPlot->graph()->setPen(QPen(QColor(255, 0, 0, 255), 7));
	m_nonlinearScaledPlot->graph()->setSelectable(QCP::stNone);

	QSharedPointer<QCPGraphDataContainer> fb_MinData(new QCPGraphDataContainer);
	for (auto it = fbpData->getMedian().begin(); it != fbpData->getMedian().end(); ++it)
		fb_MinData->add(QCPGraphData(it->first, fbpData->getEnvelope().getMin(it->first)));
	m_nonlinearScaledPlot->addGraph();
	m_nonlinearScaledPlot->graph()->setVisible(false);
	m_nonlinearScaledPlot->graph()->removeFromLegend();
	m_nonlinearScaledPlot->graph()->setData(fb_MinData);
	m_nonlinearScaledPlot->graph()->setName("Min");
	m_nonlinearScaledPlot->graph()->setPen(QPen(QColor(0, 0, 255, 255), 7));
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
			l_opacity->show();
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
			l_opacity->hide();
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

void dlg_DatasetComparator::showBkgrdThrLine()
{
	m_nonlinearScaledPlot->findChild<QCPItemStraightLine*>()->setVisible(cb_BkgrdThrLine->isChecked());
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
			m_nonlinearDataPointInfo->setVisible(false);
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

void dlg_DatasetComparator::legendClick(QCPLegend* legend, QCPAbstractLegendItem* item, QMouseEvent* e)
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