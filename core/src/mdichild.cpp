/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#include "pch.h"
#include "mdichild.h"

#include "dlg_commoninput.h"
#include "dlg_histogram.h"
#include "dlg_imageproperty.h"
#include "dlg_modalities.h"
#include "dlg_modalityRenderer.h"
#include "iAChannelVisualizationData.h"
#include "iAChildData.h"
#include "iAConsole.h"
#include "dlg_profile.h"
#include "dlg_volumePlayer.h"
#include "iAAlgorithms.h"
#include "iAFilter.h"
#include "iAIO.h"
#include "iAIOProvider.h"
#include "iALogger.h"
#include "iAMdiChildLogger.h"
#include "iAModality.h"
#include "iAObserverProgress.h"
#include "iAParametricSpline.h"
#include "iAProfileProbe.h"
#include "iAProfileWidget.h"
#include "iARenderer.h"
#include "iARenderSettings.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iASlicerWidget.h"
#include "iATransferFunction.h"
#include "iAVolumeStack.h"
#include "iAWidgetAddHelper.h"
#include "mainwindow.h"

#include "extension2id.h"

#include <vtkCamera.h>
#include <vtkCornerAnnotation.h>
#include <vtkImageAccumulate.h>
#include <vtkImageReslice.h>
#include <vtkMath.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPlane.h>
#include <vtkRenderWindow.h>
#include <vtkVolumeProperty.h>
#include <vtkWindowToImageFilter.h>
// TODO: refactor methods using the following out of mdichild!
#include <vtkBMPWriter.h>
#include <vtkPNGWriter.h>
#include <vtkJPEGWriter.h>
#include <vtkTIFFWriter.h>
#include <vtkTransform.h>
#include <vtkVolumeProperty.h>

#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressBar>
#include <QSpinBox>
#include <QToolButton>

//#include <fstream>
//#include <sstream>
//#include <string>

MdiChild::MdiChild(MainWindow * mainWnd) : m_isSmthMaximized(false), volumeStack(new iAVolumeStack),
	isMagicLensEnabled(false),
	ioThread(0),
	reInitializeRenderWindows(true),
	m_logger(new MdiChildLogger(this))
{
	m_mainWnd = mainWnd;
	setupUi(this);
	//prepare window for handling dock widgets
	this->setCentralWidget(0);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

	//insert default dock widgets and arrange them in a simple layout
	r = new dlg_renderer(this);
	sXY = new dlg_sliceXY(this);
	sXZ = new dlg_sliceXZ(this);
	sYZ = new dlg_sliceYZ(this);

	dlg_modalityRenderer * renderWidget = new dlg_modalityRenderer();
	m_dlgModalities = new dlg_modalities(renderWidget->GetRenderer());
	QSharedPointer<iAModalityList> modList(new iAModalityList);
	SetModalities(modList);

	pbar = new QProgressBar(this);
	pbar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	pbar->setMaximumSize(350, 17);
	this->statusBar()->addPermanentWidget(pbar);
	m_pbarMaxVal = pbar->maximum();
	logs = new dlg_logs(this);
	addDockWidget(Qt::LeftDockWidgetArea, r);
	m_initialLayoutState = saveState();

	splitDockWidget(r, logs, Qt::Vertical);
	splitDockWidget(r, sXZ, Qt::Horizontal);
	splitDockWidget(r, sYZ, Qt::Vertical);
	splitDockWidget(sXZ, sXY, Qt::Vertical);

	splitDockWidget(logs, m_dlgModalities, Qt::Horizontal);
	splitDockWidget(r, renderWidget, Qt::Horizontal);

	setAttribute(Qt::WA_DeleteOnClose);

	isUntitled = true;
	visibility = MULTI;
	tabsVisible = true;
	linkviews = false;
	interactorsEnabled = true;
	xCoord = 0, yCoord = 0, zCoord = 0;
	connectionState = cs_NONE;
	for (int i = 0; i < 6; i++) roi[i] = 0;

	imageData = vtkSmartPointer<vtkImageData>::New();
	imageData->AllocateScalars(VTK_DOUBLE, 1);
	polyData = vtkPolyData::New();

	imageAccumulate = vtkImageAccumulate::New();
	colorTransferFunction = vtkColorTransferFunction::New();
	piecewiseFunction = vtkPiecewiseFunction::New();
	axesTransform = vtkTransform::New();
	slicerTransform = vtkTransform::New();

	slicerXY = new iASlicer(this, iASlicerMode::XY, sXY->sliceWidget);
	slicerXZ = new iASlicer(this, iASlicerMode::XZ, sXZ->sliceWidget);
	slicerYZ = new iASlicer(this, iASlicerMode::YZ, sYZ->sliceWidget);

	Raycaster = new iARenderer(this);
	connect(r->vtkWidgetRC, SIGNAL(rightButtonReleasedSignal()), Raycaster, SLOT(mouseRightButtonReleasedSlot()) );
	connect(r->vtkWidgetRC, SIGNAL(leftButtonReleasedSignal()), Raycaster, SLOT(mouseLeftButtonReleasedSlot()) );
	Raycaster->setAxesTransform(axesTransform);

	imgHistogram = 0;
	imgProperty = 0;
	imgProfile = 0;
	SetRenderWindows();
	connectSignalsToSlots();
	pbar->setValue(100);

	saveNative = true;

	snakeSlicer = false;
	isSliceProfileEnabled = false;
	isArbProfileEnabled = false;

	profileWidgetIndex = -1;
	worldSnakePoints = vtkPoints::New();
	parametricSpline = iAParametricSpline::New();
	parametricSpline->SetPoints(worldSnakePoints);

	sXZ->spinBoxXZ->setRange(-8192,8192);
	sXY->spinBoxXY->setRange(-8192,8192);
	sYZ->spinBoxYZ->setRange(-8192,8192);

	worldProfilePoints = vtkPoints::New();
	worldProfilePoints->Allocate(2);

	hessianComputed = false;

	// TODO: move to module?
	mdCsvTable = vtkSmartPointer<vtkTable>::New();

	updateSliceIndicator = true;

	raycasterInitialized = false;
}

MdiChild::~MdiChild()
{
	cleanWorkingAlgorithms();
	polyData->ReleaseData();
	imageAccumulate->ReleaseDataFlagOn();
	colorTransferFunction->RemoveAllPoints();
	piecewiseFunction->RemoveAllPoints();
	axesTransform->Delete();
	slicerTransform->Delete();

	polyData->Delete();
	imageAccumulate->Delete();
	colorTransferFunction->Delete();
	piecewiseFunction->Delete();

	delete slicerXZ;
	delete slicerXY;
	delete slicerYZ;
	delete Raycaster; Raycaster = 0;

	if(imgProperty)		delete imgProperty;
	if(getHistogram())	delete getHistogram();
	if(imgProfile)		delete imgProfile;
}

void MdiChild::connectSignalsToSlots()
{
	connect(r->pushMaxRC, SIGNAL(clicked()), this, SLOT(maximizeRC()));
	connect(sXY->pushMaxXY, SIGNAL(clicked()), this, SLOT(maximizeXY()));
	connect(sXZ->pushMaxXZ, SIGNAL(clicked()), this, SLOT(maximizeXZ()));
	connect(sYZ->pushMaxYZ, SIGNAL(clicked()), this, SLOT(maximizeYZ()));

	connect(sXY->pushStopXY, SIGNAL(clicked()), this, SLOT(triggerInteractionXY()));
	connect(sXZ->pushStopXZ, SIGNAL(clicked()), this, SLOT(triggerInteractionXZ()));
	connect(sYZ->pushStopYZ, SIGNAL(clicked()), this, SLOT(triggerInteractionYZ()));
	connect(r->pushStopRC, SIGNAL(clicked()), this, SLOT(triggerInteractionRaycaster()));

	connect(r->pushPX, SIGNAL(clicked()), this, SLOT(camPX()));
	connect(r->pushPY, SIGNAL(clicked()), this, SLOT(camPY()));
	connect(r->pushPZ, SIGNAL(clicked()), this, SLOT(camPZ()));
	connect(r->pushMX, SIGNAL(clicked()), this, SLOT(camMX()));
	connect(r->pushMY, SIGNAL(clicked()), this, SLOT(camMY()));
	connect(r->pushMZ, SIGNAL(clicked()), this, SLOT(camMZ()));
	connect(r->pushIso, SIGNAL(clicked()), this, SLOT(camIso()));
	connect(r->pushSaveRC, SIGNAL(clicked()), this, SLOT(saveRC()));
	connect(sXY->pushSaveXY, SIGNAL(clicked()), this, SLOT(saveXY()));
	connect(sXZ->pushSaveXZ, SIGNAL(clicked()), this, SLOT(saveXZ()));
	connect(sYZ->pushSaveYZ, SIGNAL(clicked()), this, SLOT(saveYZ()));

	connect(sXY->pushSaveStackXY, SIGNAL(clicked()), this, SLOT(saveStackXY()));
	connect(sXZ->pushSaveStackXZ, SIGNAL(clicked()), this, SLOT(saveStackXZ()));
	connect(sYZ->pushSaveStackYZ, SIGNAL(clicked()), this, SLOT(saveStackYZ()));

	connect(sXY->pushMovXY, SIGNAL(clicked()), this, SLOT(saveMovXY()));
	connect(sXZ->pushMovXZ, SIGNAL(clicked()), this, SLOT(saveMovXZ()));
	connect(sYZ->pushMovYZ, SIGNAL(clicked()), this, SLOT(saveMovYZ()));
	connect(r->pushMovRC, SIGNAL(clicked()), this, SLOT(saveMovRC()));

	connect(r->spinBoxRC, SIGNAL(valueChanged(int)), this, SLOT(setChannel(int)));

	connect(sXY->spinBoxXY, SIGNAL(valueChanged(int)), this, SLOT(setSliceXYSpinBox(int)));
	connect(sXZ->spinBoxXZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceXZSpinBox(int)));
	connect(sYZ->spinBoxYZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceYZSpinBox(int)));

	connect(sXY->verticalScrollBarXY, SIGNAL(valueChanged(int)), this, SLOT(setSliceXYScrollBar(int)));
	connect(sXZ->verticalScrollBarXZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceXZScrollBar(int)));
	connect(sYZ->verticalScrollBarYZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceYZScrollBar(int)));

	connect(sXY->doubleSpinBoxXY, SIGNAL(valueChanged(double)), this, SLOT(setRotationXY(double)));
	connect(sXZ->doubleSpinBoxXZ, SIGNAL(valueChanged(double)), this, SLOT(setRotationXZ(double)));
	connect(sYZ->doubleSpinBoxYZ, SIGNAL(valueChanged(double)), this, SLOT(setRotationYZ(double)));

	connect(getSlicerXY()->widget(), SIGNAL(shiftMouseWheel(int)), this, SLOT(ChangeModality(int)));
	connect(getSlicerXZ()->widget(), SIGNAL(shiftMouseWheel(int)), this, SLOT(ChangeModality(int)));
	connect(getSlicerYZ()->widget(), SIGNAL(shiftMouseWheel(int)), this, SLOT(ChangeModality(int)));
	connect(getSlicerXY()->widget(), SIGNAL(altMouseWheel(int)), this, SLOT(ChangeMagicLensOpacity(int)));
	connect(getSlicerXZ()->widget(), SIGNAL(altMouseWheel(int)), this, SLOT(ChangeMagicLensOpacity(int)));
	connect(getSlicerYZ()->widget(), SIGNAL(altMouseWheel(int)), this, SLOT(ChangeMagicLensOpacity(int)));

	/*
	connect(this, SIGNAL(renderSettingsChanged()), this, SLOT(RenderSettingsChanged()));
	connect(this, SIGNAL(preferencesChanged()), this, SLOT(preferencesChanged()));
	*/

	connect(m_dlgModalities, SIGNAL(ShowImage(vtkSmartPointer<vtkImageData>)), this, SLOT(ChangeImage(vtkSmartPointer<vtkImageData>)));
}

void MdiChild::connectThreadSignalsToChildSlots( iAAlgorithms* thread, bool providesProgress, bool usesDoneSignal )
{
	connect(thread, SIGNAL( startUpdate(int) ), this, SLOT( updateRenderWindows(int) ));
	if (usesDoneSignal)
	{
		connect(thread, SIGNAL( done() ), this, SLOT( enableRenderWindows() ));
	}
	else
	{
		connect(thread, SIGNAL( finished() ), this, SLOT( enableRenderWindows() ));
	}
	connect(thread, SIGNAL( aprogress(int) ), this, SLOT( updateProgressBar(int) ));

	workingAlgorithms.push_back(thread);
	connect(thread, SIGNAL( finished() ), this, SLOT( removeFinishedAlgorithms() ));
	if(!providesProgress)
		pbar->setMaximum(0);
	connect(thread, SIGNAL( started() ), this, SLOT( initProgressBar() ));
	connect(thread, SIGNAL( finished() ), this, SLOT( hideProgressBar() ));
}

void MdiChild::SetRenderWindows()
{
	r->vtkWidgetRC->SetRenderWindow(Raycaster->GetRenderWindow());
}

void MdiChild::updateRenderWindows(int channels)
{
	if (channels > 1)
	{
		r->spinBoxRC->setRange(0, channels-1);
		r->stackedWidgetRC->setCurrentIndex(1);
		r->channelLabelRC->setEnabled(true);
	}
	else
	{
		r->stackedWidgetRC->setCurrentIndex(0);
		r->channelLabelRC->setEnabled(false);
	}
	disableRenderWindows(0);
}

void MdiChild::disableRenderWindows(int ch)
{
	slicerXZ->disableInteractor();
	slicerXY->disableInteractor();
	slicerYZ->disableInteractor();
	Raycaster->disableInteractor();
	emit rendererDeactivated(ch);
}

void MdiChild::enableRenderWindows()
{
	// TODO: better logic (directly couple to successful I/O finish?
	if (!currentFile().isEmpty() && GetModalitiesDlg()->GetModalities()->size() == 0)
	{
		GetModalitiesDlg()->GetModalities()->Add(QSharedPointer<iAModality>(
			new iAModality("Main", currentFile(), getImagePointer(), iAModality::MainRenderer)));
	}
	if (!IsOnlyPolyDataLoaded() && reInitializeRenderWindows)
	{
		calculateHistogram();
		getHistogram()->initialize(imageAccumulate, imageData->GetScalarRange(), true);
		getHistogram()->updateTrf();
		getHistogram()->redraw();

		Raycaster->enableInteractor();

		slicerXZ->enableInteractor();
		slicerXZ->reInitialize(imageData, slicerTransform, colorTransferFunction);

		slicerXY->enableInteractor();
		slicerXY->reInitialize(imageData, slicerTransform, colorTransferFunction);

		slicerYZ->enableInteractor();
		slicerYZ->reInitialize(imageData, slicerTransform, colorTransferFunction);
	}
	// set to true for next time, in case it is false now (i.e. default to always reinitialize,
	// unless explicitly set otherwise)
	reInitializeRenderWindows = true;

	Raycaster->reInitialize(imageData, polyData, piecewiseFunction, colorTransferFunction);

	if (!IsOnlyPolyDataLoaded())
	{
		if(this->updateSliceIndicator){
			updateSliceIndicators();
			camIso();
		}
		else{
			this->updateSliceIndicator = true;
		}

		bool anyChannelEnabled = false;
		QList<iAChannelID> keys = m_channels.keys();
		for (QList<iAChannelID>::iterator it = keys.begin();
			it != keys.end();
			++it)
		{
			iAChannelID key = *it;
			iAChannelVisualizationData * chData = m_channels.value(key).data();
			if (chData->IsEnabled()
				|| (isMagicLensEnabled && (
					key == slicerXY->getMagicLensInput() ||
					key == slicerXZ->getMagicLensInput() ||
					key == slicerYZ->getMagicLensInput()
					))
				)
			{
				anyChannelEnabled = true;
				slicerXZ->reInitializeChannel(key, chData);
				slicerXY->reInitializeChannel(key, chData);
				slicerYZ->reInitializeChannel(key, chData);
				// if 3d enabled
				if (chData->Uses3D())
				{
					Raycaster->updateChannelImages();
				}
			}
		}
		if (!anyChannelEnabled)
		{
			imgProperty->updateProperties(imageData, imageAccumulate, true);
		}
	}
}

void MdiChild::updateProgressBar(int i)
{
	pbar->show();
	pbar->setValue(i);
}

void MdiChild::updateRenderers(int x, int y, int z, int mode)
{
		double spacing[3];
		imageData->GetSpacing(spacing);

//TODO: improve using iASlicer stuff
	if (linkviews) {
		xCoord = x; yCoord = y; zCoord = z;
		if (mode != 2) {
			if (showPosition)
				slicerXZ->setPlaneCenter(x*spacing[0], z*spacing[2], 1);
			slicerXZ->setIndex(x,y,z);
			sXZ->spinBoxXZ->setValue(y);
		}
		if (mode != 0) {
			if (showPosition)
				slicerYZ->setPlaneCenter(y*spacing[1], z*spacing[2], 1);
			slicerYZ->setIndex(x,y,z);
			sYZ->spinBoxYZ->setValue(x);
		}
		if (mode != 1) {
			if (showPosition)
				slicerXY->setPlaneCenter(x*spacing[0], y*spacing[1], 1);
			slicerXY->setIndex(x,y,z);
			sXY->spinBoxXY->setValue(z);
		}
		if (showRPosition) Raycaster->setCubeCenter(x, y, z);
	}
}

void MdiChild::newFile()
{
	widgetsVisible(false);
	addMsg(tr("%1  New File.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
}

void MdiChild::showPoly()
{
	Raycaster->GetVolume()->SetVisibility(false);
	widgetsVisible(false);
	visibilityBlock(QList<QSpacerItem*>(),
		QList<QWidget*>() << r->stackedWidgetRC << r->pushSaveRC << r->pushMaxRC << r->pushStopRC, true);

	r->vtkWidgetRC->setGeometry(0, 0, 300, 200);
	r->vtkWidgetRC->setMaximumSize(QSize(16777215, 16777215));
	r->vtkWidgetRC->adjustSize();
	r->show();
	visibility &= (RC | TAB);
	changeVisibility(visibility);
}

bool MdiChild::displayResult(QString const & title, vtkImageData* image, vtkPolyData* poly)
{
	addStatusMsg("Creating Result View");
	if (poly != NULL){
		polyData->ReleaseData();
		polyData->DeepCopy(poly);
	}

	if (image != NULL){
		imageData->ReleaseData();
		imageAccumulate->SetReleaseDataFlag(true);
		imageData->DeepCopy(image);
		if ( !calculateHistogram() ) return false;
	}

	initView( );
	setWindowTitle( title );

	setupRaycaster( showVolume, showSlicers, showHelpers, showRPosition,
		linearInterpolation, shading, boundingBox, parallelProjection,
		imageData->GetSpacing()[0], imageData->GetSpacing()[0], ambientLighting, diffuseLighting,
		specularLighting, specularPower, backgroundTop, backgroundBottom,
		renderMode, false);
	setupSlicers( linkviews, showIsolines, showPosition, numberOfIsolines, minIsovalue, maxIsovalue, imageActorUseInterpolation, snakeSlices, true );

	if (imageData->GetExtent()[1] <= 1)
		visibility &= (YZ | TAB);
	else if (imageData->GetExtent()[3] <= 1)
		visibility &= (XZ | TAB);
	else if (imageData->GetExtent()[5] <= 1)
		visibility &= (XY | TAB);

	changeVisibility(visibility);

	addStatusMsg("Ready");
	return true;
}


bool MdiChild::setupLoadIO(QString const & f, bool isStack)
{
	bool setUp = false;
	polyData->ReleaseData();
	// TODO: insert plugin mechanism.
	// - iterate over file plugins; if one returns a match, use it
	if (QString::compare(fileInfo.suffix(), "STL", Qt::CaseInsensitive) == 0)
	{
		if (!ioThread->setupIO(STL_READER, f)) return false;
		setUp = true;
	}
	if (!setUp)
	{
		imageData->ReleaseData();
		imageAccumulate->ReleaseDataFlagOn();

		QString extension = fileInfo.suffix();
		extension = extension.toUpper();
		const mapQString2int * ext2id = &extensionToId;
		if(isStack)	ext2id = &extensionToIdStack;
		if (ext2id->find(extension) == ext2id->end())
		{
			return false;
		}
		IOType id = ext2id->find( extension ).value();
		if ( !ioThread->setupIO(id, f) ) return false;
	}
	return true;
}


bool MdiChild::loadFile(const QString &f, bool isStack)
{
	if(!QFile::exists(f))	return false;

	addMsg(tr("%1  Loading sequence started... \n  The duration of the loading sequence depends on the size of your data set and may last several minutes. \n  Please wait...").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	setCurrentFile(f);

	waitForPreviousIO();

	ioThread = new iAIO(imageData, polyData, m_logger, this, isStack, volumeStack->GetVolumes(), volumeStack->GetFileNames());
	connectThreadSignalsToChildSlots(ioThread, false, true);
	connect( ioThread, SIGNAL( finished() ), this, SLOT( ioFinished() ) );
	
	polyData->ReleaseData();

	if (!setupLoadIO(f, isStack))
	{
		ioFinished();
		return false;
	}

	ioThread->start();


	return true;
}


void MdiChild::setImageData(QString const & filename, vtkSmartPointer<vtkImageData> imgData)
{
	setImageData(imgData);
	setCurrentFile(filename);
	setupView(false);
	enableRenderWindows();
}


bool MdiChild::updateVolumePlayerView(int updateIndex, bool isApplyForAll) {
	volumeStack->getColorTransferFunction(previousIndexOfVolume)->DeepCopy(colorTransferFunction);
	volumeStack->getPiecewiseFunction(previousIndexOfVolume)->DeepCopy(piecewiseFunction);
	previousIndexOfVolume = updateIndex;

	int numberOfVolumes=volumeStack->getNumberOfVolumes();
	imageData->DeepCopy(volumeStack->getVolume(updateIndex));

	if(isApplyForAll) {
		for (int i=0; i<numberOfVolumes;i++) {
			if (i != updateIndex) {
				volumeStack->getColorTransferFunction(i)->DeepCopy(colorTransferFunction);
				volumeStack->getPiecewiseFunction(i)->DeepCopy(piecewiseFunction);
			}
		}
	}

	colorTransferFunction->DeepCopy(volumeStack->getColorTransferFunction(updateIndex));
	piecewiseFunction->DeepCopy(volumeStack->getPiecewiseFunction(updateIndex));

	getHistogram()->updateTransferFunctions(colorTransferFunction, piecewiseFunction);

	calculateHistogram();
	getHistogram()->initialize(imageAccumulate, imageData->GetScalarRange(), false);
	getHistogram()->updateTrf();
	getHistogram()->redraw();
	getHistogram()->drawHistogram();

	Raycaster->reInitialize(imageData, polyData, piecewiseFunction, colorTransferFunction);
	slicerXZ->reInitialize(imageData, slicerTransform, colorTransferFunction);
	slicerXY->reInitialize(imageData, slicerTransform, colorTransferFunction);
	slicerYZ->reInitialize(imageData, slicerTransform, colorTransferFunction);
	updateViews();

	if (CheckedList.at(updateIndex)!=0) {
		enableRenderWindows();
	}

	return true;
}


bool MdiChild::setupStackView(bool active)
{
	previousIndexOfVolume = 0;

	addVolumePlayer(volumeStack.data());

	int numberOfVolumes=volumeStack->getNumberOfVolumes();

	int currentIndexOfVolume=0;

	imageData->DeepCopy(volumeStack->getVolume(currentIndexOfVolume));

	for (int i=0; i<numberOfVolumes; i++) {
		vtkColorTransferFunction* cTF = GetDefaultColorTransferFunction(imageData);
		vtkPiecewiseFunction* pWF = GetDefaultPiecewiseFunction(imageData);

		volumeStack->addColorTransferFunction(cTF);
		volumeStack->addPiecewiseFunction(pWF);

		cTF->Delete(); pWF->Delete();
	}

	if (numberOfVolumes > 0) {
		colorTransferFunction->DeepCopy(volumeStack->getColorTransferFunction(0));
		piecewiseFunction->DeepCopy(volumeStack->getPiecewiseFunction(0));
	}

	setupViewInternal(active);

	Raycaster->reInitialize(imageData, polyData, piecewiseFunction, colorTransferFunction);
	slicerXZ->reInitialize(imageData, slicerTransform, colorTransferFunction);
	slicerXY->reInitialize(imageData, slicerTransform, colorTransferFunction);
	slicerYZ->reInitialize(imageData, slicerTransform, colorTransferFunction);
	updateViews();

	Raycaster->update();

	return true;
}


void MdiChild::setupViewInternal(bool active)
{
	if (IsOnlyPolyDataLoaded())
		showVolume = false;
	else
		calculateHistogram();//AMA assertion error on rendering otherwise

	if (!active) initView();

	m_mainWnd->setCurrentFile(currentFile());

	if ((imageData->GetExtent()[1] < 3) || (imageData->GetExtent()[3]) < 3 || (imageData->GetExtent()[5] < 3))
		shading = false;

	setupRaycaster(showVolume, showSlicers, showHelpers, showRPosition,
		linearInterpolation, shading, boundingBox, parallelProjection,
		imageData->GetSpacing()[0], imageData->GetSpacing()[0], ambientLighting, diffuseLighting,
		specularLighting, specularPower, backgroundTop, backgroundBottom,
		renderMode, false);//AMA 06.05.2010 was resetting results of initView when STL is opened
	setupSlicers(linkviews, showIsolines, showPosition, numberOfIsolines, minIsovalue, maxIsovalue, imageActorUseInterpolation, snakeSlices, true);

	if (imageData->GetExtent()[1] <= 1)
		visibility &= (YZ | TAB);
	else if (imageData->GetExtent()[3] <= 1)
		visibility &= (XZ | TAB);
	else if (imageData->GetExtent()[5] <= 1)
		visibility &= (XY | TAB);

	if (active) changeVisibility(visibility);

	if (imageData->GetNumberOfScalarComponents() > 1)
	{
		r->spinBoxRC->setRange(0, imageData->GetNumberOfScalarComponents() - 1);
		r->stackedWidgetRC->setCurrentIndex(1);
		r->channelLabelRC->setEnabled(true);
	}
	else
	{
		r->stackedWidgetRC->setCurrentIndex(0);
		r->channelLabelRC->setEnabled(false);
	}
}


bool MdiChild::setupView(bool active )
{
	setupViewInternal(active);

	Raycaster->update();

	check2DMode();

	return true;
}


void MdiChild::updateSliceIndicators()
{
	int val;
	sXY->spinBoxXY->setRange(imageData->GetExtent()[4],imageData->GetExtent()[5]);
	sXY->verticalScrollBarXY->setRange(imageData->GetExtent()[4],imageData->GetExtent()[5]);
	val = (imageData->GetExtent()[5]-imageData->GetExtent()[4]) / 2 + imageData->GetExtent()[4];
	sXY->spinBoxXY->setValue(val);
	sXY->verticalScrollBarXY->setValue(val);

	sYZ->spinBoxYZ->setRange(imageData->GetExtent()[0],imageData->GetExtent()[1]);
	sYZ->verticalScrollBarYZ->setRange(imageData->GetExtent()[0],imageData->GetExtent()[1]);
	val = (imageData->GetExtent()[1]-imageData->GetExtent()[0] ) / 2 + imageData->GetExtent()[0];
	sYZ->spinBoxYZ->setValue(val);
	sYZ->verticalScrollBarYZ->setValue(val);

	sXZ->spinBoxXZ->setRange(imageData->GetExtent()[2],imageData->GetExtent()[3]);
	sXZ->verticalScrollBarXZ->setRange(imageData->GetExtent()[2],imageData->GetExtent()[3]);
	val = (imageData->GetExtent()[3]-imageData->GetExtent()[2] ) / 2 + imageData->GetExtent()[2];
	sXZ->spinBoxXZ->setValue(val);
	sXZ->verticalScrollBarXZ->setValue(val);
}


void MdiChild::paintEvent(QPaintEvent * )
{
}


void MdiChild::updated(QString text)
{
	QString senderName = QObject::sender()->objectName();

	switch(connectionState)
	{
	case cs_ROI:
		{
			if (senderName.compare("IndexXSpinBox") == 0)
			{
				roi[0] = text.toInt();
			}
			else if (senderName.compare("IndexYSpinBox") == 0)
			{
				roi[1] = text.toInt();
			}
			else if (senderName.compare("IndexZSpinBox") == 0)
			{
				roi[2] = text.toInt();
			}
			else if (senderName.compare("SizeXSpinBox") == 0)
			{
				roi[3] = text.toInt();
			}
			else if (senderName.compare("SizeYSpinBox") == 0)
			{
				roi[4] = text.toInt();
			}
			else if (senderName.compare("SizeZSpinBox") == 0)
			{
				roi[5] = text.toInt();
			}
			// size may not be smaller than 1 (otherwise there's a vtk error):
			if (roi[3] <= 0) roi[3] = 1;
			if (roi[4] <= 0) roi[4] = 1;
			if (roi[5] <= 0) roi[5] = 1;

			slicerXY->updateROI();
			slicerYZ->updateROI();
			slicerXZ->updateROI();
		}
		break;
	}
}

void MdiChild::updated( int i )
{
	this->addMsg(tr("mdiCild: updated: %1").arg(i));

}

void MdiChild::updated(int i, QString text)
{
	this->addMsg(tr("mdiCild: updated(i,string): %1  %2").arg(i).arg(text));
}

bool MdiChild::save()
{
	if (isUntitled) {
		return saveAs();
	} else {
		return saveFile(curFile);
	}
}

bool MdiChild::saveAs()
{
	QString filePath = currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));

	QString f = QFileDialog::getSaveFileName(this, tr("Save As"),
		filePath, iAIOProvider::GetSupportedSaveFormats() );

	if (f.isEmpty())
		return false;

	return saveFile(f);
}


bool MdiChild::saveAsImageStack()
{
	QString filePath = currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));

	QString f = QFileDialog::getSaveFileName(this, tr("Save As"),
		filePath,
		tr("TIFF stacks (*.tif);; PNG stacks (*.png);; BMP stacks (*.bmp);; JPEG stacks (*.jpg);; DICOM series (*.dcm) "));

	if (f.isEmpty())
		return false;

	return saveFile(f);
}


void MdiChild::waitForPreviousIO()
{
	if (ioThread)
	{
		addMsg(tr("%1  Waiting for I/O operation to complete...").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
		ioThread->wait();
		ioThread = 0;
	}
}


bool MdiChild::setupSaveIO(QString const & f)
{
	QFileInfo pars(f);
	if (QString::compare(pars.suffix(), "STL", Qt::CaseInsensitive) == 0) {
		if (polyData->GetNumberOfPoints() <= 1)	{
			QMessageBox::warning(this, tr("Save File"), tr("Model contains no data. Saving aborted."));
			return false;
		} else {
			if ( !ioThread->setupIO(STL_WRITER, pars.absoluteFilePath() ) ) return false;
		}
	} else {
		if ((imageData->GetExtent()[1] <= 1) && (imageData->GetExtent()[3] <= 1) && (imageData->GetExtent()[5] <= 1) )	{
			QMessageBox::warning(this, tr("Save File"), tr("Image contains no data. Saving aborted.")); return false;
		} else {
			if ((QString::compare(pars.suffix(), "MHD", Qt::CaseInsensitive) == 0) ||
				(QString::compare(pars.suffix(), "MHA", Qt::CaseInsensitive) == 0)){
					if ( !ioThread->setupIO(MHD_WRITER, pars.absoluteFilePath(),m_mainWnd->getPrefCompression()) ) return false;
					setCurrentFile(f);
					m_mainWnd->setCurrentFile(f);
					QString t; t = f;
					t.truncate(t.lastIndexOf('/'));
					m_mainWnd->setPath(t);
			} else if ((QString::compare(pars.suffix(), "TIF", Qt::CaseInsensitive) == 0) ||
				(QString::compare(pars.suffix(), "TIFF", Qt::CaseInsensitive) == 0)){
					if ( !ioThread->setupIO(TIF_STACK_WRITER, pars.absoluteFilePath() ) ) return false;
			} else if ((QString::compare(pars.suffix(), "JPG", Qt::CaseInsensitive) == 0) ||
				(QString::compare(pars.suffix(), "JPEG", Qt::CaseInsensitive) == 0)){
					if (imageData->GetScalarType() == VTK_UNSIGNED_CHAR){
						if ( !ioThread->setupIO(JPG_STACK_WRITER, pars.absoluteFilePath() ) ) return false;
					} else { addMsg(tr("JPEGWriter only supports unsigned char input!")); addMsg(tr("   FILE I/O ABORTED!")); return false;}
			} else if (QString::compare(pars.suffix(), "PNG", Qt::CaseInsensitive) == 0){
				if (imageData->GetScalarType() == VTK_UNSIGNED_CHAR){
					if ( !ioThread->setupIO(PNG_STACK_WRITER, pars.absoluteFilePath() ) ) return false;
				} else { addMsg(tr("PNGWriter only supports unsigned char input!")); addMsg(tr("   FILE I/O ABORTED!")); return false;}
			} else if (QString::compare(pars.suffix(), "BMP", Qt::CaseInsensitive) == 0){
				if (imageData->GetScalarType() == VTK_UNSIGNED_CHAR){
					if ( !ioThread->setupIO(BMP_STACK_WRITER, pars.absoluteFilePath() ) ) return false;
				} else { addMsg(tr("BMPWriter only supports unsigned char input!")); addMsg(tr("   FILE I/O ABORTED!")); return false;}
			}
			else if (QString::compare(pars.suffix(), "DCM", Qt::CaseInsensitive) == 0) {
				if (!ioThread->setupIO(DCM_WRITER, pars.absoluteFilePath())) return false;
			}	
			else if (QString::compare(pars.suffix(), "AM", Qt::CaseInsensitive) == 0) {
				if (!ioThread->setupIO(AM_WRITER, pars.absoluteFilePath())) return false;
			}
			else return false;
		}
	}
	return true;
}


bool MdiChild::saveFile(const QString &f)
{
	waitForPreviousIO();

	ioThread = new iAIO(imageData, polyData, m_logger, this);
	connectThreadSignalsToChildSlots(ioThread, false);
	connect(ioThread, SIGNAL( finished() ), this, SLOT( ioFinished() ));

	if (!setupSaveIO(f)) {
		ioFinished();
		return false;
	}

	addMsg(tr("%1  Saving sequence started... \n"
		"  The duration of the saving sequence depends on the size of your data set and may last several minutes. \n"
		"  Please wait...").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	ioThread->start();

	return true;
}

void MdiChild::updateViews()
{
	updateSlicers();
	Raycaster->update();
	emit updatedViews();
}


int MdiChild::getVisibility() const
{

	int vis = RC | YZ | XZ | XY;
	return vis;
}

void MdiChild::maximizeXY()
{
	resizeDockWidget(sXY);
}


void MdiChild::maximizeXZ()
{
	resizeDockWidget(sXZ);
}


void MdiChild::maximizeYZ()
{
	resizeDockWidget(sYZ);
}


void MdiChild::maximizeRC()
{
	resizeDockWidget(r);
}


void MdiChild::saveRC()
{
	saveRenderWindow(Raycaster->GetRenderWindow());
}


void MdiChild::saveXY()
{
	slicerXY->saveAsImage();
}

void MdiChild::saveXZ()
{
	slicerXZ->saveAsImage();
}

void MdiChild::saveYZ()
{
	slicerYZ->saveAsImage();
}


void MdiChild::saveStackXY()
{
	slicerXY->saveImageStack();
}

void MdiChild::saveStackXZ()
{
	slicerXZ->saveImageStack();
}

void MdiChild::saveStackYZ()
{
	slicerYZ->saveImageStack();
}


void MdiChild::saveMovXY()
{
	saveMovie(slicerXY);
}

void MdiChild::saveMovXZ()
{
	saveMovie(slicerXZ);
}

void MdiChild::saveMovYZ()
{
	saveMovie(slicerYZ);
}

void MdiChild::saveMovRC()
{
	saveMovie(*Raycaster);
}


void MdiChild::camPX()
{
	Raycaster->setCamPosition( 0,0,1,		1,0,0 );
}

void MdiChild::camMX()
{
	Raycaster->setCamPosition(	0,0,1,		-1,0,0	);
}

void MdiChild::camPY()
{
	Raycaster->setCamPosition(	0,0,1,		0,1,0	);
}

void MdiChild::camMY()
{
	Raycaster->setCamPosition(	0,0,1,		0,-1,0	);
}

void MdiChild::camPZ()
{
	Raycaster->setCamPosition(	0,1,0,		0,0,1	);
}

void MdiChild::camMZ()
{
	Raycaster->setCamPosition(	0,1,0,		0,0,-1	);
}

void MdiChild::camIso()
{
	Raycaster->setCamPosition(	0,0,1,		1,1,1	);
}

void MdiChild::getCamPosition(double * camOptions)
{
	Raycaster->getCamPosition(camOptions);
}

void MdiChild::setCamPosition(double * camOptions, bool rsParallelProjection)
{
	Raycaster->setCamPosition(camOptions, rsParallelProjection);
}


void MdiChild::triggerInteractionXY()
{
	bool newState = slicerXY->changeInteractorState();
	if (!newState)
		addMsg(tr("%1  Slicer XY disabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	else
		addMsg(tr("%1  Slicer XY enabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
}


void MdiChild::triggerInteractionXZ()
{
	bool newState = slicerXZ->changeInteractorState();
	if (!newState)
		addMsg(tr("%1  Slicer XZ disabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	else
		addMsg(tr("%1  Slicer XZ enabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
}


void MdiChild::triggerInteractionYZ()
{
	bool newState = slicerYZ->changeInteractorState();
	if (!newState)
		addMsg(tr("%1  Slicer YZ disabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	else
		addMsg(tr("%1  Slicer YZ enabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
}


void MdiChild::triggerInteractionRaycaster()
{
	if (Raycaster->GetInteractor()->GetEnabled()){
		Raycaster->disableInteractor();
		addMsg(tr("%1  Renderer disabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	} else {
		Raycaster->enableInteractor();
		addMsg(tr("%1  Renderer enabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	}
}

void MdiChild::setSliceXYSpinBox(int s)
{
	setSliceXY(s);
	QSignalBlocker block(sXY->verticalScrollBarXY);
	sXY->verticalScrollBarXY->setValue(s);
}

void MdiChild::setSliceXYScrollBar(int s)
{
	sXY->spinBoxXY->repaint();
	sXY->verticalScrollBarXY->repaint();
	setSliceXY(s);
	QSignalBlocker block(sXY->spinBoxXY);
	sXY->spinBoxXY->setValue(s);
}

void MdiChild::updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s)
{
	double spacing[3];
	imageData->GetSpacing(spacing);

	double splinelength = (int)parametricSpline->GetLength();
	double length_percent = 100 / splinelength;
	double mf1 = s + 1; //multiplication factor for first point
	double mf2 = s + 2; //multiplication factor for second point
	spinBox->setRange(0, (splinelength - 1));//set the number of slices to scroll through

													//calculate the percentage for 2 points
	double t1[3] = { length_percent * mf1 / 100, length_percent*mf1 / 100, length_percent*mf1 / 100 };
	double t2[3] = { length_percent*mf2 / 100, length_percent*mf2 / 100, length_percent*mf2 / 100 };
	double point1[3], point2[3];
	//calculate the points
	parametricSpline->Evaluate(t1, point1, NULL);
	parametricSpline->Evaluate(t2, point2, NULL);

	//calculate normal
	double normal[3];
	normal[0] = point2[0] - point1[0];
	normal[1] = point2[1] - point1[1];
	normal[2] = point2[2] - point1[2];

	vtkMatrixToLinearTransform  * final_transform = vtkMatrixToLinearTransform::New();

	if (normal[0] == 0 && normal[1] == 0)
	{
		//Move the point to origin Translation
		double PointToOrigin_matrix[16] = { 1, 0, 0, point1[0],
			0, 1, 0, point1[1],
			0, 0, 1, point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4 * PointToOrigin_Translation = vtkMatrix4x4::New();
		PointToOrigin_Translation->DeepCopy(PointToOrigin_matrix);

		//Move the origin to point Translation
		double OriginToPoint_matrix[16] = { 1, 0, 0, -point1[0],
			0, 1, 0, -point1[1],
			0, 0, 1, -point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4 * OriginToPoint_Translation = vtkMatrix4x4::New();
		OriginToPoint_Translation->DeepCopy(OriginToPoint_matrix);

		///multiplication of transformation matics
		vtkMatrix4x4 * Transformation_4 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(PointToOrigin_Translation, OriginToPoint_Translation, Transformation_4);

		final_transform->SetInput(Transformation_4);
		final_transform->Update();
	}
	else
	{
		//Move the point to origin Translation
		double PointToOrigin_matrix[16] = { 1, 0, 0, point1[0],
			0, 1, 0, point1[1],
			0, 0, 1, point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4 * PointToOrigin_Translation = vtkMatrix4x4::New();
		PointToOrigin_Translation->DeepCopy(PointToOrigin_matrix);

		//rotate around Z to bring the vector to XZ plane
		double alpha = acos(pow(normal[0], 2) / (sqrt(pow(normal[0], 2)) * (sqrt(pow(normal[0], 2) + pow(normal[1], 2)))));
		double cos_theta_xz = cos(alpha);
		double sin_theta_xz = sin(alpha);

		double rxz_matrix[16] = { cos_theta_xz,	-sin_theta_xz,	0,	 0,
			sin_theta_xz,	cos_theta_xz,	0,	 0,
			0,			0,		1,	 0,
			0,			0,		0,	 1 };

		vtkMatrix4x4 * rotate_around_xz = vtkMatrix4x4::New();
		rotate_around_xz->DeepCopy(rxz_matrix);

		//rotate around Y to bring vector parallel to Z axis
		double beta = acos(pow(normal[2], 2) / sqrt(pow(normal[2], 2)) + sqrt(pow(cos_theta_xz, 2) + pow(normal[2], 2)));
		double cos_theta_y = cos(beta);
		double sin_theta_y = sin(beta);

		double ry_matrix[16] = { cos_theta_y,	0,	sin_theta_y,	0,
			0,			1,		0,			0,
			-sin_theta_y,	0,	cos_theta_y,	0,
			0,			0,		0,			1 };

		vtkMatrix4x4 * rotate_around_y = vtkMatrix4x4::New();
		rotate_around_y->DeepCopy(ry_matrix);

		//rotate around Z by 180 degree - to bring object correct view
		double cos_theta_z = cos( vtkMath::Pi() );
		double sin_theta_z = sin( vtkMath::Pi() );

		double rz_matrix[16] = { cos_theta_z,	-sin_theta_z,	0,	0,
			sin_theta_z,	cos_theta_z,	0,	0,
			0,				0,			1,	0,
			0,				0,			0,	1 };

		vtkMatrix4x4 * rotate_around_z = vtkMatrix4x4::New();
		rotate_around_z->DeepCopy(rz_matrix);

		//Move the origin to point Translation
		double OriginToPoint_matrix[16] = { 1, 0, 0, -point1[0],
			0, 1, 0, -point1[1],
			0, 0, 1, -point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4 * OriginToPoint_Translation = vtkMatrix4x4::New();
		OriginToPoint_Translation->DeepCopy(OriginToPoint_matrix);

		///multiplication of transformation matics
		vtkMatrix4x4 * Transformation_1 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(PointToOrigin_Translation, rotate_around_xz, Transformation_1);

		vtkMatrix4x4 * Transformation_2 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_1, rotate_around_y, Transformation_2);

		vtkMatrix4x4 * Transformation_3 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_2, rotate_around_z, Transformation_3);

		vtkMatrix4x4 * Transformation_4 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_3, OriginToPoint_Translation, Transformation_4);

		final_transform->SetInput(Transformation_4);
		final_transform->Update();
	}

	slicer->GetReslicer()->SetResliceTransform(final_transform);
	slicer->GetReslicer()->UpdateWholeExtent();
	slicer->setSliceNumber(point1[ptIndex]);
}

void MdiChild::setSliceXY(int s)
{
	if (snakeSlicer)
	{
		updateSnakeSlicer(sXY->spinBoxXY, slicerXY, 2, s);
	}
	else
	{
		this->zCoord = s;
		QList<iAChannelID> keys = m_channels.keys();
		for (QList<iAChannelID>::iterator it = keys.begin();
			it != keys.end();
			++it)
		{
			iAChannelID key = *it;
			if (m_channels.value(key)->IsEnabled()
				|| (isMagicLensEnabled && key == slicerXY->getMagicLensInput()))
			{
				slicerXY->setResliceChannelAxesOrigin(key, 0, 0, (double)s * imageData->GetSpacing()[2]);
			}
		}
		slicerXY->setSliceNumber(s);
		if (getShowSlicers())
		{
			Raycaster->getPlane3()->SetNormal(0.0, 0.0, 1.0);
			Raycaster->getPlane3()->SetOrigin(0.0,0.0,s*imageData->GetSpacing()[2]);
			Raycaster->update();
		}
	}
}


void MdiChild::setSliceYZSpinBox(int s)
{
	setSliceYZ(s);
	QSignalBlocker block(sYZ->verticalScrollBarYZ);
	sYZ->verticalScrollBarYZ->setValue(s);
}

void MdiChild::setSliceYZScrollBar(int s)
{
	sYZ->spinBoxYZ->repaint();
	sYZ->verticalScrollBarYZ->repaint();
	setSliceYZ(s);
	QSignalBlocker block(sYZ->spinBoxYZ);
	sYZ->spinBoxYZ->setValue(s);
}


void MdiChild::setSliceYZ(int s)
{
	if (snakeSlicer)
	{
		updateSnakeSlicer(sYZ->spinBoxYZ, slicerYZ, 0, s);
	}
	else
	{
		this->xCoord = s;
		QList<iAChannelID> keys = m_channels.keys();
		for (QList<iAChannelID>::iterator it = keys.begin();
			it != keys.end(); ++it)
		{
			iAChannelID key = *it;
			if (m_channels.value(key)->IsEnabled()
				|| (isMagicLensEnabled && key == slicerYZ->getMagicLensInput()))
			{
				slicerYZ->setResliceChannelAxesOrigin(key, (double)s * imageData->GetSpacing()[0], 0, 0);
			}
		}
		slicerYZ->setSliceNumber(s);
		if (getShowSlicers())
		{
			Raycaster->getPlane1()->SetOrigin(s*imageData->GetSpacing()[0],0,0);
			Raycaster->update();
		}
	}
}


void MdiChild::setSliceXZSpinBox(int s)
{
	setSliceXZ(s);
	QSignalBlocker block(sXZ->verticalScrollBarXZ);
	sXZ->verticalScrollBarXZ->setValue(s);
}

void MdiChild::setSliceXZScrollBar(int s)
{
	sXZ->spinBoxXZ->repaint();
	sXZ->verticalScrollBarXZ->repaint();
	setSliceXZ(s);
	QSignalBlocker block(sXZ->spinBoxXZ);
	sXZ->spinBoxXZ->setValue(s);
}


void MdiChild::setSliceXZ(int s)
{
	if (snakeSlicer)
	{
		updateSnakeSlicer(sXZ->spinBoxXZ, slicerXZ, 1, s);
	}
	else
	{
		this->yCoord = s;
		QList<iAChannelID> keys = m_channels.keys();
		for (QList<iAChannelID>::iterator it = keys.begin();
			it != keys.end(); ++it)
		{
			iAChannelID key = *it;
			if (m_channels.value(key)->IsEnabled()
				|| (isMagicLensEnabled && key == slicerXZ->getMagicLensInput()))
			{
				slicerXZ->setResliceChannelAxesOrigin(key, 0, (double)s * imageData->GetSpacing()[1], 0);
			}
		}
		slicerXZ->setSliceNumber(s);
		if (getShowSlicers())
		{
			Raycaster->getPlane2()->SetOrigin(0,s*imageData->GetSpacing()[1],0);
			Raycaster->update();
		}
	}
}


void MdiChild::setChannel(int c)
{
	disableRenderWindows(c);
	enableRenderWindows();
}


void MdiChild::setRotationXY(double a)
{
	slicerXY->rotateSlice( a );
	Raycaster->setPlaneNormals( slicerTransform );
}


void MdiChild::setRotationYZ(double a)
{
	slicerYZ->rotateSlice( a );
	Raycaster->setPlaneNormals( slicerTransform );
}


void MdiChild::setRotationXZ(double a)
{
	slicerXZ->rotateSlice( a );
	Raycaster->setPlaneNormals( slicerTransform );
}


void MdiChild::link( bool l)
{
	linkviews = l;
}

void MdiChild::enableInteraction( bool b)
{
	if (b) {
		slicerYZ->enableInteractor();
		slicerXY->enableInteractor();
		slicerXZ->enableInteractor();
	} else {
		slicerYZ->disableInteractor();
		slicerXY->disableInteractor();
		slicerXZ->disableInteractor();
	}

}

bool MdiChild::editPrefs( int h, int mls, int mlfw, int e, bool c, bool m, bool r, bool init)
{
	compression = c;
	filterHistogram = m;
	resultInNewWindow = r;
	histogramBins = h;
	statExt = e;
	magicLensSize = mls;
	magicLensFrameWidth = mlfw;

	if (!init && getHistogram())
	{
		calculateHistogram();
		getHistogram()->initialize(imageAccumulate, imageData->GetScalarRange(), false);
		getHistogram()->redraw();
	}
	slicerXY->SetMagicLensFrameWidth(magicLensFrameWidth);
	slicerXZ->SetMagicLensFrameWidth(magicLensFrameWidth);
	slicerYZ->SetMagicLensFrameWidth(magicLensFrameWidth);
	slicerXY->SetMagicLensSize(magicLensSize);
	slicerXZ->SetMagicLensSize(magicLensSize);
	slicerYZ->SetMagicLensSize(magicLensSize);

	slicerXY->setStatisticalExtent(statExt);
	slicerYZ->setStatisticalExtent(statExt);
	slicerXZ->setStatisticalExtent(statExt);
	Raycaster->setStatExt(statExt);

	if (isMagicLensToggled())
	{
		updateSlicers();
	}

	emit preferencesChanged();
	m_dlgModalities->ChangeMagicLensSize(GetMagicLensSize());

	return true;
}

void MdiChild::setupRaycaster( bool sv, bool ss, bool sh, bool spo, bool li, bool s, bool bb, bool pp,
	double isd, double sd, double al, double dl, double sl, double sp, QString backt, QString backb, int mode, bool init )
{
	showVolume = sv;
	showSlicers = ss;
	showHelpers = sh;
	showRPosition = spo;
	linearInterpolation = li;
	shading = s;
	boundingBox = bb;
	parallelProjection = pp;
	imageSampleDistance = isd;
	sampleDistance = sd;
	ambientLighting = al;
	diffuseLighting = dl;
	specularLighting = sl;
	specularPower = sp;
	backgroundTop = backt;
	backgroundBottom = backb;
	renderMode = mode;

	if (!init)
		applyCurrentSettingsToRaycaster(Raycaster);
}


void MdiChild::applyCurrentSettingsToRaycaster(iARenderer * raycaster)
{
	raycaster->GetVolume()->SetVisibility(showVolume);
	//setup slicers
	if (snakeSlicer) {
		//setSliceXY(sXY->spinBoxXY->value());
		raycaster->showSlicers(false, false, showSlicers);
	}
	else
	{
		raycaster->showSlicers(showSlicers);
	}
	// setup raycaster
	raycaster->setImageSampleDistance(imageSampleDistance);
	raycaster->setSampleDistance(sampleDistance);

	// setup properties, visibility, background
	raycaster->GetVolumeProperty()->SetAmbient(ambientLighting);
	raycaster->GetVolumeProperty()->SetDiffuse(diffuseLighting);
	raycaster->GetVolumeProperty()->SetSpecular(specularLighting);
	raycaster->GetVolumeProperty()->SetSpecularPower(specularPower);
	raycaster->GetVolumeProperty()->SetInterpolationType(linearInterpolation);
	raycaster->GetVolumeProperty()->SetShade(shading);

	raycaster->GetOutlineActor()->SetVisibility(boundingBox);
	raycaster->GetRenderer()->GetActiveCamera()->SetParallelProjection(parallelProjection);

	backgroundTop = backgroundTop.trimmed();
	backgroundBottom = backgroundBottom.trimmed();
	QColor bgTop(backgroundTop);
	QColor bgBottom(backgroundBottom);
	if (!bgTop.isValid()) {
		bgTop.setRgbF(0.5, 0.666666666666666667, 1.0);
	}
	if (!bgBottom.isValid())  {
		bgBottom.setRgbF(1.0, 1.0, 1.0);
	}
	raycaster->GetRenderer()->SetBackground(bgTop.redF(), bgTop.greenF(), bgTop.blueF());
	raycaster->GetRenderer()->SetBackground2(bgBottom.redF(), bgBottom.greenF(), bgBottom.blueF());
	raycaster->showHelpers(showHelpers);
	raycaster->showRPosition(showRPosition);
	
	raycaster->SetRenderMode(renderMode);
}

int MdiChild::GetRenderMode()
{
	return renderMode;
}

void MdiChild::setupSlicers( bool lv, bool sil, bool sp, int no, double min, double max, bool li, int ss, bool init)
{
	linkviews = lv;
	showIsolines = sil;
	showPosition = sp;
	numberOfIsolines = no;
	minIsovalue = min;
	maxIsovalue = max;
	imageActorUseInterpolation = li;
	snakeSlices = ss;

	if (snakeSlicer)
	{
		int prevMax = sXY->spinBoxXY->maximum();
		int prevValue = sXY->spinBoxXY->value();
		sXY->spinBoxXY->setRange(0, snakeSlices-1);
		sXY->spinBoxXY->setValue((double)prevValue/prevMax*(snakeSlices-1));
	}

	linkViews(lv);

	slicerYZ->setup( sil, sp, no, min, max, li);
	slicerXY->setup( sil, sp, no, min, max, li);
	slicerXZ->setup( sil, sp, no, min, max, li);

	if (init)
	{
		//this initializes the snake slicer
		slicerYZ->initializeWidget(imageData, worldSnakePoints);
		slicerXY->initializeWidget(imageData, worldSnakePoints);
		slicerXZ->initializeWidget(imageData, worldSnakePoints);

		updateSliceIndicators();

		//Adding new point to the parametric spline for snake slicer
		connect(slicerXY->widget(), SIGNAL(addedPoint(double, double, double)), slicerYZ->widget(), SLOT(addPoint(double, double, double)));
		connect(slicerXY->widget(), SIGNAL(addedPoint(double, double, double)), slicerXZ->widget(), SLOT(addPoint(double, double, double)));
		connect(slicerYZ->widget(), SIGNAL(addedPoint(double, double, double)), slicerXY->widget(), SLOT(addPoint(double, double, double)));
		connect(slicerYZ->widget(), SIGNAL(addedPoint(double, double, double)), slicerXZ->widget(), SLOT(addPoint(double, double, double)));
		connect(slicerXZ->widget(), SIGNAL(addedPoint(double, double, double)), slicerXY->widget(), SLOT(addPoint(double, double, double)));
		connect(slicerXZ->widget(), SIGNAL(addedPoint(double, double, double)), slicerYZ->widget(), SLOT(addPoint(double, double, double)));

		//Moving point
		connect(slicerXY->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicerYZ->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicerXY->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicerXZ->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicerYZ->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicerXY->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicerYZ->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicerXZ->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicerXZ->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicerXY->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicerXZ->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicerYZ->widget(), SLOT(movePoint(size_t, double, double, double)));

		//Changing arbitrary profile positioning
		connect(slicerXY->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicerYZ->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicerXY->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicerXZ->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicerXY->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), this,		 SLOT(UpdateProbe(int, double*)));
		connect(slicerYZ->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicerXY->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicerYZ->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicerXZ->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicerYZ->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), this,		 SLOT(UpdateProbe(int, double*)));
		connect(slicerXZ->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicerXY->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicerXZ->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicerYZ->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicerXZ->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), this,		 SLOT(UpdateProbe(int, double*)));


		connect(slicerXY->widget(), SIGNAL(switchedMode(int)), slicerYZ->widget(), SLOT(switchMode(int)));
		connect(slicerXY->widget(), SIGNAL(switchedMode(int)), slicerXZ->widget(), SLOT(switchMode(int)));
		connect(slicerYZ->widget(), SIGNAL(switchedMode(int)), slicerXY->widget(), SLOT(switchMode(int)));
		connect(slicerYZ->widget(), SIGNAL(switchedMode(int)), slicerXZ->widget(), SLOT(switchMode(int)));
		connect(slicerXZ->widget(), SIGNAL(switchedMode(int)), slicerXY->widget(), SLOT(switchMode(int)));
		connect(slicerXZ->widget(), SIGNAL(switchedMode(int)), slicerYZ->widget(), SLOT(switchMode(int)));

		connect(slicerXY->widget(), SIGNAL(deletedSnakeLine()), slicerYZ->widget(), SLOT(deleteSnakeLine()));
		connect(slicerXY->widget(), SIGNAL(deletedSnakeLine()), slicerXZ->widget(), SLOT(deleteSnakeLine()));
		connect(slicerYZ->widget(), SIGNAL(deletedSnakeLine()), slicerXY->widget(), SLOT(deleteSnakeLine()));
		connect(slicerYZ->widget(), SIGNAL(deletedSnakeLine()), slicerXZ->widget(), SLOT(deleteSnakeLine()));
		connect(slicerXZ->widget(), SIGNAL(deletedSnakeLine()), slicerXY->widget(), SLOT(deleteSnakeLine()));
		connect(slicerXZ->widget(), SIGNAL(deletedSnakeLine()), slicerYZ->widget(), SLOT(deleteSnakeLine()));

		connect(slicerXY->widget(), SIGNAL(deselectedPoint()), slicerYZ->widget(), SLOT(deselectPoint()));
		connect(slicerXY->widget(), SIGNAL(deselectedPoint()), slicerXZ->widget(), SLOT(deselectPoint()));
		connect(slicerYZ->widget(), SIGNAL(deselectedPoint()), slicerXY->widget(), SLOT(deselectPoint()));
		connect(slicerYZ->widget(), SIGNAL(deselectedPoint()), slicerXZ->widget(), SLOT(deselectPoint()));
		connect(slicerXZ->widget(), SIGNAL(deselectedPoint()), slicerXY->widget(), SLOT(deselectPoint()));
		connect(slicerXZ->widget(), SIGNAL(deselectedPoint()), slicerYZ->widget(), SLOT(deselectPoint()));
	}
}


bool MdiChild::editRendererSettings( bool rsShowVolume, bool rsShowSlicers,  bool rsShowHelpers, bool rsShowRPosition,
	bool rsLinearInterpolation, bool rsShading, bool rsBoundingBox, bool rsParallelProjection,
	double rsImageSampleDistance, double rsSampleDistance, double rsAmbientLightning,
	double rsDiffuseLightning, double rsSpecularLightning, double rsSpecularPower,
	QString rsBackgroundTop, QString rsBackgroundBottom, int renderMode)
{
	setupRaycaster( rsShowVolume, rsShowSlicers, rsShowHelpers, rsShowRPosition,
		rsLinearInterpolation, rsShading, rsBoundingBox, rsParallelProjection,
		rsImageSampleDistance, rsSampleDistance, rsAmbientLightning, rsDiffuseLightning,
		rsSpecularLightning, rsSpecularPower, rsBackgroundTop, rsBackgroundBottom,
		renderMode, false);

	r->vtkWidgetRC->show();

	emit renderSettingsChanged();
	m_dlgModalities->ChangeRenderSettings(GetRenderSettings());

	return true;
}

#include "iARenderSettings.h"

RenderSettings MdiChild::GetRenderSettings()
{
	return RenderSettings(linearInterpolation, shading, ambientLighting, diffuseLighting, specularLighting, specularPower, backgroundTop);
}


bool MdiChild::editSlicerSettings( bool lv, bool sil, bool sp, int no, double min, double max, bool li, int ss)
{
	setupSlicers( lv, sil, sp, no, min, max, li, ss, false);

	slicerXY->show();
	slicerYZ->show();
	slicerXZ->show();

	return true;
}


bool MdiChild::loadTransferFunction()
{
	return getHistogram()->loadTransferFunction();
}

bool MdiChild::saveTransferFunction()
{
	return getHistogram()->saveTransferFunction();
}


void MdiChild::saveRenderWindow(vtkRenderWindow *renderWindow)
{
	QString file = QFileDialog::getSaveFileName(this, tr("Save Image"),
		"",
		tr("JPEG (*.jpg);;TIFF (*.tif);;PNG (*.png);;BMP (*.bmp)" ) );

	if (file.isEmpty())
		return;

	vtkWindowToImageFilter *filter = vtkWindowToImageFilter::New();
	filter->SetInput(renderWindow);
	filter->Update();

	QFileInfo pars(file);
	if ((QString::compare(pars.suffix(), "TIF", Qt::CaseInsensitive) == 0) || (QString::compare(pars.suffix(), "TIFF", Qt::CaseInsensitive) == 0)){
		vtkTIFFWriter *writer = vtkTIFFWriter::New();
		writer->SetFileName(file.toLatin1());
		writer->SetInputData(imageData);
		writer->Write();
		writer->Delete();
	} else if (QString::compare(pars.suffix(), "PNG", Qt::CaseInsensitive) == 0) {
		vtkPNGWriter *writer = vtkPNGWriter::New();
		writer->SetFileName(file.toLatin1());
		writer->SetInputData(imageData);
		writer->Write();
		writer->Delete();
	} else if ((QString::compare(pars.suffix(), "JPG", Qt::CaseInsensitive) == 0) || (QString::compare(pars.suffix(), "JPEG", Qt::CaseInsensitive) == 0)){
		vtkJPEGWriter *writer = vtkJPEGWriter::New();
		writer->SetFileName(file.toLatin1());
		writer->SetInputData(imageData);
		writer->Write();
		writer->Delete();
	} else if (QString::compare(pars.suffix(), "BMP", Qt::CaseInsensitive) == 0) {
		vtkBMPWriter *writer = vtkBMPWriter::New();
		writer->SetFileName(file.toLatin1());
		writer->SetInputData(imageData);
		writer->Write();
		writer->Delete();
	}

	filter->Delete();
}


void MdiChild::saveMovie(iASlicer * slicer)
{
	slicer->saveMovie();
}


void MdiChild::saveMovie(iARenderer& raycaster)
{
	QString movie_file_types;

#ifdef VTK_USE_MPEG2_ENCODER
	movie_file_types += "MPEG2 (*.mpeg);;";
#endif
#ifdef VTK_USE_OGGTHEORA_ENCODER
	movie_file_types += "OGG (*.ogv);;";
#endif
#ifdef WIN32
	movie_file_types += "AVI (*.avi);;";
#endif

	// If VTK was built without video support, display error message and quit.
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(this, "Movie Export", "Sorry, but movie export support is disabled.");
		return;
	}

	QString mode;
	int imode = 0;

	QStringList modes = (QStringList() <<  tr("Rotate Z") <<  tr("Rotate X") <<  tr("Rotate Y"));
	QStringList inList = ( QStringList() << tr("+Rotation mode") );
	QList<QVariant> inPara = ( QList<QVariant>() << modes );

	dlg_commoninput *dlg = new dlg_commoninput (this, "Save movie options", 1, inList, inPara, NULL);
	if (dlg->exec() == QDialog::Accepted)
	{
		mode =  dlg->getComboBoxValues()[0];
		imode = dlg->getComboBoxIndices()[0];
	}


	// Show standard save file dialog using available movie file types.
	raycaster.saveMovie(
		QFileDialog::getSaveFileName(
		this,
		tr("Export movie %1").arg(mode),
		fileInfo.absolutePath() + "/" + ((mode.isEmpty()) ? fileInfo.baseName() : fileInfo.baseName() + "_" + mode),
		movie_file_types),
		imode);
}


int MdiChild::deletePoint()
{
	return getHistogram()->deletePoint();
}


void MdiChild::changeColor()
{
	getHistogram()->changeColor();
}


void MdiChild::autoUpdate(bool toggled)
{
	getHistogram()->autoUpdate(toggled);
}


void MdiChild::resetView()
{
	getHistogram()->resetView();
}


void MdiChild::resetTrf()
{
	getHistogram()->resetTrf();
}


void MdiChild::toggleSnakeSlicer(bool isChecked)
{
	snakeSlicer = isChecked;

	if (snakeSlicer)
	{
		//save the slicer transforms
		SlicerYZ_Transform = slicerYZ->GetReslicer()->GetResliceTransform();
		SlicerXY_Transform = slicerXY->GetReslicer()->GetResliceTransform();
		SlicerXZ_Transform = slicerXZ->GetReslicer()->GetResliceTransform();

		parametricSpline->Modified();
		double emptyper[3]; emptyper[0] = 0; emptyper[1] = 0; emptyper[2] = 0;
		double emptyp[3]; emptyp[0] = 0; emptyp[1] = 0; emptyp[2] = 0;
		parametricSpline->Evaluate(emptyper, emptyp, NULL);

		sXY->spinBoxXY->setValue(0);//set initial value
		sXZ->spinBoxXZ->setValue(0);//set initial value
		sYZ->spinBoxYZ->setValue(0);//set initial value

		slicerXY->widget()->switchMode(iASlicerWidget::NORMAL);
		slicerXZ->widget()->switchMode(iASlicerWidget::NORMAL);
		slicerYZ->widget()->switchMode(iASlicerWidget::NORMAL);
	}
	else
	{
		showSlicers = false;

		sXY->spinBoxXY->setValue(imageData->GetDimensions()[2]>>1);
		slicerXY->GetReslicer()->SetResliceAxesDirectionCosines(1,0,0,  0,1,0,  0,0,1);
		slicerXY->GetReslicer()->SetResliceTransform(SlicerXY_Transform);
		slicerXY->GetReslicer()->SetOutputExtentToDefault();
		slicerXY->GetRenderer()->ResetCamera();
		slicerXY->GetRenderer()->Render();

		sXZ->spinBoxXZ->setValue(imageData->GetDimensions()[1]>>1);
		slicerXZ->GetReslicer()->SetResliceAxesDirectionCosines(1,0,0,  0,0,1,  0,-1,0);
		slicerXZ->GetReslicer()->SetResliceTransform(SlicerXZ_Transform);
		slicerXZ->GetReslicer()->SetOutputExtentToDefault();
		slicerXZ->GetRenderer()->ResetCamera();
		slicerXZ->GetRenderer()->Render();

		sYZ->spinBoxYZ->setValue(imageData->GetDimensions()[0]>>1);
		slicerYZ->GetReslicer()->SetResliceAxesDirectionCosines(0,1,0,  0,0,1,  1,0,0);
		slicerYZ->GetReslicer()->SetResliceTransform(SlicerYZ_Transform);
		slicerYZ->GetReslicer()->SetOutputExtentToDefault();
		slicerYZ->GetRenderer()->ResetCamera();
		slicerYZ->GetRenderer()->Render();

		if (showSlicers)
			Raycaster->showSlicers(true);

		slicerXY->widget()->switchMode(iASlicerWidget::DEFINE_SPLINE);
		slicerXZ->widget()->switchMode(iASlicerWidget::DEFINE_SPLINE);
		slicerYZ->widget()->switchMode(iASlicerWidget::DEFINE_SPLINE);
	}
}

bool MdiChild::isSnakeSlicerToggled() const
{
	return snakeSlicer;
}

void MdiChild::toggleSliceProfile(bool isChecked)
{
	isSliceProfileEnabled = isChecked;
	slicerXY->setSliceProfileOn(isSliceProfileEnabled);
	slicerXZ->setSliceProfileOn(isSliceProfileEnabled);
	slicerYZ->setSliceProfileOn(isSliceProfileEnabled);
}

bool MdiChild::isSliceProfileToggled(void) const
{
	return isSliceProfileEnabled;
}

void MdiChild::toggleMagicLens( bool isEnabled )
{
	isMagicLensEnabled = isEnabled;

	if (isEnabled)
	{
		iAChannelVisualizationData * chData = GetChannelData(ch_ModalityLens);
		if (!chData)
		{
			chData = new iAChannelVisualizationData();
			InsertChannelData(ch_ModalityLens, chData);
		}
		m_currentModality = m_dlgModalities->GetSelected();
		vtkSmartPointer<vtkImageData> img = m_dlgModalities->GetModalities()->Get(m_currentModality)->GetImage();
		chData->SetActiveImage(img);
		chData->SetColorTF(m_dlgModalities->GetCTF(m_currentModality));
		chData->SetOpacityTF(m_dlgModalities->GetOTF(m_currentModality));
		chData->SetOpacity(0.5);
		InitChannelRenderer(ch_ModalityLens, false, false);
		SetMagicLensInput(ch_ModalityLens, true);
		SetMagicLensCaption(m_dlgModalities->GetModalities()->Get(m_currentModality)->GetName().toStdString());
	}
	//SetMagicLensEnabled(isEnabled);

	//updateSlicers();

	emit magicLensToggled(isMagicLensEnabled);
}

bool MdiChild::isMagicLensToggled(void) const
{
	return isMagicLensEnabled;
}

void MdiChild::updateReslicer(double point[3], double normal[3], int mode)
{
	//translation to origin
	double t_matrix[16] = {1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		-point[0], -point[1], -point[2], 1};
	vtkMatrix4x4 * translation_matrix = vtkMatrix4x4::New();
	translation_matrix->DeepCopy(t_matrix);

	//rotation to make vector parallel to the z axis
	double diagonal = sqrt (pow(normal[0],2) + pow(normal[1],2) + pow(normal[2],2) );
	double intermediate_dia = sqrt ( pow(normal[0],2) + pow(normal[1],2) );
	double cos_theta = normal[2] / diagonal;
	double sin_theta = intermediate_dia / diagonal;
	double r_matrix[16] = {cos_theta, 0, sin_theta, 0,
		0, 1, 0, 0,
		-sin_theta, 0, cos_theta, 0,
		0, 0, 0, 1};

	vtkMatrix4x4 * rotation_matrix = vtkMatrix4x4::New();
	rotation_matrix->DeepCopy(r_matrix);

	//rotation in Z axis by 180 degree
	double cos_theta_z = cos(3.14159);
	double sin_theta_z = sin(3.14159);
	double r_matrix_z[16] = {	cos_theta_z, -sin_theta_z, 0, 0,
		sin_theta_z,  cos_theta_z, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1};
	vtkMatrix4x4 * rotation_matrix_z = vtkMatrix4x4::New();
	rotation_matrix_z->DeepCopy(r_matrix_z);

	//translate back to object position
	double bt_matrix[16] = {1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		point[0], point[1], point[2], 1};
	vtkMatrix4x4 * backtranslation_matrix = vtkMatrix4x4::New();
	backtranslation_matrix->DeepCopy(bt_matrix);

	//get the final transformation matrix to apply on the image
	vtkMatrix4x4 * intermediate_transformation_1 = vtkMatrix4x4::New();
	vtkMatrix4x4 * intermediate_transformation_2 = vtkMatrix4x4::New();
	vtkMatrix4x4 * final_transformation_matrix = vtkMatrix4x4::New();
	vtkMatrix4x4::Multiply4x4(translation_matrix, rotation_matrix, intermediate_transformation_1);
	vtkMatrix4x4::Multiply4x4(intermediate_transformation_1, rotation_matrix_z, intermediate_transformation_2);
	vtkMatrix4x4::Multiply4x4(intermediate_transformation_2, backtranslation_matrix, final_transformation_matrix);

	if ( mode == 1 )
	{
		double a_matrix[16] = {1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1};
		vtkMatrix4x4 * axial_matrix = vtkMatrix4x4::New();
		axial_matrix->DeepCopy(a_matrix);
		vtkMatrix4x4 * axial_transformation_matrix = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(final_transformation_matrix, axial_matrix, axial_transformation_matrix);

		vtkMatrixToLinearTransform  * axial_transform = vtkMatrixToLinearTransform ::New();
		axial_transform->SetInput(axial_transformation_matrix);
		axial_transform->Update();

		slicerXY->GetReslicer()->SetResliceAxes(axial_transformation_matrix);
	}

	if ( mode == 0 )
	{
		double c_matrix[16] = { 0, 0, 1, 0,
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 1};
		vtkMatrix4x4 * coronial_matrix = vtkMatrix4x4::New();
		coronial_matrix->DeepCopy(c_matrix);
		vtkMatrix4x4 * coronial_transformation_matrix = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(final_transformation_matrix, coronial_matrix, coronial_transformation_matrix);

		vtkMatrixToLinearTransform  * coronial_transform = vtkMatrixToLinearTransform ::New();
		coronial_transform->SetInput(coronial_transformation_matrix);
		coronial_transform->Update();

		slicerYZ->GetReslicer()->SetResliceAxes(coronial_transformation_matrix);
	}

	if ( mode == 2 )
	{
		double s_matrix[16] = { 1, 0, 0, 0,
			0, 0, -1, 0,
			0, 1, 0, 0,
			0, 0, 0, 1};
		vtkMatrix4x4 * sagittal_matrix = vtkMatrix4x4::New();
		sagittal_matrix->DeepCopy(s_matrix);
		vtkMatrix4x4 * sagittal_transformation_matrix = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(final_transformation_matrix, sagittal_matrix, sagittal_transformation_matrix);

		vtkMatrixToLinearTransform  * sagittal_transform = vtkMatrixToLinearTransform ::New();
		sagittal_transform->SetInput(sagittal_transformation_matrix);
		sagittal_transform->Update();

		slicerXZ->GetReslicer()->SetResliceAxes(sagittal_transformation_matrix);
	}
}

void MdiChild::getSnakeNormal(int index, double point[3], double normal[3])
{
	int i1 = index;
	int i2 = index+1;

	double spacing[3];
	imageData->GetSpacing(spacing);

	if (index == (snakeSlices-1))
	{
		i1--;
		i2--;
	}

	if (index >= 0 && index < snakeSlices)
	{
		double p1[3], p2[3];
		double t1[3] =
		{ (double)i1/(snakeSlices-1), (double)i1/(snakeSlices-1), (double)i1/(snakeSlices-1) };
		double t2[3] = { (double)i2/(snakeSlices-1), (double)i2/(snakeSlices-1), (double)i2/(snakeSlices-1) };
		parametricSpline->Evaluate(t1, p1, NULL);
		parametricSpline->Evaluate(t2, p2, NULL);

		//calculate the points
		p1[0] /= spacing[0]; p1[1] /= spacing[1]; p1[2] /= spacing[2];
		p2[0] /= spacing[0]; p2[1] /= spacing[1]; p2[2] /= spacing[2];

		//calculate the vector between to points
		if (normal != NULL)
		{
			normal[0] = p2[0]-p1[0];
			normal[1] = p2[1]-p1[1];
			normal[2] = p2[2]-p1[2];
		}

		point[0] = p1[0]; point[1] = p1[1]; point[2] = p1[2];
	}
}

bool MdiChild::initTransferfunctions( )
{
	piecewiseFunction->RemoveAllPoints();
	piecewiseFunction->AddPoint ( imageData->GetScalarRange()[0], 0.0 );
	piecewiseFunction->AddPoint ( imageData->GetScalarRange()[1], 1.0 );

	colorTransferFunction->RemoveAllPoints();
	colorTransferFunction->AddRGBPoint ( imageData->GetScalarRange()[0], 0.0, 0.0, 0.0 );
	colorTransferFunction->AddRGBPoint ( imageData->GetScalarRange()[1], 1.0, 1.0, 1.0 );
	colorTransferFunction->Build();

	addMsg(tr("%1  Initializing transfer functions.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	addMsg(tr("  Adding transfer functions point: %1.   Opacity: 0.0,   Color: 0, 0, 0")
		.arg(imageData->GetScalarRange()[0]));
	addMsg(tr("  Adding transfer functions point: %1.   Opacity: 1.0,   Color: 255, 255, 255")
		.arg(imageData->GetScalarRange()[1]));

	return true;
}

bool MdiChild::initView( )
{
	colorTransferFunction->RemoveAllPoints();
	piecewiseFunction->RemoveAllPoints();

	if ( !initTransferfunctions() ) return false;

	slicerXZ->initializeData(imageData, slicerTransform, colorTransferFunction);
	slicerXY->initializeData(imageData, slicerTransform, colorTransferFunction);
	slicerYZ->initializeData(imageData, slicerTransform, colorTransferFunction);
	if (!raycasterInitialized)
	{
		Raycaster->initialize(imageData, polyData, piecewiseFunction, colorTransferFunction);
		raycasterInitialized = true;
	}
	r->stackedWidgetRC->setCurrentIndex(0);

	updateSliceIndicators();

	int *extent = imageData->GetExtent();
	if (extent[0] == 0 && extent[1] == -1 &&
		extent[2] == 0 && extent[3] == -1 &&
		extent[4] == 0 && extent[5] == -1) //Polygonal mesh is loaded
		showPoly();
	else //Scalar field is loaded
	{
		this->addHistogram();
		this->addImageProperty();
		//this->addVolumePlayer();
		this->addProfile();
	}

	connect(Raycaster->getObserverFPProgress(), SIGNAL( oprogress(int) ), this, SLOT( updateProgressBar(int))) ;
	connect(Raycaster->getObserverGPUProgress(), SIGNAL( oprogress(int) ), this, SLOT( updateProgressBar(int))) ;

	//Load the layout to the child
	updateLayout();

	return true;
}

void MdiChild::resetLayout()
{
	restoreState(m_initialLayoutState);
}

bool MdiChild::calculateHistogram( )
{
	addMsg(tr("%1  Accumulating Histogram.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));

	imageAccumulate->ReleaseDataFlagOn();
	imageAccumulate->SetComponentExtent(0, m_mainWnd->getPrefHistoBinCnt()-1, 0, 0, 0, 0); // number of bars
	imageAccumulate->SetComponentSpacing((imageData->GetScalarRange()[1] - imageData->GetScalarRange()[0])/m_mainWnd->getPrefHistoBinCnt(), 0.0, 0.0);
	imageAccumulate->SetComponentOrigin(imageData->GetScalarRange()[0], 0.0, 0.0);
	imageAccumulate->SetInputData(imageData);
	imageAccumulate->Update();

	if (m_mainWnd->getPrefMedianFilterFistogram()) medianFilterHistogram(imageAccumulate);

	addMsg(tr("  Scalar Component %1: VoxelCount: %2  Min: %3  Max: %4  Mean: %5  StdDev: %6 ")
		.arg(0)
		.arg(imageAccumulate->GetVoxelCount())
		.arg(imageAccumulate->GetMin()[0])
		.arg(imageAccumulate->GetMax()[0])
		.arg(imageAccumulate->GetMean()[0])
		.arg(imageAccumulate->GetStandardDeviation()[0])
		);

	//TODO: move away, to its separate place
	//////////////////////////////////////////////////////////////////////////
	double start[3], end[3];
	int dim[3];
	imageData->GetOrigin(start);
	imageData->GetDimensions(dim);
	for (int i=0; i<3; i++)
		end[i] = start[i] + dim[i];
	profileProbe = QSharedPointer<iAProfileProbe>( new iAProfileProbe(start, end, imageData) );
	if(imgProfile!=NULL)
		imgProfile->profileWidget->initialize(profileProbe->profileData, profileProbe->GetRayLength());
	//////////////////////////////////////////////////////////////////////////
	return true;
}

bool MdiChild::medianFilterHistogram( vtkImageAccumulate* imgA )
{

	return true;
}

bool MdiChild::addHistogram( )
{
	imgHistogram = new dlg_histogram(this, imageData, imageAccumulate, piecewiseFunction, colorTransferFunction);

	connect(getHistogram(), SIGNAL(updateViews()), this, SLOT(updateViews()));
	connect(getHistogram(), SIGNAL(pointSelected()), this, SIGNAL(pointSelected()));
	connect(getHistogram(), SIGNAL(noPointSelected()), this, SIGNAL(noPointSelected()));
	connect(getHistogram(), SIGNAL(endPointSelected()), this, SIGNAL(endPointSelected()));
	connect(getHistogram(), SIGNAL(active()), this, SIGNAL(active()));
	connect(getHistogram(), SIGNAL(autoUpdateChanged(bool)), this, SIGNAL(autoUpdateChanged(bool)));
	tabifyDockWidget(logs, imgHistogram);
	return true;
}

bool MdiChild::addImageProperty()
{
	imgProperty = new dlg_imageproperty(this, imageData, imageAccumulate, fileInfo.canonicalFilePath());
	tabifyDockWidget(logs, imgProperty);

	return true;
}

bool MdiChild::addVolumePlayer(iAVolumeStack* volumeStack)
{
	volumePlayer = new dlg_volumePlayer(this, imageData, imageAccumulate, fileInfo.canonicalFilePath(), volumeStack);
	tabifyDockWidget(logs, volumePlayer);
	for (int id=0; id<volumeStack->getNumberOfVolumes(); id++) {
		CheckedList.append(0);
	}
	connect(getHistogram(), SIGNAL(applyTFForAll()), volumePlayer, SLOT(applyForAll()));

	return true;
}

int MdiChild::EvaluatePosition(int pos, int i, bool invert)
{
	if ( pos < 0 ) invert ? (pos = imageData->GetExtent()[i]) : (pos = 0);
	if ( pos > imageData->GetExtent()[i] ) invert ? (pos = 0) : (pos = imageData->GetExtent()[i]);
	return pos;
}

void MdiChild::addMsg(QString txt)
{
	logs->listWidget->addItem(txt);
	logs->listWidget->scrollToBottom();
	logs->listWidget->repaint();
}

void MdiChild::addStatusMsg(QString txt)
{
	m_mainWnd->statusBar()->showMessage(txt, 10000);
}

int MdiChild::getSelectedFuncPoint()
{
	if (getHistogram() != NULL)
		return getHistogram()->getSelectedFuncPoint();

	return -1;
}

int MdiChild::isFuncEndPoint(int index)
{
	return getHistogram()->isFuncEndPoint(index);
}

bool MdiChild::isUpdateAutomatically()
{
	if (getHistogram() != NULL)
		return getHistogram()->isUpdateAutomatically();

	return false;
}

void MdiChild::setHistogramFocus()
{
	getHistogram()->setFocus(Qt::OtherFocusReason);
}

bool MdiChild::isMaximized()
{
	return visibility != MULTI;
}

void MdiChild::setROI(int indexX, int indexY, int indexZ, int sizeX, int sizeY, int sizeZ)
{
	roi[0] = indexX;
	roi[1] = indexY;
	roi[2] = indexZ;
	roi[3] = sizeX;
	roi[4] = sizeY;
	roi[5] = sizeZ;

	slicerXY->setROI(roi);
	slicerYZ->setROI(roi);
	slicerXZ->setROI(roi);
}

void MdiChild::hideROI()
{
	slicerXY->setROIVisible(false);
	slicerYZ->setROIVisible(false);
	slicerXZ->setROIVisible(false);
}

void MdiChild::showROI()
{
	slicerYZ->setROIVisible(true);
	slicerXY->setROIVisible(true);
	slicerXZ->setROIVisible(true);
}

QString MdiChild::userFriendlyCurrentFile()
{
	return strippedName(curFile);
}

void MdiChild::closeEvent(QCloseEvent *event)
{
	if (ioThread) {
		addStatusMsg("Cannot close window while I/O operation is in progress!");
		event->ignore();
	} else {
		emit closed();
		event->accept();
	}
}

void MdiChild::setCurrentFile(const QString &f)
{
	fileInfo.setFile(f);
	curFile = f;
	path = fileInfo.canonicalPath();
	isUntitled = false;
	setWindowModified(false);
	setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

QString MdiChild::strippedName(const QString &f)
{
	return QFileInfo(f).fileName();
}


void MdiChild::changeVisibility(unsigned char mode)
{
	visibility = mode;

	bool  rc = (mode & RC)  == RC;
	bool  xy = (mode & XY)  == XY;
	bool  yz = (mode & YZ)  == YZ;
	bool  xz = (mode & XZ)  == XZ;
	bool tab = (mode & TAB) == TAB;
	r->setVisible(rc);
	sXY->setVisible(xy);
	sYZ->setVisible(yz);
	sXZ->setVisible(xz);

	logs->setVisible(tab);
	if(showVolume)
		imgHistogram->setVisible(tab);
}

void MdiChild::widgetsVisible(bool isVisible)
{
	isVisible ? tabsVisible = true :  tabsVisible = false;

	visibilityBlock(QList<QSpacerItem*>(),
		QList<QWidget*>() << sXY << sXZ << sYZ << r, isVisible);

	Raycaster->GetVolume()->SetVisibility(true);
	this->update();
}


void MdiChild::visibilityBlock(QList<QSpacerItem*> spacerItems, QList<QWidget*> widgets, bool show)
{
	for (int i = 0; i < widgets.size(); i++)
		show ? widgets[i]->show() : widgets[i]->hide();
}


void MdiChild::updateSlicer(int index)
{
	switch(index)
	{
	case 0://XY
		slicerXY->update();
		break;
	case 1://YZ
		slicerYZ->update();
		break;
	case 2://XZ
		slicerXZ->update();
		break;
	default:
		QMessageBox::warning(this, tr("Wrong slice plane index"), tr("updateSlicer(int index) is called with the wrong index parameter"));
		break;
	}
}

void MdiChild::InitChannelRenderer(iAChannelID id, bool use3D, bool enableChannel)
{
	iAChannelVisualizationData * data = GetChannelData(id);
	assert(data);
	if (!data)
	{
		return;
	}
	slicerXY->initializeChannel(id, data);
	slicerXZ->initializeChannel(id, data);
	slicerYZ->initializeChannel(id, data);
	if (use3D)
	{
		data->Set3D(true);
		Raycaster->addChannel(data);
	}
	SetChannelRenderingEnabled(id, enableChannel);
}

void MdiChild::SetSlicerPieGlyphsEnabled( bool isOn )
{
	slicerXY->setPieGlyphsOn(isOn);
	slicerXZ->setPieGlyphsOn(isOn);
	slicerYZ->setPieGlyphsOn(isOn);
}

void MdiChild::SetPieGlyphParameters( double opacity, double spacing, double magFactor )
{
	slicerXY->setPieGlyphParameters(opacity, spacing, magFactor);
	slicerXZ->setPieGlyphParameters(opacity, spacing, magFactor);
	slicerYZ->setPieGlyphParameters(opacity, spacing, magFactor);
}

iAChannelVisualizationData * MdiChild::GetChannelData(iAChannelID id)
{
	QMap<iAChannelID, QSharedPointer<iAChannelVisualizationData> >::const_iterator it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return 0;
	}
	return it->data();
}

iAChannelVisualizationData const * MdiChild::GetChannelData(iAChannelID id) const
{
	QMap<iAChannelID, QSharedPointer<iAChannelVisualizationData> >::const_iterator it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return 0;
	}
	return it->data();
}

void MdiChild::InsertChannelData(iAChannelID id, iAChannelVisualizationData * channelData)
{
	assert(m_channels.find(id) == m_channels.end());
	m_channels.insert(id, QSharedPointer<iAChannelVisualizationData>(channelData));
}

void MdiChild::updateSlicers()
{
	updateSlicer(0);
	updateSlicer(1);
	updateSlicer(2);
}

void MdiChild::UpdateChannelSlicerOpacity(iAChannelID id, double opacity)
{
	if (!GetChannelData(id))
	{
		return;
	}
	GetChannelData(id)->SetOpacity(opacity);
	getSlicerDataXY()->setChannelOpacity(id, opacity);
	getSlicerDataXZ()->setChannelOpacity(id, opacity);
	getSlicerDataYZ()->setChannelOpacity(id, opacity);

	updateSlicers();
}

void MdiChild::SetChannelRenderingEnabled(iAChannelID id, bool enabled)
{
	iAChannelVisualizationData * data = GetChannelData(id);
	if (!data || data->IsEnabled() == enabled)
	{
		// the channel with the given ID doesn't exist or hasn't changed
		return;
	}
	data->SetEnabled(enabled);
	getSlicerDataXY()->enableChannel(id, enabled, 0, 0, static_cast<double>(sXY->spinBoxXY->value()) * imageData->GetSpacing()[2]);
	getSlicerDataXZ()->enableChannel(id, enabled, 0, static_cast<double>(sXZ->spinBoxXZ->value()) * imageData->GetSpacing()[1], 0);
	getSlicerDataYZ()->enableChannel(id, enabled, static_cast<double>(sYZ->spinBoxYZ->value()) * imageData->GetSpacing()[0], 0, 0);
	if (data->Uses3D())
	{
		getRenderer()->updateChannelImages();
	}
	updateViews();
}

void MdiChild::removeFinishedAlgorithms()
{
	for (int i = workingAlgorithms.size()-1; i >= 0;  i--)
	{
		if(workingAlgorithms[i]->isFinished())
		{
			delete workingAlgorithms[i];
			workingAlgorithms.erase(workingAlgorithms.begin()+i);
		}
	}
}

void MdiChild::cleanWorkingAlgorithms()
{
	unsigned int workingAlgorithmsSize = workingAlgorithms.size();
	for (unsigned int i=0; i<workingAlgorithmsSize; i++)
	{
		if(workingAlgorithms[i]->isRunning())
		{
			workingAlgorithms[i]->SafeTerminate();
			delete workingAlgorithms[i];
		}
	}
	workingAlgorithms.clear();
}

void MdiChild::resizeEvent( QResizeEvent * event )
{
	QMainWindow::resizeEvent(event);
}

bool MdiChild::addProfile()
{
	delete imgProfile;
	imgProfile = new dlg_profile(this, profileProbe->profileData, profileProbe->GetRayLength());
	tabifyDockWidget(logs, imgProfile);
	connect(imgProfile->profileMode, SIGNAL(toggled(bool)), this, SLOT(toggleArbitraryProfile(bool)));
	return true;
}

void MdiChild::toggleArbitraryProfile( bool isChecked )
{
	isArbProfileEnabled = (bool)isChecked;
	slicerXY->setArbitraryProfileOn(isArbProfileEnabled);
	slicerXZ->setArbitraryProfileOn(isArbProfileEnabled);
	slicerYZ->setArbitraryProfileOn(isArbProfileEnabled);
}

void MdiChild::UpdateProbe( int ptIndex, double * newPos )
{
	profileProbe->UpdateProbe(ptIndex, newPos);
	imgProfile->profileWidget->initialize(profileProbe->profileData, profileProbe->GetRayLength());
	imgProfile->profileWidget->redraw();
}

bool MdiChild::LoadCsvFile(vtkTable *table, FilterID fid)
{
	QString f = QFileDialog::getOpenFileName(this, tr("Open CSV File"), path, tr("CSV files (*.csv *.CSV)"));

	if(f.isEmpty())
	{
		addMsg(tr("Error loading csv file, file name not specified."));
		return false;
	}

	return LoadCsvFile(table, fid, f);
}

bool MdiChild::LoadCsvFile(vtkTable *table, FilterID fid, const QString &fileName)
{
	if(!QFile::exists(fileName))
	{
		addMsg(tr("Error loading csv file, file not exist."));
		return false;
	}

	iAIO* ioThread(new iAIO(imageData, polyData, m_logger, this));

	bool result = ioThread->loadCSVFile(table, fid, fileName);
	delete ioThread;
	return result;
}

bool MdiChild::LoadCsvFile( FilterID fid, const QString &fileName )
{
	return LoadCsvFile( mdCsvTable, fid, fileName );
}

int MdiChild::getSliceXY()
{
	return sXY->spinBoxXY->value();
}

int MdiChild::getSliceYZ()
{
	return sYZ->spinBoxYZ->value();
}

int MdiChild::getSliceXZ()
{
	return sXZ->spinBoxXZ->value();
}

QSpinBox * MdiChild::getSpinBoxXY()
{
	return sXY->spinBoxXY;
}

QSpinBox * MdiChild::getSpinBoxYZ()
{
	return sYZ->spinBoxYZ;
}

QSpinBox * MdiChild::getSpinBoxXZ()
{
	return sXZ->spinBoxXZ;
}

void MdiChild::maximizeDockWidget( QDockWidget * dw )
{
	m_beforeMaximizeState = this->saveState();
	QList<QDockWidget *> dockWidgets = findChildren<QDockWidget *>();
	for (int i=0; i<dockWidgets.size(); ++i)
	{
		QDockWidget * curDW = dockWidgets[i];
		if(curDW != dw)
			curDW->setVisible(false);
	}
	m_whatMaximized = dw;
	m_isSmthMaximized = true;
}

void MdiChild::demaximizeDockWidget( QDockWidget * dw )
{
	this->restoreState(m_beforeMaximizeState);
	m_isSmthMaximized = false;
}

void MdiChild::resizeDockWidget( QDockWidget * dw )
{
	if( m_isSmthMaximized && dw == m_whatMaximized )
		demaximizeDockWidget(dw);
	else
		maximizeDockWidget(dw);
}

void MdiChild::hideProgressBar()
{
	pbar->hide();
	pbar->setMaximum(m_pbarMaxVal);
}

void MdiChild::initProgressBar()
{
	updateProgressBar( pbar->minimum() );
}

void MdiChild::ioFinished()
{
	ioThread = 0;
}

iAHistogramWidget * MdiChild::getHistogram( dlg_histogram * h )
{
	if(!h)
		return 0;
	return h->histogram;
}

iASlicerData* MdiChild::getSlicerDataXZ()
{
	return slicerXZ->GetSlicerData();
}

iASlicerData* MdiChild::getSlicerDataXY()
{
	return slicerXY->GetSlicerData();
}

iASlicerData* MdiChild::getSlicerDataYZ()
{
	return slicerYZ->GetSlicerData();
}

iASlicer* MdiChild::getSlicerXZ()
{
	return slicerXZ;
}

iASlicer* MdiChild::getSlicerXY()
{
	return slicerXY;
}

iASlicer* MdiChild::getSlicerYZ()
{
	return slicerYZ;
}

dlg_sliceXY * MdiChild::getSlicerDlgXY()
{
	return sXY;
}

dlg_sliceXZ	* MdiChild::getSlicerDlgXZ()
{
	return sXZ;
}

dlg_sliceYZ	* MdiChild::getSlicerDlgYZ()
{
	return sYZ;
}

dlg_imageproperty * MdiChild::getImagePropertyDlg()
{
	return imgProperty;
}

dlg_histogram    * MdiChild::getHistogramDlg()
{
	return imgHistogram;
}

vtkTransform* MdiChild::getSlicerTransform()
{
	return slicerTransform;
}

void MdiChild::check2DMode()
{

	int arr[3];
	imageData->GetDimensions(arr);

	if (arr[0]==1 && arr[1]>1 && arr[2]>1){
		maximizeYZ();
	}

	else if (arr[0]>1 && arr[1]==1 && arr[2]>1){
		maximizeXZ();
	}

	else if (arr[0]>1 && arr[1]>1 && arr[2]==1){
		maximizeXY();
	}
}

void MdiChild::SetMagicLensInput(iAChannelID id, bool initReslice)
{
	slicerXY->SetMagicLensInput(id);
	slicerXZ->SetMagicLensInput(id);
	slicerYZ->SetMagicLensInput(id);

	if (initReslice)
	{
		slicerYZ->setResliceChannelAxesOrigin(id, static_cast<double>(sYZ->spinBoxYZ->value()) * imageData->GetSpacing()[0], 0, 0);
		slicerXZ->setResliceChannelAxesOrigin(id, 0, static_cast<double>(sXZ->spinBoxXZ->value()) * imageData->GetSpacing()[1], 0);
		slicerXY->setResliceChannelAxesOrigin(id, 0, 0, static_cast<double>(sXY->spinBoxXY->value()) * imageData->GetSpacing()[2]);
	}
}


void MdiChild::SetMagicLensEnabled(bool isOn)
{
	slicerXZ->SetMagicLensEnabled( isOn );
	slicerYZ->SetMagicLensEnabled( isOn );
	slicerXY->SetMagicLensEnabled( isOn );
}


void MdiChild::SetMagicLensCaption(std::string caption)
{
	slicerXZ->SetMagicLensCaption(caption);
	slicerYZ->SetMagicLensCaption(caption);
	slicerXY->SetMagicLensCaption(caption);
}

void MdiChild::reInitChannel(iAChannelID id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	iAChannelVisualizationData * chData = GetChannelData( id );
	if (!chData)
	{
		return;
	}
	chData->SetActiveImage( imgData );
	chData->SetColorTF( ctf );
	chData->SetOpacityTF( otf );
	slicerXZ->reInitializeChannel( id, chData );
	slicerXY->reInitializeChannel( id, chData );
	slicerYZ->reInitializeChannel( id, chData );	
}


void MdiChild::reInitMagicLens(iAChannelID id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf, std::string const & caption)
{
	if (!isMagicLensEnabled)
	{
		return;
	}
	reInitChannel(id, imgData, ctf, otf);
	SetMagicLensInput( id, true);
	SetMagicLensCaption( caption );
	updateSlicers();
}



std::vector<dlg_function*> & MdiChild::getFunctions()
{
	return getHistogram()->getFunctions();
}

iAHistogramWidget * MdiChild::getHistogram()
{
	return getHistogram( imgHistogram );
}

void MdiChild::redrawHistogram()
{
	getHistogram()->redraw();
}

void MdiChild::updateLayout()
{
	m_mainWnd->loadLayout();
}

void MdiChild::updateChannelMappers()
{
	getSlicerDataXY()->updateChannelMappers();
	getSlicerDataYZ()->updateChannelMappers();
	getSlicerDataXZ()->updateChannelMappers();
}

QString MdiChild::getFilePath() const
{
	return path;
}

iAVolumeStack * MdiChild::getVolumeStack()
{
	return volumeStack.data();
}

iALogger * MdiChild::getLogger()
{
	return m_logger;
}


bool MdiChild::IsOnlyPolyDataLoaded()
{
	return QString::compare(getFileInfo().suffix(), "STL", Qt::CaseInsensitive) == 0 ||
		QString::compare(getFileInfo().suffix(), "FEM", Qt::CaseInsensitive) == 0 && !(imageData->GetExtent()[1] > 0);
}


void MdiChild::ChangeModality(int chg)
{
	SetCurrentModality(
		(GetCurrentModality() + chg + GetModalitiesDlg()->GetModalities()->size())
		% (GetModalitiesDlg()->GetModalities()->size())
	);
	int curModIdx = GetCurrentModality();
	if (curModIdx < 0 || curModIdx >= GetModalitiesDlg()->GetModalities()->size())
	{
		DEBUG_LOG("Invalid modality index!");
		return;
	}
	ChangeImage(GetModalitiesDlg()->GetModalities()->Get(curModIdx)->GetImage(),
		GetModalitiesDlg()->GetModalities()->Get(curModIdx)->GetName().toStdString());
}

void MdiChild::ChangeMagicLensOpacity(int chg)
{
	iASlicerWidget * sliceWidget = dynamic_cast<iASlicerWidget *>(sender());
	if (!sliceWidget)
	{
		DEBUG_LOG("Invalid slice widget sender!");
		return;
	}
	sliceWidget->SetMagicLensOpacity(sliceWidget->GetMagicLensOpacity() + (chg*0.05));
}


int MdiChild::GetCurrentModality() const
{
	return m_currentModality;
}


void MdiChild::SetCurrentModality(int modality)
{
	m_currentModality = modality;
}


void MdiChild::ChangeImage(vtkSmartPointer<vtkImageData> img)
{
	int selected = m_dlgModalities->GetSelected();
	if (selected != -1)
	{
		m_currentModality = selected;
		ChangeImage(img, m_dlgModalities->GetModalities()->Get(selected)->GetName().toStdString());
	}
}

void MdiChild::ChangeImage(vtkSmartPointer<vtkImageData> img, std::string const & caption)
{
	if (!isMagicLensToggled())
	{
		return;
	}
	reInitMagicLens(ch_ModalityLens, img,
		m_dlgModalities->GetCTF(m_currentModality),
		m_dlgModalities->GetOTF(m_currentModality), caption);
}

void MdiChild::SetModalities(QSharedPointer<iAModalityList> modList)
{
	bool noDataLoaded = m_dlgModalities->GetModalities()->size() > 0;
	return m_dlgModalities->SetModalities(modList);

	if (noDataLoaded && m_dlgModalities->GetModalities()->size() > 0)
	{
		// TODO: avoid Duplication (LoadModalities!)
		setImageData(
			GetModalitiesDlg()->GetModalities()->Get(0)->GetFileName(),
			GetModalitiesDlg()->GetModalities()->Get(0)->GetImage()
		);
	}
}


/*
void MdiChild::RenderSettingsChanged()
{
	m_dlgModalities->ChangeRenderSettings(GetRenderSettings());
}

void MdiChild::preferencesChanged()
{
	m_dlgModalities->ChangeMagicLensSize(GetMagicLensSize());
}
*/

dlg_modalities* MdiChild::GetModalitiesDlg()
{
	return m_dlgModalities;
}

void MdiChild::LoadModalities()
{
	bool noDataLoaded = m_dlgModalities->GetModalities()->size() > 0;
	m_dlgModalities->Load();
	if (noDataLoaded && m_dlgModalities->GetModalities()->size() > 0)
	{
		setImageData(
			GetModalitiesDlg()->GetModalities()->Get(0)->GetFileName(),
			GetModalitiesDlg()->GetModalities()->Get(0)->GetImage()
		);
	}
}
