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
#include "mdichild.h"

#include "charts/iAHistogramData.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAProfileWidget.h"
#include "dlg_commoninput.h"
#include "dlg_imageproperty.h"
#include "dlg_modalities.h"
#include "dlg_profile.h"
#include "dlg_transfer.h"
#include "dlg_volumePlayer.h"
#include "iAAlgorithm.h"
#include "iAChannelVisualizationData.h"
#include "iAChildData.h"
#include "iAConsole.h"
#include "iADockWidgetWrapper.h"
#include "iALogger.h"
#include "iAMdiChildLogger.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAModalityTransfer.h"
#include "iAMovieHelper.h"
#include "iAParametricSpline.h"
#include "iAPreferences.h"
#include "iAProfileProbe.h"
#include "iAProgress.h"
#include "iARenderer.h"
#include "iARenderObserver.h"
#include "iARenderSettings.h"
#include "iASlicer.h"
#include "iASlicerData.h"
#include "iASlicerWidget.h"
#include "iAToolsVTK.h"
#include "iATransferFunction.h"
#include "iAVolumeStack.h"
#include "iAWidgetAddHelper.h"
#include "io/extension2id.h"
#include "io/iAIO.h"
#include "io/iAIOProvider.h"
#include "mainwindow.h"

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCornerAnnotation.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageExtractComponents.h>
#include <vtkImageReslice.h>
#include <vtkMath.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPlane.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>

// TODO: refactor methods using the following out of mdichild!
#include <vtkTransform.h>

// TODO: VOLUME: check all places using GetModality(0)->GetTransfer() !

#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QSpinBox>
#include <QToolButton>

MdiChild::MdiChild(MainWindow * mainWnd, iAPreferences const & prefs, bool unsavedChanges) :
	m_isSmthMaximized(false),
	volumeStack(new iAVolumeStack),
	isMagicLensEnabled(false),
	ioThread(nullptr),
	reInitializeRenderWindows(true),
	m_logger(new MdiChildLogger(this)),
	m_histogram(new iADiagramFctWidget(nullptr, this, " Histogram")),
	m_histogramContainer(new iADockWidgetWrapper(m_histogram, "Histogram", "Histogram")),
	m_initVolumeRenderers(false),
	preferences(prefs),
	m_currentModality(0),
	m_currentComponent(0),
	m_currentHistogramModality(-1)
{
	setWindowModified(unsavedChanges);
	m_mainWnd = mainWnd;
	setupUi(this);
	//prepare window for handling dock widgets
	this->setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

	//insert default dock widgets and arrange them in a simple layout
	renderer = new dlg_renderer(this);
	sXY = new dlg_sliceXY(this);
	sXZ = new dlg_sliceXZ(this);
	sYZ = new dlg_sliceYZ(this);

	pbar = new QProgressBar(this);
	pbar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	pbar->setMaximumSize(350, 17);
	this->statusBar()->addPermanentWidget(pbar);
	m_pbarMaxVal = pbar->maximum();
	logs = new dlg_logs(this);
	addDockWidget(Qt::LeftDockWidgetArea, renderer);
	m_initialLayoutState = saveState();

	splitDockWidget(renderer, logs, Qt::Vertical);
	splitDockWidget(renderer, sXZ, Qt::Horizontal);
	splitDockWidget(renderer, sYZ, Qt::Vertical);
	splitDockWidget(sXZ, sXY, Qt::Vertical);

	setAttribute(Qt::WA_DeleteOnClose);

	isUntitled = true;
	visibility = MULTI;
	xCoord = 0, yCoord = 0, zCoord = 0;

	imageData = vtkSmartPointer<vtkImageData>::New();
	imageData->AllocateScalars(VTK_DOUBLE, 1);
	polyData = vtkPolyData::New();

	axesTransform = vtkTransform::New();
	slicerTransform = vtkTransform::New();

	slicer[iASlicerMode::YZ] = new iASlicer(this, iASlicerMode::YZ, sYZ->sliceWidget);
	slicer[iASlicerMode::XY] = new iASlicer(this, iASlicerMode::XY, sXY->sliceWidget);
	slicer[iASlicerMode::XZ] = new iASlicer(this, iASlicerMode::XZ, sXZ->sliceWidget);
	
	sYZ->sliceWidget->setStyleSheet(QString("#sliceWidget { border: %1px solid rgb(255, 0  , 0  ) } ").arg(iASlicerWidget::BorderWidth));
	sXY->sliceWidget->setStyleSheet(QString("#sliceWidget { border: %1px solid rgb(0  , 0  , 255) } ").arg(iASlicerWidget::BorderWidth));
	sXZ->sliceWidget->setStyleSheet(QString("#sliceWidget { border: %1px solid rgb(0  , 255, 0  ) } ").arg(iASlicerWidget::BorderWidth));
	
	Raycaster = new iARenderer(this);
	Raycaster->setAxesTransform(axesTransform);

	m_dlgModalities = new dlg_modalities(renderer->vtkWidgetRC, Raycaster->GetRenderer(), preferences.HistogramBins);
	QSharedPointer<iAModalityList> modList(new iAModalityList);
	SetModalities(modList);
	splitDockWidget(logs, m_dlgModalities, Qt::Horizontal);
	m_dlgModalities->SetSlicePlanes(Raycaster->getPlane1(), Raycaster->getPlane2(), Raycaster->getPlane3());
	ApplyViewerPreferences();
	imgProperty = nullptr;
	imgProfile = nullptr;
	SetRenderWindows();
	connectSignalsToSlots();
	pbar->setValue(100);

	snakeSlicer = false;
	isSliceProfileEnabled = false;
	isArbProfileEnabled = false;

	profileWidgetIndex = -1;
	worldSnakePoints = vtkPoints::New();
	parametricSpline = iAParametricSpline::New();
	parametricSpline->SetPoints(worldSnakePoints);

	sXY->spinBoxXY->setRange(-8192, 8192);
	sXZ->spinBoxXZ->setRange(-8192,8192);
	sYZ->spinBoxYZ->setRange(-8192,8192);

	sXY->sbinBoxSlabThicknessXY->hide();
	sXY->labelSlabThicknessXY->hide();
	sXZ->sbinBoxSlabThicknessXZ->hide();
	sXZ->labelSlabThicknessXZ->hide();
	sYZ->sbinBoxSlabThicknessYZ->hide();
	sYZ->labelSlabThicknessYZ->hide();

	sXY->comboBoxSlabCompositeModeXY->hide();
	sXZ->comboBoxSlabCompositeModeXZ->hide();
	sYZ->comboBoxSlabCompositeModeYZ->hide();

	worldProfilePoints = vtkPoints::New();
	worldProfilePoints->Allocate(2);

	updateSliceIndicator = true;
	raycasterInitialized = false;
}

MdiChild::~MdiChild()
{
	cleanWorkingAlgorithms();
	polyData->ReleaseData();
	axesTransform->Delete();
	slicerTransform->Delete();

	polyData->Delete();

	for (int s=0; s<3; ++s)
		delete slicer[s];
	delete Raycaster; Raycaster = nullptr;

	if(imgProperty)		delete imgProperty;
	if(imgProfile)		delete imgProfile;
}

void MdiChild::connectSignalsToSlots()
{
	connect(renderer->pushMaxRC, SIGNAL(clicked()), this, SLOT(maximizeRC()));
	connect(sXY->pushMaxXY, SIGNAL(clicked()), this, SLOT(maximizeXY()));
	connect(sXZ->pushMaxXZ, SIGNAL(clicked()), this, SLOT(maximizeXZ()));
	connect(sYZ->pushMaxYZ, SIGNAL(clicked()), this, SLOT(maximizeYZ()));

	connect(sXY->pushStopXY, SIGNAL(clicked()), this, SLOT(triggerInteractionXY()));
	connect(sXZ->pushStopXZ, SIGNAL(clicked()), this, SLOT(triggerInteractionXZ()));
	connect(sYZ->pushStopYZ, SIGNAL(clicked()), this, SLOT(triggerInteractionYZ()));
	connect(renderer->pushStopRC, SIGNAL(clicked()), this, SLOT(triggerInteractionRaycaster()));

	connect(renderer->pushPX, SIGNAL(clicked()), this, SLOT(camPX()));
	connect(renderer->pushPY, SIGNAL(clicked()), this, SLOT(camPY()));
	connect(renderer->pushPZ, SIGNAL(clicked()), this, SLOT(camPZ()));
	connect(renderer->pushMX, SIGNAL(clicked()), this, SLOT(camMX()));
	connect(renderer->pushMY, SIGNAL(clicked()), this, SLOT(camMY()));
	connect(renderer->pushMZ, SIGNAL(clicked()), this, SLOT(camMZ()));
	connect(renderer->pushIso, SIGNAL(clicked()), this, SLOT(camIso()));
	connect(renderer->pushSaveRC, SIGNAL(clicked()), this, SLOT(saveRC()));
	connect(sXY->pushSaveXY, SIGNAL(clicked()), this, SLOT(saveXY()));
	connect(sXZ->pushSaveXZ, SIGNAL(clicked()), this, SLOT(saveXZ()));
	connect(sYZ->pushSaveYZ, SIGNAL(clicked()), this, SLOT(saveYZ()));

	connect(sXY->pushSaveStackXY, SIGNAL(clicked()), this, SLOT(saveStackXY()));
	connect(sXZ->pushSaveStackXZ, SIGNAL(clicked()), this, SLOT(saveStackXZ()));
	connect(sYZ->pushSaveStackYZ, SIGNAL(clicked()), this, SLOT(saveStackYZ()));

	connect(sXY->pushMovXY, SIGNAL(clicked()), this, SLOT(saveMovXY()));
	connect(sXZ->pushMovXZ, SIGNAL(clicked()), this, SLOT(saveMovXZ()));
	connect(sYZ->pushMovYZ, SIGNAL(clicked()), this, SLOT(saveMovYZ()));
	connect(renderer->pushMovRC, SIGNAL(clicked()), this, SLOT(saveMovRC()));

	connect(logs->pushClearLogs, SIGNAL(clicked()), this, SLOT(clearLogs()));

	connect(renderer->vtkWidgetRC, SIGNAL(rightButtonReleasedSignal()), Raycaster, SLOT(mouseRightButtonReleasedSlot()));
	connect(renderer->vtkWidgetRC, SIGNAL(leftButtonReleasedSignal()), Raycaster, SLOT(mouseLeftButtonReleasedSlot()));
	connect(renderer->spinBoxRC, SIGNAL(valueChanged(int)), this, SLOT(setChannel(int)));

	connect(sXY->spinBoxXY, SIGNAL(valueChanged(int)), this, SLOT(setSliceXYSpinBox(int)));
	connect(sXZ->spinBoxXZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceXZSpinBox(int)));
	connect(sYZ->spinBoxYZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceYZSpinBox(int)));

	connect(sXY->verticalScrollBarXY, SIGNAL(valueChanged(int)), this, SLOT(setSliceXYScrollBar(int)));
	connect(sXZ->verticalScrollBarXZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceXZScrollBar(int)));
	connect(sYZ->verticalScrollBarYZ, SIGNAL(valueChanged(int)), this, SLOT(setSliceYZScrollBar(int)));

	connect(sXY->doubleSpinBoxXY, SIGNAL(valueChanged(double)), this, SLOT(setRotationXY(double)));
	connect(sXZ->doubleSpinBoxXZ, SIGNAL(valueChanged(double)), this, SLOT(setRotationXZ(double)));
	connect(sYZ->doubleSpinBoxYZ, SIGNAL(valueChanged(double)), this, SLOT(setRotationYZ(double)));

	connect(sXY->checkBoxSlabModeXY, SIGNAL(toggled(bool)), this, SLOT(setSlabModeXY(bool)));
	connect(sXY->sbinBoxSlabThicknessXY, SIGNAL(valueChanged(int)), this, SLOT(updateSlabThicknessXY(int)));
	connect(sXZ->checkBoxSlabModeXZ, SIGNAL(toggled(bool)), this, SLOT(setSlabModeXZ(bool)));
	connect(sXZ->sbinBoxSlabThicknessXZ, SIGNAL(valueChanged(int)), this, SLOT(updateSlabThicknessXZ(int)));
	connect(sYZ->checkBoxSlabModeYZ, SIGNAL(toggled(bool)), this, SLOT(setSlabModeYZ(bool)));
	connect(sYZ->sbinBoxSlabThicknessYZ, SIGNAL(valueChanged(int)), this, SLOT(updateSlabThicknessYZ(int)));

	connect(sXY->comboBoxSlabCompositeModeXY, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSlabCompositeModeXY(int)));
	connect(sXZ->comboBoxSlabCompositeModeXZ, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSlabCompositeModeXZ(int)));
	connect(sYZ->comboBoxSlabCompositeModeYZ, SIGNAL(currentIndexChanged(int)), this, SLOT(updateSlabCompositeModeYZ(int)));

	for (int s = 0; s < 3; ++s)
	{
		connect(slicer[s]->widget(), SIGNAL(shiftMouseWheel(int)), this, SLOT(ChangeMagicLensModality(int)));
		connect(slicer[s]->widget(), SIGNAL(altMouseWheel(int)), this, SLOT(ChangeMagicLensOpacity(int)));
	}

	connect(m_histogram, SIGNAL(updateViews()), this, SLOT(updateViews()));
	connect(m_histogram, SIGNAL(pointSelected()), this, SIGNAL(pointSelected()));
	connect(m_histogram, SIGNAL(noPointSelected()), this, SIGNAL(noPointSelected()));
	connect(m_histogram, SIGNAL(endPointSelected()), this, SIGNAL(endPointSelected()));
	connect(m_histogram, SIGNAL(active()), this, SIGNAL(active()));
	connect((dlg_transfer*)(m_histogram->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(ModalityTFChanged()));

	connect(m_dlgModalities, SIGNAL(ModalitiesChanged()), this, SLOT(updateImageProperties()));
	connect(m_dlgModalities, SIGNAL(ModalitySelected(int)), this, SLOT(ShowModality(int)));
}

void MdiChild::connectThreadSignalsToChildSlots( iAAlgorithm* thread )
{
	connect(thread, SIGNAL( startUpdate(int) ), this, SLOT( updateRenderWindows(int) ));
	connect(thread, SIGNAL( finished() ), this, SLOT( enableRenderWindows() ));
	connectAlgorithmSignalsToChildSlots(thread);
}

void MdiChild::connectIOThreadSignals(iAIO * thread)
{
	connectAlgorithmSignalsToChildSlots(thread);
	connect(thread, SIGNAL(finished()), this, SLOT(ioFinished()));
}

void MdiChild::connectAlgorithmSignalsToChildSlots(iAAlgorithm* thread)
{
	connect(thread, SIGNAL(aprogress(int)), this, SLOT(updateProgressBar(int)));
	connect(thread, SIGNAL(started()), this, SLOT(initProgressBar()));
	connect(thread, SIGNAL(finished()), this, SLOT(hideProgressBar()));
	addAlgorithm(thread);
}

void MdiChild::addAlgorithm(iAAlgorithm* thread)
{
	workingAlgorithms.push_back(thread);
	connect(thread, SIGNAL(finished()), this, SLOT(removeFinishedAlgorithms()));
}

void MdiChild::SetRenderWindows()
{
	renderer->vtkWidgetRC->SetMainRenderWindow((vtkGenericOpenGLRenderWindow*)Raycaster->GetRenderWindow());
}

void MdiChild::updateRenderWindows(int channels)
{
	if (channels > 1)
	{
		renderer->spinBoxRC->setRange(0, channels-1);
		renderer->stackedWidgetRC->setCurrentIndex(1);
		renderer->channelLabelRC->setEnabled(true);
	}
	else
	{
		renderer->stackedWidgetRC->setCurrentIndex(0);
		renderer->channelLabelRC->setEnabled(false);
	}
	disableRenderWindows(0);
}

void MdiChild::disableRenderWindows(int ch)
{
	for (int s = 0; s<3; ++s)
		slicer[s]->disableInteractor();
	Raycaster->disableInteractor();
	emit rendererDeactivated(ch);
}

void MdiChild::enableRenderWindows()	// = image data available
{
	if (IsVolumeDataLoaded() && reInitializeRenderWindows)
	{
		Raycaster->enableInteractor();
		for (int s = 0; s<3; ++s)
			slicer[s]->enableInteractor();
		updateViews();
		updateImageProperties();
		if (imageData->GetNumberOfScalarComponents() == 1) //No histogram/profile for rgb, rgba or vector pixel type images
		{
			SetHistogramModality(0);
			UpdateProfile();
		}
		else
		{
			InitVolumeRenderers();
			QSharedPointer<iAModalityTransfer> modTrans = GetModality(0)->GetTransfer();
			for (int s = 0; s < 3; ++s)
			{
				slicer[s]->reInitialize(GetModality(0)->GetImage(), slicerTransform, modTrans->GetColorFunction());
				slicer[s]->GetSlicerData()->ResetCamera();
			}
		}
	}
	// set to true for next time, in case it is false now (i.e. default to always reinitialize,
	// unless explicitly set otherwise)
	reInitializeRenderWindows = true;

	Raycaster->reInitialize(imageData, polyData);

	if (!IsVolumeDataLoaded())
		return;
	if (updateSliceIndicator)
	{
		updateSliceIndicators();
		camIso();
		vtkCamera* cam = Raycaster->getCamera();
		GetModalities()->ApplyCameraSettings(cam);
	}
	else
		updateSliceIndicator = true;

	QList<iAChannelID> keys = m_channels.keys();
	for (QList<iAChannelID>::iterator it = keys.begin();
		it != keys.end();
		++it)
	{
		iAChannelID key = *it;
		iAChannelVisualizationData * chData = m_channels.value(key).data();
		if (chData->IsEnabled()
			|| (isMagicLensEnabled && (
				key == slicer[iASlicerMode::XY]->getMagicLensInput() ||
				key == slicer[iASlicerMode::XZ]->getMagicLensInput() ||
				key == slicer[iASlicerMode::YZ]->getMagicLensInput()
				))
			)
		{
			for (int s = 0; s<3; ++s)
				slicer[s]->reInitializeChannel(key, chData);
		}
	}
	m_dlgModalities->EnableUI();
}

void MdiChild::ModalityTFChanged()
{
	updateChannelMappers();
	for (int s = 0; s<3; ++s)
		slicer[s]->UpdateMagicLensColors();
	emit TransferFunctionChanged();
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
	if (slicerSettings.LinkViews) {
		xCoord = x; yCoord = y; zCoord = z;
		if (mode != iASlicerMode::XZ) {
			if (slicerSettings.SingleSlicer.ShowPosition) {
				slicer[iASlicerMode::XZ]->setPositionMarkerCenter(x*spacing[0], z*spacing[2]);
			}
			slicer[iASlicerMode::XZ]->setIndex(x,y,z);
			sXZ->spinBoxXZ->setValue(y);
		}
		if (mode != iASlicerMode::YZ) {
			if (slicerSettings.SingleSlicer.ShowPosition)
				slicer[iASlicerMode::YZ]->setPositionMarkerCenter(y*spacing[1], z*spacing[2]);
			slicer[iASlicerMode::YZ]->setIndex(x,y,z);
			sYZ->spinBoxYZ->setValue(x);
		}
		if (mode != iASlicerMode::XY) {
			if (slicerSettings.SingleSlicer.ShowPosition)
				slicer[iASlicerMode::XY]->setPositionMarkerCenter(x*spacing[0], y*spacing[1]);
			slicer[iASlicerMode::XY]->setIndex(x,y,z);
			sXY->spinBoxXY->setValue(z);
		}
		if (renderSettings.ShowRPosition)
			Raycaster->setCubeCenter(x, y, z);
	}
}

void MdiChild::showPoly()
{
	hideVolumeWidgets();
	setVisibility(QList<QWidget*>() << renderer->stackedWidgetRC << renderer->pushSaveRC << renderer->pushMaxRC << renderer->pushStopRC, true);

	renderer->vtkWidgetRC->setGeometry(0, 0, 300, 200);
	renderer->vtkWidgetRC->setMaximumSize(QSize(16777215, 16777215));
	renderer->vtkWidgetRC->adjustSize();
	renderer->show();
	visibility &= (RC | TAB);
	changeVisibility(visibility);
}

bool MdiChild::displayResult(QString const & title, vtkImageData* image, vtkPolyData* poly)	// = opening new window
{
	// TODO: image is actually not the final imagedata here (or at least not always)
	//    -> maybe skip all image-related initializations?
	addStatusMsg("Creating Result View");
	if (poly) {
		polyData->ReleaseData();
		polyData->DeepCopy(poly);
	}

	if (image) {
		imageData->ReleaseData();
		imageData->DeepCopy(image);
	}

	initView( title );
	setWindowTitle( title );
	Raycaster->ApplySettings(renderSettings);
	setupSlicers(slicerSettings, true );

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


void MdiChild::PrepareForResult()
{
	setWindowModified(true);
	GetModality(0)->GetTransfer()->Reset();
}


bool MdiChild::setupLoadIO(QString const & f, bool isStack)
{
	polyData->ReleaseData();
	// TODO: insert plugin mechanism.
	// - iterate over file plugins; if one returns a match, use it
	if (QString::compare(fileInfo.suffix(), "STL", Qt::CaseInsensitive) == 0)
	{
		return ioThread->setupIO(STL_READER, f);
	}
	imageData->ReleaseData();
	QString extension = fileInfo.suffix();
	extension = extension.toUpper();
	const mapQString2int * ext2id = &extensionToId;
	if(isStack)	ext2id = &extensionToIdStack;
	if (ext2id->find(extension) == ext2id->end())
	{
		return false;
	}
	IOType id = ext2id->find( extension ).value();
	return ioThread->setupIO(id, f);
}


bool MdiChild::loadRaw(const QString &f)
{
	if (!QFile::exists(f))	return false;
	addMsg(tr("%1  Loading file '%2', please wait...")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(f));
	setCurrentFile(f);
	waitForPreviousIO();
	ioThread = new iAIO(imageData, nullptr, m_logger, this);
	connect(ioThread, SIGNAL(done(bool)), this, SLOT(setupView(bool)));
	connectIOThreadSignals(ioThread);
	connect(ioThread, SIGNAL(done()), this, SLOT(enableRenderWindows()));
	polyData->ReleaseData();
	imageData->ReleaseData();
	IOType id = RAW_READER;
	if (!ioThread->setupIO(id, f))
	{
		ioFinished();
		return false;
	}
	ioThread->start();
	return true;
}

namespace
{
	bool Is2DImageFile(QString const & f)
	{
		return f.endsWith("bmp", Qt::CaseInsensitive) ||
			f.endsWith("tif", Qt::CaseInsensitive) ||
			f.endsWith("tiff", Qt::CaseInsensitive) ||
			f.endsWith("jpg", Qt::CaseInsensitive);
	}
}

bool MdiChild::loadFile(const QString &f, bool isStack)
{
	if(!QFile::exists(f))
		return false;

	addMsg(tr("%1  Loading file '%2', please wait...")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(f));
	setCurrentFile(f);

	waitForPreviousIO();

	ioThread = new iAIO(imageData, polyData, m_logger, this, volumeStack->GetVolumes(), volumeStack->GetFileNames());
	if (f.endsWith(iAIOProvider::ProjectFileExtension))
	{
		connect(ioThread, SIGNAL(done(bool)), this, SLOT(setupProject(bool)));
	}
	else
	{
		if (!isStack || Is2DImageFile(f)) {
			connect(ioThread, SIGNAL(done(bool)), this, SLOT(setupView(bool)));
		}
		else {
			connect(ioThread, SIGNAL(done(bool)), this, SLOT(setupStackView(bool)));
		}
		connect(ioThread, SIGNAL(done()), this, SLOT(enableRenderWindows()));
	}
	connectIOThreadSignals(ioThread);
	connect(m_dlgModalities, SIGNAL(ModalityAvailable(int)), this, SLOT(ModalityAdded(int)));
	connect(ioThread, SIGNAL(done()), this, SIGNAL(fileLoaded()));

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
	imageData = imgData;
	GetModality(0)->SetData(imageData);
	m_mainWnd->setCurrentFile(GetModalities()->GetFileName());
	setupView(false);
	enableRenderWindows();
}


void MdiChild::setImageData(vtkImageData * iData)
{
	imageData = iData;		// potential for double free!
}


vtkImageData* MdiChild::getImageData()
{
	return imageData;
}


bool MdiChild::updateVolumePlayerView(int updateIndex, bool isApplyForAll)
{
	// TODO: VOLUME: Test!!! copy from currently selected instead of fixed 0 index?
	vtkColorTransferFunction* colorTransferFunction = GetModality(0)->GetTransfer()->GetColorFunction();
	vtkPiecewiseFunction* piecewiseFunction = GetModality(0)->GetTransfer()->GetOpacityFunction();
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

	SetHistogramModality(0);

	Raycaster->reInitialize(imageData, polyData);
	for (int s = 0; s<3; ++s)
		slicer[s]->reInitialize(imageData, slicerTransform, colorTransferFunction);
	updateViews();

	if (CheckedList.at(updateIndex)!=0) {
		enableRenderWindows();
	}

	return true;
}


void MdiChild::setupStackView(bool active)
{
	// TODO: check!
	previousIndexOfVolume = 0;

	int numberOfVolumes=volumeStack->getNumberOfVolumes();

	if (numberOfVolumes == 0)
	{
		DEBUG_LOG("Invalid call to setupStackView: No Volumes loaded!");
	}

	int currentIndexOfVolume=0;

	imageData->DeepCopy(volumeStack->getVolume(currentIndexOfVolume));
	setupViewInternal(active);
	for (int i=0; i<numberOfVolumes; i++) {
		vtkSmartPointer<vtkColorTransferFunction> cTF = GetDefaultColorTransferFunction(imageData->GetScalarRange());
		vtkSmartPointer<vtkPiecewiseFunction> pWF = GetDefaultPiecewiseFunction(imageData->GetScalarRange(), imageData->GetNumberOfScalarComponents() == 1);
		volumeStack->addColorTransferFunction(cTF);
		volumeStack->addPiecewiseFunction(pWF);
	}

	QSharedPointer<iAModalityTransfer> modTrans = GetModality(0)->GetTransfer();
	if (numberOfVolumes > 0) {
		modTrans->GetColorFunction()->DeepCopy(volumeStack->getColorTransferFunction(0));
		modTrans->GetOpacityFunction()->DeepCopy(volumeStack->getPiecewiseFunction(0));
	}
	addVolumePlayer(volumeStack.data());

	Raycaster->reInitialize(imageData, polyData);
	for (int s = 0; s<3; ++s)
		slicer[s]->reInitialize(imageData, slicerTransform, modTrans->GetColorFunction());
	updateViews();

	Raycaster->update();
}


void MdiChild::setupViewInternal(bool active)
{
	if (!imageData)
	{
		DEBUG_LOG("Image Data is not set!");
		return;
	}
	if (!active)
		initView(curFile.isEmpty() ? "Untitled":"" );

	m_mainWnd->setCurrentFile(currentFile());	// TODO: VOLUME: should be done on the outside? or where setCurrentFile is done?

	if ((imageData->GetExtent()[1] < 3) || (imageData->GetExtent()[3]) < 3 || (imageData->GetExtent()[5] < 3))
		volumeSettings.Shading = false;

	volumeSettings.SampleDistance = imageData->GetSpacing()[0];
	Raycaster->ApplySettings(renderSettings);
	setupSlicers(slicerSettings, true);

	if (imageData->GetExtent()[1] <= 1)
		visibility &= (YZ | TAB);
	else if (imageData->GetExtent()[3] <= 1)
		visibility &= (XZ | TAB);
	else if (imageData->GetExtent()[5] <= 1)
		visibility &= (XY | TAB);

	if (active) changeVisibility(visibility);

	if (imageData->GetNumberOfScalarComponents() > 1 &&
		imageData->GetNumberOfScalarComponents() < 4 )
	{
		renderer->spinBoxRC->setRange(0, imageData->GetNumberOfScalarComponents() - 1);
		renderer->stackedWidgetRC->setCurrentIndex(1);
		renderer->channelLabelRC->setEnabled(true);
	}
	else
	{
		renderer->stackedWidgetRC->setCurrentIndex(0);
		renderer->channelLabelRC->setEnabled(false);
	}
}


void MdiChild::setupView(bool active )
{
	setupViewInternal(active);
	Raycaster->update();
	check2DMode();
}


void MdiChild::setupProject(bool active)
{
	SetModalities(ioThread->GetModalities());
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


int MdiChild::chooseModalityNr(QString const & caption)
{
	if (GetModalities()->size() == 1)
	{
		return 0;
	}
	QStringList parameters = (QStringList() << tr("+Channel"));
	QStringList modalities;
	for (int i = 0; i < GetModalities()->size(); ++i)
	{
		modalities << GetModality(i)->GetName();
	}
	QList<QVariant> values = (QList<QVariant>() << modalities);
	dlg_commoninput modalityChoice(this, caption, parameters, values, nullptr);
	if (modalityChoice.exec() != QDialog::Accepted)
	{
		return -1;
	}
	return modalityChoice.getComboBoxIndex(0);
}

int MdiChild::chooseComponentNr(int modalityNr)
{
	int nrOfComponents = GetModality(modalityNr)->GetImage()->GetNumberOfScalarComponents();
	if (nrOfComponents == 1)
	{
		return 0;
	}
	QStringList parameters = (QStringList() << tr("+Component"));
	QStringList components;
	for (int i = 0; i < nrOfComponents; ++i)
	{
		components << QString::number(i);
	}
	components << "All components";
	QList<QVariant> values = (QList<QVariant>() << components);
	dlg_commoninput componentChoice(this, "Choose Component", parameters, values, nullptr);
	if (componentChoice.exec() != QDialog::Accepted)
	{
		return -1;
	}
	return componentChoice.getComboBoxIndex(0);
}

bool MdiChild::save()
{
	if (isUntitled)
	{
		return saveAs();
	}
	else
	{
		int modalityNr = chooseModalityNr();
		if (modalityNr == -1)
		{
			return false;
		}
		/*
		// choice: save single modality, or modality stack!
		if (GetModality(modalityNr)->ComponentCount() > 1)
		{                         // should be ChannelCount()
		}
		*/
		int componentNr = chooseComponentNr(modalityNr);
		if (componentNr == -1)
		{
			return false;
		}

		return saveFile(GetModality(modalityNr)->GetFileName(), modalityNr, componentNr);
	}
}

bool MdiChild::saveAs()
{
	// TODO: unify with saveFile second part
	int modalityNr = chooseModalityNr();
	if (modalityNr == -1)
	{
		return false;
	}
	return saveAs(modalityNr);
}

bool MdiChild::saveAs(int modalityNr)
{
	int componentNr = chooseComponentNr(modalityNr);
	if (componentNr == -1)
	{
		return false;
	}
	QString filePath = QFileInfo(GetModality(modalityNr)->GetFileName()).absolutePath();
	QString f = QFileDialog::getSaveFileName(
		this,
		tr("Save As"),
		filePath,
		iAIOProvider::GetSupportedSaveFormats() +
		tr(";;TIFF stack (*.tif);; PNG stack (*.png);; BMP stack (*.bmp);; JPEG stack (*.jpg);; DICOM serie (*.dcm)"));

	if (f.isEmpty())
	{
		return false;
	}
	return saveFile(f, modalityNr, componentNr);
}

void MdiChild::waitForPreviousIO()
{
	if (ioThread)
	{
		addMsg(tr("%1  Waiting for I/O operation to complete...").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
		ioThread->wait();
		ioThread = nullptr;
	}
}


QString GetSupportedPixelTypeString(QVector<int> const & types)
{
	QString result;
	for (int i = 0; i < types.size(); ++i)
	{
		switch (types[i])
		{
		case VTK_UNSIGNED_CHAR: result += "unsigned char"; break;
		case VTK_UNSIGNED_SHORT: result += "unsigned short"; break;
		case VTK_FLOAT: result += "float"; break;
		}
		if (i < types.size() - 2)
		{
			result += ", ";
		}
		else if (i < types.size() - 1)
		{
			result += " and ";
		}
	}
	return result;
}

bool MdiChild::setupSaveIO(QString const & f)
{
	QFileInfo fileInfo(f);
	if (QString::compare(fileInfo.suffix(), "STL", Qt::CaseInsensitive) == 0) {
		if (polyData->GetNumberOfPoints() <= 1)	{
			QMessageBox::warning(this, tr("Save File"), tr("Model contains no data. Saving aborted."));
			return false;
		} else {
			if ( !ioThread->setupIO(STL_WRITER, fileInfo.absoluteFilePath() ) ) return false;
		}
	} else {
		if (!IsVolumeDataLoaded()) {
			QMessageBox::warning(this, tr("Save File"), tr("Image contains no data. Saving aborted.")); return false;
		} else {
			if ((QString::compare(fileInfo.suffix(), "MHD", Qt::CaseInsensitive) == 0) ||
				(QString::compare(fileInfo.suffix(), "MHA", Qt::CaseInsensitive) == 0))
			{
					if ( !ioThread->setupIO(MHD_WRITER, fileInfo.absoluteFilePath(), preferences.Compression) )
						return false;
					setCurrentFile(f);
					m_mainWnd->setCurrentFile(f);	// TODO: VOLUME: do in setCurrentFile member method?
					QString t; t = f;
					t.truncate(t.lastIndexOf('/'));
					m_mainWnd->setPath(t);
			}
			else
			{
				QMap<IOType, QVector<int> > supportedPixelTypes;
				QVector<int> tiffSupported;
				tiffSupported.push_back(VTK_UNSIGNED_CHAR);
				tiffSupported.push_back(VTK_UNSIGNED_SHORT);
				tiffSupported.push_back(VTK_FLOAT);
				supportedPixelTypes.insert(TIF_STACK_WRITER, tiffSupported);
				QVector<int> pngJpgBmpSupported;
				pngJpgBmpSupported.push_back(VTK_UNSIGNED_CHAR);
				supportedPixelTypes.insert(BMP_STACK_WRITER, pngJpgBmpSupported);
				supportedPixelTypes.insert(PNG_STACK_WRITER, pngJpgBmpSupported);
				supportedPixelTypes.insert(JPG_STACK_WRITER, pngJpgBmpSupported);

				QString suffix = fileInfo.suffix().toUpper();
				if (!extensionToSaveId.contains(suffix))
				{
					return false;
				}
				IOType ioID = extensionToSaveId[suffix];
				if (supportedPixelTypes.contains(ioID) &&
					!supportedPixelTypes[ioID].contains(imageData->GetScalarType()))
				{
					addMsg(QString("%1  Writer only supports %2 input!")
						.arg(suffix)
						.arg(GetSupportedPixelTypeString(supportedPixelTypes[ioID])));
					return false;
				}
				if (!ioThread->setupIO(ioID, fileInfo.absoluteFilePath()))
				{
					return false;
				}

			}
		}
	}
	return true;
}


bool MdiChild::saveFile(const QString &f, int modalityNr, int componentNr)
{
	waitForPreviousIO();

	tmpSaveImg = GetModality(modalityNr)->GetImage();
	if (tmpSaveImg->GetNumberOfScalarComponents() > 1 &&
		componentNr != tmpSaveImg->GetNumberOfScalarComponents())
	{
		auto imgExtract = vtkSmartPointer<vtkImageExtractComponents>::New();
		imgExtract->SetInputData(tmpSaveImg);
		imgExtract->SetComponents(componentNr);
		imgExtract->Update();
		tmpSaveImg = imgExtract->GetOutput();
	}

	ioThread = new iAIO(tmpSaveImg, polyData, m_logger, this);
	connectIOThreadSignals(ioThread);
	connect(ioThread, SIGNAL(done()), this, SLOT(SaveFinished()));
	m_storedModalityNr = modalityNr;
	if (!setupSaveIO(f)) {
		ioFinished();
		return false;
	}

	addMsg(tr("%1  Saving file '%2', please wait...")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(f));
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


void MdiChild::clearLogs()
{
	logs->listWidget->clear();
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
	resizeDockWidget(renderer);
}


void MdiChild::saveRC()
{
	QString file = QFileDialog::getSaveFileName(this, tr("Save Image"),
		"",
		iAIOProvider::GetSupportedImageFormats());
	if (file.isEmpty())
		return;
	vtkSmartPointer<vtkWindowToImageFilter> filter = vtkSmartPointer<vtkWindowToImageFilter>::New();
	filter->SetInput(Raycaster->GetRenderWindow());
	filter->Update();
	WriteSingleSliceImage(file, filter->GetOutput());
}


void MdiChild::saveXY()
{
	slicer[iASlicerMode::XY]->saveAsImage();
}

void MdiChild::saveXZ()
{
	slicer[iASlicerMode::XZ]->saveAsImage();
}

void MdiChild::saveYZ()
{
	slicer[iASlicerMode::YZ]->saveAsImage();
}


void MdiChild::saveStackXY()
{
	slicer[iASlicerMode::XY]->saveImageStack();
}

void MdiChild::saveStackXZ()
{
	slicer[iASlicerMode::XZ]->saveImageStack();
}

void MdiChild::saveStackYZ()
{
	slicer[iASlicerMode::YZ]->saveImageStack();
}


void MdiChild::saveMovXY()
{
	saveMovie(slicer[iASlicerMode::XY]);
}

void MdiChild::saveMovXZ()
{
	saveMovie(slicer[iASlicerMode::XZ]);
}

void MdiChild::saveMovYZ()
{
	saveMovie(slicer[iASlicerMode::YZ]);
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
	bool newState = slicer[iASlicerMode::XY]->changeInteractorState();
	if (!newState)
		addMsg(tr("%1  Slicer XY disabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	else
		addMsg(tr("%1  Slicer XY enabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
}


void MdiChild::triggerInteractionXZ()
{
	bool newState = slicer[iASlicerMode::XZ]->changeInteractorState();
	if (!newState)
		addMsg(tr("%1  Slicer XZ disabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
	else
		addMsg(tr("%1  Slicer XZ enabled.").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)));
}


void MdiChild::triggerInteractionYZ()
{
	bool newState = slicer[iASlicerMode::YZ]->changeInteractorState();
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

void MdiChild::setSlabModeXY(bool slabMode)
{
	sXY->labelSlabThicknessXY->setVisible(slabMode);
	sXY->sbinBoxSlabThicknessXY->setVisible(slabMode);
	sXY->comboBoxSlabCompositeModeXY->setVisible(slabMode);

	slabMode == true ?
		updateSlabThicknessXY(sXY->sbinBoxSlabThicknessXY->value()) :
		updateSlabThicknessXY(0);
}

void MdiChild::updateSlabThicknessXY(int thickness)
{
	slicer[iASlicerMode::XY]->setSlabThickness(thickness);
}

void MdiChild::updateSlabCompositeModeXY(int mode)
{
	slicer[iASlicerMode::XY]->setSlabCompositeMode(mode);
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
	parametricSpline->Evaluate(t1, point1, nullptr);
	parametricSpline->Evaluate(t2, point2, nullptr);

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
		updateSnakeSlicer(sXY->spinBoxXY, slicer[iASlicerMode::XY], 2, s);
	}
	else
	{
		this->zCoord = s;
		slicer[iASlicerMode::XY]->setSliceNumber(s);
		if (renderSettings.ShowSlicers || renderSettings.ShowSlicePlanes)
		{
			Raycaster->setSlicePlane(2, 0.0, 0.0, s*imageData->GetSpacing()[2]);
		}
	}
}


void MdiChild::setSliceYZSpinBox(int s)
{
	setSliceYZ(s);
	QSignalBlocker block(sYZ->verticalScrollBarYZ);
	sYZ->verticalScrollBarYZ->setValue(s);
}

void MdiChild::setSlabModeYZ(bool slabMode)
{
	sYZ->labelSlabThicknessYZ->setVisible(slabMode);
	sYZ->sbinBoxSlabThicknessYZ->setVisible(slabMode);
	sYZ->comboBoxSlabCompositeModeYZ->setVisible(slabMode);

	slabMode == true ?
		updateSlabThicknessYZ(sYZ->sbinBoxSlabThicknessYZ->value()) :
		updateSlabThicknessYZ(0);
}

void MdiChild::updateSlabThicknessYZ(int thickness)
{
	slicer[iASlicerMode::YZ]->setSlabThickness(thickness);
}

void MdiChild::updateSlabCompositeModeYZ(int mode)
{
	slicer[iASlicerMode::YZ]->setSlabCompositeMode(mode);
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
		updateSnakeSlicer(sYZ->spinBoxYZ, slicer[iASlicerMode::YZ], 0, s);
	}
	else
	{
		this->xCoord = s;
		slicer[iASlicerMode::YZ]->setSliceNumber(s);
		if (renderSettings.ShowSlicers || renderSettings.ShowSlicePlanes)
		{
			Raycaster->setSlicePlane(0, s*imageData->GetSpacing()[0],0,0);
		}
	}
}


void MdiChild::setSliceXZSpinBox(int s)
{
	setSliceXZ(s);
	QSignalBlocker block(sXZ->verticalScrollBarXZ);
	sXZ->verticalScrollBarXZ->setValue(s);
}

void MdiChild::setSlabModeXZ(bool slabMode)
{
	sXZ->labelSlabThicknessXZ->setVisible(slabMode);
	sXZ->sbinBoxSlabThicknessXZ->setVisible(slabMode);
	sXZ->comboBoxSlabCompositeModeXZ->setVisible(slabMode);

	slabMode == true ?
		updateSlabThicknessXZ(sXZ->sbinBoxSlabThicknessXZ->value()) :
		updateSlabThicknessXZ(0);
}

void MdiChild::updateSlabThicknessXZ(int thickness)
{
	slicer[iASlicerMode::XZ]->setSlabThickness(thickness);
}

void MdiChild::updateSlabCompositeModeXZ(int mode)
{
	slicer[iASlicerMode::XZ]->setSlabCompositeMode(mode);
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
		updateSnakeSlicer(sXZ->spinBoxXZ, slicer[iASlicerMode::XZ], 1, s);
	}
	else
	{
		this->yCoord = s;
		slicer[iASlicerMode::XZ]->setSliceNumber(s);
		if (renderSettings.ShowSlicers || renderSettings.ShowSlicePlanes)
		{
			Raycaster->setSlicePlane(1, 0,s*imageData->GetSpacing()[1],0);
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
	slicer[iASlicerMode::XY]->rotateSlice( a );
	Raycaster->setPlaneNormals( slicerTransform );
}


void MdiChild::setRotationYZ(double a)
{
	slicer[iASlicerMode::YZ]->rotateSlice( a );
	Raycaster->setPlaneNormals( slicerTransform );
}


void MdiChild::setRotationXZ(double a)
{
	slicer[iASlicerMode::XZ]->rotateSlice( a );
	Raycaster->setPlaneNormals( slicerTransform );
}


void MdiChild::link( bool l)
{
	slicerSettings.LinkViews = l;
}

void MdiChild::linkM(bool lm)
{
	slicerSettings.LinkMDIs = lm;
}

void MdiChild::enableInteraction( bool b)
{
	for (int s = 0; s < 3; ++s)
		if (b)
			slicer[s]->enableInteractor();
		else
			slicer[s]->disableInteractor();
}

bool MdiChild::editPrefs(iAPreferences const & prefs)
{
	preferences = prefs;
	if (ioThread)	// don't do any updates if image still loading
		return true;
	SetHistogramModality(m_currentModality);	// to update Histogram bin count
	ApplyViewerPreferences();
	if (isMagicLensToggled())
	{
		updateSlicers();
	}

	emit preferencesChanged();

	return true;
}

void MdiChild::ApplyViewerPreferences()
{
	for (int s = 0; s < 3; ++s)
	{
		slicer[s]->SetMagicLensFrameWidth(preferences.MagicLensFrameWidth);
		slicer[s]->SetMagicLensSize(preferences.MagicLensSize);
		slicer[s]->setStatisticalExtent(preferences.StatisticalExtent);
	}
	renderer->vtkWidgetRC->setLensSize(preferences.MagicLensSize, preferences.MagicLensSize);
	Raycaster->setStatExt(preferences.StatisticalExtent);
}

void MdiChild::setRenderSettings(iARenderSettings const & rs, iAVolumeSettings const & vs)
{
	renderSettings = rs;
	volumeSettings = vs;
}

void MdiChild::ApplyRenderSettings(iARenderer* raycaster)
{
	raycaster->ApplySettings(renderSettings);
}

void MdiChild::ApplyVolumeSettings(const bool loadSavedVolumeSettings)
{
	for (int i = 0; i < 3; ++i)
		slicer[i]->widget()->showBorder(renderSettings.ShowSlicePlanes);
	m_dlgModalities->ShowSlicers(renderSettings.ShowSlicers);
	m_dlgModalities->ChangeRenderSettings(volumeSettings, loadSavedVolumeSettings);
}


QString MdiChild::GetLayoutName() const
{
	return m_layout;
}


void MdiChild::updateLayout()
{
	m_mainWnd->loadLayout();
}


void MdiChild::LoadLayout(QString const & layout)
{
	m_layout = layout;
	QSettings settings;
	QByteArray state = settings.value("Layout/state" + layout).value<QByteArray>();
	hide();
	restoreState(state, 0);
	m_isSmthMaximized = false;
	show();
}


void MdiChild::resetLayout()
{
	restoreState(m_initialLayoutState);
	m_isSmthMaximized = false;
}


int MdiChild::GetRenderMode()
{
	return volumeSettings.Mode;
}

void MdiChild::setupSlicers(iASlicerSettings const & ss, bool init)
{
	slicerSettings = ss;

	if (snakeSlicer)
	{
		int prevMax = sXY->spinBoxXY->maximum();
		int prevValue = sXY->spinBoxXY->value();
		sXY->spinBoxXY->setRange(0, ss.SnakeSlices-1);
		sXY->spinBoxXY->setValue((double)prevValue/prevMax*(ss.SnakeSlices-1));
	}

	linkViews(ss.LinkViews);
	linkMDIs(ss.LinkMDIs);

	for (int s = 0; s<3; ++s)
		slicer[s]->setup(ss.SingleSlicer);

	if (init)
	{
		//this initializes the snake slicer
		for (int s = 0; s<3; ++s)
			slicer[s]->initializeWidget(imageData, worldSnakePoints);

		updateSliceIndicators();

		//Adding new point to the parametric spline for snake slicer
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(addedPoint(double, double, double)), slicer[iASlicerMode::YZ]->widget(), SLOT(addPoint(double, double, double)));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(addedPoint(double, double, double)), slicer[iASlicerMode::XZ]->widget(), SLOT(addPoint(double, double, double)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(addedPoint(double, double, double)), slicer[iASlicerMode::XY]->widget(), SLOT(addPoint(double, double, double)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(addedPoint(double, double, double)), slicer[iASlicerMode::XZ]->widget(), SLOT(addPoint(double, double, double)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(addedPoint(double, double, double)), slicer[iASlicerMode::XY]->widget(), SLOT(addPoint(double, double, double)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(addedPoint(double, double, double)), slicer[iASlicerMode::YZ]->widget(), SLOT(addPoint(double, double, double)));

		//Moving point
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicer[iASlicerMode::YZ]->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicer[iASlicerMode::XZ]->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicer[iASlicerMode::XY]->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicer[iASlicerMode::XZ]->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicer[iASlicerMode::XY]->widget(), SLOT(movePoint(size_t, double, double, double)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(movedPoint(size_t, double, double, double)), slicer[iASlicerMode::YZ]->widget(), SLOT(movePoint(size_t, double, double, double)));

		//Changing arbitrary profile positioning
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicer[iASlicerMode::YZ]->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicer[iASlicerMode::XZ]->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), this,		 SLOT(UpdateProbe(int, double*)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicer[iASlicerMode::XY]->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicer[iASlicerMode::XZ]->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), this,		 SLOT(UpdateProbe(int, double*)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicer[iASlicerMode::XY]->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), slicer[iASlicerMode::YZ]->widget(), SLOT(setArbitraryProfile(int, double*)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), this,		 SLOT(UpdateProbe(int, double*)));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), Raycaster, SLOT(setArbitraryProfile(int, double*)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), Raycaster, SLOT(setArbitraryProfile(int, double*)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(arbitraryProfileChanged(int, double*)), Raycaster, SLOT(setArbitraryProfile(int, double*)));

		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(switchedMode(int)), slicer[iASlicerMode::YZ]->widget(), SLOT(switchMode(int)));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(switchedMode(int)), slicer[iASlicerMode::XZ]->widget(), SLOT(switchMode(int)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(switchedMode(int)), slicer[iASlicerMode::XY]->widget(), SLOT(switchMode(int)));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(switchedMode(int)), slicer[iASlicerMode::XZ]->widget(), SLOT(switchMode(int)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(switchedMode(int)), slicer[iASlicerMode::XY]->widget(), SLOT(switchMode(int)));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(switchedMode(int)), slicer[iASlicerMode::YZ]->widget(), SLOT(switchMode(int)));

		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(deletedSnakeLine()), slicer[iASlicerMode::YZ]->widget(), SLOT(deleteSnakeLine()));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(deletedSnakeLine()), slicer[iASlicerMode::XZ]->widget(), SLOT(deleteSnakeLine()));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(deletedSnakeLine()), slicer[iASlicerMode::XY]->widget(), SLOT(deleteSnakeLine()));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(deletedSnakeLine()), slicer[iASlicerMode::XZ]->widget(), SLOT(deleteSnakeLine()));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(deletedSnakeLine()), slicer[iASlicerMode::XY]->widget(), SLOT(deleteSnakeLine()));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(deletedSnakeLine()), slicer[iASlicerMode::YZ]->widget(), SLOT(deleteSnakeLine()));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(deselectedPoint()),  slicer[iASlicerMode::YZ]->widget(), SLOT(deselectPoint()));
		connect(slicer[iASlicerMode::XY]->widget(), SIGNAL(deselectedPoint()),  slicer[iASlicerMode::XZ]->widget(), SLOT(deselectPoint()));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(deselectedPoint()),  slicer[iASlicerMode::XY]->widget(), SLOT(deselectPoint()));
		connect(slicer[iASlicerMode::YZ]->widget(), SIGNAL(deselectedPoint()),  slicer[iASlicerMode::XZ]->widget(), SLOT(deselectPoint()));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(deselectedPoint()),  slicer[iASlicerMode::XY]->widget(), SLOT(deselectPoint()));
		connect(slicer[iASlicerMode::XZ]->widget(), SIGNAL(deselectedPoint()),  slicer[iASlicerMode::YZ]->widget(), SLOT(deselectPoint()));
	}
}

bool MdiChild::editRendererSettings(iARenderSettings const & rs, iAVolumeSettings const & vs)
{
	setRenderSettings(rs, vs);
	ApplyRenderSettings(Raycaster);
	ApplyVolumeSettings(false);
	renderer->vtkWidgetRC->show();
	emit renderSettingsChanged();
	return true;
}

iARenderSettings const & MdiChild::GetRenderSettings() const
{
	return renderSettings;
}

iAVolumeSettings const &  MdiChild::GetVolumeSettings() const
{
	return volumeSettings;
}

iASlicerSettings const & MdiChild::GetSlicerSettings() const
{
	return slicerSettings;
}


iAPreferences const & MdiChild::GetPreferences() const
{
	return preferences;
}

bool MdiChild::editSlicerSettings(iASlicerSettings const & slicerSettings)
{
	setupSlicers(slicerSettings, false);
	for (int s = 0; s<3; ++s)
		slicer[s]->show();
	return true;
}

// Just proxies for histogram functions:
// {

bool MdiChild::loadTransferFunction()
{
	if (!m_histogram)
		return false;
	m_histogram->loadTransferFunction();
	return true;
}

bool MdiChild::saveTransferFunction()
{
	if (!m_histogram)
		return false;
	m_histogram->saveTransferFunction();
	return true;
}

int MdiChild::deletePoint()
{
	if (!m_histogram) return -1;
	return m_histogram->deletePoint();
}

void MdiChild::resetView()
{
	if (!m_histogram) return;
	m_histogram->resetView();
}

void MdiChild::changeColor()
{
	if (!m_histogram) return;
	m_histogram->changeColor();
}

int MdiChild::getSelectedFuncPoint()
{
	if (!m_histogram) return -1;
	return m_histogram->getSelectedFuncPoint();
}

int MdiChild::isFuncEndPoint(int index)
{
	if (!m_histogram) return -1;
	return m_histogram->isFuncEndPoint(index);
}

void MdiChild::setHistogramFocus()
{
	if (!m_histogram) return;
	m_histogram->setFocus(Qt::OtherFocusReason);
}

void MdiChild::redrawHistogram()
{
	if (!m_histogram) return;
	m_histogram->redraw();
}

void MdiChild::resetTrf()
{
	if (!m_histogram) return;
	m_histogram->resetTrf();
	addMsg(tr("Resetting Transfer Functions."));
	addMsg(tr("  Adding transfer function point: %1.   Opacity: 0.0,   Color: 0, 0, 0")
		.arg(m_histogram->XBounds()[0]));
	addMsg(tr("  Adding transfer function point: %1.   Opacity: 1.0,   Color: 255, 255, 255")
		.arg(m_histogram->XBounds()[1]));
}

std::vector<dlg_function*> & MdiChild::getFunctions()
{
	if (!m_histogram)
	{
		static std::vector<dlg_function*> nullVec;
		return nullVec;
	}
	return m_histogram->getFunctions();
}

iADiagramFctWidget* MdiChild::getHistogram()
{
	return m_histogram;
}

// }

iADockWidgetWrapper* MdiChild::getHistogramDockWidget()
{
	return m_histogramContainer;
}


void MdiChild::saveMovie(iASlicer * slicer)
{
	slicer->saveMovie();
}


void MdiChild::saveMovie(iARenderer& raycaster)
{
	QString movie_file_types = GetAvailableMovieFormats();

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
	QTextDocument * descr = new QTextDocument;
	descr->setHtml("Creates a movie by rotating the object around a user-defined axis in the 3D renderer.");

	dlg_commoninput dlg(this, "Save movie options", inList, inPara, descr);
	if (dlg.exec() != QDialog::Accepted)
		return;

	mode = dlg.getComboBoxValue(0);
	imode = dlg.getComboBoxIndex(0);

	// Show standard save file dialog using available movie file types.
	raycaster.saveMovie(
		QFileDialog::getSaveFileName(
		this,
		tr("Export movie %1").arg(mode),
		fileInfo.absolutePath() + "/" + ((mode.isEmpty()) ? fileInfo.baseName() : fileInfo.baseName() + "_" + mode),
		movie_file_types),
		imode);
}

void MdiChild::toggleSnakeSlicer(bool isChecked)
{
	snakeSlicer = isChecked;

	if (snakeSlicer)
	{
		//save the slicer transforms
		SlicerYZ_Transform = slicer[iASlicerMode::YZ]->GetReslicer()->GetResliceTransform();
		SlicerXY_Transform = slicer[iASlicerMode::XY]->GetReslicer()->GetResliceTransform();
		SlicerXZ_Transform = slicer[iASlicerMode::XZ]->GetReslicer()->GetResliceTransform();

		parametricSpline->Modified();
		double emptyper[3]; emptyper[0] = 0; emptyper[1] = 0; emptyper[2] = 0;
		double emptyp[3]; emptyp[0] = 0; emptyp[1] = 0; emptyp[2] = 0;
		parametricSpline->Evaluate(emptyper, emptyp, nullptr);

		sXY->spinBoxXY->setValue(0);//set initial value
		sXZ->spinBoxXZ->setValue(0);//set initial value
		sYZ->spinBoxYZ->setValue(0);//set initial value

		for (int s = 0; s<3; ++s)
			slicer[s]->widget()->switchMode(iASlicerWidget::NORMAL);
	}
	else
	{
		renderSettings.ShowSlicers = false;

		sXY->spinBoxXY->setValue(imageData->GetDimensions()[2]>>1);
		slicer[iASlicerMode::XY]->GetReslicer()->SetResliceAxesDirectionCosines(1,0,0,  0,1,0,  0,0,1);
		slicer[iASlicerMode::XY]->GetReslicer()->SetResliceTransform(SlicerXY_Transform);
		slicer[iASlicerMode::XY]->GetReslicer()->SetOutputExtentToDefault();
		slicer[iASlicerMode::XY]->GetRenderer()->ResetCamera();
		slicer[iASlicerMode::XY]->GetRenderer()->Render();

		sXZ->spinBoxXZ->setValue(imageData->GetDimensions()[1]>>1);
		slicer[iASlicerMode::XZ]->GetReslicer()->SetResliceAxesDirectionCosines(1,0,0,  0,0,1,  0,-1,0);
		slicer[iASlicerMode::XZ]->GetReslicer()->SetResliceTransform(SlicerXZ_Transform);
		slicer[iASlicerMode::XZ]->GetReslicer()->SetOutputExtentToDefault();
		slicer[iASlicerMode::XZ]->GetRenderer()->ResetCamera();
		slicer[iASlicerMode::XZ]->GetRenderer()->Render();

		sYZ->spinBoxYZ->setValue(imageData->GetDimensions()[0]>>1);
		slicer[iASlicerMode::YZ]->GetReslicer()->SetResliceAxesDirectionCosines(0,1,0,  0,0,1,  1,0,0);
		slicer[iASlicerMode::YZ]->GetReslicer()->SetResliceTransform(SlicerYZ_Transform);
		slicer[iASlicerMode::YZ]->GetReslicer()->SetOutputExtentToDefault();
		slicer[iASlicerMode::YZ]->GetRenderer()->ResetCamera();
		slicer[iASlicerMode::YZ]->GetRenderer()->Render();

		/*
		// TODO: VOLUME: VolumeManager
		if (renderSettings.ShowSlicers)
			Raycaster->showSlicers(true);
		*/
		for (int s = 0; s<3; ++s)
			slicer[s]->widget()->switchMode(iASlicerWidget::DEFINE_SPLINE);
	}
}

bool MdiChild::isSnakeSlicerToggled() const
{
	return snakeSlicer;
}

void MdiChild::toggleSliceProfile(bool isChecked)
{
	isSliceProfileEnabled = isChecked;
	for (int s = 0; s<3; ++s)
		slicer[s]->widget()->setSliceProfileOn(isSliceProfileEnabled);
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
		ChangeMagicLensModality(0);
	}
	SetMagicLensEnabled(isEnabled);
	updateSlicers();

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
	double cos_theta_z = cos(vtkMath::Pi());
	double sin_theta_z = sin(vtkMath::Pi());
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

		slicer[iASlicerMode::XY]->GetReslicer()->SetResliceAxes(axial_transformation_matrix);
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

		slicer[iASlicerMode::YZ]->GetReslicer()->SetResliceAxes(coronial_transformation_matrix);
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

		slicer[iASlicerMode::XZ]->GetReslicer()->SetResliceAxes(sagittal_transformation_matrix);
	}
}

void MdiChild::getSnakeNormal(int index, double point[3], double normal[3])
{
	int i1 = index;
	int i2 = index+1;

	double spacing[3];
	imageData->GetSpacing(spacing);

	int snakeSlices = slicerSettings.SnakeSlices;
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
		parametricSpline->Evaluate(t1, p1, nullptr);
		parametricSpline->Evaluate(t2, p2, nullptr);

		//calculate the points
		p1[0] /= spacing[0]; p1[1] /= spacing[1]; p1[2] /= spacing[2];
		p2[0] /= spacing[0]; p2[1] /= spacing[1]; p2[2] /= spacing[2];

		//calculate the vector between to points
		if (normal)
		{
			normal[0] = p2[0]-p1[0];
			normal[1] = p2[1]-p1[1];
			normal[2] = p2[2]-p1[2];
		}

		point[0] = p1[0]; point[1] = p1[1]; point[2] = p1[2];
	}
}

bool MdiChild::initView( QString const & title )
{
	if (!raycasterInitialized)
	{
		Raycaster->initialize(imageData, polyData);
		connect(Raycaster->getRenderObserver(), SIGNAL(InteractorModeSwitched(int)), m_dlgModalities, SLOT(InteractorModeSwitched(int)));
		raycasterInitialized = true;
	}
	if (GetModalities()->size() == 0 && IsVolumeDataLoaded())
	{
		// TODO: VOLUME: resolve duplication between here (called on loadFile) and adding modalities
		QString name;
		if (!curFile.isEmpty())
		{
			QFileInfo i(curFile);
			name = i.completeBaseName();
		}
		else
		{
			name = title;
		}
		// TODO: VOLUME: resolve indirect dependence of this call on the Raycaster->initialize method
		// before, which adds the renderers which this call will use
		QSharedPointer<iAModality> mod(new iAModality(name,
			currentFile(), -1, imageData, iAModality::MainRenderer));
		GetModalities()->Add(mod);
		m_dlgModalities->AddListItem(mod);
		m_initVolumeRenderers = true;
	}
	vtkColorTransferFunction* colorFunction = (GetModalities()->size() > 0)
		? GetModality(0)->GetTransfer()->GetColorFunction() : vtkColorTransferFunction::New();
	for (int s = 0; s<3; ++s)
		slicer[s]->initializeData(imageData, slicerTransform, colorFunction);

	renderer->stackedWidgetRC->setCurrentIndex(0);
	updateSliceIndicators();

	if (IsVolumeDataLoaded())
	{
		this->addImageProperty();
		if (imageData->GetNumberOfScalarComponents() == 1) //No histogram/profile for rgb, rgba or vector pixel type images
		{
			tabifyDockWidget(logs, m_histogramContainer);
			this->addProfile();
		}
	}
	else
	{	//Polygonal mesh is loaded
		showPoly();
		HideHistogram();
	}

	//Load the layout to the child
	updateLayout();

	return true;
}

void MdiChild::HideHistogram()
{
	m_histogramContainer->hide();
}

void MdiChild::addImageProperty()
{
	if (imgProperty)
		return;
	imgProperty = new dlg_imageproperty(this);
	tabifyDockWidget(logs, imgProperty);
}

void MdiChild::updateImageProperties()
{
	if (!imgProperty)
	{
		return;
	}
	imgProperty->Clear();
	for (int i = 0; i < GetModalities()->size(); ++i)
	{
		imgProperty->AddInfo(GetModality(i)->GetImage(), GetModality(i)->Info(), GetModality(i)->GetName(),
			(i == 0 &&
			GetModality(i)->ComponentCount() == 1 &&
			volumeStack->getNumberOfVolumes() > 1) ?
				volumeStack->getNumberOfVolumes() :
				GetModality(i)->ComponentCount()
			);
	}
}

bool MdiChild::addVolumePlayer(iAVolumeStack* volumeStack)
{
	volumePlayer = new dlg_volumePlayer(this, volumeStack);
	tabifyDockWidget(logs, volumePlayer);
	for (int id=0; id<volumeStack->getNumberOfVolumes(); id++) {
		CheckedList.append(0);
	}
	connect(m_histogram, SIGNAL(applyTFForAll()), volumePlayer, SLOT(applyForAll()));

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

bool MdiChild::isMaximized()
{
	return visibility != MULTI;
}

void MdiChild::UpdateROI(int const roi[6])
{
	for (int s = 0; s<3; ++s)
		slicer[s]->UpdateROI(roi);
}

void MdiChild::SetROIVisible(bool visible)
{
	for (int s = 0; s<3; ++s)
		slicer[s]->SetROIVisible(visible);
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
		if (isWindowModified())
		{
			auto reply = QMessageBox::question(this, "Unsaved changes",
				"You have unsaved changes. Are you sure you want to close this window?",
				QMessageBox::Yes | QMessageBox::No);
			if (reply != QMessageBox::Yes)
			{
				event->ignore();
				return;
			}
		}
		emit closed();
		event->accept();
	}
}

void MdiChild::setCurrentFile(const QString &f)
{
	fileInfo.setFile(f);
	curFile = f;
	path = fileInfo.canonicalPath();
	isUntitled = f.isEmpty();
	setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

QString MdiChild::strippedName(const QString &f)
{
	return QFileInfo(f).fileName();
}

// TODO: unify with setVisibility / check if one of the two calls redundant!
void MdiChild::changeVisibility(unsigned char mode)
{
	visibility = mode;

	bool  rc = (mode & RC)  == RC;
	bool  xy = (mode & XY)  == XY;
	bool  yz = (mode & YZ)  == YZ;
	bool  xz = (mode & XZ)  == XZ;
	bool tab = (mode & TAB) == TAB;
	renderer->setVisible(rc);
	sXY->setVisible(xy);
	sYZ->setVisible(yz);
	sXZ->setVisible(xz);

	logs->setVisible(tab);
	if (IsVolumeDataLoaded())
	{	// TODO: check redundancy with HideHistogram calls?
		m_histogramContainer->setVisible(tab);
	}
}

void MdiChild::hideVolumeWidgets()
{
	setVisibility(QList<QWidget*>() << sXY << sXZ << sYZ << renderer, false);
	this->update();
}


void MdiChild::setVisibility(QList<QWidget*> widgets, bool show)
{
	for (int i = 0; i < widgets.size(); i++)
		show ? widgets[i]->show() : widgets[i]->hide();
}


void MdiChild::updateSlicer(int index)
{
	if (index < 0 || index > 2)
	{
		QMessageBox::warning(this, tr("Wrong slice plane index"), tr("updateSlicer(int index) is called with the wrong index parameter"));
		return;
	}
	slicer[index]->update();
}

void MdiChild::InitChannelRenderer(iAChannelID id, bool use3D, bool enableChannel)
{
	iAChannelVisualizationData * data = GetChannelData(id);
	assert(data);
	if (!data)
	{
		return;
	}
	for (int s = 0; s<3; ++s)
		slicer[s]->initializeChannel(id, data);
	/*
	// TODO: VOLUME: rewrite using separate volume
	if (use3D)
	{
		data->Set3D(true);
		Raycaster->addChannel(data);
	}
	*/
	if (use3D)
	{
		data->Set3D(true);
		m_dlgModalities->AddModality(data->GetImage(), QString("Channel %1").arg(id));
	}
	SetChannelRenderingEnabled(id, enableChannel);
}

void MdiChild::SetSlicerPieGlyphsEnabled( bool isOn )
{
	for (int s = 0; s<3; ++s)
		slicer[s]->setPieGlyphsOn(isOn);
}

void MdiChild::SetPieGlyphParameters( double opacity, double spacing, double magFactor )
{
	for (int s = 0; s<3; ++s)
		slicer[s]->setPieGlyphParameters(opacity, spacing, magFactor);
}

iAChannelVisualizationData * MdiChild::GetChannelData(iAChannelID id)
{
	QMap<iAChannelID, QSharedPointer<iAChannelVisualizationData> >::const_iterator it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return nullptr;
	}
	return it->data();
}

iAChannelVisualizationData const * MdiChild::GetChannelData(iAChannelID id) const
{
	QMap<iAChannelID, QSharedPointer<iAChannelVisualizationData> >::const_iterator it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return nullptr;
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
	for (int s = 0; s < 3; ++s)
		slicer[s]->update();
}

void MdiChild::UpdateChannelSlicerOpacity(iAChannelID id, double opacity)
{
	if (!GetChannelData(id))
	{
		return;
	}
	GetChannelData(id)->SetOpacity(opacity);
	for (int s = 0; s<3; ++s)
		slicer[s]->GetSlicerData()->setChannelOpacity(id, opacity);
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
	/*
	// TODO: VOLUME: rewrite using volume manager:
	if (data->Uses3D())
	{
		getRenderer()->updateChannelImages();
	}
	*/
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

void MdiChild::addProfile()
{
	profileProbe = QSharedPointer<iAProfileProbe>(new iAProfileProbe(imageData));
	double start[3];
	imageData->GetOrigin(start);
	int const * const dim = imageData->GetDimensions();
	double const * const spacing = imageData->GetSpacing();
	double end[3];
	for (int i = 0; i<3; i++)
		end[i] = start[i] + (dim[i] - 1) * spacing[i];
	for (int s = 0; s < 3; ++s)
	{
		slicer[s]->widget()->setArbitraryProfile(0, start);
		slicer[s]->widget()->setArbitraryProfile(1, end);
	}
	Raycaster->setArbitraryProfile(0, start);
	Raycaster->setArbitraryProfile(1, end);
	profileProbe->UpdateProbe(0, start);
	profileProbe->UpdateProbe(1, end);
	profileProbe->UpdateData();
	imgProfile = new dlg_profile(this, profileProbe->profileData, profileProbe->GetRayLength());
	tabifyDockWidget(logs, imgProfile);
	connect(imgProfile->profileMode, SIGNAL(toggled(bool)), this, SLOT(toggleArbitraryProfile(bool)));
}

void MdiChild::toggleArbitraryProfile( bool isChecked )
{
	isArbProfileEnabled = (bool)isChecked;
	for (int s = 0; s<3; ++s)
		slicer[s]->widget()->setArbitraryProfileOn(isArbProfileEnabled);
	Raycaster->setArbitraryProfileOn(isArbProfileEnabled);
}

void MdiChild::UpdateProbe( int ptIndex, double * newPos )
{
	if (imageData->GetNumberOfScalarComponents() != 1) //No profile for rgb, rgba or vector pixel type images
		return;
	profileProbe->UpdateProbe(ptIndex, newPos);
	UpdateProfile();
}

void MdiChild::UpdateProfile()
{
	profileProbe->UpdateData();
	imgProfile->profileWidget->initialize(profileProbe->profileData, profileProbe->GetRayLength());
	imgProfile->profileWidget->redraw();
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
	ioThread = nullptr;
	tmpSaveImg = nullptr;
}

iASlicerData* MdiChild::getSlicerDataXZ()
{
	return slicer[iASlicerMode::XZ]->GetSlicerData();
}

iASlicerData* MdiChild::getSlicerDataXY()
{
	return slicer[iASlicerMode::XY]->GetSlicerData();
}

iASlicerData* MdiChild::getSlicerDataYZ()
{
	return slicer[iASlicerMode::YZ]->GetSlicerData();
}

iASlicer* MdiChild::getSlicerXZ()
{
	return slicer[iASlicerMode::XZ];
}

iASlicer* MdiChild::getSlicerXY()
{
	return slicer[iASlicerMode::XY];
}

iASlicer* MdiChild::getSlicerYZ()
{
	return slicer[iASlicerMode::YZ];
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


dlg_renderer * MdiChild::getRendererDlg()
{
	return renderer;
}

dlg_imageproperty * MdiChild::getImagePropertyDlg()
{
	return imgProperty;
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
	for (int s = 0; s<3; ++s)
		slicer[s]->SetMagicLensInput(id);
	if (initReslice)
	{
		slicer[iASlicerMode::YZ]->setResliceChannelAxesOrigin(id, static_cast<double>(sYZ->spinBoxYZ->value()) * imageData->GetSpacing()[0], 0, 0);
		slicer[iASlicerMode::XZ]->setResliceChannelAxesOrigin(id, 0, static_cast<double>(sXZ->spinBoxXZ->value()) * imageData->GetSpacing()[1], 0);
		slicer[iASlicerMode::XY]->setResliceChannelAxesOrigin(id, 0, 0, static_cast<double>(sXY->spinBoxXY->value()) * imageData->GetSpacing()[2]);
	}
}


void MdiChild::SetMagicLensEnabled(bool isOn)
{
	for (int s = 0; s<3; ++s)
		slicer[s]->SetMagicLensEnabled( isOn );
}


void MdiChild::reInitChannel(iAChannelID id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	iAChannelVisualizationData * chData = GetChannelData( id );
	if (!chData)
	{
		return;
	}
	chData->SetImage( imgData );
	chData->SetColorTF( ctf );
	chData->SetOpacityTF( otf );
	for (int s = 0; s<3; ++s)
		slicer[s]->reInitializeChannel( id, chData );
}


void MdiChild::reInitMagicLens(iAChannelID id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	if (!isMagicLensEnabled)
	{
		return;
	}
	iAChannelVisualizationData chData;
	chData.SetImage(imgData);
	chData.SetColorTF(ctf);
	chData.SetOpacityTF(otf);
	for (int s = 0; s<3; ++s)
		slicer[s]->reInitializeChannel(id, &chData);
	SetMagicLensInput( id, true);
	updateSlicers();
}


void MdiChild::updateChannelMappers()
{
	for (int s = 0; s<3; ++s)
		slicer[s]->GetSlicerData()->updateChannelMappers();
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


bool MdiChild::IsVolumeDataLoaded() const
{
	QString suffix = getFileInfo().suffix();
	int * extent = imageData->GetExtent();
	return QString::compare(suffix, "STL", Qt::CaseInsensitive) != 0 &&
		QString::compare(suffix, "FEM", Qt::CaseInsensitive) != 0 &&
		extent[1] >= 0 && extent[3] >= 0 && extent[5] >= 0;
}


void MdiChild::ChangeMagicLensModality(int chg)
{
	m_currentComponent = (m_currentComponent + chg);
	if (m_currentComponent < 0 || m_currentComponent >= GetModality(m_currentModality)->ComponentCount())
	{
		m_currentComponent = 0;
		m_currentModality = (m_currentModality + chg + GetModalities()->size()) % (GetModalities()->size());
	}
	if (m_currentModality < 0 || m_currentModality >= GetModalities()->size())
	{
		DEBUG_LOG("Invalid modality index!");
		m_currentModality = 0;
		return;
	}

	iAChannelVisualizationData chData;
	vtkSmartPointer<vtkImageData> img = GetModality(m_currentModality)->GetComponent(m_currentComponent);
	chData.SetImage(img);
	chData.SetColorTF(m_dlgModalities->GetCTF(m_currentModality));
	chData.SetOpacityTF(m_dlgModalities->GetOTF(m_currentModality));
	chData.SetOpacity(0.5);
	QString name(GetModality(m_currentModality)->GetImageName(m_currentComponent));
	chData.SetName(name);
	for (int s = 0; s<3; ++s)
		slicer[s]->initializeChannel(ch_SlicerMagicLens, &chData);
	SetMagicLensInput(ch_SlicerMagicLens, true);
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


void MdiChild::ShowModality(int modIdx)
{
	if (m_currentModality == modIdx)
		return;
	m_currentModality = modIdx;
	m_currentComponent = 0;
	SetHistogramModality(modIdx);
}


void MdiChild::SetModalities(QSharedPointer<iAModalityList> modList)
{
	bool noDataLoaded = GetModalities()->size() == 0;
	m_dlgModalities->SetModalities(modList);

	if (noDataLoaded && GetModalities()->size() > 0)
	{
		InitModalities();
	}
}


dlg_modalities* MdiChild::GetModalitiesDlg()
{
	return m_dlgModalities;
}

QSharedPointer<iAModalityList> MdiChild::GetModalities()
{
	return m_dlgModalities->GetModalities();
}

QSharedPointer<iAModality> MdiChild::GetModality(int idx)
{
	return GetModalities()->Get(idx);
}

void MdiChild::InitModalities()
{
	for (int i = 0; i < GetModalities()->size(); ++i)
		m_dlgModalities->AddListItem(GetModality(i));
	// TODO: VOLUME: rework - workaround: "initializes" renderer and slicers with modality 0
	m_initVolumeRenderers = true;
	setImageData(
		currentFile().isEmpty() ? GetModality(0)->GetFileName() : currentFile(),
		GetModality(0)->GetImage()
	);
	m_dlgModalities->SelectRow(0);
}


void MdiChild::SetHistogramModality(int modalityIdx)
{
	if (GetModality(modalityIdx)->GetImage()->GetNumberOfScalarComponents() != 1) //No histogram/profile for rgb, rgba or vector pixel type images
		return;

	if (!m_histogram)
		return;
	auto histData = GetModality(modalityIdx)->GetTransfer()->GetHistogramData();
	size_t newBinCount = preferences.HistogramBins;
	auto img = GetModality(modalityIdx)->GetImage();
	auto scalarRange = img->GetScalarRange();
	if (isVtkIntegerType(GetModality(modalityIdx)->GetImage()->GetScalarType()))
		newBinCount = std::min(newBinCount, static_cast<size_t>(scalarRange[1] - scalarRange[0] + 1));
	if (histData &&	histData->GetNumBin() == newBinCount)
	{
		if (modalityIdx != m_currentHistogramModality)
			HistogramDataAvailable(modalityIdx);
		return;
	}
	auto workerThread = new iAHistogramUpdater(modalityIdx,
		GetModality(modalityIdx), newBinCount);
	connect(workerThread, &iAHistogramUpdater::HistogramReady, this, &MdiChild::HistogramDataAvailable);
	connect(workerThread, &iAHistogramUpdater::StatisticsReady, this, &MdiChild::StatisticsAvailable);
	connect(workerThread, &iAHistogramUpdater::finished, workerThread, &QObject::deleteLater);
	addMsg(QString("%1  Computing statistics and histogram for modality %2...")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(GetModality(modalityIdx)->GetName()));
	workerThread->start();
}


void MdiChild::ModalityAdded(int modalityIdx)
{
	if (GetModality(modalityIdx)->GetImage()->GetNumberOfScalarComponents() == 1) //No histogram/profile for rgb, rgba or vector pixel type images
	{
		SetHistogramModality(modalityIdx);
	}
	else
	{
		m_dlgModalities->InitDisplay(GetModality(modalityIdx));
		ApplyVolumeSettings(false);
	}
}


void MdiChild::HistogramDataAvailable(int modalityIdx)
{
	QString modalityName = GetModality(modalityIdx)->GetName();
	m_currentHistogramModality = modalityIdx;
	addMsg(QString("%1  Displaying histogram for modality %2.")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(modalityName));
	m_histogram->RemovePlot(m_histogramPlot);
	m_histogramPlot = QSharedPointer<iAPlot>(new
		iABarGraphDrawer(GetModality(modalityIdx)->GetHistogramData(),
			QColor(70, 70, 70, 255)));
	m_histogram->AddPlot(m_histogramPlot);
	m_histogram->SetXCaption("Histogram " + modalityName);
	m_histogram->SetTransferFunctions(GetModality(modalityIdx)->GetTransfer()->GetColorFunction(),
		GetModality(modalityIdx)->GetTransfer()->GetOpacityFunction());
	m_histogram->updateTrf();	// will also redraw() the histogram
	updateImageProperties();
	if (!findChild<iADockWidgetWrapper*>("Histogram"))
	{
		tabifyDockWidget(logs, m_histogramContainer);
		this->addProfile();
	}
}


void MdiChild::StatisticsAvailable(int modalityIdx)
{
	addMsg(QString("%1  Displaying statistics for modality %2.")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat))
		.arg(GetModality(modalityIdx)->GetName()));
	InitVolumeRenderers();
	if (modalityIdx == 0)
	{
		QSharedPointer<iAModalityTransfer> modTrans = GetModality(0)->GetTransfer();
		for (int s = 0; s < 3; ++s)
		{
			slicer[s]->reInitialize(GetModality(0)->GetImage(), slicerTransform, modTrans->GetColorFunction());
			slicer[s]->GetSlicerData()->ResetCamera();
		}
	}
	ChangeMagicLensModality(0);
	ModalityTFChanged();
	updateViews();
}


void MdiChild::InitVolumeRenderers()
{
	if (!m_initVolumeRenderers)
	{
		for (int i = 0; i < GetModalities()->size(); ++i)
			GetModality(i)->UpdateRenderer();
		return;
	}
	m_initVolumeRenderers = false;
	for (int i = 0; i < GetModalities()->size(); ++i)
	{
		m_dlgModalities->InitDisplay(GetModality(i));

	}
	ApplyVolumeSettings(true);
	connect(GetModalities().data(), SIGNAL(Added(QSharedPointer<iAModality>)),
		m_dlgModalities, SLOT(ModalityAdded(QSharedPointer<iAModality>)));
	Raycaster->GetRenderer()->ResetCamera();
}

void MdiChild::saveProject(QString const & fileName)
{
	ioThread = new iAIO(m_dlgModalities->GetModalities(), Raycaster->GetRenderer()->GetActiveCamera(), m_logger);
	connectIOThreadSignals(ioThread);
	QFileInfo fileInfo(fileName);
	if (!ioThread->setupIO(PROJECT_WRITER, fileInfo.absoluteFilePath()))
	{
		ioFinished();
		return;
	}
	addMsg(tr("%1  Saving file '%2', please wait...")
		.arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(fileName));
	ioThread->start();
	// TODO: only set new project file name if saving succeeded
	setCurrentFile(fileName);
}

void MdiChild::StoreProject()
{
	QVector<int> unsavedModalities;
	for (int i=0; i<GetModalities()->size(); ++i)
	{
		if (GetModality(i)->GetFileName().isEmpty())
			unsavedModalities.push_back(i);
	}
	if (unsavedModalities.size() > 0)
	{
		if (QMessageBox::question(m_mainWnd, "Unsaved modalities",
			"This window has some unsaved modalities, you need to save them before you can store the project. Save them now?",
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
			return;
		for (int modNr : unsavedModalities)
			if (!saveAs(modNr))
				return;
	}
	QString modalitiesFileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Select Output File"),
		path,
		iAIOProvider::ProjectFileTypeFilter);
	if (modalitiesFileName.isEmpty())
		return;
	saveProject(modalitiesFileName);
}

MainWindow* MdiChild::getMainWnd()
{
	return m_mainWnd;
}


vtkPiecewiseFunction * MdiChild::getPiecewiseFunction()
{
	return GetModality(0)->GetTransfer()->GetOpacityFunction();
}

vtkColorTransferFunction * MdiChild::getColorTransferFunction()
{
	return GetModality(0)->GetTransfer()->GetColorFunction();
}

void MdiChild::SaveFinished()
{
	m_dlgModalities->SetFileName(m_storedModalityNr, ioThread->getFileName());
	setWindowModified(GetModalities()->HasUnsavedModality());
}

void MdiChild::SplitDockWidget(QDockWidget* ref, QDockWidget* newWidget, Qt::Orientation orientation)
{
	QList<QDockWidget*> tabified = m_mainWnd->tabifiedDockWidgets(ref);
	if (tabified.size() > 0)
	{
		tabifyDockWidget(ref, newWidget);
	}
	else
	{
		splitDockWidget(ref, newWidget, orientation);
	}
}

bool MdiChild::IsFullyLoaded() const
{
	int const * dim = imageData->GetDimensions();
	return dim[0] > 0 && dim[1] > 0 && dim[2] > 0;
}
