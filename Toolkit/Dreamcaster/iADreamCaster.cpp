/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iADreamCaster.h"

#include "iAComparisonAndWeighting.h"
#include "dlg_histogram_simple.h"
#include "raycast/include/iACutFigList.h"
#include "raycast/include/iARayTracer.h"
#include "raycast/include/iAScene.h"
#include "raycast/include/iAScreenBuffer.h"
#include "raycast/include/iASTLLoader.h"
#include "raycast/include/iADataFormat.h"
#include "raycast/include/iABSPTree.h"
#include "raycast/include/iAPlot3DVtk.h"
#include "iAPaintWidget.h"
#include "iAStabilityWidget.h"

#include <iAVtkWidget.h>
#include <io/iAFileUtils.h>

#include <itkMacro.h>

#include <vtkActor.h>
#include <vtkAppendPolyData.h>
#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCameraPass.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkDepthPeelingPass.h>
#include <vtkDepthSortPolyData.h>
#include <vtkDoubleArray.h>
#include <vtkCylinderSource.h>
#include <vtkFloatArray.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkLightsPass.h>
#include <vtkLookupTable.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkOpaquePass.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkQuad.h>
#include <vtkRenderer.h>
#include <vtkRenderPassCollection.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkSequencePass.h>
#include <vtkSphereSource.h>
#include <vtkSTLReader.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkTranslucentPass.h>

#include <QElapsedTimer>
#include <QFileDialog>

#define DEG_IN_PI  180
#define DEG2RAD M_PI/DEG_IN_PI

std::vector<iAwald_tri> wald;
std::vector<iABSPNode> nodes;

// OpenMP
#ifndef __APPLE__
#ifndef __MACOSX
	#include <omp.h>
#endif
#endif

namespace
{
	const QString SettingsWindowStateKey = "DreamCaster/windowState";
}

//#include "enable_memleak.h"
const int CutAABSkipedSize = iACutAAB::getSkipedSizeInFile();
const int RenderFromPositionSkipedSize = iARenderFromPosition::getSkipedSizeInFile();

#define PLATE_HEIGHT 20./stngs.SCALE_COEF
extern QApplication * app;

iADreamCaster* dcast;//!< used just for logging, to call log() method

const QColor qcolBlack = QColor(0, 0, 0);
const QColor qcolYellow = QColor(255, 255, 0);
inline float GetSliderNormalizedValue(QSlider * slider)
{
	return ((float)(slider->value() - slider->minimum())) / ((float)(slider->maximum() - slider->minimum()));
}

inline void SetSliderNormalizedValue(QSlider * slider, float val)
{
	int value = (int)(slider->minimum() + (slider->maximum() - slider->minimum())*val);
	slider->setValue(value);
}

iADreamCaster::iADreamCaster(QWidget *parent, Qt::WindowFlags flags)
	: QMainWindow(parent, flags), modelOpened(false), datasetOpened(false)
{
	Q_INIT_RESOURCE(dreamcaster);
	ParseConfigFile(&stngs);
	indices[0] = 0;
	indices[1] = 2;
	indices[2] = 3;
	isOneWidgetMaximized = 0;
	CutFigParametersChangedOFF = false;
	scrBuffer = new iAScreenBuffer( stngs.RFRAME_W, stngs.RFRAME_H );
	tracer = 0;
	cuda_avpl_buff = 0;
	cuda_dipang_buff = 0;
	
	isStopped = false;
	
	ui.setupUi(this);
	connect(ui.openSTLFile,  SIGNAL(clicked()), this, SLOT(OpenModelSlot()));
	///connect(ui.tb_openModel,  SIGNAL(clicked()), this, SLOT(OpenModelSlot()));
	///connect(ui.actionShow_logs,  SIGNAL(triggered()), this, SLOT(ShowLogsSlot()));
	connect(ui.saveTree,  SIGNAL(clicked()), this, SLOT(SaveTree()));
	logsUi.setupUi(ui.logsWidget);
	//logs.show();
	resUi.setupUi(&res);
	settingsUi.setupUi(&settings);
	connect(resUi.pb_Save, SIGNAL(clicked()), this, SLOT(SaveResultsSlot()));
	connect(settingsUi.pb_SaveSettings, SIGNAL(clicked()), this, SLOT(SaveSettingsSlot()));
	connect(settingsUi.pb_Reset,        SIGNAL(clicked()), this, SLOT(ResetSettingsSlot()));
	//histUi.setupUi(&hist);
	//connect(histUi.pb_update,  SIGNAL(clicked()), this, SLOT(UpdateHistogramSlot()));
	//hist.show();
	initHistograms();
	modelFileName = "No model opened";
	setFileName = "";
	formPainter = new QPainter();
	//int s = VFRAME_W*VFRAME_H;
	viewsBuffer = 0;//new Pixel[s];
	//for ( int i = 0; i < s; i++ ) viewsBuffer[i] = 0;
	//
	comparisonTab = new iAParametersView(stngs.VFRAME_W, stngs.VFRAME_H, ui.w_comparison1, ui.w_comparison2, ui.w_comparison3);
	//connect heightmaps of parameters on comparison tab to each other
	for (unsigned int i=0; i<3; i++)
	{
		comparisonTab->paramWidgets[i].paintWidget->SetHighlightStyle(qcolYellow, 2.0);
		connect(comparisonTab->paramWidgets[i].paintWidget, SIGNAL(mouseReleaseEventSignal(int, int)), this, SLOT(ComparisonTabPlacementPickedSlot(int, int)));
		for (unsigned int j=0; j<3; j++)
			if(i != j)
				connect(comparisonTab->paramWidgets[i].paintWidget, SIGNAL(ChangedSignal(double&, double&, double&)), 
						comparisonTab->paramWidgets[j].paintWidget, SLOT(UpdateSlot(double&, double&, double&)));
	}
	//
	weightingTab = new iACombinedParametersView(ui.w_results, stngs.VFRAME_W, stngs.VFRAME_H);
	connect(weightingTab->results.paintWidget, SIGNAL(mouseReleaseEventSignal(int, int)), this, SLOT(WeightingResultsPlacementPickedSlot(int, int)));
	renderPxmp = new QPixmap(stngs.RFRAME_W, stngs.RFRAME_H);
	float ratio = stngs.RFRAME_H/stngs.RFRAME_W;
	RenderFrame = new iAPaintWidget(renderPxmp, ui.RenderViewWidget);
	QSizePolicy sp = RenderFrame->sizePolicy();
	sp.setHeightForWidth(true);
	RenderFrame->setSizePolicy(sp);
	RenderFrame->setGeometry(0, 0, ui.RenderViewWidget->geometry().width(), (int)(ui.RenderViewWidget->geometry().width()*ratio));
	viewsPxmp = new QPixmap(stngs.VFRAME_W, stngs.VFRAME_H);
	viewsPxmp->fill(qcolBlack);
	ViewsFrame = new iAPaintWidget(viewsPxmp, ui.HeightWidget);
	ViewsFrame->setGeometry(0, 0, ui.HeightWidget->geometry().width(), ui.HeightWidget->geometry().height());
	ViewsFrame->setCursor(ui.HeightWidget->cursor());
	ViewsFrame->SetHighlightStyle(qcolYellow, 2.0);
	stabilityView = new iAStabilityWidget(ui.w_stabilityWidget);
	stabilityView->setGeometry(0, 0, ui.w_stabilityWidget->geometry().width(), ui.w_stabilityWidget->geometry().height());
	connect(ViewsFrame, SIGNAL(mouseReleaseEventSignal()), this, SLOT(RenderFrameMouseReleasedSlot()));
	//
	hist = new dlg_histogram_simple(ui.histWidget);
	hist->setGeometry(0, 0, ui.histWidget->geometry().width(), ui.histWidget->geometry().height());

	ren = vtkRenderer::New();

	CREATE_OLDVTKWIDGET(qvtkWidget);
	CREATE_OLDVTKWIDGET(qvtkPlot3d);
	CREATE_OLDVTKWIDGET(qvtkWeighing);

	qvtkWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	qvtkWidget->setMinimumSize(100, 250);
	qvtkWidget->setAutoFillBackground(true);
	qvtkWeighing->setAutoFillBackground(true);
	ui.verticalLayout_63->addWidget(qvtkWidget);
	ui.gridLayout_4->addWidget(qvtkPlot3d);
	ui.gridLayout_1->addWidget(qvtkWeighing);

	qvtkWidget->GetRenderWindow()->AddRenderer(ren);
	stlReader = vtkSTLReader::New();
	mapper = vtkPolyDataMapper::New();
	actor = vtkActor::New();
	originMapper = vtkPolyDataMapper::New();
	originActor = vtkActor::New();
	planeMapper = vtkPolyDataMapper::New();
	planeActor = vtkActor::New();
	raysMapper = vtkPolyDataMapper::New();
	raysActor = vtkActor::New();
	raysProjMapper = vtkPolyDataMapper::New();
	raysProjActor = vtkActor::New();
	plateMapper = vtkPolyDataMapper::New();
	plateActor = vtkActor::New();
	cutAABMapper = vtkPolyDataMapper::New();
	cutAABActor = vtkActor::New();
	depthSort = 0;
	vtkInteractorStyleSwitch *style = vtkInteractorStyleSwitch::New();
	style->SetCurrentStyleToTrackballCamera();
	qvtkWidget->GetInteractor()->SetInteractorStyle(style);
	style->Delete();

	rotations=0;
	rotationsParams = 0;
	placementsParams = 0;
	weightedParams = 0;
	cntX = 0; cntZ = 0; cntY = 0;
	curIndX=0; curIndZ=0; curIndY = 0;
	//
//TODO: peredelat' pod openCL!!
// 	int            deviceCount;
// 	cudaDeviceProp devProp;
// 	cudaGetDeviceCount ( &deviceCount );
// 	this->log( QString("Found ")+QString::number(deviceCount)+ QString(" CUDA devices"));
// 	if(deviceCount==0)//if no cuda devices disable cuda enable option
// 	{
// 		ui.cudaEnabled->setChecked(0);
// 		ui.cudaEnabled->setDisabled(1);
// 	}
// 	int device;
// 	for ( device = 0; device < deviceCount; device++ )
// 	{
// 		cudaGetDeviceProperties ( &devProp, device );
// 
// 		this->log( "Device "+QString::number(device) );
// 		this->log( "Compute capability "+QString::number(devProp.major)+QString::number(devProp.minor) );
// 		this->log( "Name                   : "+QString(devProp.name) );
// 		this->log( "mult.proc. on device   : "+QString::number(devProp.multiProcessorCount) );
// 		this->log( "Total Global Memory    : "+QString::number(devProp.totalGlobalMem) );
// 		this->log( "Shared memory per block: "+QString::number(devProp.sharedMemPerBlock) );
// 		this->log( "Registers per block    : "+QString::number(devProp.regsPerBlock) );
// 		this->log( "Warp size              : "+QString::number(devProp.warpSize) );
// 		this->log( "Max threads per block  : "+QString::number(devProp.maxThreadsPerBlock) );
// 		this->log( "Total constant memory  : "+QString::number(devProp.totalConstMem) );
// 		this->log( "Maximum size of each dimension of a grid: ["+QString::number(devProp.maxGridSize[0])+", "+QString::number(devProp.maxGridSize[1])+", "+QString::number(devProp.maxGridSize[2])+"]");
// 		this->log( "Maximum size of each dimension of a block:["+QString::number(devProp.maxThreadsDim[0])+", "+QString::number(devProp.maxThreadsDim[1])+", "+QString::number(devProp.maxThreadsDim[2])+"]");
// 		this->log(" ");
// 	}
// 	//do batch size check
// 	cudaGetDevice(&device);
// 	cudaGetDeviceProperties ( &devProp, device );
	//TODO: define constants in common namespace
	#define THREAD_W 2
	#define THREAD_H 32
	#define MAX_BATCH_SIZE 500
	int maxGridDim[3] = {2048, 2048, 2048};//{devProp.maxGridSize[0], devProp.maxGridSize[1], devProp.maxGridSize[2]};//TODO: peredelat' pod openCL!!
	int maxBatchSize = 100;//maxGridDim[0] * THREAD_W / stngs.RFRAME_W;//TODO: peredelat' pod openCL!!
	if((int)stngs.BATCH_SIZE > maxBatchSize)
	{
		this->log( "Warning: Batch size is too big for GPU compatibilities! Batch size decreased to: "+QString::number(maxBatchSize) );
		stngs.BATCH_SIZE = maxBatchSize;
	}
	if(stngs.BATCH_SIZE > MAX_BATCH_SIZE)
	{
		this->log( "Warning: Batch size is bigger than maximum! Batch size decreased to: "+QString::number(MAX_BATCH_SIZE) );
		stngs.BATCH_SIZE = MAX_BATCH_SIZE;
	}
	if(stngs.RFRAME_H/THREAD_H > maxGridDim[1])
		this->log( "Warning: Resolution is too high for GPU compatibilities!");	
	//
	cutFigList = new iACutFigList();
	//
	dcast = this;
	//CONNECTIONS
	connect(ui.pb_stop, SIGNAL(clicked()), this, SLOT(StopRenderingSlot()));
	connect(ui.btnStart, SIGNAL(clicked()), this, SLOT(RenderSingleViewSlot()));
	connect(ui.btnRenderViews, SIGNAL(clicked()), this, SLOT(RenderViewsSlot()));
	///connect(ui.tb_new, SIGNAL(clicked()), this, SLOT(NewSetSlot()));
	///connect(ui.tb_open, SIGNAL(clicked()), this, SLOT(OpenSetSlot()));
	connect(ui.pbSetPosition, SIGNAL(clicked()), this, SLOT(pbSetPositionSlot()));
	connect(ui.pb_grab3D, SIGNAL(clicked()), this, SLOT(pbGrab3DSlot()));
	connect(ui.pbShowResults, SIGNAL(clicked()), this, SLOT(ShowResultsSlot()));
	connect(ui.tb_opends,  SIGNAL(clicked()), this, SLOT(OpenSetSlot()));
	connect(ui.tb_specifyds,  SIGNAL(clicked()), this, SLOT(NewSetSlot()));
	connect(ui.configureSettings, SIGNAL(clicked()), this, SLOT(ConfigureSettingsSlot()));
	connect(ui.cb_rangeParameter, SIGNAL(currentIndexChanged(int)), this, SLOT(CurrentParameterChangedSlot()));
	connect(ui.hs_projection, SIGNAL(valueChanged(int)), this, SLOT(ProjectionChangedSlot()));
	connect(ui.pb_showRays, SIGNAL(clicked()), this, SLOT(ShowRangeRays()));
	connect(ui.pb_hideRays, SIGNAL(clicked()), this, SLOT(HideRays()));
	connect(ui.pb_updatePlot, SIGNAL(clicked()), this, SLOT(UpdatePlotSlot()));
	connect(ui.s_sensitivity, SIGNAL(valueChanged(int)), this, SLOT(SensitivityChangedSlot()));
	connect(ui.cb_updateStabilityOnMouseMove, SIGNAL(clicked(bool)), this, SLOT(UpdateStabilityOnMouseMoveCheckedSlot()));
	connect(ui.pb_update, SIGNAL(clicked()), this, SLOT(UpdateHistogramSlot()));
	connect(ui.s_sensRes, SIGNAL(valueChanged(int)), this, SLOT(StabilityResolutionChangedSlot()));
	connect(ui.hs_topPlacements, SIGNAL(valueChanged(int)), this, SLOT(TopPlacementsChangedSlot()));
	connect(ui.s_lowCut1, SIGNAL(valueChanged(int)), this, SLOT(LowCutParam1Slot()));
	connect(ui.s_lowCut2, SIGNAL(valueChanged(int)), this, SLOT(LowCutParam2Slot()));
	connect(ui.s_lowCut3, SIGNAL(valueChanged(int)), this, SLOT(LowCutParam3Slot()));
	connect(ui.pb_updateResults, SIGNAL(clicked()), this, SLOT(UpdateWeightingResultsSlot()));
	connect(ui.s_lowCutRes, SIGNAL(valueChanged(int)), this, SLOT(LowCutWeightingResSlot()));
	connect(ui.tb_add, SIGNAL(clicked()), this, SLOT(AddCutFigSlot()));
	connect(ui.tb_remove, SIGNAL(clicked()), this, SLOT(RemoveCutFigSlot()));
	connect(ui.listCutFig, SIGNAL(clicked(QModelIndex)), this, SLOT(CutFigPicked()));
	connect(ui.s_aab_maxx, SIGNAL(valueChanged(int)), this, SLOT(CutFigParametersChanged()));
	connect(ui.s_aab_maxy, SIGNAL(valueChanged(int)), this, SLOT(CutFigParametersChanged()));
	connect(ui.s_aab_maxz, SIGNAL(valueChanged(int)), this, SLOT(CutFigParametersChanged()));
	connect(ui.s_aab_minx, SIGNAL(valueChanged(int)), this, SLOT(CutFigParametersChanged()));
	connect(ui.s_aab_miny, SIGNAL(valueChanged(int)), this, SLOT(CutFigParametersChanged()));
	connect(ui.s_aab_minz, SIGNAL(valueChanged(int)), this, SLOT(CutFigParametersChanged()));
	connect(ui.pb_colorAngles, SIGNAL(clicked()), this, SLOT(ColorBadAngles()));
	connect(ui.pb_hideColoring, SIGNAL(clicked()), this, SLOT(HideColoring()));
	connect(ui.push3DView, SIGNAL(clicked()), this, SLOT(maximize3DView()));
	connect(ui.pushRaycast, SIGNAL(clicked()), this, SLOT(maximizeRC()));
	connect(ui.pushPlacements, SIGNAL(clicked()), this, SLOT(maximizePlacements()));
	connect(ui.pushStability, SIGNAL(clicked()), this, SLOT(maximizeStability()));
	connect(ui.pushMaxTab, SIGNAL(clicked()), this, SLOT(maximizeBottom()));
	//restore previous window state
	QSettings settingsStore;
	if (settingsStore.contains(SettingsWindowStateKey))
	{
		QByteArray state = settingsStore.value(SettingsWindowStateKey).toByteArray();
		restoreState(state, 0);
	}
	//plot3d stuff
	plot3d = new iAPlot3DVtk;
	//plot3d->GetRenderer()->GetActiveCamera()->SetParallelProjection(1);
	plot3d->GetRenderer()->SetBackground(stngs.BG_COL_R/255.0, stngs.BG_COL_G/255.0, stngs.BG_COL_B/255.0);//(0,0,0);//
	plot3d->GetRenderer()->SetBackground2(0.5, 0.66666666666666666666666666666667, 1);
	qvtkPlot3d->GetRenderWindow()->AddRenderer(plot3d->GetRenderer());
	vtkInteractorStyleSwitch *cube_style = vtkInteractorStyleSwitch::New();
	cube_style->SetCurrentStyleToTrackballCamera();
	qvtkPlot3d->GetInteractor()->SetInteractorStyle(cube_style);
	cube_style->Delete();
	plot3d->SetPalette(100, stngs.COL_RANGE_MIN_R, stngs.COL_RANGE_MIN_G, stngs.COL_RANGE_MIN_B, stngs.COL_RANGE_MAX_R, stngs.COL_RANGE_MAX_G, stngs.COL_RANGE_MAX_B);
	plot3d->Update();
	//TODO: callback not used?
	vtkCallbackCommand* callback = vtkCallbackCommand::New();
	//callback->SetCallback(&(plot3d->Pick));
	qvtkPlot3d->GetRenderWindow()->GetInteractor()->AddObserver(vtkCommand::LeftButtonPressEvent, callback, 1.0);
	callback->Delete();
	//plot3dWeighting stuff
	plot3dWeighting = new iAPlot3DVtk;
	plot3dWeighting->GetRenderer()->SetBackground(stngs.BG_COL_R/255.0, stngs.BG_COL_G/255.0, stngs.BG_COL_B/255.0);//(0,0,0);//
	plot3dWeighting->GetRenderer()->SetBackground2(0.5, 0.66666666666666666666666666666667, 1);
	qvtkWeighing->GetRenderWindow()->AddRenderer(plot3dWeighting->GetRenderer());
	vtkInteractorStyleSwitch *cube_style2 = vtkInteractorStyleSwitch::New();
	cube_style2->SetCurrentStyleToTrackballCamera();
	qvtkWeighing->GetInteractor()->SetInteractorStyle(cube_style2);
	cube_style2->Delete();
	plot3dWeighting->SetPalette(100, stngs.COL_RANGE_MIN_R, stngs.COL_RANGE_MIN_G, stngs.COL_RANGE_MIN_B, stngs.COL_RANGE_MAX_R, stngs.COL_RANGE_MAX_G, stngs.COL_RANGE_MAX_B);
	plot3dWeighting->Update();

	qvtkPlot3d->GetRenderWindow()->Render();
	//
	qvtkPlot3d->installEventFilter(this);
	ui.RenderViewWidget->installEventFilter(this);
	ui.HeightWidget->installEventFilter(this);
	ui.w_stabilityWidget->installEventFilter(this);
	ui.histWidget->installEventFilter(this);

	ui.w_results->installEventFilter(this);

	ui.w_comparison1->installEventFilter(this);
	ui.w_comparison2->installEventFilter(this);
	ui.w_comparison3->installEventFilter(this);
	//
	UpdateSlot();
	UpdateStabilityOnMouseMoveCheckedSlot();
}

void iADreamCaster::initHistograms()
{
	///
	/*QwtPlot * hplot = ui.histPlot;
	hplot->setCanvasBackground(QColor(Qt::white));
	hplot->setTitle("Histogram");
	plotGrid.enableXMin(false);
	plotGrid.enableYMin(false);
	plotGrid.setMajPen(QPen(Qt::black, 0, Qt::DotLine));
	plotGrid.setMinPen(QPen(Qt::gray, 0 , Qt::DotLine));
	plotGrid.attach(hplot);

	histogram.setColor(Qt::darkCyan);

	histogram.attach(hplot);

	//hplot->setAxisAutoScale(QwtPlot::yLeft);
	//hplot->setAxisAutoScale(QwtPlot::xBottom);
	hplot->replot();*/
}

void iADreamCaster::initRaycast()
{
	//clear prev data
	delete scrBuffer;
	scrBuffer = new iAScreenBuffer( stngs.RFRAME_W, stngs.RFRAME_H );

	delete [] cuda_avpl_buff;
	cuda_avpl_buff = new float[stngs.RFRAME_W*stngs.RFRAME_H*stngs.BATCH_SIZE];
	delete [] cuda_dipang_buff;
	cuda_dipang_buff = new float[stngs.RFRAME_W*stngs.RFRAME_H*stngs.BATCH_SIZE];

	// prepare renderer
	delete tracer;
	try
	{
		tracer = new iAEngine(&stngs, cuda_avpl_buff, cuda_dipang_buff);
	}
	catch( itk::ExceptionObject &excep)
	{
		log( tr( "Engine creation terminated unexpectedly." ) );
		log( tr( "  %1 in File %2, Line %3" ).arg( excep.GetDescription() )
			.arg( excep.GetFile() )
			.arg( excep.GetLine() ) );
		return;
	}
	catch (std::exception &excep)
	{
		log( tr( "Engine creation terminated unexpectedly." ) );
		log( tr( "  %1" ).arg( excep.what() ) );
		return;
	}

	loadModel();
	setWindowTitle(QString("DreamCaster ")+modelFileName);
	//readSTLFile("../datasets/ContourFilter.stl");
	//readSTLFile("D:/MasterThesis/DreamCaster/datasets/ContourFilter.stl");
	//readSTLFile("D:\\MasterThesis\\DreamCaster\\datasets\\cylinders.stl");
	
	dcast = this;//for correct logging when there are several DC childs open
	if(ui.rb_buldNewTree->isChecked())
		tracer->scene()->initScene(mdata, &stngs);
	else
	{
		QString treefilename=modelFileName+".kdtree";
		if(!tracer->scene()->initScene(mdata, &stngs, treefilename))
			return;
	}
	PositionSpecimen();
	tracer->SetTarget( scrBuffer->buffer());
	//setup area of interest
	ui.l_xmin1->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.x1));
	ui.l_xmin2->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.x1));
	ui.l_xmax1->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.x2));
	ui.l_xmax2->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.x2));
	ui.l_ymin1->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.y1));
	ui.l_ymin2->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.y1));
	ui.l_ymax1->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.y2));
	ui.l_ymax2->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.y2));
	ui.l_zmin1->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.z1));
	ui.l_zmin2->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.z1));
	ui.l_zmax1->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.z2));
	ui.l_zmax2->setText(QString::number(tracer->scene()->getBSPTree()->m_aabb.z2));
	//setup VTK stuff
	setup3DView();
	
	size_t tri_count = tracer->scene()->getNrTriangles();
	size_t nodes_count = tracer->scene()->getBSPTree()->nodes.size();
	
	wald.clear();
	nodes.clear();
	for (size_t i=0; i<tri_count; i++)
		wald.push_back(((iATriPrim*)tracer->scene()->getTriangle((int)i))->GetWaldTri());
	
	for (size_t i=0; i<nodes_count; i++)
		nodes.push_back(*(tracer->scene()->getBSPTree()->nodes[i]));
	 
	//TODO!!
	try
	{
		tracer->AllocateOpenCLBuffers();
		tracer->setup_nodes(&nodes[0]);
		tracer->setup_tris(&wald[0]);
		tracer->setup_ids(&tracer->scene()->getBSPTree()->tri_ind[0]);
	}
	catch( itk::ExceptionObject &excep)
	{
		log(tr("OpenCL texture setup terminated unexpectedly."));
		log(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
}

iADreamCaster::~iADreamCaster()
{
	//save window state
	QSettings settingsStore;
	QByteArray state = saveState(0);
	settingsStore.setValue("DreamCaster/windowState", state);

	ClearPrevData();//plotData,rotations,plotColumnData
	delete scrBuffer;
	delete cutFigList;
	delete [] cuda_avpl_buff;
	delete [] cuda_dipang_buff;
	///
	/*histogram.detach();
	plotGrid.detach();
	if(plot)
		delete plot;*/
	delete comparisonTab;
	delete weightingTab;
	//
	delete formPainter;
	delete [] viewsBuffer;
	//
	delete RenderFrame;
	delete ViewsFrame;
	delete renderPxmp;
	delete viewsPxmp;
	delete stabilityView;
	delete plot3d;
	delete plot3dWeighting;
	delete hist;
	//
	//VTK cleanup
	stlReader->Delete();
	mapper->Delete();
	actor->Delete();
	ren->Delete();
	originMapper->Delete();
	originActor->Delete();
	planeMapper->Delete();
	planeActor->Delete();
	raysMapper->Delete();
	raysActor->Delete();
	raysProjMapper->Delete();
	raysProjActor->Delete();
	plateMapper->Delete();
	plateActor->Delete();
	cutAABMapper->Delete();
	cutAABActor->Delete();
}

void iADreamCaster::log(QString text, bool appendToPrev)
{
	if(appendToPrev)
	{
		QString prev_text = logsUi.listWidget->item(logsUi.listWidget->count()-1)->text();
		logsUi.listWidget->item(logsUi.listWidget->count()-1)->setText(prev_text+text);
	}
	else
		logsUi.listWidget->insertItem(logsUi.listWidget->count(), text);
}

void iADreamCaster::OpenModelSlot()
{
	QString res = QFileDialog::getOpenFileName();
	if(res == "")
		return;
	modelFileName = res;
	log("Opening new model:");
	log(modelFileName, true);
	initRaycast();
	log("Opened model size (triangles):");
	log(QString::number(mdata.stlMesh.size()),true);
	///ui.l_modelName->setText(modelFileName);
	modelOpened = true;
	for (int i=0; i<cutFigList->count(); i++)
	{
		iACutAAB *cutAAB = cutFigList->item(i);
		cutFigList->SetCurIndex(i);
		ui.listCutFig->setCurrentRow(i);
		CutFigParametersChangedOFF = true;
		ui.s_aab_minx->setValue(cutAAB->slidersValues[0]);
		ui.s_aab_maxx->setValue(cutAAB->slidersValues[1]);
		ui.s_aab_miny->setValue(cutAAB->slidersValues[2]);
		ui.s_aab_maxy->setValue(cutAAB->slidersValues[3]);
		ui.s_aab_minz->setValue(cutAAB->slidersValues[4]);
		ui.s_aab_maxz->setValue(cutAAB->slidersValues[5]);
		CutFigParametersChangedOFF = false;
		CutFigParametersChanged();
	}
}

void iADreamCaster::NewSetSlot()
{
	QString res = QFileDialog::getSaveFileName(nullptr, "Choose Set Filename", QFileInfo(setFileName).absolutePath());
	if(res=="")
		return;
	setFileName = res;
	ui.l_setName->setText(setFileName);
	log("Created new set:");
	log(setFileName, true);
}

void iADreamCaster::OpenSetSlot()
{
	QString res = QFileDialog::getOpenFileName(nullptr, "Open existing set", QFileInfo(setFileName).absolutePath());
	if (!res.isEmpty())
		OpenSetFile(res);
}

void iADreamCaster::OpenSetFile(QString const & fileName)
{
	setFileName = fileName;
	ui.l_setName->setText(setFileName);
	log("Opening new set:");
	log(setFileName, true);
	UpdatePlotSlot();
	datasetOpened = true;
}

void iADreamCaster::RenderViewsSlot()
{
	ui.simulationProgress->setValue(0);
	isStopped = false;
	if(!modelOpened) return;
	
	curIndX=0;
	curIndY=0;
	curIndZ=0;
	//delete prev data
	ClearPrevData();//plotData,rotations,plotColumnData
	//
	cntX = ui.sb_countX->value();
	if(ui.cb_RadonSA->currentIndex() == 2)
		ui.sb_numProj->setValue(1);
	cntY = ui.sb_numProj->value();
	cntZ = ui.sb_countZ->value();
	ViewsReset();
	ui.hs_projection->setMaximum(cntY-1);
	if(viewsBuffer)
	{
		delete [] viewsBuffer;
		viewsBuffer = 0;
	}
	int s = cntX*cntZ;
	viewsBuffer = new unsigned int[s];
	memset(viewsBuffer, 0, s*sizeof(viewsBuffer[0]));
	AllocateData();//plotData,rotations,plotColumnData

	unsigned int curRend=0;
	float minValX = ui.sb_min_x->value()*DEG2RAD;
	float minValZ = ui.sb_min_z->value()*DEG2RAD;
	float maxValX = ui.sb_max_x->value()*DEG2RAD;
	float maxValZ = ui.sb_max_z->value()*DEG2RAD;
	float deltaX = (maxValX-minValX)/cntX;
	float deltaY = 2*M_PI/cntY;
	float deltaZ = (maxValZ-minValZ)/cntZ;
	//open file for writing in binary mode and write header
	FILE *fptr = fopen( getLocalEncodingFileName(setFileName).c_str(),"wb");
	if(!fptr)
	{
		log("Error! Cannot open set file for writing.");
		return;
	}
	float cur_minValX = ui.sb_min_x->value()/DEG_IN_PI;
	float cur_minValZ = ui.sb_min_z->value()/DEG_IN_PI;
	float cur_maxValX = ui.sb_max_x->value()/DEG_IN_PI;
	float cur_maxValZ = ui.sb_max_z->value()/DEG_IN_PI;
	fwrite(&cntX, sizeof(cntX), 1, fptr);
	fwrite(&cur_minValX, sizeof(cur_minValX), 1, fptr);
	fwrite(&cur_maxValX, sizeof(cur_maxValX), 1, fptr);

	fwrite(&cntY, sizeof(cntY), 1, fptr);

	fwrite(&cntZ, sizeof(cntZ), 1, fptr);
	fwrite(&cur_minValZ, sizeof(cur_minValZ), 1, fptr);
	fwrite(&cur_maxValZ, sizeof(cur_maxValZ), 1, fptr);

	int aabCount = cutFigList->count();
	fwrite(&aabCount, sizeof(aabCount), 1, fptr);
	for (int i=0; i<aabCount; i++)
	{
		cutFigList->item(i)->Write2File(fptr);
	}
	//
	int totalTime=0;
	QElapsedTimer totalQTime;
	totalQTime.start();
	//int totalStart = GetTickCount();
	//float max_param=-1000;
	//float min_param=100000;
	set_pos[0] = ui.sb_posx->value()/stngs.SCALE_COEF;
	set_pos[1] = ui.sb_posy->value()/stngs.SCALE_COEF;
	set_pos[2] = ui.sb_posz->value()/stngs.SCALE_COEF;
	tracer->setPositon(set_pos);
	iAVec3f transl = -iAVec3f(set_pos);
	tracer->scene()->recalculateD(&transl);
	int paramIndex = ui.cb_rangeParameter->currentIndex();
	tracer->SetCutAABBList(&cutFigList->aabbs);
	findSelectedTriangles();
	int totalRends = cntX*cntY*cntZ;
	if(ui.cb_RadonSA->currentIndex() == 2)//do only Radon space analysis
	{
		iAVec3f l_o; // rays' origin point
		iAVec3f l_vp_corners[2];// plane's corners in 3d
		iAVec3f l_vp_delta[2];// plane's x and y axes' directions in 3D
		int counter = 0;
		for (int x=0; x<cntX; x++)
		{
			for (int z=0; z<cntZ; z++)
			{
				for (int y=0; y<cntY; y++)
				{
					if(isStopped)
						return;

					placementsParams[x][z].avDipAng = 0;
					placementsParams[x][z].avPenLen = 0;
					placementsParams[x][z].maxPenLen = 0;
					
					rotationsParams[x][y][z].avPenLen = 0;
					rotationsParams[x][y][z].avDipAng = 0;
					rotationsParams[x][y][z].maxPenLen = 0;

					float rx = minValX + deltaX * x;
					float ry =           deltaY * y;
					float rz = minValZ + deltaZ * z;
				
					tracer->curRender.clear();					
					tracer->curRender.rotX = rx;
					tracer->curRender.rotY = ry;
					tracer->curRender.rotZ = rz;
					tracer->setRotations(rx, ry, rz);
					rotations[x][y][z] = iArotation_t(rx/M_PI, ry/M_PI, rz/M_PI);
					tracer->InitRender(l_vp_corners, l_vp_delta, &l_o);
					if(y == 0)
						placementsParams[x][z].badAreaPercentage = RandonSpaceAnalysis();
					for(unsigned int i=0; i<3; i++)
						tracer->curRender.pos[i] = set_pos[i];
					tracer->curRender.avPenetrLen = 0.f;
					tracer->curRender.maxPenetrLen = 0.f;
					tracer->curRender.avDipAngle = 0.f;
					tracer->curRender.badAreaPercentage = placementsParams[x][z].badAreaPercentage;
					//write data to file
					tracer->curRender.write2BinaryFile(fptr, ui.cb_saveAdditionalData->isChecked());
					//show progress and calculation time
					totalTime = totalQTime.elapsed();//GetTickCount() - totalStart;
					char t2[] = "00:00.000";
					Time2Char(totalTime, t2);
					double percentage = ((double)counter) / totalRends * 100;
					ui.simulationProgress->setValue((int)percentage);
					ui.l_ttime->setText( QString(t2) + "  "+QString::number(percentage)+"%");
					counter++;
				}
			}
			app->processEvents();
		}
		totalTime = totalQTime.elapsed();//GetTickCount() - totalStart;
		char t2[] = "00:00.000";
		Time2Char(totalTime, t2);
		ui.l_ttime->setText(QString(t2));
		ui.simulationProgress->setValue(100);
	}
	else
	if(ui.cudaEnabled->isChecked())//using GPU
	{
		SetupGPUBuffers();
		int counter = 0;
		unsigned int batch_counter=0;
		//batch parameters for every render
		iAVec3f * s1_o = new iAVec3f[stngs.BATCH_SIZE]; 
		iAVec3f * corners = new iAVec3f[stngs.BATCH_SIZE];
		iAVec3f * dxs = new iAVec3f[stngs.BATCH_SIZE];
		iAVec3f * dys = new iAVec3f[stngs.BATCH_SIZE];
		unsigned int * xs = new unsigned int[stngs.BATCH_SIZE];//prev skipped positions when batching
		unsigned int * ys = new unsigned int[stngs.BATCH_SIZE];//prev skipped positions when batching
		unsigned int * zs = new unsigned int[stngs.BATCH_SIZE];//prev skipped positions when batching
		float * rotsX = new float[stngs.BATCH_SIZE];
		float * rotsY = new float[stngs.BATCH_SIZE]; 
		float * rotsZ = new float[stngs.BATCH_SIZE]; 
		iAVec3f s1_vp_corners[2];
		iAVec3f s1_vp_delta[2];
		unsigned int s1_x = 0;
		unsigned int s1_y = 0;
		unsigned int s1_z = 0;
		int numberOfRenderings = cntX*cntY*cntZ;
		while(counter < numberOfRenderings)
		{
			if(isStopped)
			{
				if(s1_o) delete [] s1_o;
				if(corners) delete [] corners;
				if(dxs) delete [] dxs;
				if(dys) delete [] dys;
				if(xs) delete [] xs;
				if(ys) delete [] ys;
				if(zs) delete [] zs;
				if(rotsX) delete [] rotsX;
				if(rotsY) delete [] rotsY;
				if(rotsZ) delete [] rotsZ;

				return;
			}
			s1_x = counter / (cntY*cntZ);
			s1_y = counter % cntY;
			s1_z = (counter / cntY)%cntZ;
			counter++;
			if( s1_y==0 )
			{
				placementsParams[s1_x][s1_z] = iAparameters_t();
			}
			actor->SetOrientation(0,0,0);
			actor->RotateWXYZ( vtkMath::DegreesFromRadians(minValX) + vtkMath::DegreesFromRadians(deltaX)*s1_x	,1,0,0 );//degrees
			actor->RotateWXYZ( vtkMath::DegreesFromRadians(deltaY)*s1_y						,0,1,0 );
			actor->RotateZ(    vtkMath::DegreesFromRadians(minValZ) + vtkMath::DegreesFromRadians(deltaZ)*s1_z);
			actor->SetPosition(set_pos[0], set_pos[1], set_pos[2]);
			cutAABActor->SetOrientation(0,0,0);
			cutAABActor->RotateWXYZ( vtkMath::DegreesFromRadians(minValX) + vtkMath::DegreesFromRadians(deltaX)*s1_x	,1,0,0 );//degrees
			cutAABActor->RotateWXYZ( vtkMath::DegreesFromRadians(deltaY)*s1_y						,0,1,0 );
			cutAABActor->RotateZ(    vtkMath::DegreesFromRadians(minValZ) + vtkMath::DegreesFromRadians(deltaZ)*s1_z);
			cutAABActor->SetPosition(set_pos[0], set_pos[1], set_pos[2]);
			double bottom = actor->GetBounds()[2];
			plateActor->SetPosition(0.0, bottom-0.5*PLATE_HEIGHT, 0.0);
			float rx = minValX + deltaX*s1_x;
			float ry =         + deltaY*s1_y;
			float rz = minValZ + deltaZ*s1_z;
			rotations[s1_x][s1_y][s1_z] = iArotation_t(rx/M_PI, ry/M_PI, rz/M_PI);

			tracer->setRotations(rx, ry, rz);
			tracer->InitRender(s1_vp_corners, s1_vp_delta, &s1_o[batch_counter]);
			if(ui.cb_RadonSA->currentIndex() == 1 && s1_y==0 )
			{
				placementsParams[ s1_x ][ s1_z ].badAreaPercentage = RandonSpaceAnalysis();
			}
			
			corners[batch_counter] = s1_vp_corners[0];
			dxs[batch_counter] = s1_vp_delta[0];
			dys[batch_counter] = s1_vp_delta[1];
			xs[batch_counter] = s1_x;
			ys[batch_counter] = s1_y;
			zs[batch_counter] = s1_z;
			rotsX[batch_counter] = rx;
			rotsY[batch_counter] = ry;
			rotsZ[batch_counter] = rz;
			batch_counter++;
			if(batch_counter == stngs.BATCH_SIZE || (s1_x == cntX-1 && s1_y == cntY-1 && s1_z == cntZ-1))
			{
				QElapsedTimer localTime;
				localTime.start();//int fstart = GetTickCount();
				try
				{
					tracer->RenderBatchGPU(batch_counter, s1_o, corners, dxs, dys, rotsX, rotsY, rotsZ, true, ui.cb_dipAsColor->isChecked());
				}
				catch( itk::ExceptionObject &excep)
				{
					log(tr("RenderBatchGPU terminated unexpectedly."));
					log(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
						.arg(excep.GetFile())
						.arg(excep.GetLine()));
					return;
				}
				
				int ftime = localTime.elapsed();//GetTickCount() - fstart;
				char t[] = "00:00.000";
				Time2Char(ftime, t);
				ui.TimeLabel->setText(t);
				totalTime = totalQTime.elapsed();//GetTickCount() - totalStart;
				char t2[] = "00:00.000";
				Time2Char(totalTime, t2);
				double percentage = ((double)counter)/numberOfRenderings*100;
				ui.simulationProgress->setValue((int)percentage);
				ui.l_ttime->setText( QString(t2) + "  "+QString::number(percentage)+"%");
				for (unsigned int batch = 0; batch < batch_counter; batch++)
				{
					//remember bad area percentage
					tracer->curBatchRenders[batch].badAreaPercentage = placementsParams[ xs[batch] ][ zs[batch] ].badAreaPercentage;
					//write data to file
					tracer->curBatchRenders[batch].write2BinaryFile(fptr, ui.cb_saveAdditionalData->isChecked());
					//					
					rotationsParams[ xs[batch] ][ ys[batch] ][ zs[batch] ].avPenLen = tracer->curBatchRenders[batch].avPenetrLen;
					rotationsParams[ xs[batch] ][ ys[batch] ][ zs[batch] ].avDipAng = tracer->curBatchRenders[batch].avDipAngle;
					rotationsParams[ xs[batch] ][ ys[batch] ][ zs[batch] ].maxPenLen = tracer->curBatchRenders[batch].maxPenetrLen;
					
					placementsParams[ xs[batch] ][ zs[batch] ].avPenLen += rotationsParams[ xs[batch] ][ ys[batch] ][ zs[batch] ].avPenLen;
					placementsParams[ xs[batch] ][ zs[batch] ].avDipAng += rotationsParams[ xs[batch] ][ ys[batch] ][ zs[batch] ].avDipAng;
					if(rotationsParams[ xs[batch] ][ ys[batch] ][ zs[batch] ].maxPenLen > placementsParams[ xs[batch] ][ zs[batch] ].maxPenLen)
						placementsParams[ xs[batch] ][ zs[batch] ].maxPenLen = rotationsParams[ xs[batch] ][ ys[batch] ][ zs[batch] ].maxPenLen;
					

					if(ys[batch]==(cntY-1))
					{						
						placementsParams[ xs[batch] ][ zs[batch] ].avPenLen /= cntY;
						placementsParams[ xs[batch] ][ zs[batch] ].avDipAng /= cntY;
						float s1_cur_param = 0.0f;
						switch(paramIndex)
						{
						case 0:
							s1_cur_param=stngs.COLORING_COEF*placementsParams[ xs[batch] ][ zs[batch] ].avPenLen;
							break;
						case 1:
							s1_cur_param = (1-placementsParams[ xs[batch] ][ zs[batch] ].avDipAng);//s1_cur_param = (s1_cur_param+1)/2.0;
							break;
						case 2:
							s1_cur_param=stngs.COLORING_COEF*placementsParams[ xs[batch] ][ zs[batch] ].maxPenLen;
							break;
						case 3:
							s1_cur_param=stngs.COLORING_COEF*placementsParams[ xs[batch] ][ zs[batch] ].badAreaPercentage;
							break;
						default:
							break;
						}
						if (s1_cur_param>1.f) s1_cur_param=1.f;
						//Pixel lencol = (lenval << 16) + (0 << 8) + (255-lenval);
						unsigned int s1_lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*s1_cur_param) << 16) + 
							((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*s1_cur_param) << 8) + 
							(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*s1_cur_param);
						viewsBuffer[ xs[batch] + zs[batch]*cntX] = s1_lencol;
					}
				}
				///plot->loadFromData(plotData, cntX, cntY, 0,1,0,1);
				//if(max_param==0)
				//	max_param=1;
				///plot->setScale(1.,1., 1./max_param);
				///plot->updateData();
				///plot->updateGL();
				UpdateSlot();
				RenderFrame->repaint();
				stabilityView->repaint();
				app->processEvents();
				curRend++;
				batch_counter=0;
			}	
		}
		if(s1_o) delete [] s1_o;
		if(corners) delete [] corners;
		if(dxs) delete [] dxs;
		if(dys) delete [] dys;
		if(xs) delete [] xs;
		if(ys) delete [] ys;
		if(zs) delete [] zs;
		if(rotsX) delete [] rotsX;
		if(rotsY) delete [] rotsY;
		if(rotsZ) delete [] rotsZ;

		totalTime = totalQTime.elapsed();//GetTickCount() - totalStart;
		char t2[] = "00:00.000";
		Time2Char(totalTime, t2);
		ui.simulationProgress->setValue(100);
		ui.l_ttime->setText(t2);
	}//GPU
	else//using CPU
	{
		iAVec3f o; // rays' origin point
		iAVec3f vp_corners[2];// plane's corners in 3d
		iAVec3f vp_delta[2];// plane's x and y axes' directions in 3D
		int counter = 0;
		for (int x=0; x<cntX; x++)
		{
			for (int z=0; z<cntZ; z++)
			{
				placementsParams[x][z]= iAparameters_t();
				for (int y=0; y<cntY; y++)
				{
					if(isStopped)
						return;
					actor->SetOrientation(0,0,0);
					actor->RotateWXYZ( vtkMath::DegreesFromRadians(minValX) + vtkMath::DegreesFromRadians(deltaX)*x	,1,0,0 );//degrees
					actor->RotateWXYZ(                    vtkMath::DegreesFromRadians(deltaY)*y	,0,1,0 );
					actor->RotateZ   ( vtkMath::DegreesFromRadians(minValZ) + vtkMath::DegreesFromRadians(deltaZ)*z);
					actor->SetPosition(set_pos[0], set_pos[1], set_pos[2]);
					cutAABActor->SetOrientation(0,0,0);
					cutAABActor->RotateWXYZ( vtkMath::DegreesFromRadians(minValX) + vtkMath::DegreesFromRadians(deltaX)*x	,1,0,0 );//degrees
					cutAABActor->RotateWXYZ(                    vtkMath::DegreesFromRadians(deltaY)*y	,0,1,0 );
					cutAABActor->RotateZ   ( vtkMath::DegreesFromRadians(minValZ) + vtkMath::DegreesFromRadians(deltaZ)*z);
					cutAABActor->SetPosition(set_pos[0], set_pos[1], set_pos[2]);
					float rx = minValX + deltaX*x;
					float ry =         + deltaY*y;
					float rz = minValZ + deltaZ*z;
					tracer->setRotations(rx, ry, rz);
					rotations[x][y][z] = iArotation_t(rx/M_PI, ry/M_PI, rz/M_PI);
					InitRender(vp_corners, vp_delta, &o);
					if(ui.cb_RadonSA->currentIndex() == 1 && y == 0)
					{
						placementsParams[x][z].badAreaPercentage = RandonSpaceAnalysis();
					}
					QElapsedTimer localTime;
					localTime.start();//int fstart = GetTickCount();
					//while (!tracer->Render()) UpdateSlot();
					Render(vp_corners, vp_delta, &o, true);
					int ftime = localTime.elapsed();// GetTickCount() - fstart;
					char t[] = "00:00.000";
					Time2Char(ftime, t);
					ui.TimeLabel->setText(t);
					totalTime = totalQTime.elapsed();//GetTickCount() - totalStart;
					char t2[] = "00:00.000";
					Time2Char(totalTime, t2);
					double percentage = ((double)counter)/totalRends*100;
					ui.simulationProgress->setValue((int)percentage);
					ui.l_ttime->setText( QString(t2) + "  "+QString::number(percentage)+"%");
					//remember bad area percentage
					tracer->curRender.badAreaPercentage = placementsParams[x][z].badAreaPercentage;
					//write data to file
					tracer->curRender.write2BinaryFile(fptr, ui.cb_saveAdditionalData->isChecked());
					//
					/*float cur_param;// = tracer->curRender.m_avPenetrLen;
					switch(paramIndex)
					{
					case 0:
						cur_param = tracer->curRender.m_avPenetrLen;
						break;
					case 1:
						cur_param = tracer->curRender.m_avDipAngle;
						break;
					case 2:
						cur_param = tracer->curRender.m_maxPenetrLen;
						break;
					default:
						break;
					}
					plotData[x][y][z] = cur_param;*/

					rotationsParams[x][y][z].avPenLen = tracer->curRender.avPenetrLen;
					rotationsParams[x][y][z].avDipAng = tracer->curRender.avDipAngle;
					rotationsParams[x][y][z].maxPenLen = tracer->curRender.maxPenetrLen;

					/*switch(paramIndex)
					{
					case 0:
						plotColumnData[x][z] += cur_param;
						break;
					case 1:
						plotColumnData[x][z] += cur_param;
						break;
					case 2:
						if(cur_param > plotColumnData[x][z])
							plotColumnData[x][z] = cur_param;
						break;
					}*/
					placementsParams[x][z].avPenLen += rotationsParams[x][y][z].avPenLen;
					placementsParams[x][z].avDipAng += rotationsParams[x][y][z].avDipAng;
					if(rotationsParams[x][y][z].maxPenLen > placementsParams[x][z].maxPenLen)
						placementsParams[x][z].maxPenLen = rotationsParams[x][y][z].maxPenLen;
		
					curRend++;
					counter++;
				}

				/*switch(paramIndex)
				{
				case 0:
					plotColumnData[x][z] /= cntY;
					break;
				case 1:
					plotColumnData[x][z] /= cntY;
					break;
				case 2:
					;
					break;
				}*/
				placementsParams[x][z].avPenLen /= cntY;
				placementsParams[x][z].avDipAng /= cntY;
				//if(plotColumnData[x][z] > max_param)
				//	max_param =  plotColumnData[x][z];
				//if(plotColumnData[x][z] < min_param)
				//	min_param =  plotColumnData[x][z];
				//float avlen = 0.5f*cur_param/SCALE_COEF;
				float cur_param = 0.0f;
				switch(paramIndex)
				{
				case 0://av. penetratoin length
					cur_param=stngs.COLORING_COEF*placementsParams[x][z].avPenLen;
					break;
				case 1://dip angle cos
					cur_param = (1-placementsParams[x][z].avDipAng);
					break;
				case 2://max penetratoin length
					cur_param=stngs.COLORING_COEF*placementsParams[x][z].maxPenLen;
					break;
				case 3://bad area percentage
					cur_param=stngs.COLORING_COEF*placementsParams[x][z].badAreaPercentage;
					break;
				default:
					break;
				}
				if (cur_param>1.f) cur_param=1.f;
				//Pixel lencol = (lenval << 16) + (0 << 8) + (255-lenval);
				unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*cur_param) << 16) + 
					((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*cur_param) << 8) + 
					(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*cur_param);
				viewsBuffer[x + z*cntX] = lencol;
				UpdateSlot();
				RenderFrame->repaint();
				stabilityView->repaint();
				app->processEvents();
				///plot->loadFromData(plotData, cntX, cntY, 0,1,0,1);
				//if(max_param==0)
				//	max_param=1;
				///plot->setScale(1.,1., 1./max_param);
				///plot->updateData();
				///plot->updateGL();
			}
		}
		totalTime = totalQTime.elapsed();//GetTickCount() - totalStart;
		char t2[] = "00:00.000";
		Time2Char(totalTime, t2);
		ui.simulationProgress->setValue(100);
		ui.l_ttime->setText( QString(t2));
	}
	tracer->SetCutAABBList(0);
	datasetOpened = true;
	/*//normalization of heightmap //TODO: unite with UpdatePlotSlot
	curParamInd = paramIndex;
	float curParam;
	double max_param=-1000;
	double min_param=100000;
	for (unsigned int x=0; x<cntX; x++)
	for (unsigned int z=0; z<cntZ; z++)
	{
		if( (placementsParams[x][z])[paramIndex] > max_param )
			max_param =  (placementsParams[x][z])[paramIndex];
		if( (placementsParams[x][z])[paramIndex] < min_param )
			min_param =  (placementsParams[x][z])[paramIndex];
	}
	if(max_param==0)
		max_param=1;
	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;

	double **plotData;
	plotData = new double*[cntX];
	for (unsigned int x=0; x<cntX; x++)
		plotData[x] = new double[cntZ];

	for (unsigned int x=0; x<cntX; x++)
		for (unsigned int z=0; z<cntZ; z++)
	plotData[x][z] = (placementsParams[x][z])[paramIndex];

	plot->loadFromData(plotData, cntX, cntZ, 0,1,0,1);

	if(plotData)
	{
		for (unsigned int x=0; x<cntX; x++)
		{
			if(plotData[x])
				delete [] plotData[x];
		}
		delete [] plotData;
	}

	plot->setScale(1.,1., 1./max_param);
	plot->updateData();
	plot->updateGL();
	plot->setScale(1.,1., 1./scalec);
	for (unsigned int x=0; x<cntX; x++)
	{
		for (unsigned int z=0; z<cntZ; z++)
		{
			switch(paramIndex)
			{
			case 0://av. penetration length
				curParam = (placementsParams[x][z].av_pen_len - min_param)/scalec;
				break;
			case 1://dip angle cosine
				curParam = (max_param - placementsParams[x][z].av_dip_ang)/scalec;
				break;
			case 2://max penetration length
				curParam = (placementsParams[x][z].max_pen_len - min_param)/scalec;
				break;
			default:
				break;
			}
			if (curParam>1.f) curParam=1.f;
			unsigned int lencol = ((unsigned int)(COL_RANGE_MIN_R+COL_RANGE_DR*curParam) << 16) + 
				((unsigned int)(COL_RANGE_MIN_G+COL_RANGE_DG*curParam) << 8) + 
				(unsigned int)(COL_RANGE_MIN_B+COL_RANGE_DB*curParam);
			viewsBuffer[x + z*cntX] = lencol;
		}
	}
	//setup stability widget sensitivity
	paramMin = min_param;
	paramMax = max_param;
	SensitivityChangedSlot();
	UpdateSlot();*/
//	mat->Delete();
	fclose(fptr);
	UpdatePlotSlot();
}

void iADreamCaster::UpdateHistogramSlot()
{
	if(!datasetOpened || !modelOpened)
		return;
	///SetMemLeakCheckActive(true);
	const unsigned int numIntervals = ui.sb_bars->value();
	const double min_x = ui.sb_x_min->value();
	const double max_x = ui.sb_x_max->value();
	const double maxval = max_x - min_x;
	const double width = maxval / ((double)numIntervals);
	double dataRange[2] = {min_x, max_x};
	std::vector<unsigned int> values;
	for(unsigned int i=0; i < numIntervals; i++)
		values.push_back(0);

	unsigned int numValues;
	tracer->SetCutAABBList(&(cutFigList->aabbs));
	int startY = curIndY;//for single rendering
	int endY = curIndY + 1;
	if(ui.cb_hist4placement->isChecked())//for placement
	{
		startY = 0;
		endY = cntY;
	}
	for (int y=startY; y<endY; y++)
	{
		iARenderFromPosition * readRender = new iARenderFromPosition;
		readRenderFromBinaryFile(curIndX, y, curIndZ, readRender);
		//////////////////////////////////////////////////////////////////////////
		// obtain rays data for current rendering
		float pos[3] ={readRender->pos[0], readRender->pos[1], readRender->pos[2]};
		tracer->setPositon(pos);
		iAVec3f transl = -iAVec3f(pos);
		tracer->scene()->recalculateD(&transl);
		tracer->setRotations(readRender->rotX, readRender->rotY, readRender->rotZ);
		//tracer->setRotations(((2*M_PI)/ui.sb_countX->value())*ui.sb_curX->value(), 
		//	((2*M_PI)/ui.sb_countY->value())*ui.sb_curY->value());
		iAVec3f o; // rays' origin point
		iAVec3f vp_corners[2];// plane's corners in 3d
		iAVec3f vp_delta[2];// plane's x and y axes' directions in 3D
		InitRender(vp_corners, vp_delta, &o);

		if(ui.cudaEnabled->isChecked())
			SetupGPUBuffers();
		Render(vp_corners, vp_delta, &o, true);
		iARenderFromPosition * curRender = &tracer->curRender;
		//////////////////////////////////////////////////////////////////////////
		numValues = (unsigned int) curRender->rays.size();
		for (unsigned int i = 0; i < numValues; i++ )
		{
			double rayPenLen = curRender->rays[i]->totalPenetrLen;
			if(rayPenLen < min_x || rayPenLen > max_x)
				continue;
			int bar = (int)((rayPenLen - min_x) / width);
			if(bar < (int)numIntervals)
				values[bar]++;
		}
		//curRender->clear();
		//delete curRender;
		delete readRender;
		app->processEvents();
	}
	hist->initialize(&values[0], numIntervals, dataRange);
	hist->drawHistogram();
	// DumpUnfreed();
	// SetMemLeakCheckActive(false);
}

void iADreamCaster::RenderSingleViewSlot()
{
	if (!modelOpened)
		return;
	// go
	float pos[3] ={
		static_cast<float>(ui.sb_posx_2->value()/stngs.SCALE_COEF),
		static_cast<float>(ui.sb_posy_2->value()/stngs.SCALE_COEF),
		static_cast<float>(ui.sb_posz_2->value()/stngs.SCALE_COEF)
	};
	tracer->setPositon(pos);
	iAVec3f transl = -iAVec3f(pos);
	tracer->scene()->recalculateD(&transl);
	tracer->setRotations(DEG2RAD*ui.sb_curX->value(), DEG2RAD*ui.sb_curY->value(), DEG2RAD*ui.sb_curZ->value());
	//tracer->setRotations(((2*M_PI)/ui.sb_countX->value())*ui.sb_curX->value(), 
	//	((2*M_PI)/ui.sb_countY->value())*ui.sb_curY->value());
	iAVec3f o; // rays' origin point
	iAVec3f vp_corners[2];// plane's corners in 3d
	iAVec3f vp_delta[2];// plane's x and y axes' directions in 3D
	InitRender(vp_corners, vp_delta, &o);
	/*ct_state state;
	state.o = tracer->o;
	state.c = tracer->vp_corners[0];
	state.dx = tracer->vp_delta[0];
	state.dy = tracer->vp_delta[1];*/
	//unsigned int tri_cnt = tracer->scene()->GetNrPrimitives();
	long ftime;//,fstart
	QElapsedTimer time;
	time.start();
	//fstart = GetTickCount();
	if(ui.cudaEnabled->isChecked())
		SetupGPUBuffers();
	tracer->SetCutAABBList(&cutFigList->aabbs);
	Render(vp_corners, vp_delta, &o, false);
	ftime = time.elapsed();//GetTickCount() - fstart;
	char t[] = "00:00.000";
	t[6] = (ftime / 100) % 10 + '0';
	t[7] = (ftime / 10) % 10 + '0';
	t[8] = (ftime % 10) + '0';
	int secs = (ftime / 1000) % 60;
	int mins = (ftime / 60000) % 100;
	t[3] = ((secs / 10) % 10) + '0';
	t[4] = (secs % 10) + '0';
	t[1] = (mins % 10) + '0';
	t[0] = ((mins / 10) % 10) + '0';
	ui.TimeLabel->setText(t);

	actor->SetOrientation(0,0,0);
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curX->value()), 1.0f, 0.0f, 0.0f );//degrees
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curY->value()), 0.0f, 1.0f, 0.0f );
	actor->RotateZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curZ->value()));
	actor->SetPosition(pos[0], pos[1], pos[2]);
	cutAABActor->SetOrientation(0,0,0);
	cutAABActor->RotateWXYZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curX->value()), 1.0f, 0.0f, 0.0f );//degrees
	cutAABActor->RotateWXYZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curY->value()), 0.0f, 1.0f, 0.0f );
	cutAABActor->RotateZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curZ->value()));
	cutAABActor->SetPosition(pos[0], pos[1], pos[2]);
	UpdateSlot();
	//UpdateHistogramSlot();
}

void iADreamCaster::UpdateSlot()
{
	QImage img((uchar*)scrBuffer->buffer(), stngs.RFRAME_W, stngs.RFRAME_H, QImage::Format_RGB32); 
	formPainter->begin(renderPxmp);
		formPainter->drawImage(QRect(0,0,img.width(),img.height()), img);
	formPainter->end();
	RenderFrame->update();
	stabilityView->update();
	//
	img = QImage((uchar*)viewsBuffer, cntX, cntZ, QImage::Format_RGB32); 
	formPainter->begin(viewsPxmp);
		formPainter->drawImage(QRect(0, 0, viewsPxmp->width(), viewsPxmp->height()), img);
	formPainter->end();
	ViewsFrame->update();
	ren->GetRenderWindow()->Render();
}

void iADreamCaster::SaveSlot()
{
}

void iADreamCaster::readRenderFromBinaryFile(unsigned int x, unsigned int y, unsigned int z, iARenderFromPosition *rend)
{
	FILE *fptr = fopen( getLocalEncodingFileName(setFileName).c_str() ,"rb");
	if(!fptr)
	{
		log("Error! Cannot open set file for reading.");
		return;
	}

	int cntX, cntY, cntZ;
	float minValX, maxValX, minValZ, maxValZ;
	if (fread(&cntX, sizeof(cntX), 1, fptr) != 1 ||
		fread(&minValX, sizeof(minValX), 1, fptr) != 1 ||
		fread(&maxValX, sizeof(maxValX), 1, fptr) != 1 ||
		fread(&cntY, sizeof(cntY), 1, fptr) != 1 ||
		fread(&cntZ, sizeof(cntZ), 1, fptr) != 1 ||
		fread(&minValZ, sizeof(minValZ), 1, fptr) != 1 ||
		fread(&maxValZ, sizeof(maxValZ), 1, fptr) != 1)
	{
		fclose(fptr);
		log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
		return;
	}
	
		setRangeSB( minValX, maxValX, minValZ, maxValZ );
	int cutAABCount;
	//skip cut AAB data
	if (fread(&cutAABCount, sizeof(cutAABCount), 1, fptr) != 1)
	{
		fclose(fptr);
		log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
		return;
	}
	for (int i=0; i<cutAABCount; i++)
	{
		if( fseek(fptr, CutAABSkipedSize, SEEK_CUR)!=0 )
			this->log("fseek error in readRenderFromBinaryFile!");
	}
	//
	if(x>=(unsigned int)cntX || y>=(unsigned int)cntY || z>=(unsigned int)cntZ)
	{
		log("Error! Set reading. Invalid index.");
		fclose(fptr);
		return;
	}
	int curRend = cntZ*cntY*x + y + cntY*z;//cntX*y + x;
	unsigned int raysSize;
	unsigned int intersectionsSize;
	for(int i=0; i<curRend; i++)
	{
		if( fseek(fptr, RenderFromPositionSkipedSize, SEEK_CUR)!=0 )//skip rotx, roty, rotz, avpenlen, avdipangle, maxpenlen, badAreaPercentage, position//20+4*3
			this->log("fseek error in readRenderFromBinaryFile!");
		if( fread(&raysSize, sizeof(raysSize), 1, fptr)<1 ) 
			this->log("fread error in readRenderFromBinaryFile!");
		if( fread(&intersectionsSize, sizeof(intersectionsSize), 1, fptr)<1 )
			this->log("fread error in readRenderFromBinaryFile!");
	}
	//now reading part =)
	if (fread(&rend->rotX, sizeof(rend->rotX), 1, fptr) != 1 ||
		fread(&rend->rotY, sizeof(rend->rotY), 1, fptr) != 1 ||
		fread(&rend->rotZ, sizeof(rend->rotY), 1, fptr) != 1 ||
		fread(rend->pos, sizeof(rend->pos), 1, fptr) != 1 ||
		fread(&rend->avPenetrLen, sizeof(rend->avPenetrLen), 1, fptr) != 1 ||
		fread(&rend->avDipAngle, sizeof(rend->avDipAngle), 1, fptr) != 1 ||
		fread(&rend->maxPenetrLen, sizeof(rend->maxPenetrLen), 1, fptr) != 1 ||
		fread(&rend->badAreaPercentage, sizeof(rend->badAreaPercentage), 1, fptr) != 1 ||
		fread(&rend->raysSize, sizeof(rend->raysSize), 1, fptr) != 1)
	{
		fclose(fptr);
		log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
		return;
	}
	//NOTE: let this code stay, saving additional data it is not used, but who knows 
	int rx,ry;
	float rtpl, avpl;
	unsigned int psize;
	for (unsigned int i=0; i<rend->raysSize; i++)
	{
		if (fread(&rx, sizeof(rx), 1, fptr) != 1 ||
			fread(&ry, sizeof(ry), 1, fptr) != 1 ||
			fread(&rtpl, sizeof(rtpl), 1, fptr) != 1 ||
			fread(&avpl, sizeof(avpl), 1, fptr) != 1 ||
			fread(&psize, sizeof(psize), 1, fptr) != 1)
		{
			fclose(fptr);
			log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
			return;
		}
		rend->rays.push_back(new iARayPenetration(rx,ry,rtpl,avpl));
	}
	if (fread(&rend->intersectionsSize, sizeof(rend->intersectionsSize), 1, fptr) != 1)
	{
		log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
		return;
	}
	int tri_index;
	float cos_ang;
	for (unsigned int i=0; i<rend->intersectionsSize; i++)
	{
		if (fread(&tri_index, sizeof(tri_index), 1, fptr) != 1 ||
			fread(&cos_ang, sizeof(cos_ang), 1, fptr) != 1)
		{
			fclose(fptr);
			log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
			return;
		}
		rend->intersections.push_back(new iAIntersection(tri_index, cos_ang));
	}
	//that's all, folks
	fclose(fptr);
}

void iADreamCaster::closeEvent ( QCloseEvent * /*event*/ )
{
	//hist.close();
	//logs.close();
	res.close();
}

void iADreamCaster::loadModel()
{
	dcast = this;//for correct logging when there are several DC childs open
	readSTLFile(modelFileName, mdata.stlMesh, mdata.vertices, mdata.box);
}

void iADreamCaster::setup3DView()
{
	vtkPolyData *modelPD = vtkPolyData::New();
	vtkPoints *modelPoints = vtkPoints::New();
	vtkCellArray *modelPolys = vtkCellArray::New();
	vtkIdType tids[3] = {0,1,2};
	vtkDoubleArray* scalars = vtkDoubleArray::New();
	scalars->SetNumberOfComponents(1);
	//scalars->SetNumberOfTuples(stlMesh.size());
	scalars->SetNumberOfValues(mdata.stlMesh.size());
	for (unsigned int i=0; i<mdata.stlMesh.size(); i++)
	{
		modelPoints->InsertNextPoint(mdata.stlMesh[i]->vertices[0]->data());
		modelPoints->InsertNextPoint(mdata.stlMesh[i]->vertices[1]->data());
		modelPoints->InsertNextPoint(mdata.stlMesh[i]->vertices[2]->data());
		modelPolys->InsertNextCell(3, tids);
		tids[0]+=3;
		tids[1]+=3;
		tids[2]+=3;
		scalars->InsertNextValue(0.0);
	}
	modelPD->SetPoints(modelPoints);
	modelPoints->Delete();
	modelPD->SetPolys(modelPolys);
	modelPolys->Delete();
	modelPD->GetCellData()->SetScalars((vtkDataArray*)scalars);
	scalars->Delete();

	//VTK window
	//model
	//no need in vtk stl-loader, we use own crooked loader =)
	///stlReader->SetFileName (modelFileName.toAscii().constData()); //param: char * filename
	///stlReader->Update();
	//transform
	// 	vtkTransform * scale_center = vtkTransform::New();
	// 	float scale_coef, translate[3];
	// 	scale_center->Translate(getTranslate());
	// 	scale_center->Scale(scale, scale, scale);
	// 	vtkTransformPolyDataFilter* transformer = vtkTransformPolyDataFilter::New();
	// 	transformer->SetInputConnection(stlReader->GetOutputPort());
	// 	transformer->SetTransform(scale_center);

	//mapper->ImmediateModeRenderingOff();
	//mapper->SetInput(stlReader->GetOutput());//transformer->GetOutput());
	mapper->SetInputData(modelPD);
	mapper->SetScalarRange(0,100);
	vtkLookupTable *lt = (vtkLookupTable*)mapper->GetLookupTable();
	lt->SetRampToLinear();
	lt->SetNumberOfTableValues(100);//     
	lt->SetTableValue(0, 1, 1, 1);
	for( int i = 0; i < 100; i++)//     
	{
		lt->SetTableValue(i, 
			(stngs.COL_RANGE_MIN_R + stngs.COL_RANGE_DR * ((double)i - 100) / 100.0)/255.0, 
			(stngs.COL_RANGE_MIN_G + stngs.COL_RANGE_DG * ((double)i - 100) / 100.0)/255.0,
			(stngs.COL_RANGE_MIN_B + stngs.COL_RANGE_DB * ((double)i - 100) / 100.0)/255.0);
	}	
	lt->SetTableValue(0, 0.7, 1.0, 0.7);
	lt->Build();
	mapper->ScalarVisibilityOff();
	actor->SetMapper(mapper);
	double pos[3];
	actor->GetPosition(pos);

	//origin as sphere
	vtkSphereSource *sphere = vtkSphereSource::New();
	sphere->SetThetaResolution(10);
	sphere->SetRadius(10./stngs.SCALE_COEF);
	sphere->SetPhiResolution(10);
	originMapper->SetInputConnection(sphere->GetOutputPort());
	originActor->SetMapper(originMapper);
	originActor->GetProperty()->SetColor(1,0,0);
	originActor->SetPosition(0., 0., stngs.ORIGIN_Z);
	//plate
	vtkCylinderSource *cylinder = vtkCylinderSource::New();
	cylinder->SetHeight(PLATE_HEIGHT);
	cylinder->SetRadius(100./stngs.SCALE_COEF);
	cylinder->SetResolution(20);
	plateMapper->SetInputConnection(cylinder->GetOutputPort());
	plateActor->SetMapper(plateMapper);
	plateActor->GetProperty()->SetColor(1,0,0);
	plateActor->GetProperty()->SetOpacity(0.15);
	plateActor->SetPosition(0.0, tracer->scene()->getBSPTree()->m_aabb.y1-0.5*cylinder->GetHeight(), 0.0);
	//plane
	vtkPolyData *planePD = vtkPolyData::New();
	vtkPoints *points = vtkPoints::New();
	vtkCellArray *polys = vtkCellArray::New();
	//Cut AAB
	cutAABMapper->ScalarVisibilityOn();
	cutAABMapper->SetScalarModeToUseCellData();
	cutAABActor->SetMapper(cutAABMapper);
	cutAABActor->GetProperty()->SetColor(1,1,0);
	cutAABActor->GetProperty()->SetRepresentationToWireframe();
	cutAABActor->GetProperty()->SetLineWidth(3.0f);
	cutAABActor->GetProperty()->LightingOff();
	// Load the point, cell, and data attributes.
	points->InsertPoint(0, -stngs.PLANE_H_W, -stngs.PLANE_H_H, stngs.PLANE_Z);//0 because point coords are local
	points->InsertPoint(1,  stngs.PLANE_H_W, -stngs.PLANE_H_H, stngs.PLANE_Z);
	points->InsertPoint(2,  stngs.PLANE_H_W,  stngs.PLANE_H_H, stngs.PLANE_Z);
	points->InsertPoint(3, -stngs.PLANE_H_W,  stngs.PLANE_H_H, stngs.PLANE_Z);
	vtkIdType ids[4] = {0,1,2,3};
	polys->InsertNextCell(4, ids);


	// We now assign the pieces to the vtkPolyData.
	planePD->SetPoints(points);
	points->Delete();
	planePD->SetPolys(polys);
	polys->Delete();

	planeMapper->SetInputData(planePD);
	planePD->Delete();
	planeActor->SetMapper(planeMapper);
	planeActor->GetProperty()->SetColor(stngs.PLATE_COL_R/255.0, stngs.PLATE_COL_G/255.0, stngs.PLATE_COL_B/255.0);//0.0, 0.0, 1.0);
	planeActor->GetProperty()->SetOpacity(1.);
	//
	raysActor->SetMapper(raysMapper);
	raysActor->GetProperty()->SetColor(1.0, 1.0, 0.0);
	raysActor->GetProperty()->SetOpacity(0.1*128.0/stngs.RFRAME_H);
	
	raysProjActor->SetMapper(raysProjMapper);
	raysProjActor->GetProperty()->SetColor(1.0, 1.0, 0.0);
	raysProjActor->GetProperty()->SetOpacity(0.5);
	raysProjActor->GetProperty()->SetPointSize(3.0);
	// Add Actors to renderer
	ren->AddActor(actor);
	ren->AddActor(originActor);
	ren->AddActor(planeActor);
	ren->AddActor(raysActor);
	ren->AddActor(raysProjActor);
	ren->AddActor(cutAABActor);
	//ren->AddActor(plateActor);
	// Reset camera
	ren->ResetCamera();
	vtkCamera* cam = ren->GetActiveCamera();
	cam->Azimuth(180);
	//cam->Zoom(2.0);
	//ren->SetBackground(BG_COL_R/255.0, BG_COL_G/255.0, BG_COL_B/255.0);
	ren->GradientBackgroundOn();
	ren->SetBackground(1,1,1);
	ren->SetBackground2(0.5,0.66666666666666666666666666666667,1);
	ren->GetRenderWindow()->Render();
	// 	transformer->Delete();
	// 	scale_center->Delete();
}

void iADreamCaster::ShowRangeRays()
{
	if(!modelOpened) return;
	iARenderFromPosition * curRender = new iARenderFromPosition();//renders[curRend];
	readRenderFromBinaryFile(curIndX, curIndY, curIndZ,curRender);//ui.sb_xind->value(), ui.sb_yind->value(), curRender);
	//orient model, so it correspond to analyzed rendering 
	actor->SetOrientation(0,0,0);
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(curRender->rotX), 1.0f, 0.0f, 0.0f );//degrees
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(curRender->rotY), 0.0f, 1.0f, 0.0f);
	actor->RotateZ(vtkMath::DegreesFromRadians(curRender->rotZ));
	actor->SetPosition(curRender->pos[0], curRender->pos[1], curRender->pos[2]);
	cutAABActor->SetOrientation(0,0,0);
	cutAABActor->RotateWXYZ(vtkMath::DegreesFromRadians(curRender->rotX), 1.0f, 0.0f, 0.0f );//degrees
	cutAABActor->RotateWXYZ(vtkMath::DegreesFromRadians(curRender->rotY), 0.0f, 1.0f, 0.0f);
	cutAABActor->RotateZ(vtkMath::DegreesFromRadians(curRender->rotZ));
	cutAABActor->SetPosition(curRender->pos[0], curRender->pos[1], curRender->pos[2]);
	//////////////////////////////////////////////////////////////////////////
	// obtain rays data for current rendering
	float pos[3] ={curRender->pos[0], curRender->pos[1], curRender->pos[2]};
	tracer->setPositon(pos);
	iAVec3f transl = -iAVec3f(pos);
	tracer->scene()->recalculateD(&transl);
	tracer->setRotations(curRender->rotX, curRender->rotY, curRender->rotZ);
	//tracer->setRotations(((2*M_PI)/ui.sb_countX->value())*ui.sb_curX->value(), 
	//	((2*M_PI)/ui.sb_countY->value())*ui.sb_curY->value());
	iAVec3f o; // rays' origin point
	iAVec3f vp_corners[2];// plane's corners in 3d
	iAVec3f vp_delta[2];// plane's x and y axes' directions in 3D
	InitRender(vp_corners, vp_delta, &o);
	
	if(ui.cudaEnabled->isChecked())
		SetupGPUBuffers();
	tracer->SetCutAABBList(&cutFigList->aabbs);
	Render(vp_corners, vp_delta, &o, true);
	for (unsigned int i=0; i<curRender->rays.size(); i++)
		delete curRender->rays[i];
	delete curRender;
	curRender = &tracer->curRender;
	//////////////////////////////////////////////////////////////////////////
	//if(ui.cb_rangeParameter->currentIndex()!=1)//av. penetration parameter choosed
	{
		const unsigned int numValues = (const unsigned int) curRender->rays.size();
		double rmin = ui.sb_range_min->value();
		double rmax = ui.sb_range_max->value();
		vtkPoints *raysPts = vtkPoints::New();
		raysPts->InsertNextPoint(0.0, 0.0, stngs.ORIGIN_Z);
		float deltax = stngs.PLANE_H_W/stngs.RFRAME_W;
		float deltay = stngs.PLANE_H_H/stngs.RFRAME_H;
		//check all rays of current render, if they are in defined range
		unsigned int raysCount = 0;
		vtkIdType a_cell[2] = {0,1};
		vtkIdType a_ptCell[1];
		vtkCellArray* cells = vtkCellArray::New();
		vtkCellArray* ptCells = vtkCellArray::New();
		for (unsigned int i = 0; i < numValues; i++ )
		{
			iARayPenetration* tpl = curRender->rays[i];
			if(tpl->totalPenetrLen>=rmin && tpl->totalPenetrLen<=rmax)
			{
				//TODO: ,         ,  
				raysPts->InsertNextPoint(stngs.PLANE_H_W-(2*tpl->m_X)*deltax,  stngs.PLANE_H_H-(2*tpl->m_Y)*deltay, stngs.PLANE_Z);
				raysCount++;
				a_cell[1] = raysCount;
				a_ptCell[0] = raysCount;
				if(!ui.cb_ProjOnly->isChecked())
					cells->InsertNextCell(2, a_cell);
				ptCells->InsertNextCell(1, a_ptCell);
			}
		}
		
		vtkPolyData *pd = vtkPolyData::New();
		vtkPolyData *pdProj = vtkPolyData::New();
		pd->SetPoints(raysPts);
		pdProj->SetPoints(raysPts);
		raysPts->Delete();
		pd->SetLines(cells);
		cells->Delete();
		pdProj->SetVerts(ptCells);
		ptCells->Delete();
		raysMapper->SetInputData(pd);
		raysMapper->Update();
		raysProjMapper->SetInputData(pdProj);
		raysProjMapper->Update();
		raysActor->SetMapper(raysMapper);
		raysProjActor->SetMapper(raysProjMapper);
		pd->Delete();
		pdProj->Delete();
		
		/*if (depthSort!=0) {
			depthSort->Delete();
			depthSort = 0;
		}
		depthSort = vtkDepthSortPolyData::New();
		depthSort->SetCamera(ren->GetActiveCamera());
		depthSort->SetDepthSortModeToParametricCenter();
		depthSort->SetDirectionToBackToFront();
		depthSort->SetInput(mapper->GetInput());
		mapper->SetInput(depthSort->GetOutput());
		mapper->Update();
		actor->SetMapper(mapper);*/
		//actor->GetProperty()->SetOpacity(0.9);
		planeActor->GetProperty()->SetOpacity(1.0);

		/*vtkCameraPass *cameraP=vtkCameraPass::New();

		vtkSequencePass *seq=vtkSequencePass::New();
		vtkOpaquePass *opaque=vtkOpaquePass::New();
		vtkDepthPeelingPass *peeling=vtkDepthPeelingPass::New();
		peeling->SetMaximumNumberOfPeels(200);
		peeling->SetOcclusionRatio(0.1);

		vtkTranslucentPass *translucent=vtkTranslucentPass::New();
		peeling->SetTranslucentPass(translucent);
		vtkLightsPass *lights=vtkLightsPass::New();

		vtkRenderPassCollection *passes=vtkRenderPassCollection::New();
		passes->AddItem(lights);
		passes->AddItem(opaque);
		passes->AddItem(peeling);
		//  passes->AddItem(translucent);

		seq->SetPasses(passes);
		cameraP->SetDelegatePass(seq);

		ren->SetPass(cameraP);
		//  renderer->SetPass(cameraP);

		opaque->Delete();
		peeling->Delete();
		translucent->Delete();
		seq->Delete();
		passes->Delete();
		cameraP->Delete();
		lights->Delete();*/

		/*//SETUP DEPTH PEELING
		vtkRenderWindow *renderWindow = ren->GetRenderWindow();
		renderWindow->SetAlphaBitPlanes(true);
		// 2. Force to not pick a framebuffer with a multisample buffer
		// (as initial value is 8):
		//renderWindow->SetMultiSamples(8);
		// 3. Choose to use depth peeling (if supported) (initial value is 0 (false)):
		ren->SetUseDepthPeeling(true);
		// 4. Set depth peeling parameters
		// - Set the maximum number of rendering passes (initial value is 4):
		ren->SetMaximumNumberOfPeels(10);
		// - Set the occlusion ratio (initial value is 0.0, exact image):
		ren->SetOcclusionRatio(0.0);*/

	}//av. penetration parameter choosed
	/*else//dip angle cos parameter choosed
	{
		//clear prev scalars
		vtkDataArray *da = vtkFloatArray::New();//mapper->GetInput()->GetCellData()->GetScalars();
		da->SetNumberOfComponents(1);
		da->SetNumberOfTuples(0);
		double val=0.0;
		unsigned int numTris = mapper->GetInput()->GetNumberOfPolys();
		unsigned int * numTriHits = new unsigned int[numTris];
		double * scalars = new double[numTris];
		for (unsigned int i=0; i<numTris; i++)
		{
			da->InsertNextTuple1(val);
			numTriHits[i]=0;
			scalars[i]=0;
		}
		//
		const unsigned int numValues = curRender->intersections.size();
		double rmin = ui.sb_range_min->value();
		double rmax = ui.sb_range_max->value();
		//check all intersections of current render, if they are in defined range
		float abscos;
		unsigned int tri_ind;
		for (unsigned int i = 0; i < numValues; i++ )
		{
			iAIntersection* tpl = curRender->intersections[i]; 
			abscos = abs(tpl->dip_angle);
			if(abscos>=rmin && abscos<=rmax)
			{
				//colour corresponding tris
				val = (abscos-rmin)/(rmax-rmin)*100;
				//val=100;
				tri_ind=tpl->tri_index;
				numTriHits[tri_ind]++;
				scalars[tri_ind]+=val;
			}
		}
		for (unsigned int i = 0; i < numTris; i++ )
		{
			da->SetTuple1(i, scalars[i]/numTriHits[i]); 
		}
		delete [] numTriHits;
		delete [] scalars;
		mapper->GetInput()->GetCellData()->SetScalars(da);
		da->Delete();
		mapper->ScalarVisibilityOn();
	}*/

	UpdateSlot();
}

void iADreamCaster::HideRays()
{
	if (depthSort!=0) {
		mapper->SetInputData((vtkPolyData*)depthSort->GetInput());
		depthSort->Delete();
		depthSort = 0;
		mapper->Update();
	}
	actor->GetProperty()->SetOpacity(1.0);
	planeActor->GetProperty()->SetOpacity(0.8);

	raysMapper->RemoveAllInputs();
	raysProjMapper->RemoveAllInputs();
	mapper->ScalarVisibilityOff();
	UpdateSlot();
}

void iADreamCaster::pbSetPositionSlot()
{
	ui.sb_posx_2->setValue(ui.sb_posx->value());
	ui.sb_posy_2->setValue(ui.sb_posy->value());
	ui.sb_posz_2->setValue(ui.sb_posz->value());
}

void iADreamCaster::pbGrab3DSlot()
{
	double pos[3], rot[3];
	actor->GetPosition(pos);
	ui.sb_posx_2->setValue(pos[0]);
	ui.sb_posy_2->setValue(pos[1]);
	ui.sb_posz_2->setValue(pos[2]);
	actor->GetOrientation(rot);
	//actor->SetOrientation(rot[0], rot[1], 0);
	rot[0]=vtkMath::RadiansFromDegrees(rot[0])/M_PI;
	rot[1]=vtkMath::RadiansFromDegrees(rot[1])/M_PI;
	if(rot[0]<0) rot[0] += 2.0;
	if(rot[1]<0) rot[1] += 2.0;
	ui.sb_curX->setValue(rot[0]*DEG_IN_PI);
	ui.sb_curY->setValue(rot[1]*DEG_IN_PI);
	actor->SetOrientation(0, 0, 0);
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curX->value()), 1.0f, 0.0f, 0.0f );//degrees
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curY->value()), 0.0f, 1.0f, 0.0f);
	actor->RotateZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curZ->value()));
	cutAABActor->SetOrientation(0, 0, 0);
	cutAABActor->RotateWXYZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curX->value()), 1.0f, 0.0f, 0.0f );//degrees
	cutAABActor->RotateWXYZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curY->value()), 0.0f, 1.0f, 0.0f);
	cutAABActor->RotateZ(vtkMath::DegreesFromRadians(DEG2RAD*ui.sb_curZ->value()));
}

void iADreamCaster::UpdatePlotSlot()
{
	FILE *fptr = fopen( getLocalEncodingFileName(setFileName).c_str(),"rb");
	if(!fptr)
	{
		log("Error! Cannot open set file for reading.");
		return;
	}
	//delete prev data
	ClearPrevData();//plotData,rotations,plotColumnData
	//
	float minValX, maxValX, minValZ, maxValZ;

	if (fread(&cntX, sizeof(cntX), 1, fptr) != 1 ||
		fread(&minValX, sizeof(minValX), 1, fptr) != 1 ||
		fread(&maxValX, sizeof(maxValX), 1, fptr) != 1 ||
		fread(&cntY, sizeof(cntY), 1, fptr) != 1 ||
		fread(&cntZ, sizeof(cntZ), 1, fptr) != 1 ||
		fread(&minValZ, sizeof(minValZ), 1, fptr) != 1 ||
		fread(&maxValZ, sizeof(maxValZ), 1, fptr) != 1)
	{
		fclose(fptr);
		log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
		return;
	}

	setRangeSB( minValX, maxValX, minValZ, maxValZ );
	//read cut AABs
	cutFigList->clear();
	ui.listCutFig->clear();
	int aabSize;
	iAaabb box;
	if (fread(&aabSize, sizeof(aabSize), 1, fptr) != 1)
	{
		fclose(fptr);
		log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
		return;
	}
	for (int i=0; i<aabSize; i++)
	{
		iACutAAB *newCutAAB = new iACutAAB("BOX"+QString::number(i));
		if (fread(&newCutAAB->box, sizeof(iAaabb), 1, fptr) != 1 ||
			fread(newCutAAB->slidersValues, sizeof(int)* 6, 1, fptr) != 1)
		{
			fclose(fptr);
			log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
			return;
		}
		int index = cutFigList->add(newCutAAB);
		cutFigList->SetCurIndex(index);
		ui.listCutFig->insertItem( ui.listCutFig->count(), newCutAAB->name()+": "+newCutAAB->GetDimString());
		ui.listCutFig->setCurrentRow(index);
		CutFigParametersChangedOFF = true;
		ui.s_aab_minx->setValue(newCutAAB->slidersValues[0]);
		ui.s_aab_maxx->setValue(newCutAAB->slidersValues[1]);
		ui.s_aab_miny->setValue(newCutAAB->slidersValues[2]);
		ui.s_aab_maxy->setValue(newCutAAB->slidersValues[3]);
		ui.s_aab_minz->setValue(newCutAAB->slidersValues[4]);
		ui.s_aab_maxz->setValue(newCutAAB->slidersValues[5]);
		CutFigParametersChangedOFF = false;
		CutFigParametersChanged();
	}
	//
	ViewsReset();
	ui.sb_countX->setValue(cntX);
	ui.sb_numProj->setValue(cntY);
	ui.sb_countZ->setValue(cntZ);
	ui.hs_projection->setMaximum(cntY-1);

	if(viewsBuffer)
	{
		delete [] viewsBuffer;
		viewsBuffer = 0;
	}
	int s = cntX*cntZ;
	viewsBuffer = new unsigned int[s];
	memset(viewsBuffer, 0, s*sizeof(viewsBuffer[0]));

	AllocateData();//plotData, rotations, plotColumnData
	unsigned int raysSize;
	unsigned int isecSize;
	float avpl;
	float avang;
	float maxpl;
	float badArPrcnt;
	float curParam = 0.0f;
	double max_param=-1000;
	double min_param = 10000;
	int paramIndex = ui.cb_rangeParameter->currentIndex();
	curParamInd = paramIndex;
	//read parameter values, store them, assign to 3d plot
	for (int x=0; x<cntX; x++)
	for (int z=0; z<cntZ; z++)
		placementsParams[x][z] = iAparameters_t();
	for (int x=0; x<cntX; x++)
	for (int z=0; z<cntZ; z++)
	for (int y=0; y<cntY; y++)
	{
		if (fread(&rotations[x][y][z].rotX, sizeof(rotations[x][y][z].rotX), 1, fptr) != 1)
		{
			fclose(fptr);
			log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
			return;
		}
		rotations[x][y][z].rotX/=M_PI;
		if (fread(&rotations[x][y][z].rotY, sizeof(rotations[x][y][z].rotY), 1, fptr) != 1)
		{
			fclose(fptr);
			log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
			return;
		}
		rotations[x][y][z].rotY/=M_PI;
		if (fread(&rotations[x][y][z].rotZ, sizeof(rotations[x][y][z].rotZ), 1, fptr) != 1)
		{
			fclose(fptr);
			log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
			return;
		}
		rotations[x][y][z].rotZ/=M_PI;
		if (fread(&set_pos[0], sizeof(float), 1, fptr) != 1 ||
			fread(&set_pos[1], sizeof(float), 1, fptr) != 1 ||
			fread(&set_pos[2], sizeof(float), 1, fptr) != 1 ||
			//fseek(fptr, 4*3, SEEK_CUR);//skip postition
			fread(&avpl, sizeof(avpl), 1, fptr) != 1 ||
			fread(&avang, sizeof(avang), 1, fptr) != 1 ||
			fread(&maxpl, sizeof(maxpl), 1, fptr) != 1 ||
			fread(&badArPrcnt, sizeof(badArPrcnt), 1, fptr) != 1)
		{
			fclose(fptr);
			log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
			return;
		}
		/*switch(paramIndex)
		{
		case 0:
			curParam = avpl;
			break;
		case 1:
			curParam = avang;
			break;
		case 2:
			curParam = maxpl;
			break;
		default:
			break;
		}
		rotationsParams[x][y][z] = curParam;
		*/
		rotationsParams[x][y][z].avPenLen = avpl;
		rotationsParams[x][y][z].avDipAng = avang;
		rotationsParams[x][y][z].maxPenLen = maxpl;
		rotationsParams[x][y][z].badAreaPercentage = badArPrcnt;

		/*switch(paramIndex)
		{
		case 0:
			placementsParams[x][z] += curParam;
			break;
		case 1:
			placementsParams[x][z] += curParam;
			break;
		case 2:
			if(curParam > placementsParams[x][z])
				placementsParams[x][z] = curParam;
			break;
		default:
			break;
		}
		*/
		placementsParams[x][z].avPenLen += rotationsParams[x][y][z].avPenLen;
		placementsParams[x][z].avDipAng += rotationsParams[x][y][z].avDipAng;
		if(rotationsParams[x][y][z].maxPenLen > placementsParams[x][z].maxPenLen)
			placementsParams[x][z].maxPenLen = rotationsParams[x][y][z].maxPenLen;
		placementsParams[x][z].badAreaPercentage = rotationsParams[x][y][z].badAreaPercentage;
		
		if (fread(&raysSize, sizeof(raysSize), 1, fptr) != 1 ||
			fread(&isecSize, sizeof(isecSize), 1, fptr) != 1)
		{
			fclose(fptr);
			log("Reading file failed - expected number of bytes read does not match actual number of bytes read.");
			return;
		}
	}
	for (int x=0; x<cntX; x++)
	for (int z=0; z<cntZ; z++)
	{
		/*switch(paramIndex)
		{
		case 0:
			placementsParams[x][z] /= cntY;
			break;
		case 1:
			placementsParams[x][z] /= cntY;
			break;
		case 2:
			;
			break;
		default:
			break;
		}*/
		placementsParams[x][z].avPenLen /= cntY;
		placementsParams[x][z].avDipAng /= cntY;

		if( (placementsParams[x][z])[paramIndex] > max_param )
			max_param = (placementsParams[x][z])[paramIndex];
		if( (placementsParams[x][z])[paramIndex] < min_param )
			min_param = (placementsParams[x][z])[paramIndex];
	}
	if(max_param==0)
		max_param=1.0;
	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;
	//setup stability widget sensitivity
	paramMin = min_param;
	paramMax = max_param;
	datasetOpened = true;
	SensitivityChangedSlot();
///
	double * scalarVals = new double[cntX*cntZ];
	double * plotData = new double[cntX*cntZ];
	double lookupNumber = plot3d->GetNumberOfLookupTableValues();
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			switch(paramIndex)
			{
			case 0://av. penetration length
				curParam = (placementsParams[x][z].avPenLen - min_param)/scalec;
				break;
			case 1://dip angle cosine
				curParam = (max_param - placementsParams[x][z].avDipAng)/scalec;
				break;
			case 2://max penetration length
				curParam = (placementsParams[x][z].maxPenLen - min_param)/scalec;
				break;
			case 3://bad area percentage
				curParam = (placementsParams[x][z].badAreaPercentage - min_param)/scalec;
				break;
			default:
				break;
			}
			if (curParam>1.f) curParam=1.f;
			unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R + stngs.COL_RANGE_DR*curParam) << 16) + 
						   ((unsigned int)(stngs.COL_RANGE_MIN_G + stngs.COL_RANGE_DG*curParam) << 8) + 
							(unsigned int)(stngs.COL_RANGE_MIN_B + stngs.COL_RANGE_DB*curParam);
			viewsBuffer[x + z*cntX] = lencol;
			plotData[x + z*cntX] = curParam;
			scalarVals[x + z*cntX] = curParam*lookupNumber;
		}
	}
	//Comparison and weighting tabs
	weightingTab->results.AllocateBuffer(cntX, cntZ);
	//TODO: vynesty vverh indices
	for (unsigned int i=0; i<3; i++)
	{
		/*if(i==1) 
			i++;*/
		comparisonTab->paramWidgets[i].AllocateBuffer(cntX, cntZ);
		//weightingTab->paramWidgets[i].AllocateBuffer(cntX, cntZ);
		fillParamBuffer(comparisonTab->paramWidgets[i].buffer, indices[i]);
		//fillParamBuffer(weightingTab->paramWidgets[i].buffer, indices[i]);
	}
	comparisonTab->Update();
	weightingTab->Update();
///
	//plot3D fill with data
	plot3d->SetUserScalarRange(0, 100);
	/*double *plotData = new double[cntX*cntZ];
	for (int z=0; z<cntZ; z++)
		for (int x=0; x<cntX; x++)	
			plotData[x + z*cntX] = (placementsParams[x][z])[paramIndex];*/
	plot3d->loadFromData(plotData, scalarVals, cntX, cntZ);
	plot3d->ShowWireGrid(1, 0,0,0);
	plot3d->Update();
	plot3d->GetRenderer()->GetRenderWindow()->Render();
	delete [] plotData;
	delete [] scalarVals;
///
	//plot->setScale(1.,1., 1./scalec);
	if(cntX || cntZ) 
	{
		double **plotData;
			plotData = new double*[cntX];
		for (int x=0; x<cntX; x++)
			plotData[x] = new double[cntZ];
		for (int x=0; x<cntX; x++)
			for (int z=0; z<cntZ; z++)
				plotData[x][z] = (placementsParams[x][z])[paramIndex];
		//plot->loadFromData(plotData, cntX, cntZ, 0,1,0,1);
		if(plotData)
		{
			for (int x=0; x<cntX; x++)
			{
				if(plotData[x])
					delete [] plotData[x];
			}
			delete [] plotData;
		}
	}
	//plot->updateData();
	//plot->updateGL();
	UpdateSlot();
	//that's all, folks
	fclose(fptr);
}

void iADreamCaster::SaveTree()
{
	log("Saving current KD-tree...............");
	QString treefilename=modelFileName+".kdtree";
	tracer->scene()->getBSPTree()->SaveTree(treefilename);
}

void iADreamCaster::RenderFrameMouseReleasedSlot()
{
	if(!cntZ || !cntX) return;
	int buf = (int)((float)ViewsFrame->lastX/((float)ViewsFrame->width()/cntX));
	if(buf<0 || buf>=cntX) return;
	curIndX = buf;
	buf = (int)((float)ViewsFrame->lastY/((float)ViewsFrame->width()/cntZ));
	if(buf<0 || buf>=cntZ) return;
	curIndZ = buf;
	setPickedPlacement(curIndX, curIndY, curIndZ);
	/*int inds[2] = {(int)curIndX, (int)curIndZ};
	ViewsFrame->SetHiglightedIndices(&inds[0], &inds[1], 1);
	ViewsFrame->update();
	plot3d->setPicked(inds[0], inds[1]);
	UpdateInfoLabels();
	if(ui.cb_UpdateViews->isChecked() && modelOpened && datasetOpened)
		UpdateView();
	UpdateStabilityWidget();
	//log("Yes!");*/
}

void iADreamCaster::ShowResultsSlot()
{
	if(!cntZ || !cntX) return;
	QString rstr;
	unsigned int xind = 0, zind = 0;
	float optimalParam=10000.0f;
	switch(ui.cb_rangeParameter->currentIndex())
	{
	case 0://av. penetration parameter choosed
		optimalParam=10000.0f;
		for (int i=0; i<cntX; i++)
			for (int j=0; j<cntZ; j++)
			{
				if( (placementsParams[i][j])[curParamInd] < optimalParam )
				{
					optimalParam = (placementsParams[i][j])[curParamInd];
					xind = i;
					zind = j;
				}
			}
		break;
	case 1://av. dip angle
		optimalParam=0.f;
		for (int i=0; i<cntX; i++)
			for (int j=0; j<cntZ; j++)
			{
				if( (placementsParams[i][j])[curParamInd] > optimalParam)
				{
					optimalParam = (placementsParams[i][j])[curParamInd];
					xind = i;
					zind = j;
				}
			}
		break;
	case 2://max. penetration parameter choosed
		optimalParam=10000.0f;
		for (int i=0; i<cntX; i++)
			for (int j=0; j<cntZ; j++)
			{
				if((placementsParams[i][j])[curParamInd] < optimalParam)
				{
					optimalParam = (placementsParams[i][j])[curParamInd];
					xind = i;
					zind = j;
				}
			}
		break;
	}
	
	int hl_indices[2] = {(int)xind,(int)zind};
	ViewsFrame->SetHiglightedIndices(&hl_indices[0], &hl_indices[1], 1);
	rstr = "Object: " + modelFileName + "\n";
	rstr += "Number of renderings by X: "+QString::number(cntX)+"\n";
	rstr += "Number of renderings by Y: "+QString::number(cntY)+"\n";
	rstr += "Number of renderings by Z: "+QString::number(cntZ)+"\n";
	rstr += "Parameter: ";
	switch(ui.cb_rangeParameter->currentIndex())
	{
	case 0:
		rstr+="average ray's penetration\n";
		break;
	case 1:
		rstr+="average dip angle cosine\n";
		break;
	case 2:
		rstr+="maximum ray's penetration\n";
		break;
	}
	rstr+="Optimal parameter's value is: " + QString::number( (placementsParams[xind][zind])[curParamInd] )+"\n";
	rstr+="Optimal X-rotation is: " + QString::number(rotations[xind][0][zind].rotX)+" (PI)\n";
	rstr+="Optimal Z-rotation is: " + QString::number(rotations[xind][0][zind].rotZ)+" (PI)\n";
	resUi.textEdit->setText(rstr);
	res.show();
}

void iADreamCaster::SaveResultsSlot()
{
	QFile file(modelFileName+".result");
	if (!file.open(QIODevice::WriteOnly)) 
	{
		log("Error: Cannot open file results.txt for writing!");
		return;
	}
	QTextStream out(&file);
	out<<resUi.textEdit->toPlainText();
	file.close();
}

void iADreamCaster::ShowLogsSlot()
{
	//logs.show();
}

void iADreamCaster::ShowDipAnglesSlot()
{
	/*iARenderFromPosition * curRender = new iARenderFromPosition();//renders[curRend];
	readRenderFromBinaryFile(ui.sb_xind->value(), ui.sb_yind->value(), curRender);
	//orient model, so it correspond to analyzed rendering 
	actor->SetOrientation(0,0,0);
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(curRender->m_rotX), 1.0f, 0.0f, 0.0f );//degrees
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(curRender->m_rotY), 0.0f, 1.0f, 0.0f);
	actor->SetPosition(curRender->m_pos[0], curRender->m_pos[1], curRender->m_pos[2]);
	
	//clear prev scalars
	vtkDataArray *da = vtkFloatArray::New();//mapper->GetInput()->GetCellData()->GetScalars();
	da->SetNumberOfComponents(1);
	da->SetNumberOfTuples(0);
	double val=0.0;
	std::vector<iAtriangle*> triangles = getLoadedMesh();
	unsigned int numTris = triangles.size();
	double * scalars = new double[numTris];
	for (unsigned int i=0; i<numTris; i++)
	{
		da->InsertNextTuple1(val);
		scalars[i]=0;
	}
	//
	const unsigned int numValues = curRender->intersections.size();
	double rmin = ui.sb_dip_min->value();
	double rmax = ui.sb_dip_max->value();
	//check all intersections of current render, if they are in defined range
	float abscos;
	unsigned int tri_ind;
	//
	iAVec3f pos =iAVec3f(curRender->m_pos[0],curRender->m_pos[1],curRender->m_pos[2]);
	Mat4 mat = rotationX(curRender->m_rotX)*rotationY(curRender->m_rotY);
	mat = mat*translate(pos);
	//
	for (unsigned int i = 0; i < numTris; i++ )
	{
		iAVec3f tri_center = mat*((*triangles[i]->m_Vertex[0]+*triangles[i]->m_Vertex[1]+*triangles[i]->m_Vertex[2])/3);
		iAVec3f tri_norm = mat*triangles[i]->m_N;
		iAVec3f ray = tri_center - tracer->o;
		ray.normalize();
		abscos = abs(ray&tri_norm);
		if(abscos>=rmin && abscos<=rmax)
		{
			//colour corresponding tris
			val = (1-(abscos-rmin)/(rmax-rmin))*100;
			scalars[i]+=val;
		}
	}
	for (unsigned int i = 0; i < numTris; i++ )
	{
		da->SetTuple1(i, scalars[i]); 
	}
	delete [] scalars;
	mapper->GetInput()->GetCellData()->SetScalars(da);
	da->Delete();
	mapper->ScalarVisibilityOn();
	
	UpdateSlot();
	for (unsigned int i=0; i<curRender->rays.size(); i++)
		delete curRender->rays[i];
	delete curRender;*/
}

void iADreamCaster::HideDipAnglesSlot()
{
	mapper->ScalarVisibilityOff();
	UpdateSlot();
}

void iADreamCaster::ConfigureSettingsSlot()
{
	SetupSettingsFromConfigFile();
	settings.show();
}

void iADreamCaster::SaveSettingsSlot()
{
	QSettings settings;
	settings.setValue( "DreamCaster/THREAD_GRID_X", settingsUi.tableWidget->item(0, 0)->text().toInt());
	settings.setValue( "DreamCaster/THREAD_GRID_Y", settingsUi.tableWidget->item(1, 0)->text().toInt());
	settings.setValue( "DreamCaster/SCALE_COEF",    settingsUi.tableWidget->item(2, 0)->text().toFloat());
	settings.setValue( "DreamCaster/COLORING_COEF", settingsUi.tableWidget->item(3, 0)->text().toFloat());
	settings.setValue( "DreamCaster/RFRAME_W",      settingsUi.tableWidget->item(4, 0)->text().toInt());
	settings.setValue( "DreamCaster/RFRAME_H",      settingsUi.tableWidget->item(5, 0)->text().toInt());
	settings.setValue( "DreamCaster/VFRAME_W",      settingsUi.tableWidget->item(6, 0)->text().toInt());
	settings.setValue( "DreamCaster/VFRAME_H",      settingsUi.tableWidget->item(7, 0)->text().toInt());
	settings.setValue( "DreamCaster/ORIGIN_Z",      settingsUi.tableWidget->item(8, 0)->text().toFloat());
	settings.setValue( "DreamCaster/PLANE_Z",       settingsUi.tableWidget->item(9, 0)->text().toFloat());
	settings.setValue( "DreamCaster/PLANE_H_W",     settingsUi.tableWidget->item(10, 0)->text().toFloat());
	settings.setValue( "DreamCaster/PLANE_H_H",     settingsUi.tableWidget->item(11, 0)->text().toFloat());
	settings.setValue( "DreamCaster/TREE_L1",       settingsUi.tableWidget->item(12, 0)->text().toInt());
	settings.setValue( "DreamCaster/TREE_L2",       settingsUi.tableWidget->item(13, 0)->text().toInt());
	settings.setValue( "DreamCaster/TREE_L3",       settingsUi.tableWidget->item(14, 0)->text().toInt());
	settings.setValue( "DreamCaster/TREE_SPLIT1",   settingsUi.tableWidget->item(15, 0)->text().toInt());
	settings.setValue( "DreamCaster/TREE_SPLIT2",   settingsUi.tableWidget->item(16, 0)->text().toInt());
	settings.setValue( "DreamCaster/BG_COL_R",      settingsUi.tableWidget->item(17, 0)->text().toInt());
	settings.setValue( "DreamCaster/BG_COL_G",      settingsUi.tableWidget->item(18, 0)->text().toInt());
	settings.setValue( "DreamCaster/BG_COL_B",      settingsUi.tableWidget->item(19, 0)->text().toInt());
	settings.setValue( "DreamCaster/PLATE_COL_R",   settingsUi.tableWidget->item(20, 0)->text().toInt());
	settings.setValue( "DreamCaster/PLATE_COL_G",   settingsUi.tableWidget->item(21, 0)->text().toInt());
	settings.setValue( "DreamCaster/PLATE_COL_B",   settingsUi.tableWidget->item(22, 0)->text().toInt());
	settings.setValue( "DreamCaster/COL_RANGE_MIN_R", settingsUi.tableWidget->item(23, 0)->text().toInt());
	settings.setValue( "DreamCaster/COL_RANGE_MIN_G", settingsUi.tableWidget->item(24, 0)->text().toInt());
	settings.setValue( "DreamCaster/COL_RANGE_MIN_B", settingsUi.tableWidget->item(25, 0)->text().toInt());
	settings.setValue( "DreamCaster/COL_RANGE_MAX_R", settingsUi.tableWidget->item(26, 0)->text().toInt());
	settings.setValue( "DreamCaster/COL_RANGE_MAX_G", settingsUi.tableWidget->item(27, 0)->text().toInt());
	settings.setValue( "DreamCaster/COL_RANGE_MAX_B", settingsUi.tableWidget->item(28, 0)->text().toInt());
	settings.setValue( "DreamCaster/BATCH_SIZE",      settingsUi.tableWidget->item(29, 0)->text().toInt());
}

void iADreamCaster::ResetSettingsSlot()
{
	SetupSettingsFromConfigFile();
}

int iADreamCaster::SetupSettingsFromConfigFile()
{
	QSettings settings;
	settingsUi.tableWidget->item(0, 0)->setText(QString::number(settings.value( "DreamCaster/THREAD_GRID_X", stngs.THREAD_GRID_X).toInt()));
	settingsUi.tableWidget->item(1, 0)->setText(QString::number(settings.value( "DreamCaster/THREAD_GRID_Y", stngs.THREAD_GRID_Y ).toInt()));
	settingsUi.tableWidget->item(2, 0)->setText(QString::number(settings.value( "DreamCaster/SCALE_COEF", stngs.SCALE_COEF ).value<float>()));
	settingsUi.tableWidget->item(3, 0)->setText(QString::number(settings.value( "DreamCaster/COLORING_COEF", stngs.COLORING_COEF ).value<float>()));
	settingsUi.tableWidget->item(4, 0)->setText(QString::number(settings.value( "DreamCaster/RFRAME_W", stngs.RFRAME_W ).toInt()));
	settingsUi.tableWidget->item(5, 0)->setText(QString::number(settings.value( "DreamCaster/RFRAME_H", stngs.RFRAME_H ).toInt()));
	settingsUi.tableWidget->item(6, 0)->setText(QString::number(settings.value( "DreamCaster/VFRAME_W", stngs.VFRAME_W ).toInt()));
	settingsUi.tableWidget->item(7, 0)->setText(QString::number(settings.value( "DreamCaster/VFRAME_H", stngs.VFRAME_H ).toInt()));
	settingsUi.tableWidget->item(8, 0)->setText(QString::number(settings.value( "DreamCaster/ORIGIN_Z", stngs.ORIGIN_Z ).value<float>()));
	settingsUi.tableWidget->item(9, 0)->setText(QString::number(settings.value( "DreamCaster/PLANE_Z", stngs.PLANE_Z ).value<float>()));
	settingsUi.tableWidget->item(10,0)->setText(QString::number(settings.value( "DreamCaster/PLANE_H_W", stngs.PLANE_H_W ).value<float>()));
	settingsUi.tableWidget->item(11,0)->setText(QString::number(settings.value( "DreamCaster/PLANE_H_H", stngs.PLANE_H_H ).value<float>()));
	settingsUi.tableWidget->item(12,0)->setText(QString::number(settings.value( "DreamCaster/TREE_L1", stngs.TREE_L1 ).toInt()));
	settingsUi.tableWidget->item(13,0)->setText(QString::number(settings.value( "DreamCaster/TREE_L2", stngs.TREE_L2 ).toInt()));
	settingsUi.tableWidget->item(14,0)->setText(QString::number(settings.value( "DreamCaster/TREE_L3", stngs.TREE_L3 ).toInt()));
	settingsUi.tableWidget->item(15,0)->setText(QString::number(settings.value( "DreamCaster/TREE_SPLIT1", stngs.TREE_SPLIT1 ).toInt()));
	settingsUi.tableWidget->item(16,0)->setText(QString::number(settings.value( "DreamCaster/TREE_SPLIT2", stngs.TREE_SPLIT2 ).toInt()));
	settingsUi.tableWidget->item(17,0)->setText(QString::number(settings.value( "DreamCaster/BG_COL_R", stngs.BG_COL_R ).toInt()));
	settingsUi.tableWidget->item(18,0)->setText(QString::number(settings.value( "DreamCaster/BG_COL_G", stngs.BG_COL_G ).toInt()));
	settingsUi.tableWidget->item(19,0)->setText(QString::number(settings.value( "DreamCaster/BG_COL_B", stngs.BG_COL_B ).toInt()));
	settingsUi.tableWidget->item(20,0)->setText(QString::number(settings.value( "DreamCaster/PLATE_COL_R", stngs.PLATE_COL_R ).toInt()));
	settingsUi.tableWidget->item(21,0)->setText(QString::number(settings.value( "DreamCaster/PLATE_COL_G", stngs.PLATE_COL_G ).toInt()));
	settingsUi.tableWidget->item(22,0)->setText(QString::number(settings.value( "DreamCaster/PLATE_COL_B", stngs.PLATE_COL_B ).toInt()));
	settingsUi.tableWidget->item(23,0)->setText(QString::number(settings.value( "DreamCaster/COL_RANGE_MIN_R", stngs.COL_RANGE_MIN_R ).toInt()));
	settingsUi.tableWidget->item(24,0)->setText(QString::number(settings.value( "DreamCaster/COL_RANGE_MIN_G", stngs.COL_RANGE_MIN_G ).toInt()));
	settingsUi.tableWidget->item(25,0)->setText(QString::number(settings.value( "DreamCaster/COL_RANGE_MIN_B", stngs.COL_RANGE_MIN_B ).toInt()));
	settingsUi.tableWidget->item(26,0)->setText(QString::number(settings.value( "DreamCaster/COL_RANGE_MAX_R", stngs.COL_RANGE_MAX_R ).toInt()));
	settingsUi.tableWidget->item(27,0)->setText(QString::number(settings.value( "DreamCaster/COL_RANGE_MAX_G", stngs.COL_RANGE_MAX_G ).toInt()));
	settingsUi.tableWidget->item(28,0)->setText(QString::number(settings.value( "DreamCaster/COL_RANGE_MAX_B", stngs.COL_RANGE_MAX_B ).toInt()));
	settingsUi.tableWidget->item(29,0)->setText(QString::number(settings.value( "DreamCaster/BATCH_SIZE", stngs.BATCH_SIZE ).toInt()));
	return 1;
}

void iADreamCaster::SensitivityChangedSlot()
{
	if(!datasetOpened) return;
	stabilitySensitivity = (paramMax-paramMin)*ui.s_sensitivity->value()/(ui.s_sensitivity->maximum()-ui.s_sensitivity->minimum());
	ui.l_sens_val->setText(QString::number(stabilitySensitivity));
	ui.l_sens_min->setText(QString::number(0));
	ui.l_sens_max->setText(QString::number(paramMax-paramMin));
	UpdateStabilityWidget();
}

int iADreamCaster::UpdateStabilityWidget()
{
	if(!datasetOpened) return 0;
	unsigned int j=0;
	float minVal = 255, maxVal = 0, delta;
	for (int i=-(int)stabilityView->countX(); i<=(int)stabilityView->countX(); i++, j++)
	{
		int indx = (int)curIndX+i;
		if(indx<0)
		{
			int buf = indx/cntX;
			indx-= buf*cntX;
			indx+=cntX-1;
		}
		else
			indx = indx%cntX;
		double val =  fabs( ( (placementsParams[indx][curIndZ])[curParamInd] - (placementsParams[curIndX][curIndZ])[curParamInd] ) / stabilitySensitivity );
		if(val>1.) val = 1.0;
		val=255-val*255;
		if(val<minVal) minVal = val;
		if(val>maxVal) maxVal = val;
		//stabilityView->colsXY[j][0] = QColor((int)val, (int)val, (int)val);
	}
	delta = (maxVal-minVal)/255.0;
	{
	int colComponent = (int)((1-delta)*255);
	stabilityView->m_colArrowX = QColor(colComponent, colComponent, colComponent);
	}
	//stabilityView->colArrowX = QColor(	minr+delta*(maxr - minr), 
	//									ming+delta*(maxg - ming),
	//									minb+delta*(maxb - minb));
	j=0;
	minVal = 255; maxVal = 0;
	for (int i=-(int)stabilityView->countY(); i<=(int)stabilityView->countY(); i++, j++)
	{
		int indx = (int)curIndZ+i;
		if(indx<0)
		{
			int buf = indx/cntZ;
			indx-= buf*cntZ;
			indx+=cntZ-1;
		}
		else
			indx = indx%cntZ;
		double val =  fabs( ( (placementsParams[curIndX][indx])[curParamInd] - (placementsParams[curIndX][curIndZ])[curParamInd] ) / stabilitySensitivity );
		if(val>1.) val = 1.0;
		val=255-val*255;
		if(val<minVal) minVal = val;
		if(val>maxVal) maxVal = val;
		//stabilityView->colsXY[0][j] = QColor((int)val, (int)val, (int)val);
	}
	delta = (maxVal-minVal)/255.0;
	{
	int colComponent = (int)((1-delta)*255);
	stabilityView->m_colArrowY = QColor( colComponent, colComponent, colComponent);
	}
	//////////////////////////////////////////////////////////////////////////
	for (int i=-(int)stabilityView->countX(), j=0; i<=(int)stabilityView->countX(); i++, j++)
	{
		for (int k=-(int)stabilityView->countY(), l=0; k<=(int)stabilityView->countY(); k++, l++)
		{
			int indxZ = (int)curIndZ+k;
			if(indxZ<0)
			{
				int buf = indxZ/cntZ;
				indxZ-= buf*cntZ;
				indxZ+=cntZ-1;
			}
			else
				indxZ = indxZ%cntZ;

			int indxX = (int)curIndX+i;
			if(indxX<0)
			{
				int buf = indxX/cntX;
				indxX-= buf*cntX;
				indxX+=cntX-1;
			}
			else
				indxX = indxX%cntX;
			double val =  fabs( ( (placementsParams[indxX][indxZ])[curParamInd] - (placementsParams[curIndX][curIndZ])[curParamInd] ) / stabilitySensitivity );
			if(val>1.) val = 1.0;
			val=255-val*255;
			bool worse = true;
			switch(ui.cb_rangeParameter->currentIndex())
			{
			case 0://av penetration length
				worse = (placementsParams[indxX][indxZ])[curParamInd] > (placementsParams[curIndX][curIndZ])[curParamInd];
				break;
			case 1://dip angle cosine
				worse = (placementsParams[indxX][indxZ])[curParamInd] < (placementsParams[curIndX][curIndZ])[curParamInd];
				break;
			case 2://max penetration length
				worse = (placementsParams[indxX][indxZ])[curParamInd] > (placementsParams[curIndX][curIndZ])[curParamInd];
				break;
			case 3://bad sufrace area %
				worse = (placementsParams[indxX][indxZ])[curParamInd] > (placementsParams[curIndX][curIndZ])[curParamInd];
				break;
			default:
				break;
			}
			if(worse)
				stabilityView->m_colsXY[j][l] = QColor(255, (int)val, (int)val);//red
			else
				stabilityView->m_colsXY[j][l] = QColor((int)val, 255, (int)val);//green

		}
	}
	//////////////////////////////////////////////////////////////////////////
	stabilityView->repaint();
	return 1;
}

void iADreamCaster::UpdateStabilityOnMouseMoveCheckedSlot()
{
	if(ui.cb_updateStabilityOnMouseMove->isChecked())
		connect(ViewsFrame, SIGNAL(mouseMoveEventSignal()), this, SLOT(ViewsMouseMoveSlot()));
	else
		disconnect(ViewsFrame, SIGNAL(mouseMoveEventSignal()), this, SLOT(ViewsMouseMoveSlot()));
}

void iADreamCaster::ViewsMouseMoveSlot()
{
	int buf = (int)((float)ViewsFrame->lastMoveX/((float)ViewsFrame->width()/cntX));
	if(buf<0 || buf>=cntX) return;
	curIndX = buf;
	buf = (int)((float)ViewsFrame->lastMoveY/((float)ViewsFrame->width()/cntZ));
	if(buf<0 || buf>=cntX) return;
	curIndZ = buf;
	UpdateStabilityWidget();
}

void iADreamCaster::CurrentParameterChangedSlot()
{

}

void iADreamCaster::ClearPrevData()
{
	//plotData
	if(rotationsParams)
	{
		for (int x=0; x<cntX; x++)
		{
			if(rotationsParams[x])
			{
				for (int y=0; y<cntY; y++)
				{
					if(rotationsParams[x][y])
						delete [] rotationsParams[x][y];
				}
				delete [] rotationsParams[x];
			}
		}
		delete [] rotationsParams;
	}
	//rotations
	if(rotations)
	{
		for (int x=0; x<cntX; x++)
		{
			if(rotations[x])
			{
				for (int y=0; y<cntY; y++)
				{
					if(rotations[x][y])
						delete [] rotations[x][y];
				}
				delete [] rotations[x];
			}
		}
		delete [] rotations;
	}
	//plotColumnData
	if(placementsParams)
	{
		for (int x=0; x<cntX; x++)
		{
			if(placementsParams[x])
				delete [] placementsParams[x];
			placementsParams[x] = 0;
		}
		delete [] placementsParams;
		placementsParams=0;
	}
	//weightedParams
	if(weightedParams)
	{
		for (int x=0; x<cntX; x++)
		{
			if(weightedParams[x])
				delete [] weightedParams[x];
			weightedParams[x] = 0;
		}
		delete [] weightedParams;
		weightedParams=0;
	}
}

void iADreamCaster::AllocateData()
{
	//plotData
	rotationsParams = new iAparameters_t**[cntX];
	for (int x=0; x<cntX; x++)
	{
		rotationsParams[x] = new iAparameters_t*[cntY];
		for (int y=0; y<cntY; y++)
			rotationsParams[x][y] = new iAparameters_t[cntZ];

	}
	//rotations
	rotations = new iArotation_t**[cntX];
	for (int x=0; x<cntX; x++)
	{
		rotations[x] = new iArotation_t*[cntY];
		for (int y=0; y<cntY; y++)
			rotations[x][y] = new iArotation_t[cntZ];
	}
	//plotColumnData
	placementsParams = new iAparameters_t*[cntX];
	for (int x=0; x<cntX; x++)
		placementsParams[x] = new iAparameters_t[cntZ];
	//weightedParams
	weightedParams = new double*[cntX];
	for (int x=0; x<cntX; x++)
		weightedParams[x] = new double[cntZ];
}

void iADreamCaster::ProjectionChangedSlot()
{
	curIndY = ui.hs_projection->value();
	ui.l_currentProjection->setText(QString::number(curIndY));
	if(ui.cb_UpdateViewsSlider->isChecked() && modelOpened && datasetOpened)
	{
		UpdateInfoLabels();
		UpdateView();
	}
	else 
		UpdateSlot();
}

void iADreamCaster::UpdateView()
{
	// go
	tracer->setPositon(set_pos);
	tracer->setRotations(M_PI*rotations[curIndX][curIndY][curIndZ].rotX, M_PI*rotations[curIndX][curIndY][curIndZ].rotY, M_PI*rotations[curIndX][curIndY][curIndZ].rotZ);
	iAVec3f o; // rays' origin point
	iAVec3f vp_corners[2];// plane's corners in 3d
	iAVec3f vp_delta[2];// plane's x and y axes' directions in 3D
	InitRender(vp_corners, vp_delta, &o);
	//unsigned int tri_cnt = tracer->scene()->GetNrPrimitives();
	long ftime;//fstart,
	QElapsedTimer time;
	time.start();
	//fstart = GetTickCount();
	tracer->SetCutAABBList(&cutFigList->aabbs);
	Render(vp_corners, vp_delta, &o, false);
	ftime = time.elapsed();//GetTickCount() - fstart;
	char t[] = "00:00.000";
	t[6] = (ftime / 100) % 10 + '0';
	t[7] = (ftime / 10) % 10 + '0';
	t[8] = (ftime % 10) + '0';
	int secs = (ftime / 1000) % 60;
	int mins = (ftime / 60000) % 100;
	t[3] = ((secs / 10) % 10) + '0';
	t[4] = (secs % 10) + '0';
	t[1] = (mins % 10) + '0';
	t[0] = ((mins / 10) % 10) + '0';
	ui.TimeLabel->setText(t);

	actor->SetOrientation(0,0,0);
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(M_PI*rotations[curIndX][curIndY][curIndZ].rotX), 1.0f, 0.0f, 0.0f );//degrees
	actor->RotateWXYZ(vtkMath::DegreesFromRadians(M_PI*rotations[curIndX][curIndY][curIndZ].rotY), 0.0f, 1.0f, 0.0f);
	actor->RotateZ(vtkMath::DegreesFromRadians(M_PI*rotations[curIndX][curIndY][curIndZ].rotZ));
	actor->SetPosition(set_pos[0], set_pos[1], set_pos[2]);
	cutAABActor->SetOrientation(0,0,0);
	cutAABActor->RotateWXYZ(vtkMath::DegreesFromRadians(M_PI*rotations[curIndX][curIndY][curIndZ].rotX), 1.0f, 0.0f, 0.0f );//degrees
	cutAABActor->RotateWXYZ(vtkMath::DegreesFromRadians(M_PI*rotations[curIndX][curIndY][curIndZ].rotY), 0.0f, 1.0f, 0.0f);
	cutAABActor->RotateZ(vtkMath::DegreesFromRadians(M_PI*rotations[curIndX][curIndY][curIndZ].rotZ));
	cutAABActor->SetPosition(set_pos[0], set_pos[1], set_pos[2]);
	UpdateSlot();
}

void iADreamCaster::UpdateInfoLabels()
{
	ui.avParamRend->setText(QString::number( (rotationsParams[curIndX][curIndY][curIndZ])[curParamInd] ));
	ui.avParamRot->setText(QString::number( (placementsParams[curIndX][curIndZ])[curParamInd] ));
	ui.lb_rendX->setText(QString::number(curIndX));
	ui.lb_rendY->setText(QString::number(curIndY));
	ui.lb_rendZ->setText(QString::number(curIndZ));
	ui.lb_rotX->setText(QString::number(vtkMath::DegreesFromRadians(rotations[curIndX][curIndY][curIndZ].rotX)*M_PI));
	ui.lb_rotY->setText(QString::number(vtkMath::DegreesFromRadians(rotations[curIndX][curIndY][curIndZ].rotY)*M_PI));
	ui.lb_rotZ->setText(QString::number(vtkMath::DegreesFromRadians(rotations[curIndX][curIndY][curIndZ].rotZ)*M_PI));
	ui.lb_posx->setText(QString::number(set_pos[0]));
	ui.lb_posy->setText(QString::number(set_pos[1]));
	ui.lb_posz->setText(QString::number(set_pos[2]));
}

void iADreamCaster::StabilityResolutionChangedSlot()
{
	if(!datasetOpened) return;
	stabilityView->SetCount(ui.s_sensRes->value());
	ui.l_sensRes->setText(QString::number(ui.s_sensRes->value()));
	UpdateStabilityWidget();
}

void iADreamCaster::ViewsReset()
{
	if(viewsPxmp)
		delete viewsPxmp;
	viewsPxmp = new QPixmap(cntX, cntZ);
	ViewsFrame->SetPixmap(viewsPxmp);
	ViewsFrame->RemoveHighlights();
}

void iADreamCaster::TopPlacementsChangedSlot()
{
	float curParam;
	double max_param=-1000;
	double min_param=100000;
	for (int x=0; x<cntX; x++)
		for (int z=0; z<cntZ; z++)
		{
			if( (placementsParams[x][z])[curParamInd] > max_param)
				max_param =  (placementsParams[x][z])[curParamInd];
			if((placementsParams[x][z])[curParamInd] < min_param)
				min_param =  (placementsParams[x][z])[curParamInd];
		}
	if(max_param==0)
		max_param=1;
	double delta = max_param - min_param;
	switch(ui.cb_rangeParameter->currentIndex())
	{
	case 0://av penetration length
		max_param = min_param + delta*((100.f-(float)ui.hs_topPlacements->value())/100.f);
		break;
	case 1://dip angle cosine
		min_param = max_param - delta*((100.f-(float)ui.hs_topPlacements->value())/100.f);
		break;
	case 2://max penetration length
		max_param = min_param + delta*((100.f-(float)ui.hs_topPlacements->value())/100.f);
		break;
	case 3://bad surface area
		max_param = min_param + delta*((100.f-(float)ui.hs_topPlacements->value())/100.f);
		break;
	default:
		break;
	}

	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			if( 
				(placementsParams[x][z])[curParamInd] <= max_param 
				 && 
				(placementsParams[x][z])[curParamInd] >= min_param
			  )
			{
				switch(curParamInd)
				{
				case 0://av. penetration length
					curParam = ( (placementsParams[x][z])[curParamInd] - min_param ) / scalec;
					break;
				case 1://dip angle cosine
					curParam = (max_param - (placementsParams[x][z])[curParamInd]) / scalec;
					break;
				case 2://max penetration length
					curParam = ( (placementsParams[x][z])[curParamInd] - min_param ) / scalec;
					break;
				case 3://bad surface area %
					curParam = ( (placementsParams[x][z])[curParamInd] - min_param ) / scalec;
					break;
				default:
					break;
				}
				if (curParam>1.f) curParam=1.f;
				unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*curParam) << 16) + 
					((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*curParam) << 8) + 
					(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*curParam);
				viewsBuffer[x + z*cntX] = lencol;
			}
			else
			{
				viewsBuffer[x + z*cntX] = (unsigned int)(0);
			}
		}
	}
	ui.l_lowCut->setText(QString::number( ui.hs_topPlacements->value() ) );
	UpdateSlot();
}

void iADreamCaster::fillParamBuffer( unsigned int* dest, int paramInd)
{
	double max_param=-1000;
	double min_param = 10000;
	for (int x=0; x<cntX; x++)
		for (int z=0; z<cntZ; z++)
		{
			if( (placementsParams[x][z])[paramInd] > max_param )
				max_param = (placementsParams[x][z])[paramInd];
			if( (placementsParams[x][z])[paramInd] < min_param )
				min_param = (placementsParams[x][z])[paramInd];
		}

	if(max_param==0)
		max_param=1.0;
	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;
	
	double curParam;
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			switch(paramInd)
			{
			case 0://av. penetration length
				curParam = (placementsParams[x][z].avPenLen - min_param)/scalec;
				break;
			case 1://dip angle cosine
				curParam = (max_param - placementsParams[x][z].avDipAng)/scalec;
				break;
			case 2://max penetration length
				curParam = (placementsParams[x][z].maxPenLen - min_param)/scalec;
				break;
			case 3://bad surface area %
				curParam = (placementsParams[x][z].badAreaPercentage - min_param)/scalec;
				break;
			default:
				break;
			}
			if (curParam>1.f) curParam=1.f;
			unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*curParam) << 16) + 
				((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*curParam) << 8) + 
				(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*curParam);
			dest[x + z*cntX] = lencol;
		}
	}
}

void iADreamCaster::ComparisonTabPlacementPickedSlot(int x, int y)
{
	int pickedX, pickedZ;
	if(!cntZ || !cntX) return;
	int buf = (int)((float)x/((float)comparisonTab->paramWidgets[0].paintWidget->width()/cntX));
	if(buf<0 || buf>=cntX) return;
	pickedX = buf;
	buf = (int)((float)y/((float)comparisonTab->paramWidgets[0].paintWidget->height()/cntZ));
	if(buf<0 || buf>=cntZ) return;
	pickedZ = buf;
	setPickedPlacement(pickedX, curIndY, pickedZ);
	/*for (int i=0; i<3; i++)
	{
		comparisonTab->paramWidgets[i].paintWidget->SetHiglightedIndices(&pickedX, &pickedZ, 1);
		comparisonTab->paramWidgets[i].paintWidget->update();
	}

	ui.lb_paramRot1->setText(QString::number( (placementsParams[pickedX][pickedZ])[0] ));
	ui.lb_paramRot2->setText(QString::number( (placementsParams[pickedX][pickedZ])[1] ));
	ui.lb_paramRot3->setText(QString::number( (placementsParams[pickedX][pickedZ])[2] ));
	///ui.lb_rendX_4->setText(QString::number(pickedX));
	///ui.lb_rendZ_4->setText(QString::number(pickedZ));
	///ui.lb_rotX_4->setText(QString::number(rotations[pickedX][0][pickedZ].rotX));
	///ui.lb_rotZ_4->setText(QString::number(rotations[pickedX][0][pickedZ].rotZ));
	///ui.lb_posx_4->setText(QString::number(set_pos[0]));
	///ui.lb_posy_4->setText(QString::number(set_pos[1]));
	///ui.lb_posz_4->setText(QString::number(set_pos[2]));
	//update view if needed
	if(ui.cb_UpdateViews->isChecked() && modelOpened && datasetOpened)
	{
		curIndX = pickedX;
		curIndZ = pickedZ;
		UpdateView();
	}*/
}

void iADreamCaster::LowCutParam1Slot()
{
	int paramInd = 0;
	float curParam;
	double max_param=-1000;
	double min_param=100000;
	for (int x=0; x<cntX; x++)
		for (int z=0; z<cntZ; z++)
		{
			if( (placementsParams[x][z])[paramInd] > max_param)
				max_param =  (placementsParams[x][z])[paramInd];
			if((placementsParams[x][z])[paramInd] < min_param)
				min_param =  (placementsParams[x][z])[paramInd];
		}
	if(max_param==0)
		max_param=1;
	double delta = max_param - min_param;
	
	//av penetration length
	max_param = min_param + delta*((100.f-(float)ui.s_lowCut1->value())/100.f);
		

	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			if( 
				(placementsParams[x][z])[paramInd] <= max_param 
			 && 
			 (placementsParams[x][z])[paramInd] >= min_param
		  )
			{
				//av. penetration length
				curParam = ( (placementsParams[x][z])[paramInd] - min_param ) / scalec;
	
				if (curParam>1.f) curParam=1.f;
				unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*curParam) << 16) + 
					((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*curParam) << 8) + 
					(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*curParam);
				comparisonTab->paramWidgets[paramInd].buffer[x + z*cntX] = lencol;
			}
			else
			{
				comparisonTab->paramWidgets[paramInd].buffer[x + z*cntX] = (unsigned int)0;
			}
		}
	}
	ui.l_lowCut1->setText( QString::number( ui.s_lowCut1->value()) );
	comparisonTab->Update();
}

void iADreamCaster::LowCutParam2Slot()
{
	int paramInd = 2;
	float curParam;
	double max_param=-1000;
	double min_param=100000;
	for (int x=0; x<cntX; x++)
		for (int z=0; z<cntZ; z++)
		{
			if( (placementsParams[x][z])[paramInd] > max_param)
				max_param =  (placementsParams[x][z])[paramInd];
			if((placementsParams[x][z])[paramInd] < min_param)
				min_param =  (placementsParams[x][z])[paramInd];
		}
	if(max_param==0)
		max_param=1;
	double delta = max_param - min_param;
	//max pen len
	max_param = min_param + delta*((100.f-(float)ui.s_lowCut2->value())/100.f);

	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			if( 
				(placementsParams[x][z])[paramInd] <= max_param 
				&& 
				(placementsParams[x][z])[paramInd] >= min_param
				)
			{
				//max pen len
				curParam = ( (placementsParams[x][z])[paramInd] - min_param ) / scalec;
					
				if (curParam>1.f) curParam=1.f;
				unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*curParam) << 16) + 
					((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*curParam) << 8) + 
					(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*curParam);
				comparisonTab->paramWidgets[1].buffer[x + z*cntX] = lencol;
			}
			else
			{
				comparisonTab->paramWidgets[1].buffer[x + z*cntX] = (unsigned int)0;
			}
		}
	}
	ui.l_lowCut2->setText(QString::number( ui.s_lowCut2->value()) );
	comparisonTab->Update();
}

void iADreamCaster::LowCutParam3Slot()
{
	int paramInd = 3;
	float curParam;
	double max_param=-1000;
	double min_param=100000;
	for (int x=0; x<cntX; x++)
		for (int z=0; z<cntZ; z++)
		{
			if( (placementsParams[x][z])[paramInd] > max_param)
				max_param =  (placementsParams[x][z])[paramInd];
			if((placementsParams[x][z])[paramInd] < min_param)
				min_param =  (placementsParams[x][z])[paramInd];
		}
	if(max_param==0)
		max_param=1;
	double delta = max_param - min_param;
	//max penetration length
	max_param = min_param + delta*((100.f-(float)ui.s_lowCut3->value())/100.f);	

	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			if( 
				(placementsParams[x][z])[paramInd] <= max_param 
				&& 
				(placementsParams[x][z])[paramInd] >= min_param
				)
			{
				//max penetration length
				curParam = ( (placementsParams[x][z])[paramInd] - min_param ) / scalec;
					
				if (curParam>1.f) curParam=1.f;
				unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*curParam) << 16) + 
					((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*curParam) << 8) + 
					(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*curParam);
				comparisonTab->paramWidgets[2].buffer[x + z*cntX] = lencol;
			}
			else
			{
				comparisonTab->paramWidgets[2].buffer[x + z*cntX] = (unsigned int)0;
			}
		}
	}
	ui.l_lowCut3->setText(QString::number( ui.s_lowCut3->value()) );
	comparisonTab->Update();
}

void iADreamCaster::UpdateWeightingResultsSlot()
{
	double coefs[3] = { ui.dsb_weightCoef1->value(), ui.dsb_weightCoef2->value(), ui.dsb_weightCoef3->value() };
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			//TODO: do not forget that dip angle cosine used inversed!
			weightedParams[x][z] = coefs[0]*(placementsParams[x][z])[indices[0]] + coefs[1]*((placementsParams[x][z])[indices[1]]) + coefs[2]*(placementsParams[x][z])[indices[2]];
		}
	}
	//
	double max_param=-1000;
	double min_param=100000;
	for (int x=0; x<cntX; x++)
		for (int z=0; z<cntZ; z++)
		{
			if( weightedParams[x][z] > max_param)
				max_param =  weightedParams[x][z];
			if(weightedParams[x][z] < min_param)
				min_param =  weightedParams[x][z];
		}
	if(max_param==0)
		max_param=1;
	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;
	double lookupNumber = plot3dWeighting->GetNumberOfLookupTableValues();
	double * scalarVals = new double[cntX*cntZ];
	double * plotData = new double[cntX*cntZ];
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			double curParam = ( weightedParams[x][z] - min_param ) / scalec;
			if (curParam>1.f) curParam=1.f;
			unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*curParam) << 16) + 
							((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*curParam) << 8) + 
							(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*curParam);
			weightingTab->results.buffer[x + z*cntX] = lencol;
			scalarVals[x + z*cntX] = curParam*lookupNumber;
			plotData[x + z*cntX] = curParam;
		}
	}
	weightingTab->Update();

	//plot3DWeighting fill with data
	plot3dWeighting->SetAutoScalarRange();
	plot3dWeighting->loadFromData(plotData, scalarVals, cntX, cntZ);
	plot3dWeighting->ShowWireGrid(1, 0,0,0);
	plot3dWeighting->Update();
	plot3dWeighting->GetRenderer()->GetRenderWindow()->Render();
	delete [] plotData;
	delete [] scalarVals;
}

void iADreamCaster::WeightingResultsPlacementPickedSlot(int x, int y)
{
	int pickedX, pickedZ;
	if(!cntZ || !cntX) return;
	int buf = (int)((float)x/((float)weightingTab->results.paintWidget->width()/cntX));
	if(buf<0 || buf>=cntX) return;
	pickedX = buf;
	buf = (int)((float)y/((float)weightingTab->results.paintWidget->height()/cntZ));
	if(buf<0 || buf>=cntZ) return;
	pickedZ = buf;

	setPickedPlacement(pickedX, curIndY, pickedZ);
	/*for (int i=0; i<3; i++)
	{
		weightingTab->results.paintWidget->SetHiglightedIndices(&pickedX, &pickedZ, 1);
		weightingTab->results.paintWidget->update();
	}

	///ui.lb_rendX_3->setText(QString::number(pickedX));
	///ui.lb_rendZ_3->setText(QString::number(pickedZ));
	///ui.lb_rotX_3->setText(QString::number(rotations[pickedX][0][pickedZ].rotX));
	///ui.lb_rotZ_3->setText(QString::number(rotations[pickedX][0][pickedZ].rotZ));
	///ui.lb_posx_3->setText(QString::number(set_pos[0]));
	///ui.lb_posy_3->setText(QString::number(set_pos[1]));
	///ui.lb_posz_3->setText(QString::number(set_pos[2]));
	ui.lb_avParamRot_3->setText( QString::number( weightedParams[pickedX][pickedZ] ) );
	//update view if needed
	if(ui.cb_UpdateViews->isChecked() && modelOpened && datasetOpened)
	{
		curIndX = pickedX;
		curIndZ = pickedZ;
		UpdateView();
	}*/
}

void iADreamCaster::LowCutWeightingResSlot()
{
	float curParam;
	double max_param=-1000;
	double min_param=100000;
	for (int x=0; x<cntX; x++)
		for (int z=0; z<cntZ; z++)
		{
			if( weightedParams[x][z] > max_param)
				max_param =  weightedParams[x][z];
			if(weightedParams[x][z] < min_param)
				min_param =  weightedParams[x][z];
		}
	if(max_param==0)
		max_param=1;
	double delta = max_param - min_param;
	max_param = min_param + delta*((100.f-(float)ui.s_lowCutRes->value())/100.f);

	double scalec = max_param-min_param;
	if(scalec==0) scalec=1;
	for (int x=0; x<cntX; x++)
	{
		for (int z=0; z<cntZ; z++)
		{
			if( 
				weightedParams[x][z] <= max_param 
				&& 
				weightedParams[x][z] >= min_param
			  )
			{
				curParam = ( weightedParams[x][z] - min_param ) / scalec;
				if (curParam>1.f) curParam=1.f;
				unsigned int lencol = ((unsigned int)(stngs.COL_RANGE_MIN_R+stngs.COL_RANGE_DR*curParam) << 16) + 
								((unsigned int)(stngs.COL_RANGE_MIN_G+stngs.COL_RANGE_DG*curParam) << 8) + 
								(unsigned int)(stngs.COL_RANGE_MIN_B+stngs.COL_RANGE_DB*curParam);
				weightingTab->results.buffer[x + z*cntX] = lencol;
			}
			else
			{
				weightingTab->results.buffer[x + z*cntX] = (unsigned int)0;
			}
		}
	}
	ui.l_lowCutRes->setText(QString::number( ui.s_lowCut3->value()) );
	weightingTab->Update();
}

void iADreamCaster::AddCutFigSlot()
{
	if(!modelOpened)
		return;
	int index = cutFigList->add( new iACutAAB( "BOX"+QString::number( cutFigList->count() ) ) );
	//if first item added then set selected row on that item
	cutFigList->SetCurIndex(index);
	ui.listCutFig->insertItem( ui.listCutFig->count(), "" );
	ui.listCutFig->setCurrentRow(index);
	CutFigParametersChanged();
}

void iADreamCaster::RemoveCutFigSlot()
{
	if(!modelOpened || cutFigList->count() <= 0 )
		return;
	ui.listCutFig->takeItem( cutFigList->curIndex());
	cutFigList->remove(cutFigList->curIndex());
	updateCutAABVtk();
}

void iADreamCaster::CutFigPicked()
{
	cutFigList->SetCurIndex(ui.listCutFig->currentRow());
	iACutAAB *curBox = cutFigList->item(cutFigList->curIndex());
	CutFigParametersChangedOFF = true;
	ui.s_aab_minx->setValue(curBox->slidersValues[0]);
	ui.s_aab_maxx->setValue(curBox->slidersValues[1]);
	ui.s_aab_miny->setValue(curBox->slidersValues[2]);
	ui.s_aab_maxy->setValue(curBox->slidersValues[3]);
	ui.s_aab_minz->setValue(curBox->slidersValues[4]);
	ui.s_aab_maxz->setValue(curBox->slidersValues[5]);
	CutFigParametersChangedOFF = false;
	CutFigParametersChanged();
}

void iADreamCaster::CutFigParametersChanged()
{
	if(!modelOpened || CutFigParametersChangedOFF || cutFigList->count()==0)
		return;
	float val = 0.0f, delta = 0.0f;
	iACutAAB * curAAB = cutFigList->item(cutFigList->curIndex());
	//x
	if(ui.s_aab_maxx->value()<ui.s_aab_minx->value())
		ui.s_aab_maxx->setValue(ui.s_aab_minx->value());
	delta = tracer->scene()->getBSPTree()->m_aabb.x2-tracer->scene()->getBSPTree()->m_aabb.x1;
	val = tracer->scene()->getBSPTree()->m_aabb.x1+delta*GetSliderNormalizedValue(ui.s_aab_minx);
	ui.l_aab_minx->setText(QString::number(val));
	curAAB->box.x1 = val;
	val = tracer->scene()->getBSPTree()->m_aabb.x1+delta*GetSliderNormalizedValue(ui.s_aab_maxx);
	ui.l_aab_maxx->setText(QString::number(val));
	curAAB->box.x2 = val;
	//y
	if(ui.s_aab_maxy->value()<ui.s_aab_miny->value())
		ui.s_aab_maxy->setValue(ui.s_aab_miny->value());
	delta = tracer->scene()->getBSPTree()->m_aabb.y2-tracer->scene()->getBSPTree()->m_aabb.y1;
	val = tracer->scene()->getBSPTree()->m_aabb.y1+delta*GetSliderNormalizedValue(ui.s_aab_miny);
	ui.l_aab_miny->setText(QString::number(val));
	curAAB->box.y1 = val;
	val = tracer->scene()->getBSPTree()->m_aabb.y1+delta*GetSliderNormalizedValue(ui.s_aab_maxy);
	ui.l_aab_maxy->setText(QString::number(val));
	curAAB->box.y2 = val;
	//z
	if(ui.s_aab_maxz->value()<ui.s_aab_minz->value())
		ui.s_aab_maxz->setValue(ui.s_aab_minz->value());
	delta = tracer->scene()->getBSPTree()->m_aabb.z2-tracer->scene()->getBSPTree()->m_aabb.z1;
	val = tracer->scene()->getBSPTree()->m_aabb.z1+delta*GetSliderNormalizedValue(ui.s_aab_minz);
	ui.l_aab_minz->setText(QString::number(val));
	curAAB->box.z1 = val;
	val = tracer->scene()->getBSPTree()->m_aabb.z1+delta*GetSliderNormalizedValue(ui.s_aab_maxz);
	ui.l_aab_maxz->setText(QString::number(val));
	curAAB->box.z2 = val;
	//
	curAAB->SetSlidersValues(ui.s_aab_minx->value(),ui.s_aab_maxx->value(),
		ui.s_aab_miny->value(),ui.s_aab_maxy->value(),
		ui.s_aab_minz->value(),ui.s_aab_maxz->value());
	ui.listCutFig->currentItem()->setText(curAAB->name() +": "+curAAB->GetDimString() );
	updateCutAABVtk();
}

int iADreamCaster::updateCutAABVtk()
{
	vtkQuad *GridQuad=vtkQuad::New();
	vtkCellArray *gridCells=vtkCellArray::New();
	vtkPoints *points = vtkPoints::New();
	vtkPolyData *grid = vtkPolyData::New();
	for( int i = 0; i < cutFigList->count(); i++)
	{
		vtkIdType k = points->GetNumberOfPoints();
		iAaabb * curAABB = cutFigList->aabbs[i];
		points->InsertNextPoint(curAABB->x1, curAABB->y1, curAABB->z1);
		points->InsertNextPoint(curAABB->x1, curAABB->y2, curAABB->z1);
		points->InsertNextPoint(curAABB->x2, curAABB->y2, curAABB->z1);
		points->InsertNextPoint(curAABB->x2, curAABB->y1, curAABB->z1);

		points->InsertNextPoint(curAABB->x1, curAABB->y1, curAABB->z2);
		points->InsertNextPoint(curAABB->x1, curAABB->y2, curAABB->z2);
		points->InsertNextPoint(curAABB->x2, curAABB->y2, curAABB->z2);
		points->InsertNextPoint(curAABB->x2, curAABB->y1, curAABB->z2);
		GridQuad->GetPointIds()->SetId(0, k+0);
		GridQuad->GetPointIds()->SetId(1, k+1);
		GridQuad->GetPointIds()->SetId(2, k+2);
		GridQuad->GetPointIds()->SetId(3, k+3);
		gridCells->InsertNextCell(GridQuad);

		GridQuad->GetPointIds()->SetId(0, k+0);
		GridQuad->GetPointIds()->SetId(1, k+4);
		GridQuad->GetPointIds()->SetId(2, k+5);
		GridQuad->GetPointIds()->SetId(3, k+1);
		gridCells->InsertNextCell(GridQuad);

		GridQuad->GetPointIds()->SetId(0, k+4);
		GridQuad->GetPointIds()->SetId(1, k+5);
		GridQuad->GetPointIds()->SetId(2, k+6);
		GridQuad->GetPointIds()->SetId(3, k+7);
		gridCells->InsertNextCell(GridQuad);

		GridQuad->GetPointIds()->SetId(0, k+0);
		GridQuad->GetPointIds()->SetId(1, k+4);
		GridQuad->GetPointIds()->SetId(2, k+7);
		GridQuad->GetPointIds()->SetId(3, k+3);
		gridCells->InsertNextCell(GridQuad);

		GridQuad->GetPointIds()->SetId(0, k+2);
		GridQuad->GetPointIds()->SetId(1, k+3);
		GridQuad->GetPointIds()->SetId(2, k+7);
		GridQuad->GetPointIds()->SetId(3, k+6);
		gridCells->InsertNextCell(GridQuad);

		GridQuad->GetPointIds()->SetId(0, k+1);
		GridQuad->GetPointIds()->SetId(1, k+5);
		GridQuad->GetPointIds()->SetId(2, k+6);
		GridQuad->GetPointIds()->SetId(3, k+2);
		gridCells->InsertNextCell(GridQuad);
	}
	grid->SetPoints(points);
	grid->SetPolys(gridCells);
	cutAABMapper->SetInputData(grid);
	gridCells->Delete();
	GridQuad->Delete();
	points->Delete();
	grid->Delete();
	ren->GetRenderWindow()->Render();
	return 1;
}

void iADreamCaster::ColorBadAngles()
{
	if(!modelOpened) return;

	//clear prev scalars
	vtkDataArray *da = vtkFloatArray::New();//mapper->GetInput()->GetCellData()->GetScalars();
	da->SetNumberOfComponents(1);
	da->SetNumberOfTuples(0);
	double val=0.0;
	unsigned int numTris = mapper->GetInput()->GetNumberOfPolys();
	double * scalars = new double[numTris];
	for (unsigned int i=0; i<numTris; i++)
	{
		da->InsertNextTuple1(val);
		scalars[i]=0;
	}
	//
	const unsigned int numTriangles = tracer->scene()->getNrTriangles();//curRender->intersections.size();
	iAVec3f o; // rays' origin point
	iAVec3f vp_corners[2];// plane's corners in 3d
	iAVec3f vp_delta[2];// plane's x and y axes' directions in 3D
	tracer->InitRender(vp_corners, vp_delta, &o);
	iAVec3f rotAxis(0.f, 1.f, 0.f);
	tracer->Transform(&rotAxis);
	rotAxis.normalize();

	float abscos;
	//unsigned int tri_ind;
	float bad_area=0; float good_area=0;

	iATriPrim* tri;
	iAVec3f triNorm;
	float d;
	float torusRadius = fabs(0.5f*stngs.ORIGIN_Z);
	for (unsigned int i = 0; i < numTriangles; i++ )
	{
		tri = tracer->scene()->getTriangle((int)i);
		triNorm = tri->normal();
		d = tri->d();
		float triSurf = tri->surface();
		abscos = fabs( triNorm & rotAxis);
		float a = d*abscos;
		float b = sqrt(d*d-a*a);
		b = b - torusRadius;
		float dist2TorusCenter = sqrt(a*a+b*b);
		if(dist2TorusCenter>torusRadius)
		{
			scalars[i]=99;
			bad_area+=triSurf;
		}
		else
		{
			scalars[i]=0;
			good_area+=triSurf;
		}
	}
	/*for (unsigned int i = 0; i < numTriangles; i++ )
	{
		const iAtriangle* tri = tracer->scene()->getTriangle((int)i)->getTri();
		cur_area = 0.5f*((*tri->vertices[1]-*tri->vertices[0])^(*tri->vertices[2]-*tri->vertices[0])).length();
		iAVec3f tri_center = (*tri->vertices[0]+*tri->vertices[1]+*tri->vertices[2])/3.0f;
		iAVec3f o2tri_center = tri_center-o; o2tri_center.normalize();
		abscos = abs( tri->N & o2tri_center);
		if(abscos<badCos)
		{
			scalars[i]=1;
			bad_area+=cur_area;
		}
		else
		{
			scalars[i]=100;
			good_area+=cur_area;
		}
	}*/
	for (unsigned int i = 0; i < numTris; i++ )
	{
		da->SetTuple1(i, scalars[i]); 
	}

	delete [] scalars;
	mapper->GetInput()->GetCellData()->SetScalars(da);
	da->Delete();
	mapper->ScalarVisibilityOn();
	ui.l_badAreaPercent->setText(QString::number(100.f*bad_area/(good_area+bad_area))+"%");
	UpdateSlot();
}

void iADreamCaster::HideColoring()
{
	HideDipAnglesSlot();
}

double iADreamCaster::RandonSpaceAnalysis()
{
	const int numTriangles = (int)trisInsideAoI.size();
	float abscos;
	//unsigned int tri_ind;
	iAVec3f rotAxis(0.f, 1.f, 0.f);
	tracer->Transform(&rotAxis);
	rotAxis.normalize();
	iATriPrim* tri;
	iAVec3f triNorm;
	float d;
	float torusRadius = fabs(0.5f*stngs.ORIGIN_Z);
	float bad_area = 0.f; 
	float good_area = 0.f;
	int i;

	#pragma omp parallel for private(i, abscos, triNorm, d) reduction(+: bad_area, good_area)
	for (i = 0; i < numTriangles; i++ )
	{
		tri = tracer->scene()->getTriangle(trisInsideAoI[i]);
		triNorm = tri->normal();
		d = tri->d();
		float triSurfArea = tri->surface();
		abscos = fabs( triNorm & rotAxis);
		float a = d*abscos;
		float b = sqrt(d*d-a*a);
		b = b - torusRadius;
		float dist2TorusCenter = sqrt(a*a+b*b);
		if(dist2TorusCenter>torusRadius)
			bad_area+=triSurfArea;
		else
			good_area+=triSurfArea;
	}
	return bad_area/(good_area+bad_area);
}

int iADreamCaster::findSelectedTriangles()
{
	trisInsideAoI.clear();
	iATriPrim* tri;
	const unsigned int numTriangles = tracer->scene()->getNrTriangles();
	int featuresCnt = cutFigList->count();
	iAaabb * box;
	if(featuresCnt==0)//no selected features
	{
		for (unsigned int i = 0; i < numTriangles; i++ )
					trisInsideAoI.push_back((int)i);
	}
	else
	{
		for (unsigned int i = 0; i < numTriangles; i++ )
		{
			tri = tracer->scene()->getTriangle((int)i);
			for (int j=0; j<featuresCnt; j++)
			{
				box = cutFigList->aabbs[j];
				//if tri inside box add it to vector
				//if(tri->getMinX() >= box->x1 && tri->getMinY() >= box->y1 && tri->getMinZ() >= box->z1 &&
				//	tri->getMaxX() <= box->x2 && tri->getMaxY() <= box->y2 && tri->getMaxZ() >= box->z2)
				if(box->isInside(*(tri->getVertex(0))) && box->isInside(*(tri->getVertex(1))) && box->isInside(*(tri->getVertex(2))))
				{
					trisInsideAoI.push_back((int)i);
					break;
				}
			}
		}
	}
	return 1;
}

void iADreamCaster::StopRenderingSlot()
{
	isStopped = true;
}

void iADreamCaster::SetupGPUBuffers()
{
	/*unsigned int tri_count = (unsigned int) tracer->scene()->getNrTriangles();
	unsigned int nodes_count = tracer->scene()->getBSPTree()->nodes.size();
	const unsigned int id_count = tracer->scene()->getBSPTree()->tri_ind.size();
	// Bind all textures 
	try
	{
		tracer->setup_nodes(&nodes[0], nodes_count);
		tracer->setup_tris(&wald[0], tri_count);
		tracer->setup_ids(&tracer->scene()->getBSPTree()->tri_ind[0], id_count);
	}
	catch( itk::ExceptionObject &excep)
	{
		log(tr("OpenCL texture setup terminated unexpectedly."));
		log(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}*/
}

void iADreamCaster::Pick(int pickPos[2] )
{
	plot3d->Pick(pickPos[0], pickPos[1]);
	if(plot3d->lastPickSuccessful)
	{
		curIndX = plot3d->pickData.xInd;
		curIndZ = plot3d->pickData.zInd;
		int inds[2] = {(int)curIndX, (int)curIndZ};
		ViewsFrame->SetHiglightedIndices(&inds[0], &inds[1], 1);
		ViewsFrame->update();
		UpdateInfoLabels();
		if(ui.cb_UpdateViews->isChecked() && modelOpened && datasetOpened)
			UpdateView();
		UpdateStabilityWidget();
	}
}

bool iADreamCaster::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == qvtkPlot3d)
	{
		if (event->type() == QEvent::MouseButtonDblClick) 
		{
			QMouseEvent * mevent = static_cast<QMouseEvent*>(event);
			int pcoords[2] = {mevent->x(), qvtkPlot3d->height() - mevent->y()};
			Pick(pcoords);
		} 
	}
	if (event->type() == QEvent::Resize) 
	{
		float minDim, offset[2];

		if( (obj == ui.RenderViewWidget) 
			|| (obj == ui.HeightWidget) 
			|| (obj == ui.RenderViewWidget)
			|| (obj == ui.w_results)
			|| (obj == ui.w_comparison1)
			|| (obj == ui.w_comparison2)
			|| (obj == ui.w_comparison3)
		  )
		{
			QWidget * wobj = (QWidget*)obj;
			if(wobj->geometry().width() < wobj->geometry().height())
			{
				minDim = wobj->geometry().width();
				offset[0] = 0.0f;
				offset[1] = 0.5 * (wobj->geometry().height() - minDim);
			}
			else
			{
				minDim = wobj->geometry().height();
				offset[0] = 0.5 * (wobj->geometry().width() - minDim);
				offset[1] = 0.0f;
			}
			((QWidget*)wobj->children()[0])->setGeometry((int)offset[0], (int)offset[1], (int)minDim, (int)minDim);
			//RenderFrame->setGeometry(offset[0], offset[1], minDim, minDim);
		}
		if(obj == ui.w_stabilityWidget || obj == ui.histWidget)
		{
			QWidget * wobj = (QWidget*)obj;
			((QWidget*)wobj->children()[0])->setGeometry(0, 0, wobj->geometry().width(), wobj->geometry().height());
		}
	}
	return QMainWindow::eventFilter(obj, event);
}

void iADreamCaster::maximize3DView()
{
	changeVisibility(isOneWidgetMaximized);
	if (!isOneWidgetMaximized)
	{
		ui.topFrame->show();
		ui.leftFrame->show();
		ui.frame3DV->show();
	}
	isOneWidgetMaximized = !isOneWidgetMaximized;
}

void iADreamCaster::maximizeStability()
{
	changeVisibility(isOneWidgetMaximized);
	if (!isOneWidgetMaximized)
	{
		ui.topFrame->show();
		ui.rightFrame->show();
		ui.frameStability->show();
	}
	isOneWidgetMaximized = !isOneWidgetMaximized;
}

void iADreamCaster::maximizeRC()
{
	changeVisibility(isOneWidgetMaximized);
	if (!isOneWidgetMaximized)
	{
		ui.topFrame->show();
		ui.leftFrame->show();
		ui.frameRC->show();
	}
	isOneWidgetMaximized = !isOneWidgetMaximized;
}

void iADreamCaster::maximizePlacements()
{
	changeVisibility(isOneWidgetMaximized);
	if (!isOneWidgetMaximized)
	{
		ui.topFrame->show();
		ui.rightFrame->show();
		ui.framePlacements->show();
	}
	isOneWidgetMaximized = !isOneWidgetMaximized;
}

void iADreamCaster::maximizeBottom()
{
	changeVisibility(isOneWidgetMaximized);
	if (!isOneWidgetMaximized)
	{
		ui.bottomFrame->show();
	}
	isOneWidgetMaximized = !isOneWidgetMaximized;
}

void iADreamCaster::changeVisibility( int isVisible )
{
	if(isVisible)
	{
		ui.frameStability->show();
		ui.framePlacements->show();
		ui.frameRC->show();
		ui.bottomFrame->show();
		ui.frame3DV->show();
		ui.leftFrame->show();
		ui.rightFrame->show();
		ui.topFrame->show();
	}
	else
	{
		ui.frameStability->hide();
		ui.framePlacements->hide();
		ui.frameRC->hide();
		ui.bottomFrame->hide();
		ui.frame3DV->hide();
		ui.leftFrame->hide();
		ui.rightFrame->hide();
		ui.topFrame->hide();
	}
}

void iADreamCaster::setPickedPlacement( int indX, int indY, int indZ )
{
	curIndX = indX;
	curIndY = indY;
	curIndZ = indZ;
	int indicesxz[2] = {(int)curIndX, (int)curIndZ};
	ViewsFrame->SetHiglightedIndices(&indicesxz[0], &indicesxz[1], 1);
	ViewsFrame->update();
	plot3d->setPicked(curIndX, curIndZ);
	UpdateInfoLabels();
	if(ui.cb_UpdateViews->isChecked() && modelOpened && datasetOpened)
		UpdateView();
	UpdateStabilityWidget();
	//Comparison tab
	for (int i=0; i<3; i++)
	{
		comparisonTab->paramWidgets[i].paintWidget->SetHiglightedIndices(&indicesxz[0], &indicesxz[1], 1);
		comparisonTab->paramWidgets[i].paintWidget->update();
	}
	//TODO: indices!!!!
	ui.lb_paramRot1->setText(QString::number( (placementsParams[curIndX][curIndZ])[indices[0]] ));
	ui.lb_paramRot2->setText(QString::number( (placementsParams[curIndX][curIndZ])[indices[1]] ));
	ui.lb_paramRot3->setText(QString::number( (placementsParams[curIndX][curIndZ])[indices[2]] ));
	//Weighting tab
	for (int i=0; i<3; i++)
	{
		weightingTab->results.paintWidget->SetHiglightedIndices(&indicesxz[0], &indicesxz[1], 1);
		weightingTab->results.paintWidget->update();
	}	
	ui.lb_avParamRot_3->setText( QString::number( weightedParams[curIndX][curIndZ] ) );
	if(ui.cb_UpdateViews->isChecked() && modelOpened && datasetOpened)
	{
		UpdateView();
	}
}

void iADreamCaster::setRangeSB( float minX, float maxX, float minZ, float maxZ )
{
	ui.sb_min_x->setValue(minX*DEG_IN_PI);
	ui.sb_max_x->setValue(maxX*DEG_IN_PI);

	ui.sb_min_z->setValue(minZ*DEG_IN_PI);
	ui.sb_max_z->setValue(maxZ*DEG_IN_PI);
}

void iADreamCaster::loadFile(const QString filename)
{
	modelFileName = filename;
	log("Opening new model:");
	log(modelFileName, true);
	initRaycast();
	log("Opened model size (triangles):");
	log(QString::number(mdata.stlMesh.size()), true);
	///ui.l_modelName->setText(modelFileName);
	modelOpened = true;
	for (int i=0; i<cutFigList->count(); i++)
	{
		iACutAAB *cutAAB = cutFigList->item(i);
		cutFigList->SetCurIndex(i);
		ui.listCutFig->setCurrentRow(i);
		CutFigParametersChangedOFF = true;
		ui.s_aab_minx->setValue(cutAAB->slidersValues[0]);
		ui.s_aab_maxx->setValue(cutAAB->slidersValues[1]);
		ui.s_aab_miny->setValue(cutAAB->slidersValues[2]);
		ui.s_aab_maxy->setValue(cutAAB->slidersValues[3]);
		ui.s_aab_minz->setValue(cutAAB->slidersValues[4]);
		ui.s_aab_maxz->setValue(cutAAB->slidersValues[5]);
		CutFigParametersChangedOFF = false;
		CutFigParametersChanged();
	}
	UpdateSlot();
	if (setFileName.isEmpty())
	{
		setFileName = filename + ".set";
		if (QFile(setFileName).exists())
			OpenSetFile(setFileName);
		else
			ui.l_setName->setText(setFileName);
	}
}

void iADreamCaster::InitRender( iAVec3f * vp_corners, iAVec3f * vp_delta, iAVec3f * o )
{
	scrBuffer->clear();
	tracer->InitRender(vp_corners, vp_delta, o);
}

void iADreamCaster::PositionSpecimen()
{
	float freeSpacePart = 0.1;
	float maxSize = mdata.box.half_size().length(); 
	float o2plane_dist = fabs(stngs.ORIGIN_Z) + fabs(stngs.PLANE_Z);	
	stngs.ORIGIN_Z = o2plane_dist * ( maxSize / ( stngs.PLANE_H_W * (1 - freeSpacePart) ) );
	stngs.PLANE_Z = o2plane_dist - stngs.ORIGIN_Z;
	stngs.ORIGIN_Z = -stngs.ORIGIN_Z;
	ui.sourceZ->setText(QString::number(stngs.ORIGIN_Z));
	ui.detectorZ->setText(QString::number(stngs.PLANE_Z));
}

void iADreamCaster::Render(const iAVec3f * vp_corners, const iAVec3f * vp_delta, const iAVec3f * o, bool rememberData)
{
	try
	{
		tracer->Render(vp_corners, vp_delta, o, rememberData, ui.cb_dipAsColor->isChecked(), ui.cudaEnabled->isChecked(), ui.cb_rasterization->isChecked());
	}
	catch( itk::ExceptionObject &excep)
	{
		log(tr("OpenCL single frame render terminated unexpectedly."));
		log(tr("  %1 in File %2, Line %3").arg(excep.GetDescription())
			.arg(excep.GetFile())
			.arg(excep.GetLine()));
		return;
	}
}
