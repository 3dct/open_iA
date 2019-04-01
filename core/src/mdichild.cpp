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
#include "mdichild.h"

#include "charts/iAHistogramData.h"
#include "charts/iADiagramFctWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAProfileWidget.h"
#include "dlg_commoninput.h"
#include "dlg_imageproperty.h"
#include "dlg_modalities.h"
#include "dlg_profile.h"
#include "dlg_slicer.h"
#include "dlg_transfer.h"
#include "dlg_volumePlayer.h"
#include "iAAlgorithm.h"
#include "iAChannelData.h"
#include "iAChannelSlicerData.h"
#include "iAConsole.h"
#include "qthelper/iADockWidgetWrapper.h"
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
#include "iAToolsVTK.h"
#include "iATransferFunction.h"
#include "iAVolumeStack.h"
#include "qthelper/iAWidgetAddHelper.h"
#include "io/extension2id.h"
#include "io/iAIO.h"
#include "io/iAIOProvider.h"
#include "mainwindow.h"

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCornerAnnotation.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
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

// TODO: VOLUME: check all places using getModality(0)->GetTransfer() !

#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QSpinBox>
#include <QToolButton>
#include <QtGlobal> // for QT_VERSION


MdiChild::MdiChild(MainWindow * mainWnd, iAPreferences const & prefs, bool unsavedChanges) :
	m_isSmthMaximized(false),
	isMagicLensEnabled(false),
	reInitializeRenderWindows(true),
	m_initVolumeRenderers(false),
	isUntitled(true),
	snakeSlicer(false),
	isSliceProfileEnabled(false),
	isArbProfileEnabled(false),
	raycasterInitialized(false),
	m_mainWnd(mainWnd),
	volumeStack(new iAVolumeStack),
	ioThread(nullptr),
	m_logger(new MdiChildLogger(this)),
	m_histogram(new iADiagramFctWidget(nullptr, this, " Histogram", "Frequency")),
	m_histogramContainer(new iADockWidgetWrapper(m_histogram, "Histogram", "Histogram")),
	preferences(prefs),
	m_currentModality(0),
	m_currentComponent(0),
	m_currentHistogramModality(-1),
	m_magicLensChannel(NotExistingChannel),
	m_nextChannelID(0),
	slicerTransform(vtkTransform::New()),
	worldSnakePoints(vtkPoints::New())
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif
	setWindowModified(unsavedChanges);
	setupUi(this);
	//prepare window for handling dock widgets
	this->setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

	//insert default dock widgets and arrange them in a simple layout
	renderer = new dlg_renderer(this);
	for (int i = 0; i < 3; ++i)
	{
		m_slicer[i] = new iASlicer(this, static_cast<iASlicerMode>(i), true, true, slicerTransform, worldSnakePoints);
		m_dlgSlicer[i] = new dlg_slicer(m_slicer[i]);
	}

	pbar = new QProgressBar(this);
	pbar->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	pbar->setMaximumSize(350, 17);
	this->statusBar()->addPermanentWidget(pbar);
	m_pbarMaxVal = pbar->maximum();
	m_dlgLog = new dlg_logs(this);
	addDockWidget(Qt::LeftDockWidgetArea, renderer);
	m_initialLayoutState = saveState();

	splitDockWidget(renderer, m_dlgLog, Qt::Vertical);
	splitDockWidget(renderer, m_dlgSlicer[iASlicerMode::XZ], Qt::Horizontal);
	splitDockWidget(renderer, m_dlgSlicer[iASlicerMode::YZ], Qt::Vertical);
	splitDockWidget(m_dlgSlicer[iASlicerMode::XZ], m_dlgSlicer[iASlicerMode::XY], Qt::Vertical);

	setAttribute(Qt::WA_DeleteOnClose);

	visibility = MULTI;
	std::fill(m_position, m_position + 3, 0);

	imageData = vtkSmartPointer<vtkImageData>::New();
	imageData->AllocateScalars(VTK_DOUBLE, 1);
	polyData = vtkPolyData::New();

	axesTransform = vtkTransform::New();
	parametricSpline = iAParametricSpline::New();
	parametricSpline->SetPoints(worldSnakePoints);
	
	Raycaster = new iARenderer(this);
	Raycaster->setAxesTransform(axesTransform);

	m_dlgModalities = new dlg_modalities(renderer->vtkWidgetRC, Raycaster->GetRenderer(), preferences.HistogramBins, this);
	QSharedPointer<iAModalityList> modList(new iAModalityList);
	setModalities(modList);
	splitDockWidget(m_dlgLog, m_dlgModalities, Qt::Horizontal);
	m_dlgModalities->SetSlicePlanes(Raycaster->getPlane1(), Raycaster->getPlane2(), Raycaster->getPlane3());
	applyViewerPreferences();
	imgProperty = nullptr;
	imgProfile = nullptr;
	setRenderWindows();
	connectSignalsToSlots();
	pbar->setValue(100);

	worldProfilePoints = vtkPoints::New();
	worldProfilePoints->Allocate(2);
}

MdiChild::~MdiChild()
{
	cleanWorkingAlgorithms();
	polyData->ReleaseData();
	axesTransform->Delete();
	slicerTransform->Delete();

	polyData->Delete();

	for (int s=0; s<3; ++s)
		delete m_slicer[s];
	delete Raycaster; Raycaster = nullptr;

	if(imgProperty)		delete imgProperty;
	if(imgProfile)		delete imgProfile;
}

void MdiChild::connectSignalsToSlots()
{
	connect(m_dlgSlicer[iASlicerMode::XY]->pbMax, SIGNAL(clicked()), this, SLOT(maximizeXY()));
	connect(m_dlgSlicer[iASlicerMode::XZ]->pbMax, SIGNAL(clicked()), this, SLOT(maximizeXZ()));
	connect(m_dlgSlicer[iASlicerMode::YZ]->pbMax, SIGNAL(clicked()), this, SLOT(maximizeYZ()));

	connect(renderer->pushMaxRC, SIGNAL(clicked()), this, SLOT(maximizeRC()));
	connect(renderer->pushStopRC, SIGNAL(clicked()), this, SLOT(triggerInteractionRaycaster()));

	connect(renderer->pushPX, SIGNAL(clicked()), this, SLOT(camPX()));
	connect(renderer->pushPY, SIGNAL(clicked()), this, SLOT(camPY()));
	connect(renderer->pushPZ, SIGNAL(clicked()), this, SLOT(camPZ()));
	connect(renderer->pushMX, SIGNAL(clicked()), this, SLOT(camMX()));
	connect(renderer->pushMY, SIGNAL(clicked()), this, SLOT(camMY()));
	connect(renderer->pushMZ, SIGNAL(clicked()), this, SLOT(camMZ()));
	connect(renderer->pushIso, SIGNAL(clicked()), this, SLOT(camIso()));
	connect(renderer->pushSaveRC, SIGNAL(clicked()), this, SLOT(saveRC()));
	connect(renderer->pushMovRC, SIGNAL(clicked()), this, SLOT(saveMovRC()));

	connect(m_dlgLog->pushClearLogs, SIGNAL(clicked()), this, SLOT(clearLogs()));

	connect(renderer->vtkWidgetRC, SIGNAL(rightButtonReleasedSignal()), Raycaster, SLOT(mouseRightButtonReleasedSlot()));
	connect(renderer->vtkWidgetRC, SIGNAL(leftButtonReleasedSignal()), Raycaster, SLOT(mouseLeftButtonReleasedSlot()));
	connect(renderer->spinBoxRC, SIGNAL(valueChanged(int)), this, SLOT(setChannel(int)));

	for (int s = 0; s < 3; ++s)
	{
		connect(m_slicer[s], &iASlicer::shiftMouseWheel, this, &MdiChild::changeMagicLensModality);
		connect(m_slicer[s], &iASlicer::altMouseWheel, this, &MdiChild::changeMagicLensOpacity);
		connect(m_slicer[s], &iASlicer::ctrlMouseWheel, this, &MdiChild::changeMagicLensSize);
		connect(m_slicer[s], &iASlicer::sliceRotated, this, &MdiChild::slicerRotationChanged);
		connect(m_slicer[s], &iASlicer::sliceNumberChanged, this, &MdiChild::setSlice);

		connect(m_slicer[s], &iASlicer::oslicerPos, this, &MdiChild::updatePositionMarker);
		connect(m_slicer[s], &iASlicer::msg, this, &MdiChild::addMsg);
		connect(m_slicer[s], &iASlicer::progress, this, &MdiChild::updateProgressBar);
	}

	connect(m_histogram, SIGNAL(updateViews()), this, SLOT(updateViews()));
	connect(m_histogram, SIGNAL(pointSelected()), this, SIGNAL(pointSelected()));
	connect(m_histogram, SIGNAL(noPointSelected()), this, SIGNAL(noPointSelected()));
	connect(m_histogram, SIGNAL(endPointSelected()), this, SIGNAL(endPointSelected()));
	connect(m_histogram, SIGNAL(active()), this, SIGNAL(active()));
	connect((dlg_transfer*)(m_histogram->getFunctions()[0]), SIGNAL(Changed()), this, SLOT(modalityTFChanged()));

	connect(m_dlgModalities, SIGNAL(ModalitiesChanged()), this, SLOT(updateImageProperties()));
	connect(m_dlgModalities, SIGNAL(ModalitiesChanged()), this, SLOT(updateViews()));
	connect(m_dlgModalities, SIGNAL(ModalitySelected(int)), this, SLOT(showModality(int)));
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

void MdiChild::setRenderWindows()
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
		m_slicer[s]->disableInteractor();
	Raycaster->disableInteractor();
	emit rendererDeactivated(ch);
}

void MdiChild::enableRenderWindows()	// = image data available
{
	if (isVolumeDataLoaded() && reInitializeRenderWindows)
	{
		Raycaster->enableInteractor();
		for (int s = 0; s<3; ++s)
			m_slicer[s]->enableInteractor();
		updateViews();
		updateImageProperties();
		if (imageData->GetNumberOfScalarComponents() == 1)
		{
			setHistogramModality(0);
			updateProfile();
		}
		else  // No histogram/profile for rgb, rgba or vector pixel type images
		{
			initVolumeRenderers();
			QSharedPointer<iAModalityTransfer> modTrans = getModality(0)->GetTransfer();
			for (int s = 0; s < 3; ++s)
			{
				m_slicer[s]->addChannel(0, iAChannelData(getModality(0)->GetImage(), modTrans->getColorFunction()), false);
				m_slicer[s]->resetCamera();
			}
		}
	}
	// set to true for next time, in case it is false now (i.e. default to always reinitialize,
	// unless explicitly set otherwise)
	reInitializeRenderWindows = true;

	Raycaster->reInitialize(imageData, polyData);

	if (!isVolumeDataLoaded())
		return;
	camIso();
	vtkCamera* cam = Raycaster->getCamera();
	getModalities()->ApplyCameraSettings(cam);

	for (auto channelID: m_channels.keys())
	{
		iAChannelData * chData = m_channels.value(channelID).data();
		if (chData->isEnabled()
			|| (isMagicLensEnabled && (
				channelID == m_slicer[iASlicerMode::XY]->getMagicLensInput() ||
				channelID == m_slicer[iASlicerMode::XZ]->getMagicLensInput() ||
				channelID == m_slicer[iASlicerMode::YZ]->getMagicLensInput()
				))
			)
		{
			for (int s = 0; s<3; ++s)
				m_slicer[s]->updateChannel(channelID, *chData);
		}
	}
	m_dlgModalities->EnableUI();
}

void MdiChild::modalityTFChanged()
{
	updateChannelMappers();
	for (int s = 0; s<3; ++s)
		m_slicer[s]->updateMagicLensColors();
	emit transferFunctionChanged();
}

void MdiChild::updateProgressBar(int i)
{
	pbar->show();
	pbar->setValue(i);
}

void MdiChild::updatePositionMarker(int x, int y, int z, int mode)
{
	double spacing[3];
	imageData->GetSpacing(spacing);
//TODO: improve using iASlicer stuff
	if (slicerSettings.LinkViews)
	{
		m_position[0] = x; m_position[1] = y; m_position[2] = z;
		if (mode != iASlicerMode::XZ)
		{
			if (slicerSettings.SingleSlicer.ShowPosition)
				m_slicer[iASlicerMode::XZ]->setPositionMarkerCenter(x*spacing[0], z*spacing[2]);
			m_slicer[iASlicerMode::XZ]->setIndex(x, y, z);
			m_dlgSlicer[iASlicerMode::XZ]->sbSlice->setValue(y);
		}
		if (mode != iASlicerMode::YZ)
		{
			if (slicerSettings.SingleSlicer.ShowPosition)
				m_slicer[iASlicerMode::YZ]->setPositionMarkerCenter(y*spacing[1], z*spacing[2]);
			m_slicer[iASlicerMode::YZ]->setIndex(x,y,z);
			m_dlgSlicer[iASlicerMode::YZ]->sbSlice->setValue(x);
		}
		if (mode != iASlicerMode::XY) {
			if (slicerSettings.SingleSlicer.ShowPosition)
				m_slicer[iASlicerMode::XY]->setPositionMarkerCenter(x*spacing[0], y*spacing[1]);
			m_slicer[iASlicerMode::XY]->setIndex(x,y,z);
			m_dlgSlicer[iASlicerMode::XY]->sbSlice->setValue(z);
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
	if (poly)
	{
		polyData->ReleaseData();
		polyData->DeepCopy(poly);
	}

	if (image)
	{
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

void MdiChild::prepareForResult()
{
	setWindowModified(true);
	getModality(0)->GetTransfer()->reset();
}

bool MdiChild::setupLoadIO(QString const & f, bool isStack)
{
	polyData->ReleaseData();
	// TODO: insert plugin mechanism.
	// - iterate over file plugins; if one returns a match, use it
	if (QString::compare(fileInfo.suffix(), "STL", Qt::CaseInsensitive) == 0)
	{
		return ioThread->setupIO(STL_READER, f);
	}else
	if (QString::compare(fileInfo.suffix(), "VTK", Qt::CaseInsensitive) == 0)
	{
		return ioThread->setupIO(VTK_READER, f);
	}
	imageData->ReleaseData();
	QString extension = fileInfo.suffix();
	extension = extension.toUpper();
	const mapQString2int * ext2id = &extensionToId;
	if(isStack)	ext2id = &extensionToIdStack;
	if (ext2id->find(extension) == ext2id->end())
	{
		DEBUG_LOG(QString("Could not find loader for extension '%1' of file '%2'!").arg(extension).arg(f));
		return false;
	}
	IOType id = ext2id->find( extension ).value();
	return ioThread->setupIO(id, f);
}

bool MdiChild::loadRaw(const QString &f)
{
	if (!QFile::exists(f))	return false;
	addMsg(tr("Loading file '%1', please wait...").arg(f));
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
			f.endsWith("jpg", Qt::CaseInsensitive) ||
			f.endsWith("jpeg", Qt::CaseInsensitive) ||
			f.endsWith("png", Qt::CaseInsensitive) ||
			f.endsWith("tif", Qt::CaseInsensitive) ||
			f.endsWith("tiff", Qt::CaseInsensitive);
	}
}

bool MdiChild::loadFile(const QString &f, bool isStack)
{
	if (!QFile::exists(f))
	{
		DEBUG_LOG(QString("File '%1' does not exist!").arg(f));
		return false;
	}

	addMsg(tr("Loading file '%1', please wait...").arg(f));
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
	connect(m_dlgModalities, SIGNAL(ModalityAvailable(int)), this, SLOT(modalityAdded(int)));
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
	getModality(0)->SetData(imageData);
	m_mainWnd->setCurrentFile(getModalities()->GetFileName());
	setupView(false);
	enableRenderWindows();
}

vtkImageData*                 MdiChild::getImageData()    { return imageData; }
vtkSmartPointer<vtkImageData> MdiChild::getImagePointer() { return imageData; }

void MdiChild::setImageData(vtkImageData * iData)
{
	imageData = iData;		// potential for double free!
}

vtkPolyData* MdiChild::getPolyData() { return polyData; }
iARenderer*  MdiChild::getRenderer() { return Raycaster; }

bool MdiChild::updateVolumePlayerView(int updateIndex, bool isApplyForAll)
{
	// TODO: VOLUME: Test!!! copy from currently selected instead of fixed 0 index?
	vtkColorTransferFunction* colorTransferFunction = getModality(0)->GetTransfer()->getColorFunction();
	vtkPiecewiseFunction* piecewiseFunction = getModality(0)->GetTransfer()->getOpacityFunction();
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

	setHistogramModality(0);

	Raycaster->reInitialize(imageData, polyData);
	for (int s = 0; s < 3; ++s)
	{
		// TODO: check how to update s:
		m_slicer[s]->updateChannel(0, iAChannelData(imageData, colorTransferFunction));
	}
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
		return;
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

	QSharedPointer<iAModalityTransfer> modTrans = getModality(0)->GetTransfer();
	if (numberOfVolumes > 0) {
		modTrans->getColorFunction()->DeepCopy(volumeStack->getColorTransferFunction(0));
		modTrans->getOpacityFunction()->DeepCopy(volumeStack->getPiecewiseFunction(0));
	}
	addVolumePlayer(volumeStack.data());

	Raycaster->reInitialize(imageData, polyData);
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateChannel(0, iAChannelData(imageData, modTrans->getColorFunction()));
	}
	updateViews();
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
	setModalities(ioThread->GetModalities());
}

int MdiChild::chooseModalityNr(QString const & caption)
{
	if (!isVolumeDataLoaded())
		return 0;
	if (getModalities()->size() == 1)
	{
		return 0;
	}
	QStringList parameters = (QStringList() << tr("+Channel"));
	QStringList modalities;
	for (int i = 0; i < getModalities()->size(); ++i)
	{
		modalities << getModality(i)->GetName();
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
	if (!isVolumeDataLoaded())
		return 0;
	int nrOfComponents = getModality(modalityNr)->GetImage()->GetNumberOfScalarComponents();
	if (nrOfComponents == 1)
		return 0;
	QStringList parameters = (QStringList() << tr("+Component"));
	QStringList components;
	for (int i = 0; i < nrOfComponents; ++i)
		components << QString::number(i);
	components << "All components";
	QList<QVariant> values = (QList<QVariant>() << components);
	dlg_commoninput componentChoice(this, "Choose Component", parameters, values, nullptr);
	if (componentChoice.exec() != QDialog::Accepted)
		return -1;
	return componentChoice.getComboBoxIndex(0);
}

bool MdiChild::save()
{
	if (isUntitled)
		return saveAs();
	else
	{
		int modalityNr = chooseModalityNr();
		if (modalityNr == -1)
			return false;
		/*
		// choice: save single modality, or modality stack!
		if (getModality(modalityNr)->ComponentCount() > 1)
		{                         // should be ChannelCount()
		}
		*/
		int componentNr = chooseComponentNr(modalityNr);
		if (componentNr == -1)
			return false;

		return saveFile(getModality(modalityNr)->GetFileName(), modalityNr, componentNr);
	}
}

bool MdiChild::saveAs()
{
	// TODO: unify with saveFile second part
	int modalityNr = chooseModalityNr();
	if (modalityNr == -1)
		return false;
	return saveAs(modalityNr);
}

bool MdiChild::saveAs(int modalityNr)
{
	int componentNr = chooseComponentNr(modalityNr);
	if (componentNr == -1)
		return false;
	QString filePath = (getModalities()->size() > 0) ? QFileInfo(getModality(modalityNr)->GetFileName()).absolutePath() : path;
	QString f = QFileDialog::getSaveFileName(
		this,
		tr("Save As"),
		filePath,
		iAIOProvider::GetSupportedSaveFormats() +
		tr(";;TIFF stack (*.tif);; PNG stack (*.png);; BMP stack (*.bmp);; JPEG stack (*.jpg);; DICOM serie (*.dcm)"));
	if (f.isEmpty())
		return false;
	return saveFile(f, modalityNr, componentNr);
}

void MdiChild::waitForPreviousIO()
{
	if (ioThread)
	{
		addMsg(tr("Waiting for I/O operation to complete..."));
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
		if (!isVolumeDataLoaded()) {
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
					addMsg(QString("Writer for %1 only supports %2 input!")
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

	if (isVolumeDataLoaded())
	{
		tmpSaveImg = getModality(modalityNr)->GetImage();
		if (tmpSaveImg->GetNumberOfScalarComponents() > 1 &&
			componentNr != tmpSaveImg->GetNumberOfScalarComponents())
		{
			auto imgExtract = vtkSmartPointer<vtkImageExtractComponents>::New();
			imgExtract->SetInputData(tmpSaveImg);
			imgExtract->SetComponents(componentNr);
			imgExtract->Update();
			tmpSaveImg = imgExtract->GetOutput();
		}
	}

	ioThread = new iAIO(tmpSaveImg, polyData, m_logger, this);
	connectIOThreadSignals(ioThread);
	connect(ioThread, SIGNAL(done()), this, SLOT(saveFinished()));
	m_storedModalityNr = modalityNr;
	if (!setupSaveIO(f)) {
		ioFinished();
		return false;
	}

	addMsg(tr("Saving file '%1', please wait...").arg(f));
	ioThread->start();

	return true;
}

void MdiChild::updateViews()
{
	updateSlicers();

	Raycaster->update();
	Raycaster->GetRenderWindow()->GetInteractor()->Modified();
	Raycaster->GetRenderWindow()->GetInteractor()->Render();
	renderer->vtkWidgetRC->update();
	emit viewsUpdated();
}

int MdiChild::getVisibility() const
{

	int vis = RC | YZ | XZ | XY;
	return vis;
}

void MdiChild::clearLogs()
{
	m_dlgLog->listWidget->clear();
}

void MdiChild::maximizeXY()
{
	resizeDockWidget(m_dlgSlicer[iASlicerMode::XY]);
}

void MdiChild::maximizeXZ()
{
	resizeDockWidget(m_dlgSlicer[iASlicerMode::XZ]);
}

void MdiChild::maximizeYZ()
{
	resizeDockWidget(m_dlgSlicer[iASlicerMode::YZ]);
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

void MdiChild::triggerInteractionRaycaster()
{
	if (Raycaster->GetInteractor()->GetEnabled()){
		Raycaster->disableInteractor();
		addMsg(tr("Renderer disabled."));
	} else {
		Raycaster->enableInteractor();
		addMsg(tr("Renderer enabled."));
	}
}

void MdiChild::setSlice(int mode, int s)
{
	int sliceAxis = getSlicerDimension(mode);
	if (snakeSlicer)
	{
		updateSnakeSlicer(m_dlgSlicer[mode]->sbSlice, m_slicer[mode], sliceAxis, s);
	}
	else
	{
		m_position[mode] = s;
		if (renderSettings.ShowSlicers || renderSettings.ShowSlicePlanes)
		{
			double plane[3];
			std::fill(plane, plane + 3, 0);
			plane[sliceAxis] = s * imageData->GetSpacing()[sliceAxis];
			Raycaster->setSlicePlane(sliceAxis, plane[0], plane[1], plane[2]);
		}
	}
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

	slicer->setTransform(final_transform);
	slicer->setSliceNumber(point1[ptIndex]);
}

void MdiChild::setChannel(int c)
{
	disableRenderWindows(c);
	enableRenderWindows();
}

void MdiChild::slicerRotationChanged()
{
	Raycaster->setPlaneNormals( slicerTransform );
}

void MdiChild::linkViews( bool l)
{
	slicerSettings.LinkViews = l;
}

void MdiChild::linkMDIs(bool lm)
{
	slicerSettings.LinkMDIs = lm;
}

void MdiChild::enableInteraction( bool b)
{
	for (int s = 0; s < 3; ++s)
		if (b)
			m_slicer[s]->enableInteractor();
		else
			m_slicer[s]->disableInteractor();
}

bool MdiChild::editPrefs(iAPreferences const & prefs)
{
	preferences = prefs;
	if (ioThread)	// don't do any updates if image still loading
		return true;
	setHistogramModality(m_currentModality);	// to update Histogram bin count
	applyViewerPreferences();
	if (isMagicLensToggled())
	{
		updateSlicers();
	}

	emit preferencesChanged();

	return true;
}

void MdiChild::applyViewerPreferences()
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensFrameWidth(preferences.MagicLensFrameWidth);
		m_slicer[s]->setMagicLensSize(preferences.MagicLensSize);
		m_slicer[s]->setStatisticalExtent(preferences.StatisticalExtent);
	}
	renderer->vtkWidgetRC->setLensSize(preferences.MagicLensSize, preferences.MagicLensSize);
	Raycaster->setStatExt(preferences.StatisticalExtent);
}

void MdiChild::setRenderSettings(iARenderSettings const & rs, iAVolumeSettings const & vs)
{
	renderSettings = rs;
	volumeSettings = vs;
}

void MdiChild::applyRenderSettings(iARenderer* raycaster)
{
	raycaster->ApplySettings(renderSettings);
}

void MdiChild::applyVolumeSettings(const bool loadSavedVolumeSettings)
{
	for (int i = 0; i < 3; ++i)
		m_dlgSlicer[i]->showBorder(renderSettings.ShowSlicePlanes);
	m_dlgModalities->ShowSlicers(renderSettings.ShowSlicers);
	m_dlgModalities->ChangeRenderSettings(volumeSettings, loadSavedVolumeSettings);
}

QString MdiChild::getLayoutName() const
{
	return m_layout;
}

void MdiChild::updateLayout()
{
	m_mainWnd->loadLayout();
}

void MdiChild::loadLayout(QString const & layout)
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

int MdiChild::getRenderMode() const
{
	return volumeSettings.RenderMode;
}

int const * MdiChild::position() const
{
	return m_position;
}

void MdiChild::setupSlicers(iASlicerSettings const & ss, bool init)
{
	slicerSettings = ss;

	if (snakeSlicer)
	{
		// TODO: check why only XY slice here?
		int prevMax   = m_dlgSlicer[iASlicerMode::XY]->sbSlice->maximum();
		int prevValue = m_dlgSlicer[iASlicerMode::XY]->sbSlice->value();
		m_dlgSlicer[iASlicerMode::XY]->sbSlice->setRange(0, ss.SnakeSlices-1);
		m_dlgSlicer[iASlicerMode::XY]->sbSlice->setValue((double)prevValue/prevMax*(ss.SnakeSlices-1));
	}

	linkViews(ss.LinkViews);
	linkMDIs(ss.LinkMDIs);

	for (int s = 0; s<3; ++s)
		m_slicer[s]->setup(ss.SingleSlicer);

	if (init)
	{
		// connect signals for making slicers update other views in snake slicers mode:
		for (int i = 0; i < 3; ++i)
		{
			connect(m_slicer[i], SIGNAL(arbitraryProfileChanged(int, double*)), this, SLOT(updateProbe(int, double*)));
			connect(m_slicer[i], SIGNAL(arbitraryProfileChanged(int, double*)), Raycaster, SLOT(setArbitraryProfile(int, double*)));
			for (int j = 0; j < 3; ++j)
			{
				if (i != j)	// connect each slicer's signals to the other slicer's slots, except for its own:
				{
					//Adding new point to the parametric spline for snake slicer
					connect(m_slicer[i], SIGNAL(addedPoint(double, double, double)), m_slicer[j], SLOT(addPoint(double, double, double)));
					//Moving point
					connect(m_slicer[i], SIGNAL(movedPoint(size_t, double, double, double)), m_slicer[j], SLOT(movePoint(size_t, double, double, double)));
					//Changing arbitrary profile positioning
					connect(m_slicer[i], SIGNAL(arbitraryProfileChanged(int, double*)), m_slicer[j], SLOT(setArbitraryProfile(int, double*)));

					connect(m_slicer[i], SIGNAL(switchedMode(int)),  m_slicer[j], SLOT(switchInteractionMode(int)));
					connect(m_slicer[i], SIGNAL(deletedSnakeLine()), m_slicer[j], SLOT(deleteSnakeLine()));
					connect(m_slicer[i], SIGNAL(deselectedPoint()),  m_slicer[j], SLOT(deselectPoint()));
				}
			}
		}
	}
}

bool MdiChild::editRendererSettings(iARenderSettings const & rs, iAVolumeSettings const & vs)
{
	setRenderSettings(rs, vs);
	applyVolumeSettings(false);
	applyRenderSettings(Raycaster);
	renderer->vtkWidgetRC->show();
	renderer->vtkWidgetRC->GetRenderWindow()->Render();
	emit renderSettingsChanged();
	return true;
}

iARenderSettings const & MdiChild::getRenderSettings() const
{
	return renderSettings;
}

iAVolumeSettings const &  MdiChild::getVolumeSettings() const
{
	return volumeSettings;
}

iASlicerSettings const & MdiChild::getSlicerSettings() const
{
	return slicerSettings;
}

iAPreferences const & MdiChild::getPreferences() const
{
	return preferences;
}

bool MdiChild::editSlicerSettings(iASlicerSettings const & slicerSettings)
{
	setupSlicers(slicerSettings, false);
	for (int s = 0; s<3; ++s)
		m_slicer[s]->show();
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
	if (m_histogram)
		m_histogram->update();
}

void MdiChild::resetTrf()
{
	if (!m_histogram) return;
	m_histogram->resetTrf();
	addMsg(tr("Resetting Transfer Functions."));
	addMsg(tr("  Adding transfer function point: %1.   Opacity: 0.0,   Color: 0, 0, 0")
		.arg(m_histogram->xBounds()[0]));
	addMsg(tr("  Adding transfer function point: %1.   Opacity: 1.0,   Color: 255, 255, 255")
		.arg(m_histogram->xBounds()[1]));
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
	/*
	TODO: slicer remove first image - check snake slicer!
	snakeSlicer = isChecked;

	if (snakeSlicer)
	{
		//save the slicer transforms
		SlicerYZ_Transform = m_slicer[iASlicerMode::YZ]->GetReslicer()->GetResliceTransform();
		SlicerXY_Transform = m_slicer[iASlicerMode::XY]->GetReslicer()->GetResliceTransform();
		SlicerXZ_Transform = m_slicer[iASlicerMode::XZ]->GetReslicer()->GetResliceTransform();

		parametricSpline->Modified();
		double emptyper[3]; emptyper[0] = 0; emptyper[1] = 0; emptyper[2] = 0;
		double emptyp[3]; emptyp[0] = 0; emptyp[1] = 0; emptyp[2] = 0;
		parametricSpline->Evaluate(emptyper, emptyp, nullptr);

		m_dlgSlicer[iASlicerMode::XY]->spinBoxXY->setValue(0);//set initial value
		m_dlgSlicer[iASlicerMode::XZ]->spinBoxXZ->setValue(0);//set initial value
		m_dlgSlicer[iASlicerMode::YZ]->spinBoxYZ->setValue(0);//set initial value

		for (int s = 0; s<3; ++s)
			m_slicer[s]->switchInteractionMode(iASlicer::NORMAL);
	}
	else
	{
		renderSettings.ShowSlicers = false;

		m_dlgSlicer[iASlicerMode::XY]->spinBoxXY->setValue(imageData->GetDimensions()[2]>>1);
		m_slicer[iASlicerMode::XY]->GetReslicer()->SetResliceAxesDirectionCosines(1,0,0,  0,1,0,  0,0,1);
		m_slicer[iASlicerMode::XY]->GetReslicer()->SetResliceTransform(SlicerXY_Transform);
		m_slicer[iASlicerMode::XY]->GetReslicer()->SetOutputExtentToDefault();
		m_slicer[iASlicerMode::XY]->getRenderer()->ResetCamera();
		m_slicer[iASlicerMode::XY]->getRenderer()->Render();

		m_dlgSlicer[iASlicerMode::XZ]->spinBoxXZ->setValue(imageData->GetDimensions()[1]>>1);
		m_slicer[iASlicerMode::XZ]->GetReslicer()->SetResliceAxesDirectionCosines(1,0,0,  0,0,1,  0,-1,0);
		m_slicer[iASlicerMode::XZ]->GetReslicer()->SetResliceTransform(SlicerXZ_Transform);
		m_slicer[iASlicerMode::XZ]->GetReslicer()->SetOutputExtentToDefault();
		m_slicer[iASlicerMode::XZ]->getRenderer()->ResetCamera();
		m_slicer[iASlicerMode::XZ]->getRenderer()->Render();

		m_dlgSlicer[iASlicerMode::YZ]->spinBoxYZ->setValue(imageData->GetDimensions()[0]>>1);
		m_slicer[iASlicerMode::YZ]->GetReslicer()->SetResliceAxesDirectionCosines(0,1,0,  0,0,1,  1,0,0);
		m_slicer[iASlicerMode::YZ]->GetReslicer()->SetResliceTransform(SlicerYZ_Transform);
		m_slicer[iASlicerMode::YZ]->GetReslicer()->SetOutputExtentToDefault();
		m_slicer[iASlicerMode::YZ]->getRenderer()->ResetCamera();
		m_slicer[iASlicerMode::YZ]->getRenderer()->Render();

		/*
		// TODO: VOLUME: VolumeManager
		if (renderSettings.ShowSlicers)
			Raycaster->showSlicers(true);
		* /
		for (int s = 0; s<3; ++s)
			m_slicer[s]->switchInteractionMode(iASlicer::DEFINE_SPLINE);
	}
	*/
}

/*
void MdiChild::updateReslicer(double point[3], double normal[3], int mode)
{
	// translation to origin
	double t_matrix[16] = {
		        1,         0,         0, 0,
		        0,         1,         0, 0,
		        0,         0,         1, 0,
		-point[0], -point[1], -point[2], 1
	};
	vtkMatrix4x4 * translation_matrix = vtkMatrix4x4::New();
	translation_matrix->DeepCopy(t_matrix);

	// rotation to make vector parallel to the z axis
	double diagonal = sqrt (pow(normal[0],2) + pow(normal[1],2) + pow(normal[2],2) );
	double intermediate_dia = sqrt ( pow(normal[0],2) + pow(normal[1],2) );
	double cos_theta = normal[2] / diagonal;
	double sin_theta = intermediate_dia / diagonal;
	double r_matrix[16] = {
		 cos_theta, 0, sin_theta, 0,
		         0, 1,         0, 0,
		-sin_theta, 0, cos_theta, 0,
		         0, 0,         0, 1
	};
	vtkMatrix4x4 * rotation_matrix = vtkMatrix4x4::New();
	rotation_matrix->DeepCopy(r_matrix);

	// rotation in Z axis by 180 degree
	double cos_theta_z = cos(vtkMath::Pi());
	double sin_theta_z = sin(vtkMath::Pi());
	double r_matrix_z[16] = {
		cos_theta_z, -sin_theta_z, 0, 0,
		sin_theta_z,  cos_theta_z, 0, 0,
		          0,            0, 1, 0,
		          0,            0, 0, 1
	};
	vtkMatrix4x4 * rotation_matrix_z = vtkMatrix4x4::New();
	rotation_matrix_z->DeepCopy(r_matrix_z);

	// translate back to object position
	double bt_matrix[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		point[0], point[1], point[2], 1};
	vtkMatrix4x4 * backtranslation_matrix = vtkMatrix4x4::New();
	backtranslation_matrix->DeepCopy(bt_matrix);

	// get the final transformation matrix to apply on the image
	vtkMatrix4x4 * intermediate_transformation_1 = vtkMatrix4x4::New();
	vtkMatrix4x4 * intermediate_transformation_2 = vtkMatrix4x4::New();
	vtkMatrix4x4 * final_transformation_matrix = vtkMatrix4x4::New();
	vtkMatrix4x4::Multiply4x4(translation_matrix, rotation_matrix, intermediate_transformation_1);
	vtkMatrix4x4::Multiply4x4(intermediate_transformation_1, rotation_matrix_z, intermediate_transformation_2);
	vtkMatrix4x4::Multiply4x4(intermediate_transformation_2, backtranslation_matrix, final_transformation_matrix);

	if ( mode == iASlicerMode::XY )
	{
		double a_matrix[16] = {
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		};
		vtkMatrix4x4 * axial_matrix = vtkMatrix4x4::New();
		axial_matrix->DeepCopy(a_matrix);
		vtkMatrix4x4 * axial_transformation_matrix = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(final_transformation_matrix, axial_matrix, axial_transformation_matrix);

		vtkMatrixToLinearTransform  * axial_transform = vtkMatrixToLinearTransform ::New();
		axial_transform->SetInput(axial_transformation_matrix);
		axial_transform->Update();

		m_slicer[iASlicerMode::XY]->GetReslicer()->SetResliceAxes(axial_transformation_matrix);
	}

	if ( mode == iASlicerMode::YZ )
	{
		double c_matrix[16] = {
			0, 0, 1, 0,
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 0, 1
		};
		vtkMatrix4x4 * coronial_matrix = vtkMatrix4x4::New();
		coronial_matrix->DeepCopy(c_matrix);
		vtkMatrix4x4 * coronial_transformation_matrix = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(final_transformation_matrix, coronial_matrix, coronial_transformation_matrix);

		vtkMatrixToLinearTransform  * coronial_transform = vtkMatrixToLinearTransform ::New();
		coronial_transform->SetInput(coronial_transformation_matrix);
		coronial_transform->Update();

		m_slicer[iASlicerMode::YZ]->GetReslicer()->SetResliceAxes(coronial_transformation_matrix);
	}

	if ( mode == iASlicerMode::XZ )
	{
		double s_matrix[16] = {
			1, 0,  0, 0,
			0, 0, -1, 0,
			0, 1,  0, 0,
			0, 0,  0, 1
		};
		vtkMatrix4x4 * sagittal_matrix = vtkMatrix4x4::New();
		sagittal_matrix->DeepCopy(s_matrix);
		vtkMatrix4x4 * sagittal_transformation_matrix = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(final_transformation_matrix, sagittal_matrix, sagittal_transformation_matrix);

		vtkMatrixToLinearTransform  * sagittal_transform = vtkMatrixToLinearTransform ::New();
		sagittal_transform->SetInput(sagittal_transformation_matrix);
		sagittal_transform->Update();

		m_slicer[iASlicerMode::XZ]->GetReslicer()->SetResliceAxes(sagittal_transformation_matrix);
	}
}
*/

void MdiChild::getSnakeNormal(int index, double point[3], double normal[3])
{
	int i1 = index;
	int i2 = index + 1;

	double spacing[3];
	imageData->GetSpacing(spacing);

	int snakeSlices = slicerSettings.SnakeSlices;
	if (index == (snakeSlices - 1))
	{
		i1--;
		i2--;
	}

	if (index >= 0 && index < snakeSlices)
	{
		double p1[3], p2[3];
		double t1[3] =
		{ (double)i1 / (snakeSlices - 1), (double)i1 / (snakeSlices - 1), (double)i1 / (snakeSlices - 1) };
		double t2[3] = { (double)i2 / (snakeSlices - 1), (double)i2 / (snakeSlices - 1), (double)i2 / (snakeSlices - 1) };
		parametricSpline->Evaluate(t1, p1, nullptr);
		parametricSpline->Evaluate(t2, p2, nullptr);

		//calculate the points
		p1[0] /= spacing[0]; p1[1] /= spacing[1]; p1[2] /= spacing[2];
		p2[0] /= spacing[0]; p2[1] /= spacing[1]; p2[2] /= spacing[2];

		//calculate the vector between to points
		if (normal)
		{
			normal[0] = p2[0] - p1[0];
			normal[1] = p2[1] - p1[1];
			normal[2] = p2[2] - p1[2];
		}

		point[0] = p1[0]; point[1] = p1[1]; point[2] = p1[2];
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
		m_slicer[s]->setSliceProfileOn(isSliceProfileEnabled);
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
		changeMagicLensModality(0);
	}
	setMagicLensEnabled(isEnabled);
	updateSlicers();

	emit magicLensToggled(isMagicLensEnabled);
}

bool MdiChild::isMagicLensToggled(void) const
{
	return isMagicLensEnabled;
}

bool MdiChild::initView( QString const & title )
{
	if (!raycasterInitialized)
	{
		Raycaster->initialize(imageData, polyData);
		connect(Raycaster->getRenderObserver(), SIGNAL(InteractorModeSwitched(int)), m_dlgModalities, SLOT(InteractorModeSwitched(int)));
		raycasterInitialized = true;
	}
	if (getModalities()->size() == 0 && isVolumeDataLoaded())
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
		getModalities()->Add(mod);
		m_dlgModalities->AddListItem(mod);
		QSharedPointer<iAModalityTransfer> modTrans = getModality(0)->GetTransfer();
		uint channelID = createChannel();
		assert(channelID == 0); // first modality we create, there shouldn't be another channel yet!
		getModality(0)->setChannelID(channelID);
		for (int s = 0; s < 3; ++s)
		{
			m_slicer[s]->addChannel(channelID, iAChannelData(getModality(0)->GetImage(), modTrans->getColorFunction()), true);
			m_slicer[s]->resetCamera();
		}
		m_initVolumeRenderers = true;
	}
	vtkColorTransferFunction* colorFunction = (getModalities()->size() > 0)
		? getModality(0)->GetTransfer()->getColorFunction() : vtkColorTransferFunction::New();

	renderer->stackedWidgetRC->setCurrentIndex(0);

	if (isVolumeDataLoaded())
	{
		this->addImageProperty();
		if (imageData->GetNumberOfScalarComponents() == 1) //No histogram/profile for rgb, rgba or vector pixel type images
		{
			tabifyDockWidget(m_dlgLog, m_histogramContainer);
			this->addProfile();
		}
	}
	else
	{	//Polygonal mesh is loaded
		showPoly();
		hideHistogram();
	}

	//Load the layout to the child
	updateLayout();

	return true;
}

void MdiChild::hideHistogram()
{
	m_histogramContainer->hide();
}

void MdiChild::addImageProperty()
{
	if (imgProperty)
		return;
	imgProperty = new dlg_imageproperty(this);
	tabifyDockWidget(m_dlgLog, imgProperty);
}

void MdiChild::updateImageProperties()
{
	if (!imgProperty)
	{
		return;
	}
	imgProperty->Clear();
	for (int i = 0; i < getModalities()->size(); ++i)
	{
		imgProperty->AddInfo(getModality(i)->GetImage(), getModality(i)->Info(), getModality(i)->GetName(),
			(i == 0 &&
			getModality(i)->ComponentCount() == 1 &&
			volumeStack->getNumberOfVolumes() > 1) ?
				volumeStack->getNumberOfVolumes() :
				getModality(i)->ComponentCount()
			);
	}
}

bool MdiChild::addVolumePlayer(iAVolumeStack* volumeStack)
{
	volumePlayer = new dlg_volumePlayer(this, volumeStack);
	tabifyDockWidget(m_dlgLog, volumePlayer);
	for (int id=0; id<volumeStack->getNumberOfVolumes(); id++) {
		CheckedList.append(0);
	}
	connect(m_histogram, SIGNAL(applyTFForAll()), volumePlayer, SLOT(applyForAll()));

	return true;
}

int MdiChild::evaluatePosition(int pos, int i, bool invert)
{
	if ( pos < 0 ) invert ? (pos = imageData->GetExtent()[i]) : (pos = 0);
	if ( pos > imageData->GetExtent()[i] ) invert ? (pos = 0) : (pos = imageData->GetExtent()[i]);
	return pos;
}

void MdiChild::addMsg(QString txt)
{
	m_dlgLog->listWidget->addItem(tr("%1  %2").arg(QLocale().toString(QDateTime::currentDateTime(), QLocale::ShortFormat)).arg(txt));
	m_dlgLog->listWidget->scrollToBottom();
	m_dlgLog->listWidget->repaint();
}

void MdiChild::addStatusMsg(QString txt)
{
	m_mainWnd->statusBar()->showMessage(txt, 10000);
}

bool MdiChild::isMaximized()
{
	return visibility != MULTI;
}

void MdiChild::updateROI(int const roi[6])
{
	for (int s = 0; s<3; ++s)
		m_slicer[s]->updateROI(roi);

	const double* spacing = getModality(0)->GetSpacing();
	getRenderer()->setSlicingBounds(roi, spacing);
}

void MdiChild::setROIVisible(bool visible)
{
	for (int s = 0; s<3; ++s)
		m_slicer[s]->setROIVisible(visible);
	getRenderer()->setCubeVisible(visible);
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
	m_dlgSlicer[iASlicerMode::XY]->setVisible(xy);
	m_dlgSlicer[iASlicerMode::YZ]->setVisible(yz);
	m_dlgSlicer[iASlicerMode::XZ]->setVisible(xz);

	m_dlgLog->setVisible(tab);
	if (isVolumeDataLoaded())
	{	// TODO: check redundancy with hideHistogram calls?
		m_histogramContainer->setVisible(tab);
	}
}

void MdiChild::hideVolumeWidgets()
{
	setVisibility(QList<QWidget*>() << m_dlgSlicer[iASlicerMode::XY] << m_dlgSlicer[iASlicerMode::XZ] << m_dlgSlicer[iASlicerMode::YZ] << renderer, false);
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
	m_slicer[index]->update();
}

void MdiChild::initChannelRenderer(uint id, bool use3D, bool enableChannel)
{
	iAChannelData * data = getChannelData(id);
	assert(data);
	if (!data)
	{
		return;
	}
	for (int s = 0; s < 3; ++s)
		m_slicer[s]->addChannel(id, *data, false);
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
		data->set3D(true);
		m_dlgModalities->AddModality(data->getImage(), QString("Channel %1").arg(id));
	}
	setChannelRenderingEnabled(id, enableChannel);
}

void MdiChild::setSlicerPieGlyphsEnabled( bool isOn )
{
	for (int s = 0; s<3; ++s)
		m_slicer[s]->setPieGlyphsOn(isOn);
}

void MdiChild::setPieGlyphParameters( double opacity, double spacing, double magFactor )
{
	for (int s = 0; s<3; ++s)
		m_slicer[s]->setPieGlyphParameters(opacity, spacing, magFactor);
}

iAChannelData * MdiChild::getChannelData(uint id)
{
	auto it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return nullptr;
	}
	return it->data();
}

iAChannelData const * MdiChild::getChannelData(uint id) const
{
	auto it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return nullptr;
	}
	return it->data();
}

uint MdiChild::createChannel()
{
	uint newChannelID = m_nextChannelID;
	++m_nextChannelID;
	m_channels.insert(newChannelID, QSharedPointer<iAChannelData>(new iAChannelData()));
	return newChannelID;
}


void MdiChild::updateSlicers()
{
	for (int s = 0; s < 3; ++s)
		m_slicer[s]->update();
}

void MdiChild::updateChannelOpacity(uint id, double opacity)
{
	if (!getChannelData(id))
	{
		return;
	}
	getChannelData(id)->setOpacity(opacity);
	for (int s = 0; s<3; ++s)
		m_slicer[s]->setChannelOpacity(id, opacity);
	updateSlicers();
}

void MdiChild::setChannelRenderingEnabled(uint id, bool enabled)
{
	iAChannelData * data = getChannelData(id);
	if (!data || data->isEnabled() == enabled)
	{
		// the channel with the given ID doesn't exist or hasn't changed
		return;
	}
	data->setEnabled(enabled);
	setSlicerChannelEnabled(id, enabled);
	/*
	// TODO: VOLUME: rewrite using volume manager:
	if (data->Uses3D())
	{
		getRenderer()->updateChannelImages();
	}
	*/
	updateViews();
}

void MdiChild::setSlicerChannelEnabled(uint id, bool enabled)
{
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
		slicer(i)->enableChannel(id, enabled);
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
		m_slicer[s]->setArbitraryProfile(0, start);
		m_slicer[s]->setArbitraryProfile(1, end);
	}
	Raycaster->setArbitraryProfile(0, start);
	Raycaster->setArbitraryProfile(1, end);
	profileProbe->UpdateProbe(0, start);
	profileProbe->UpdateProbe(1, end);
	profileProbe->UpdateData();
	imgProfile = new dlg_profile(this, profileProbe->profileData, profileProbe->GetRayLength());
	tabifyDockWidget(m_dlgLog, imgProfile);
	connect(imgProfile->profileMode, SIGNAL(toggled(bool)), this, SLOT(toggleArbitraryProfile(bool)));
}

void MdiChild::toggleArbitraryProfile( bool isChecked )
{
	isArbProfileEnabled = (bool)isChecked;
	for (int s = 0; s<3; ++s)
		m_slicer[s]->setArbitraryProfileOn(isArbProfileEnabled);
	Raycaster->setArbitraryProfileOn(isArbProfileEnabled);
}

void MdiChild::updateProbe( int ptIndex, double * newPos )
{
	if (imageData->GetNumberOfScalarComponents() != 1) //No profile for rgb, rgba or vector pixel type images
		return;
	profileProbe->UpdateProbe(ptIndex, newPos);
	updateProfile();
}

void MdiChild::updateProfile()
{
	profileProbe->UpdateData();
	imgProfile->profileWidget->initialize(profileProbe->profileData, profileProbe->GetRayLength());
	imgProfile->profileWidget->update();
}

int MdiChild::sliceNumber(int mode) const
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_slicer[mode]->sliceNumber();
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
	if (m_isSmthMaximized)
		if (m_whatMaximized == dw)
			demaximizeDockWidget(dw);
		else
		{
			demaximizeDockWidget(m_whatMaximized);
			maximizeDockWidget(dw);
		}
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

iASlicer* MdiChild::slicer(int mode)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_slicer[mode];
}

dlg_slicer * MdiChild::slicerDlg(int mode)
{
	assert(0 >= mode && mode < 3);
	return m_dlgSlicer[mode];
}

dlg_renderer * MdiChild::getRendererDlg()
{
	return renderer;
}

dlg_imageproperty * MdiChild::getImagePropertyDlg()
{
	return imgProperty;
}

dlg_profile * MdiChild::getProfileDlg()
{
	return imgProfile;
}

dlg_logs * MdiChild::getLogDlg()
{
	return m_dlgLog;
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

void MdiChild::setMagicLensInput(uint id, bool initReslice)
{
	for (int s = 0; s<3; ++s)
		m_slicer[s]->setMagicLensInput(id);
}

void MdiChild::setMagicLensEnabled(bool isOn)
{
	for (int s = 0; s<3; ++s)
		m_slicer[s]->setMagicLensEnabled( isOn );
}

void MdiChild::updateChannel(uint id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf,  bool enable)
{
	iAChannelData * chData = getChannelData( id );
	if (!chData)
		return;
	chData->setData( imgData, ctf, otf );
	for (uint s = 0; s < 3; ++s)
	{
		if (m_slicer[s]->hasChannel(id))
			m_slicer[s]->updateChannel(id, *chData);
		else
			m_slicer[s]->addChannel(id, *chData, enable);
	}
}

void MdiChild::reInitMagicLens(uint id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf)
{
	if (!isMagicLensEnabled)
		return;

	// TODO: check - magic lens should probably not use otf!
	iAChannelData chData(imgData, ctf, otf);
	for (int s = 0; s<3; ++s)
		m_slicer[s]->updateChannel(id, chData);
	setMagicLensInput( id, true);
	updateSlicers();
}

void MdiChild::updateChannelMappers()
{
	for (int s = 0; s<3; ++s)
		m_slicer[s]->updateChannelMappers();
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

bool MdiChild::isVolumeDataLoaded() const
{
	QString suffix = getFileInfo().suffix();
	int * extent = imageData->GetExtent();
	return QString::compare(suffix, "STL", Qt::CaseInsensitive) != 0 &&
		QString::compare(suffix, "VTK", Qt::CaseInsensitive) != 0 &&
		QString::compare(suffix, "FEM", Qt::CaseInsensitive) != 0 &&
		extent[1] >= 0 && extent[3] >= 0 && extent[5] >= 0;
}

void MdiChild::changeMagicLensModality(int chg)
{
	m_currentComponent = (m_currentComponent + chg);
	if (m_currentComponent < 0 || m_currentComponent >= getModality(m_currentModality)->ComponentCount())
	{
		m_currentComponent = 0;
		m_currentModality = (m_currentModality + chg + getModalities()->size()) % (getModalities()->size());
	}
	if (m_currentModality < 0 || m_currentModality >= getModalities()->size())
	{
		DEBUG_LOG("Invalid modality index!");
		m_currentModality = 0;
		return;
	}
	if (m_magicLensChannel == NotExistingChannel)
		m_magicLensChannel = createChannel();
	vtkSmartPointer<vtkImageData> img = getModality(m_currentModality)->GetComponent(m_currentComponent);
	getChannelData(m_magicLensChannel)->setOpacity(0.5);
	QString name(getModality(m_currentModality)->GetImageName(m_currentComponent));
	getChannelData(m_magicLensChannel)->setName(name);
	updateChannel(m_magicLensChannel, img, m_dlgModalities->GetCTF(m_currentModality), m_dlgModalities->GetOTF(m_currentModality), false);
	setMagicLensInput(m_magicLensChannel, true);
	setHistogramModality(m_currentModality);	// TODO: don't change histogram, just read/create min/max and transfer function?
}

void MdiChild::changeMagicLensOpacity(int chg)
{
	for (int s=0; s<3; ++s)
		m_slicer[s]->setMagicLensOpacity(m_slicer[s]->getMagicLensOpacity() + (chg*0.05));
}

void MdiChild::changeMagicLensSize(int chg)
{
	if (!isMagicLensToggled())
		return;
	double sizeFactor = 1.1 * (std::abs(chg));
	if (chg < 0)
		sizeFactor = 1 / sizeFactor;
	int newSize = std::max(MinimumMagicLensSize, static_cast<int>(preferences.MagicLensSize * sizeFactor));
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensSize(newSize);
		newSize = std::min(m_slicer[s]->getMagicLensSize(), newSize);
	}
	preferences.MagicLensSize = newSize;
	updateSlicers();
}

int MdiChild::getCurrentModality() const
{
	return m_currentModality;
}

void MdiChild::showModality(int modIdx)
{
	if (m_currentModality == modIdx)
		return;
	m_currentModality = modIdx;
	m_currentComponent = 0;
	setHistogramModality(modIdx);
}

void MdiChild::setModalities(QSharedPointer<iAModalityList> modList)
{
	bool noDataLoaded = getModalities()->size() == 0;
	m_dlgModalities->SetModalities(modList);

	if (noDataLoaded && getModalities()->size() > 0)
	{
		initModalities();
	}
}

dlg_modalities* MdiChild::getModalitiesDlg()
{
	return m_dlgModalities;
}

QSharedPointer<iAModalityList> MdiChild::getModalities()
{
	return m_dlgModalities->GetModalities();
}

QSharedPointer<iAModality> MdiChild::getModality(int idx)
{
	return getModalities()->Get(idx);
}

void MdiChild::initModalities()
{
	for (int i = 0; i < getModalities()->size(); ++i)
		m_dlgModalities->AddListItem(getModality(i));
	// TODO: VOLUME: rework - workaround: "initializes" renderer and slicers with modality 0
	m_initVolumeRenderers = true;
	setImageData(
		currentFile().isEmpty() ? getModality(0)->GetFileName() : currentFile(),
		getModality(0)->GetImage()
	);
	m_dlgModalities->SelectRow(0);
}

void MdiChild::setHistogramModality(int modalityIdx)
{
	if (!m_histogram || getModality(modalityIdx)->GetImage()->GetNumberOfScalarComponents() != 1) //No histogram/profile for rgb, rgba or vector pixel type images
		return;
	if (getModality(modalityIdx)->GetTransfer()->statisticsComputed())
	{
		displayHistogram(modalityIdx);
		return;
	}
	addMsg(QString("Computing statistics for modality %1...")
		.arg(getModality(modalityIdx)->GetName()));
	getModality(modalityIdx)->GetTransfer()->Info().setComputing();
	updateImageProperties();
	auto workerThread = new iAStatisticsUpdater(modalityIdx, getModality(modalityIdx));
	connect(workerThread, &iAStatisticsUpdater::StatisticsReady, this, &MdiChild::statisticsAvailable);
	connect(workerThread, &iAStatisticsUpdater::finished, workerThread, &QObject::deleteLater);
	workerThread->start();
}

void MdiChild::modalityAdded(int modalityIdx)
{
	if (getModality(modalityIdx)->GetImage()->GetNumberOfScalarComponents() == 1) //No histogram/profile for rgb, rgba or vector pixel type images
	{
		setHistogramModality(modalityIdx);
	}
	else
	{
		m_dlgModalities->InitDisplay(getModality(modalityIdx));
		applyVolumeSettings(false);
	}
}

void MdiChild::histogramDataAvailable(int modalityIdx)
{
	QString modalityName = getModality(modalityIdx)->GetName();
	m_currentHistogramModality = modalityIdx;
	addMsg(QString("Displaying histogram for modality %1.").arg(modalityName));
	m_histogram->removePlot(m_histogramPlot);
	m_histogramPlot = QSharedPointer<iAPlot>(new
		iABarGraphPlot(getModality(modalityIdx)->GetHistogramData(),
			QColor(70, 70, 70, 255)));
	m_histogram->addPlot(m_histogramPlot);
	m_histogram->setXCaption("Histogram " + modalityName);
	m_histogram->setTransferFunctions(getModality(modalityIdx)->GetTransfer()->getColorFunction(),
		getModality(modalityIdx)->GetTransfer()->getOpacityFunction());
	m_histogram->updateTrf();	// will also redraw() the histogram
	updateImageProperties();
	if (!findChild<iADockWidgetWrapper*>("Histogram"))
	{
		tabifyDockWidget(m_dlgLog, m_histogramContainer);
		this->addProfile();
	}
	emit histogramAvailable();
}

void MdiChild::displayHistogram(int modalityIdx)
{
	auto histData = getModality(modalityIdx)->GetTransfer()->getHistogramData();
	size_t newBinCount = preferences.HistogramBins;
	auto img = getModality(modalityIdx)->GetImage();
	auto scalarRange = img->GetScalarRange();
	if (isVtkIntegerType(getModality(modalityIdx)->GetImage()->GetScalarType()))
		newBinCount = std::min(newBinCount, static_cast<size_t>(scalarRange[1] - scalarRange[0] + 1));
	if (histData &&	histData->GetNumBin() == newBinCount)
	{
		if (modalityIdx != m_currentHistogramModality)
			histogramDataAvailable(modalityIdx);
		return;
	}

	addMsg(QString("Computing histogram for modality %1...")
		.arg(getModality(modalityIdx)->GetName()));
	auto workerThread = new iAHistogramUpdater(modalityIdx,
		getModality(modalityIdx), newBinCount);
	connect(workerThread, &iAHistogramUpdater::HistogramReady, this, &MdiChild::histogramDataAvailable);
	connect(workerThread, &iAHistogramUpdater::finished, workerThread, &QObject::deleteLater);
	workerThread->start();
}

void MdiChild::clearHistogram()
{
	m_histogram->removePlot(m_histogramPlot);
	m_histogramPlot = nullptr;
	m_histogram->setTransferFunctions(nullptr, nullptr);
	m_histogram->update();
}

void MdiChild::statisticsAvailable(int modalityIdx)
{
	displayHistogram(modalityIdx);
	// TODO: only initialize volume renderer of modalityIdx modality here!
	initVolumeRenderers();
	modalityTFChanged();
	updateViews();
}

void MdiChild::initVolumeRenderers()
{
	if (!m_initVolumeRenderers)
	{
		for (int i = 0; i < getModalities()->size(); ++i)
			getModality(i)->UpdateRenderer();
		return;
	}
	m_initVolumeRenderers = false;
	for (int i = 0; i < getModalities()->size(); ++i)
	{
		m_dlgModalities->InitDisplay(getModality(i));
	}
	applyVolumeSettings(true);
	connect(getModalities().data(), SIGNAL(Added(QSharedPointer<iAModality>)),
		m_dlgModalities, SLOT(modalityAdded(QSharedPointer<iAModality>)));
	Raycaster->GetRenderer()->ResetCamera();
}

void MdiChild::saveProject(QString const & fileName)
{
	ioThread = new iAIO(getModalities(), Raycaster->GetRenderer()->GetActiveCamera(), m_logger);
	connectIOThreadSignals(ioThread);
	QFileInfo fileInfo(fileName);
	if (!ioThread->setupIO(PROJECT_WRITER, fileInfo.absoluteFilePath()))
	{
		ioFinished();
		return;
	}
	addMsg(tr("Saving file '%1', please wait...").arg(fileName));
	ioThread->start();
	// TODO: only set new project file name if saving succeeded
	setCurrentFile(fileName);
}

void MdiChild::storeProject()
{
	QVector<int> unsavedModalities;
	for (int i=0; i<getModalities()->size(); ++i)
	{
		if (getModality(i)->GetFileName().isEmpty())
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
	return getModality(0)->GetTransfer()->getOpacityFunction();
}

vtkColorTransferFunction * MdiChild::getColorTransferFunction()
{
	return getModality(0)->GetTransfer()->getColorFunction();
}

void MdiChild::saveFinished()
{
	if (m_storedModalityNr < getModalities()->size() && ioThread->getIOID() != STL_WRITER)
		m_dlgModalities->SetFileName(m_storedModalityNr, ioThread->getFileName());
	setWindowModified(getModalities()->HasUnsavedModality());
}

/*
void MdiChild::splitDockWidget(QDockWidget* ref, QDockWidget* newWidget, Qt::Orientation orientation)
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
*/

bool MdiChild::isFullyLoaded() const
{
	int const * dim = imageData->GetDimensions();
	return dim[0] > 0 && dim[1] > 0 && dim[2] > 0;
}
