/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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
#include "charts/iAChartFunctionTransfer.h"
#include "charts/iAChartWithFunctionsWidget.h"
#include "charts/iAPlotTypes.h"
#include "charts/iAProfileWidget.h"
#include "dlg_commoninput.h"
#include "dlg_imageproperty.h"
#include "dlg_modalities.h"
#include "dlg_profile.h"
#include "dlg_slicer.h"
#include "dlg_volumePlayer.h"
#include "iAAlgorithm.h"
#include "iAChannelData.h"
#include "iAChannelSlicerData.h"
#include "iALog.h"
#include "iARunAsync.h"
#include "qthelper/iADockWidgetWrapper.h"
#include "iAModality.h"
#include "iAModalityList.h"
#include "iAModalityTransfer.h"
#include "iAMovieHelper.h"
#include "iAParametricSpline.h"
#include "iAPreferences.h"
#include "iAProfileProbe.h"
#include "iAProgress.h"
#include "iAProjectBase.h"
#include "iAProjectRegistry.h"
#include "iARenderer.h"
#include "iARenderObserver.h"
#include "iARenderSettings.h"
#include "iASlicer.h"
#include "iAToolsVTK.h"
#include "iATransferFunction.h"
#include "iAVolumeStack.h"
#include "iAVtkVersion.h"
#include "io/extension2id.h"
#include "io/iAFileUtils.h"    // for fileNameOnly
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

// TODO: VOLUME: check all places using modality(0)->transfer() !

#include <QByteArray>
#include <QFile>
#include <QFileDialog>
#include <QMainWindow>
#include <QMessageBox>
#include <QSettings>
#include <QSpinBox>
#include <QToolButton>
#include <QtGlobal> // for QT_VERSION


MdiChild::MdiChild(MainWindow* mainWnd, iAPreferences const& prefs, bool unsavedChanges) :
	m_mainWnd(mainWnd),
	m_preferences(prefs),
	m_isSmthMaximized(false),
	m_isUntitled(true),
	m_isSliceProfileEnabled(false),
	m_profileHandlesEnabled(false),
	m_isMagicLensEnabled(false),
	m_reInitializeRenderWindows(true),
	m_raycasterInitialized(false),
	m_snakeSlicer(false),
	m_worldProfilePoints(vtkPoints::New()),
	m_worldSnakePoints(vtkPoints::New()),
	m_parametricSpline(iAParametricSpline::New()),
	m_imageData(vtkSmartPointer<vtkImageData>::New()),
	m_polyData(vtkPolyData::New()),
	m_axesTransform(vtkTransform::New()),
	m_slicerTransform(vtkTransform::New()),
	m_volumeStack(new iAVolumeStack),
	m_ioThread(nullptr),
	m_histogram(new iAChartWithFunctionsWidget(nullptr, " Histogram", "Frequency")),
	m_dwHistogram(new iADockWidgetWrapper(m_histogram, "Histogram", "Histogram")),
	m_dwImgProperty(nullptr),
	m_dwProfile(nullptr),
	m_nextChannelID(0),
	m_magicLensChannel(NotExistingChannel),
	m_currentModality(0),
	m_currentComponent(0),
	m_currentHistogramModality(-1),
	m_initVolumeRenderers(false),
	m_interactionMode(imCamera)
{
	m_histogram->setYMappingMode(prefs.HistogramLogarithmicYAxis ? iAChartWidget::Logarithmic : iAChartWidget::Linear);
	std::fill(m_slicerVisibility, m_slicerVisibility + 3, false);
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
	setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
#endif
	setWindowModified(unsavedChanges);
	setupUi(this);
	//prepare window for handling dock widgets
	setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);

	//insert default dock widgets and arrange them in a simple layout
	m_dwRenderer = new dlg_renderer(this);
	for (int i = 0; i < 3; ++i)
	{
		m_slicer[i] = new iASlicer(this, static_cast<iASlicerMode>(i), true, true, m_slicerTransform, m_worldSnakePoints);
		m_dwSlicer[i] = new dlg_slicer(m_slicer[i]);
	}

	addDockWidget(Qt::LeftDockWidgetArea, m_dwRenderer);
	m_initialLayoutState = saveState();

	splitDockWidget(m_dwRenderer, m_dwSlicer[iASlicerMode::XZ], Qt::Horizontal);
	splitDockWidget(m_dwRenderer, m_dwSlicer[iASlicerMode::YZ], Qt::Vertical);
	splitDockWidget(m_dwSlicer[iASlicerMode::XZ], m_dwSlicer[iASlicerMode::XY], Qt::Vertical);

	setAttribute(Qt::WA_DeleteOnClose);

	m_visibility = MULTI;
	std::fill(m_position, m_position + 3, 0);

	m_parametricSpline->SetPoints(m_worldSnakePoints);

	m_renderer = new iARenderer(this);
	m_renderer->setAxesTransform(m_axesTransform);
	m_dwRenderer->vtkWidgetRC->SetMainRenderWindow((vtkGenericOpenGLRenderWindow*)m_renderer->renderWindow());

	m_dwModalities = new dlg_modalities(m_dwRenderer->vtkWidgetRC, m_renderer->renderer(), this);
	QSharedPointer<iAModalityList> modList(new iAModalityList);
	setModalities(modList);
	applyViewerPreferences();
	connectSignalsToSlots();

	m_worldProfilePoints->Allocate(2);
	connect(mainWnd, &MainWindow::fullScreenToggled, this, &MdiChild::toggleFullScreen);
	connect(mainWnd, &MainWindow::styleChanged, this, &MdiChild::styleChanged);
}

void MdiChild::toggleFullScreen()
{
	QWidget* mdiSubWin = qobject_cast<QWidget*>(parent());
	if (m_mainWnd->isFullScreen())
	{
		mdiSubWin->setWindowFlags(windowFlags() | Qt::FramelessWindowHint);
	}
	else
	{
		mdiSubWin->setWindowFlags(windowFlags() & ~Qt::FramelessWindowHint);
	}
	mdiSubWin->show();
}

void MdiChild::rendererKeyPressed(int keyCode)
{
	if (keyCode == 'a' || keyCode == 'c')
	{
		iAInteractionMode mode = (keyCode == 'a') ? imCamera : imRegistration;
		setInteractionMode(mode);
	}
}

MdiChild::~MdiChild()
{
	cleanWorkingAlgorithms();
	m_polyData->ReleaseData();
	m_axesTransform->Delete();
	m_slicerTransform->Delete();

	m_polyData->Delete();

	for (int s = 0; s < 3; ++s)
	{
		delete m_slicer[s];
	}
	delete m_renderer; m_renderer = nullptr;

	delete m_dwImgProperty;
	delete m_dwProfile;
}

void MdiChild::slicerVisibilityChanged(int mode)
{
	if (m_dwSlicer[mode]->isVisible() == m_slicerVisibility[mode])
	{
		return; // no change
	}
	m_slicerVisibility[mode] = m_dwSlicer[mode]->isVisible();
	if (m_renderSettings.ShowSlicePlanes)
	{
		m_renderer->showSlicePlane(mode, m_slicerVisibility[mode]);
	}
	m_renderer->update();
}

void MdiChild::connectSignalsToSlots()
{
	for (int mode = 0; mode < iASlicerMode::SlicerCount; ++mode)
	{
		connect(m_dwSlicer[mode]->pbMax, &QPushButton::clicked, [this, mode] { maximizeSlicer(mode); });
		connect(m_dwSlicer[mode], &QDockWidget::visibilityChanged, [this, mode]{ slicerVisibilityChanged(mode); });
	}

	connect(m_dwRenderer->pushMaxRC, &QPushButton::clicked, this, &MdiChild::maximizeRC);
	connect(m_dwRenderer->pushStopRC, &QPushButton::clicked, this, &MdiChild::triggerInteractionRaycaster);

	connect(m_dwRenderer->pushPX,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PX); });
	connect(m_dwRenderer->pushPY,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PY); });
	connect(m_dwRenderer->pushPZ,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PZ); });
	connect(m_dwRenderer->pushMX,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MX); });
	connect(m_dwRenderer->pushMY,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MY); });
	connect(m_dwRenderer->pushMZ,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MZ); });
	connect(m_dwRenderer->pushIso, &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::Iso); });
	connect(m_dwRenderer->pushSaveRC, &QPushButton::clicked, this, &MdiChild::saveRC);
	connect(m_dwRenderer->pushMovRC, &QPushButton::clicked, this, &MdiChild::saveMovRC);

	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::rightButtonReleasedSignal, m_renderer, &iARenderer::mouseRightButtonReleasedSlot);
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::leftButtonReleasedSignal, m_renderer, &iARenderer::mouseLeftButtonReleasedSlot);
	connect(m_dwRenderer->spinBoxRC, QOverload<int>::of(&QSpinBox::valueChanged), this, &MdiChild::setChannel);

	for (int s = 0; s < 3; ++s)
	{
		connect(m_slicer[s], &iASlicer::shiftMouseWheel, this, &MdiChild::changeMagicLensModality);
		connect(m_slicer[s], &iASlicer::altMouseWheel, this, &MdiChild::changeMagicLensOpacity);
		connect(m_slicer[s], &iASlicer::ctrlMouseWheel, this, &MdiChild::changeMagicLensSize);
		connect(m_slicer[s], &iASlicer::sliceRotated, this, &MdiChild::slicerRotationChanged);
		connect(m_slicer[s], &iASlicer::sliceNumberChanged, this, &MdiChild::setSlice);

		connect(m_slicer[s], &iASlicer::oslicerPos, this, &MdiChild::updatePositionMarker);
	}

	connect(m_histogram, &iAChartWithFunctionsWidget::updateViews, this, &MdiChild::updateViews);
	connect(m_histogram, &iAChartWithFunctionsWidget::pointSelected, this, &MdiChild::pointSelected);
	connect(m_histogram, &iAChartWithFunctionsWidget::noPointSelected, this, &MdiChild::noPointSelected);
	connect(m_histogram, &iAChartWithFunctionsWidget::endPointSelected, this, &MdiChild::endPointSelected);
	connect(m_histogram, &iAChartWithFunctionsWidget::active, this, &MdiChild::active);
	connect((iAChartTransferFunction*)(m_histogram->functions()[0]), &iAChartTransferFunction::changed, this, &MdiChild::modalityTFChanged);

	connect(m_dwModalities, &dlg_modalities::modalitiesChanged, this, &MdiChild::updateImageProperties);
	connect(m_dwModalities, &dlg_modalities::modalitiesChanged, this, &MdiChild::updateViews);
	connect(m_dwModalities, &dlg_modalities::modalitySelected , this, &MdiChild::showModality);
	connect(m_dwModalities, &dlg_modalities::modalitiesChanged, this, &MdiChild::resetCamera);
}

void MdiChild::connectThreadSignalsToChildSlots(iAAlgorithm* thread)
{
	connect(thread, &iAAlgorithm::startUpdate, this, &MdiChild::updateRenderWindows);
	connect(thread, &iAAlgorithm::finished, this, &MdiChild::enableRenderWindows);
	connectAlgorithmSignalsToChildSlots(thread);
}

void MdiChild::connectIOThreadSignals(iAIO* thread)
{
	connectAlgorithmSignalsToChildSlots(thread);
	connect(thread, &iAIO::finished, this, &MdiChild::ioFinished);
}

void MdiChild::connectAlgorithmSignalsToChildSlots(iAAlgorithm* thread)
{
	addAlgorithm(thread);
}

void MdiChild::addAlgorithm(iAAlgorithm* thread)
{
	m_workingAlgorithms.push_back(thread);
	connect(thread, &iAAlgorithm::finished, this, &MdiChild::removeFinishedAlgorithms);
}

void MdiChild::updateRenderWindows(int channels)
{
	if (channels > 1)
	{
		m_dwRenderer->spinBoxRC->setRange(0, channels - 1);
		m_dwRenderer->stackedWidgetRC->setCurrentIndex(1);
		m_dwRenderer->channelLabelRC->setEnabled(true);
	}
	else
	{
		m_dwRenderer->stackedWidgetRC->setCurrentIndex(0);
		m_dwRenderer->channelLabelRC->setEnabled(false);
	}
	disableRenderWindows(0);
}

void MdiChild::disableRenderWindows(int ch)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->disableInteractor();
	}
	m_renderer->disableInteractor();
	emit rendererDeactivated(ch);
}

void MdiChild::enableRenderWindows()	// = image data available
{
	if (isVolumeDataLoaded() && m_reInitializeRenderWindows)
	{
		m_renderer->enableInteractor();
		for (int s = 0; s < 3; ++s)
		{
			m_slicer[s]->enableInteractor();
			m_slicer[s]->triggerSliceRangeChange();
		}
		updateViews();
		updateImageProperties();
		if (modalities()->size() > 0 && modality(0)->image()->GetNumberOfScalarComponents() == 1)
		{
			setHistogramModality(0);
			updateProfile();
		}
		else  // No histogram/profile for rgb, rgba or vector pixel type images,
		{     // which means the present values are directly used for color anyway,
			initVolumeRenderers();    // so initialize volume rendering
		}     // (slicers already initialized before)
	}
	// set to true for next time, in case it is false now (i.e. default to always reinitialize,
	// unless explicitly set otherwise)
	m_reInitializeRenderWindows = true;

	m_renderer->reInitialize(modalities()->size() > 0 ? modality(0)->image() : nullptr, m_polyData);

	if (!isVolumeDataLoaded())
	{
		return;
	}
	setCamPosition(iACameraPosition::Iso);
	vtkCamera* cam = m_renderer->camera();
	modalities()->applyCameraSettings(cam);

	for (auto channelID : m_channels.keys())
	{
		iAChannelData* chData = m_channels.value(channelID).data();
		if (chData->isEnabled()
			|| (m_isMagicLensEnabled && (
				channelID == m_slicer[iASlicerMode::XY]->magicLensInput() ||
				channelID == m_slicer[iASlicerMode::XZ]->magicLensInput() ||
				channelID == m_slicer[iASlicerMode::YZ]->magicLensInput()
				))
			)
		{
			for (int s = 0; s < 3; ++s)
			{
				m_slicer[s]->updateChannel(channelID, *chData);
			}
		}
	}
	m_dwModalities->enableUI();
}

void MdiChild::modalityTFChanged()
{
	updateChannelMappers();
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateMagicLensColors();
	}
	emit transferFunctionChanged();
}

void MdiChild::updatePositionMarker(int x, int y, int z, int mode)
{
	m_position[0] = x; m_position[1] = y; m_position[2] = z;
	if (m_renderSettings.ShowRPosition)
	{
		m_renderer->setCubeCenter(x, y, z);
	}
	double spacing[3];
	// TODO: Use a separate "display" spacing here instead of the one from the first modality?
	modality(0)->image()->GetSpacing(spacing);
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (mode == i)  // only update other slicers
		{
			continue;
		}
		if (m_slicerSettings.LinkViews)
		{
			m_slicer[i]->setIndex(x, y, z);
			m_dwSlicer[i]->sbSlice->setValue(m_position[mapSliceToGlobalAxis(i, iAAxisIndex::Z)]);
		}
		if (m_slicerSettings.SingleSlicer.ShowPosition)
		{
			int slicerXAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::X);
			int slicerYAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::Y);
			int slicerZAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::Z);
			m_slicer[i]->setPositionMarkerCenter(
				m_position[slicerXAxisIdx] * spacing[slicerXAxisIdx],
				m_position[slicerYAxisIdx] * spacing[slicerYAxisIdx],
				m_position[slicerZAxisIdx] * spacing[slicerZAxisIdx]);
		}
	}
}

void MdiChild::showPoly()
{
	hideVolumeWidgets();
	setVisibility(QList<QWidget*>() << m_dwRenderer->stackedWidgetRC << m_dwRenderer->pushSaveRC << m_dwRenderer->pushMaxRC << m_dwRenderer->pushStopRC, true);

	m_dwRenderer->vtkWidgetRC->setGeometry(0, 0, 300, 200);
	m_dwRenderer->vtkWidgetRC->setMaximumSize(QSize(16777215, 16777215));
	m_dwRenderer->vtkWidgetRC->adjustSize();
	m_dwRenderer->show();
	m_visibility &= (RC | TAB);
	changeVisibility(m_visibility);
}

bool MdiChild::displayResult(QString const& title, vtkImageData* image, vtkPolyData* poly)	// = opening new window
{
	// TODO: image is actually not the final imagedata here (or at least not always)
	//    -> maybe skip all image-related initializations?
	addStatusMsg("Creating Result View");
	if (poly)
	{
		m_polyData->ReleaseData();
		m_polyData->DeepCopy(poly);
	}

	if (image)
	{
		m_imageData->ReleaseData();
		m_imageData->DeepCopy(image);
	}

	initView(title);
	setWindowTitle(title);
	m_renderer->applySettings(m_renderSettings, m_slicerVisibility);
	setupSlicers(m_slicerSettings, true);

	if (m_imageData->GetExtent()[1] <= 1)
	{
		m_visibility &= (YZ | TAB);
	}
	else if (m_imageData->GetExtent()[3] <= 1)
	{
		m_visibility &= (XZ | TAB);
	}
	else if (m_imageData->GetExtent()[5] <= 1)
	{
		m_visibility &= (XY | TAB);
	}
	changeVisibility(m_visibility);
	addStatusMsg("Ready");
	return true;
}

void MdiChild::prepareForResult()
{
	setWindowModified(true);
	modality(0)->transfer()->reset();
}

bool MdiChild::setupLoadIO(QString const& f, bool isStack)
{
	m_polyData->ReleaseData();
	// TODO: insert plugin mechanism.
	// - iterate over file plugins; if one returns a match, use it
	QString extension = m_fileInfo.suffix();
	extension = extension.toUpper();
	const mapQString2int* ext2id = &extensionToId;
	if (isStack)
	{
		ext2id = &extensionToIdStack;
	}
	if (ext2id->find(extension) == ext2id->end())
	{
		LOG(lvlError, QString("Could not find loader for extension '%1' of file '%2'!").arg(extension).arg(f));
		return false;
	}
	iAIOType id = ext2id->find(extension).value();
	return m_ioThread->setupIO(id, f);
}

bool MdiChild::loadRaw(const QString& f)
{
	if (!QFile::exists(f))
	{
		LOG(lvlWarn, QString("File '%1' does not exist!").arg(f));
		return false;
	}
	LOG(lvlInfo, tr("Loading file '%1'.").arg(f));
	setCurrentFile(f);
	waitForPreviousIO();
	m_ioThread = new iAIO(m_imageData, nullptr, iALog::get(), this);
	connect(m_ioThread, &iAIO::done, this, &MdiChild::setupView);
	connectIOThreadSignals(m_ioThread);
	connect(m_ioThread, &iAIO::done, this, &MdiChild::enableRenderWindows);
	m_polyData->ReleaseData();
	//m_imageData->ReleaseData();
	if (!m_ioThread->setupIO(RAW_READER, f))
	{
		ioFinished();
		return false;
	}
	m_ioThread->start();
	return true;
}

namespace
{
	bool Is2DImageFile(QString const& f)
	{
		return f.endsWith("bmp", Qt::CaseInsensitive) ||
			f.endsWith("jpg", Qt::CaseInsensitive) ||
			f.endsWith("jpeg", Qt::CaseInsensitive) ||
			f.endsWith("png", Qt::CaseInsensitive) ||
			f.endsWith("tif", Qt::CaseInsensitive) ||
			f.endsWith("tiff", Qt::CaseInsensitive);
	}
}

bool MdiChild::loadFile(const QString& f, bool isStack)
{
	if (!QFile::exists(f))
	{
		LOG(lvlError, QString("File '%1' does not exist!").arg(f));
		return false;
	}

	LOG(lvlInfo, tr("Loading file '%1', please wait...").arg(f));
	setCurrentFile(f);

	waitForPreviousIO();

	m_ioThread = new iAIO(m_imageData, m_polyData, iALog::get(), this, m_volumeStack->volumes(), m_volumeStack->fileNames());
	if (f.endsWith(iAIOProvider::ProjectFileExtension) ||
		f.endsWith(iAIOProvider::NewProjectFileExtension))
	{
		connect(m_ioThread, &iAIO::done, this, &MdiChild::setupProject);
	}
	else
	{
		if (!isStack || Is2DImageFile(f))
		{
			connect(m_ioThread, &iAIO::done, this, &MdiChild::setupView);
		}
		else
		{
			connect(m_ioThread, &iAIO::done, this, &MdiChild::setupStackView);
		}
		connect(m_ioThread, &iAIO::done, this, &MdiChild::enableRenderWindows);
	}
	connectIOThreadSignals(m_ioThread);
	connect(m_dwModalities, &dlg_modalities::modalityAvailable, this, &MdiChild::modalityAdded);
	connect(m_ioThread, &iAIO::done, this, &MdiChild::fileLoaded);

	m_polyData->ReleaseData();

	if (!setupLoadIO(f, isStack))
	{
		ioFinished();
		return false;
	}
	m_ioThread->start();
	return true;
}

void MdiChild::setImageData(QString const& /*filename*/, vtkSmartPointer<vtkImageData> imgData)
{
	m_imageData = imgData;
	modality(0)->setData(m_imageData);
	m_mainWnd->setCurrentFile(modalities()->fileName());
	setupView(false);
	enableRenderWindows();
}

vtkImageData* MdiChild::imageData()
{
	return m_imageData;
}

vtkSmartPointer<vtkImageData> MdiChild::imagePointer()
{
	return m_imageData;
}

void MdiChild::setImageData(vtkImageData* iData)
{
	m_imageData = iData;		// potential for double free!
}

vtkPolyData* MdiChild::polyData()
{
	return m_polyData;
}

iARenderer* MdiChild::renderer()
{
	return m_renderer;
}

bool MdiChild::updateVolumePlayerView(int updateIndex, bool isApplyForAll)
{
	// TODO: VOLUME: Test!!! copy from currently selected instead of fixed 0 index?
	// This function probbl never called, update(int, bool) signal doesn't seem to be emitted anywhere?
	vtkColorTransferFunction* colorTransferFunction = modality(0)->transfer()->colorTF();
	vtkPiecewiseFunction* piecewiseFunction = modality(0)->transfer()->opacityTF();
	m_volumeStack->colorTF(m_previousIndexOfVolume)->DeepCopy(colorTransferFunction);
	m_volumeStack->opacityTF(m_previousIndexOfVolume)->DeepCopy(piecewiseFunction);
	m_previousIndexOfVolume = updateIndex;

	m_imageData->DeepCopy(m_volumeStack->volume(updateIndex));
	assert(m_volumeStack->numberOfVolumes() < std::numeric_limits<int>::max());
	if (isApplyForAll)
	{
		for (size_t i = 0; i < m_volumeStack->numberOfVolumes(); ++i)
		{
			if (static_cast<int>(i) != updateIndex)
			{
				m_volumeStack->colorTF(i)->DeepCopy(colorTransferFunction);
				m_volumeStack->opacityTF(i)->DeepCopy(piecewiseFunction);
			}
		}
	}

	colorTransferFunction->DeepCopy(m_volumeStack->colorTF(updateIndex));
	piecewiseFunction->DeepCopy(m_volumeStack->opacityTF(updateIndex));

	setHistogramModality(0);

	m_renderer->reInitialize(m_imageData, m_polyData);
	for (int s = 0; s < 3; ++s)
	{
		// TODO: check how to update s:
		m_slicer[s]->updateChannel(0, iAChannelData(modality(0)->name(), m_imageData, colorTransferFunction));
	}
	updateViews();

	if (m_checkedList.at(updateIndex) != 0)
	{
		enableRenderWindows();
	}

	return true;
}

void MdiChild::setupStackView(bool active)
{
	// TODO: check!
	m_previousIndexOfVolume = 0;

	if (m_volumeStack->numberOfVolumes() == 0)
	{
		LOG(lvlError, "Invalid call to setupStackView: No Volumes loaded!");
		return;
	}

	int currentIndexOfVolume = 0;

	m_imageData->DeepCopy(m_volumeStack->volume(currentIndexOfVolume));
	setupViewInternal(active);
	for (size_t i = 0; i < m_volumeStack->numberOfVolumes(); ++i)
	{
		vtkSmartPointer<vtkColorTransferFunction> cTF = defaultColorTF(m_imageData->GetScalarRange());
		vtkSmartPointer<vtkPiecewiseFunction> pWF = defaultOpacityTF(m_imageData->GetScalarRange(), m_imageData->GetNumberOfScalarComponents() == 1);
		m_volumeStack->addColorTransferFunction(cTF);
		m_volumeStack->addPiecewiseFunction(pWF);
	}

	QSharedPointer<iAModalityTransfer> modTrans = modality(0)->transfer();
	modTrans->colorTF()->DeepCopy(m_volumeStack->colorTF(0));
	modTrans->opacityTF()->DeepCopy(m_volumeStack->opacityTF(0));
	addVolumePlayer();

	m_renderer->reInitialize(m_imageData, m_polyData);
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateChannel(0, iAChannelData(modality(0)->name(), m_imageData, modTrans->colorTF()));
	}
	updateViews();
}

void MdiChild::setupViewInternal(bool active)
{
	if (!m_imageData)
	{
		LOG(lvlError, "Image Data is not set!");
		return;
	}
	if (!active)
	{
		initView(m_curFile.isEmpty() ? "Untitled" : "");
	}

	m_mainWnd->setCurrentFile(currentFile());	// TODO: VOLUME: should be done on the outside? or where setCurrentFile is done?

	if ((m_imageData->GetExtent()[1] < 3) || (m_imageData->GetExtent()[3]) < 3 || (m_imageData->GetExtent()[5] < 3))
	{
		m_volumeSettings.Shading = false;
	}

	m_volumeSettings.SampleDistance = m_imageData->GetSpacing()[0];
	m_renderer->applySettings(m_renderSettings, m_slicerVisibility);
	setupSlicers(m_slicerSettings, true);

	if (m_imageData->GetExtent()[1] <= 1)
	{
		m_visibility &= (YZ | TAB);
	}
	else if (m_imageData->GetExtent()[3] <= 1)
	{
		m_visibility &= (XZ | TAB);
	}
	else if (m_imageData->GetExtent()[5] <= 1)
	{
		m_visibility &= (XY | TAB);
	}

	if (active)
	{
		changeVisibility(m_visibility);
	}

	if (m_imageData->GetNumberOfScalarComponents() > 1 &&
		m_imageData->GetNumberOfScalarComponents() < 4)
	{
		m_dwRenderer->spinBoxRC->setRange(0, m_imageData->GetNumberOfScalarComponents() - 1);
		m_dwRenderer->stackedWidgetRC->setCurrentIndex(1);
		m_dwRenderer->channelLabelRC->setEnabled(true);
	}
	else
	{
		m_dwRenderer->stackedWidgetRC->setCurrentIndex(0);
		m_dwRenderer->channelLabelRC->setEnabled(false);
	}
}

void MdiChild::setupView(bool active)
{
	setupViewInternal(active);
	m_renderer->update();
	check2DMode();
}

void MdiChild::setupProject(bool /*active*/)
{
	setModalities(m_ioThread->modalities());
	QString fileName = m_ioThread->fileName();
	setCurrentFile(fileName);
	m_mainWnd->setCurrentFile(fileName);
	if (fileName.toLower().endsWith(iAIOProvider::NewProjectFileExtension))
	{
		// TODO: make asynchronous, put into iASavableProject?
		QSettings projectFile(fileName, QSettings::IniFormat);
		projectFile.setIniCodec("UTF-8");
		auto registeredProjects = iAProjectRegistry::projectKeys();
		auto projectFileGroups = projectFile.childGroups();
		for (auto projectKey : registeredProjects)
		{
			if (projectFileGroups.contains(projectKey))
			{
				auto project = iAProjectRegistry::createProject(projectKey);
				project->setMainWindow(m_mainWnd);
				project->setChild(this);
				projectFile.beginGroup(projectKey);
				project->loadProject(projectFile, fileName);
				projectFile.endGroup();
				addProject(projectKey, project);
			}
		}
	}
}

int MdiChild::chooseModalityNr(QString const& caption)
{
	if (!isVolumeDataLoaded())
	{
		return -1;
	}
	if (modalities()->size() == 1)
	{
		return 0;
	}
	QStringList parameters = (QStringList() << tr("+Channel"));
	QStringList modalityNames;
	for (int i = 0; i < modalities()->size(); ++i)
	{
		modalityNames << modality(i)->name();
	}
	QList<QVariant> values = (QList<QVariant>() << modalityNames);
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
	{
		return -1;
	}
	int nrOfComponents = modality(modalityNr)->image()->GetNumberOfScalarComponents();
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
	if (m_isUntitled)
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
		if (modality(modalityNr)->ComponentCount() > 1)
		{                         // should be ChannelCount()
		}
		*/
		int componentNr = chooseComponentNr(modalityNr);
		if (componentNr == -1)
		{
			return false;
		}
		return saveFile(modality(modalityNr)->fileName(), modalityNr, componentNr);
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
	// TODO: ask for filename first, then for modality (if only one modality can be saved in chosen format)
	QString filePath = (modalities()->size() > 0) ? QFileInfo(modality(modalityNr)->fileName()).absolutePath() : m_path;
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
	if (m_ioThread)
	{
		LOG(lvlInfo, tr("Waiting for I/O operation to complete..."));
		m_ioThread->wait();
		m_ioThread = nullptr;
	}
}

QString GetSupportedPixelTypeString(QVector<int> const& types)
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

bool MdiChild::setupSaveIO(QString const& f)
{
	QFileInfo fileInfo(f);
	if (QString::compare(fileInfo.suffix(), "STL", Qt::CaseInsensitive) == 0)
	{
		if (m_polyData->GetNumberOfPoints() <= 1)
		{
			QMessageBox::warning(this, tr("Save File"), tr("Model contains no data. Saving aborted."));
			return false;
		}
		else
		{
			if (!m_ioThread->setupIO(STL_WRITER, fileInfo.absoluteFilePath()))
			{
				return false;
			}
		}
	}
	else
	{
		if (!isVolumeDataLoaded())
		{
			QMessageBox::warning(this, tr("Save File"), tr("Image contains no data. Saving aborted.")); return false;
		}
		else
		{
			if ((QString::compare(fileInfo.suffix(), "MHD", Qt::CaseInsensitive) == 0) ||
				(QString::compare(fileInfo.suffix(), "MHA", Qt::CaseInsensitive) == 0))
			{
				if (!m_ioThread->setupIO(MHD_WRITER, fileInfo.absoluteFilePath(), m_preferences.Compression))
				{
					return false;
				}
				setCurrentFile(f);
				m_mainWnd->setCurrentFile(f);	// TODO: VOLUME: do in setCurrentFile member method?
				QString t; t = f;
				t.truncate(t.lastIndexOf('/'));
				m_mainWnd->setPath(t);
			}
			else
			{
				QMap<iAIOType, QVector<int> > supportedPixelTypes;
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
				iAIOType ioID = extensionToSaveId[suffix];
				if (supportedPixelTypes.contains(ioID) &&
					!supportedPixelTypes[ioID].contains(m_imageData->GetScalarType()))
				{
					LOG(lvlWarn, QString("Writer for %1 only supports %2 input!")
						.arg(suffix)
						.arg(GetSupportedPixelTypeString(supportedPixelTypes[ioID])));
					return false;
				}
				if (!m_ioThread->setupIO(ioID, fileInfo.absoluteFilePath()))
				{
					return false;
				}

			}
		}
	}
	return true;
}

bool MdiChild::saveFile(const QString& f, int modalityNr, int componentNr)
{
	waitForPreviousIO();

	if (isVolumeDataLoaded())
	{
		m_tmpSaveImg = modality(modalityNr)->image();
		if (m_tmpSaveImg->GetNumberOfScalarComponents() > 1 &&
			componentNr != m_tmpSaveImg->GetNumberOfScalarComponents())
		{
			auto imgExtract = vtkSmartPointer<vtkImageExtractComponents>::New();
			imgExtract->SetInputData(m_tmpSaveImg);
			imgExtract->SetComponents(componentNr);
			imgExtract->Update();
			m_tmpSaveImg = imgExtract->GetOutput();
		}
	}

	m_ioThread = new iAIO(m_tmpSaveImg, m_polyData, iALog::get(), this);
	connectIOThreadSignals(m_ioThread);
	connect(m_ioThread, &iAIO::done, this, &MdiChild::saveFinished);
	m_storedModalityNr = modalityNr;
	if (!setupSaveIO(f))
	{
		ioFinished();
		return false;
	}

	LOG(lvlInfo, tr("Saving file '%1'.").arg(f));
	m_ioThread->start();

	return true;
}

void MdiChild::updateViews()
{
	updateSlicers();

	m_renderer->update();
	m_renderer->renderWindow()->GetInteractor()->Modified();
	m_renderer->renderWindow()->GetInteractor()->Render();
	m_dwRenderer->vtkWidgetRC->update();
	emit viewsUpdated();
}

int MdiChild::visibility() const
{
	int vis = RC | YZ | XZ | XY;
	return vis;
}

void MdiChild::maximizeSlicer(int mode)
{
	resizeDockWidget(m_dwSlicer[mode]);
}

void MdiChild::maximizeRC()
{
	resizeDockWidget(m_dwRenderer);
}

void MdiChild::saveRC()
{
	QString file = QFileDialog::getSaveFileName(this, tr("Save Image"),
		"",
		iAIOProvider::GetSupportedImageFormats());
	if (file.isEmpty())
	{
		return;
	}
	vtkSmartPointer<vtkWindowToImageFilter> filter = vtkSmartPointer<vtkWindowToImageFilter>::New();
	filter->SetInput(m_renderer->renderWindow());
	filter->Update();
	writeSingleSliceImage(file, filter->GetOutput());
}

void MdiChild::saveMovRC()
{
	saveMovie(*m_renderer);
}

void MdiChild::camPosition(double* camOptions)
{
	m_renderer->camPosition(camOptions);
}

void MdiChild::setCamPosition(int pos)
{
	m_renderer->setCamPosition(pos);
}

void MdiChild::setCamPosition(double* camOptions, bool rsParallelProjection)
{
	m_renderer->setCamPosition(camOptions, rsParallelProjection);
}

void MdiChild::triggerInteractionRaycaster()
{
	if (m_renderer->interactor()->GetEnabled())
	{
		m_renderer->disableInteractor();
		LOG(lvlInfo, tr("Renderer disabled."));
	}
	else
	{
		m_renderer->enableInteractor();
		LOG(lvlInfo, tr("Renderer enabled."));
	}
}

void MdiChild::setSlice(int mode, int s)
{
	if (m_snakeSlicer)
	{
		int sliceAxis = mapSliceToGlobalAxis(mode, iAAxisIndex::Z);
		updateSnakeSlicer(m_dwSlicer[mode]->sbSlice, m_slicer[mode], sliceAxis, s);
	}
	else
	{
		//Update Slicer if changed
		if (m_dwSlicer[mode]->sbSlice->value() != s) {
			QSignalBlocker block(m_dwSlicer[mode]->sbSlice);
			m_dwSlicer[mode]->sbSlice->setValue(s);
		}
		if (m_dwSlicer[mode]->verticalScrollBar->value() != s) {
			QSignalBlocker block(m_dwSlicer[mode]->verticalScrollBar);
			m_dwSlicer[mode]->verticalScrollBar->setValue(s);
		}

		m_position[mode] = s;
		if (m_renderSettings.ShowSlicers || m_renderSettings.ShowSlicePlanes)
		{
			set3DSlicePlanePos(mode, s);
		}
	}
}

void MdiChild::set3DSlicePlanePos(int mode, int slice)
{
	int sliceAxis = mapSliceToGlobalAxis(mode, iAAxisIndex::Z);
	double plane[3];
	std::fill(plane, plane + 3, 0);
	// + 0.5 to place slice plane in the middle of the sliced voxel:
	plane[sliceAxis] = (slice + 0.5) * m_imageData->GetSpacing()[sliceAxis];
	m_renderer->setSlicePlanePos(sliceAxis, plane[0], plane[1], plane[2]);
}

void MdiChild::updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s)
{
	double spacing[3];
	m_imageData->GetSpacing(spacing);

	double splinelength = (int)m_parametricSpline->GetLength();
	double length_percent = 100 / splinelength;
	double mf1 = s + 1; //multiplication factor for first point
	double mf2 = s + 2; //multiplication factor for second point
	spinBox->setRange(0, (splinelength - 1));//set the number of slices to scroll through

													//calculate the percentage for 2 points
	double t1[3] = { length_percent * mf1 / 100, length_percent * mf1 / 100, length_percent * mf1 / 100 };
	double t2[3] = { length_percent * mf2 / 100, length_percent * mf2 / 100, length_percent * mf2 / 100 };
	double point1[3], point2[3];
	//calculate the points
	m_parametricSpline->Evaluate(t1, point1, nullptr);
	m_parametricSpline->Evaluate(t2, point2, nullptr);

	//calculate normal
	double normal[3];
	normal[0] = point2[0] - point1[0];
	normal[1] = point2[1] - point1[1];
	normal[2] = point2[2] - point1[2];

	vtkMatrixToLinearTransform* final_transform = vtkMatrixToLinearTransform::New();

	if (normal[0] == 0 && normal[1] == 0)
	{
		//Move the point to origin Translation
		double PointToOrigin_matrix[16] = { 1, 0, 0, point1[0],
			0, 1, 0, point1[1],
			0, 0, 1, point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4* PointToOrigin_Translation = vtkMatrix4x4::New();
		PointToOrigin_Translation->DeepCopy(PointToOrigin_matrix);

		//Move the origin to point Translation
		double OriginToPoint_matrix[16] = { 1, 0, 0, -point1[0],
			0, 1, 0, -point1[1],
			0, 0, 1, -point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4* OriginToPoint_Translation = vtkMatrix4x4::New();
		OriginToPoint_Translation->DeepCopy(OriginToPoint_matrix);

		///multiplication of transformation matics
		vtkMatrix4x4* Transformation_4 = vtkMatrix4x4::New();
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
		vtkMatrix4x4* PointToOrigin_Translation = vtkMatrix4x4::New();
		PointToOrigin_Translation->DeepCopy(PointToOrigin_matrix);

		//rotate around Z to bring the vector to XZ plane
		double alpha = acos(pow(normal[0], 2) / (sqrt(pow(normal[0], 2)) * (sqrt(pow(normal[0], 2) + pow(normal[1], 2)))));
		double cos_theta_xz = cos(alpha);
		double sin_theta_xz = sin(alpha);

		double rxz_matrix[16] = { cos_theta_xz,	-sin_theta_xz,	0,	 0,
			sin_theta_xz,	cos_theta_xz,	0,	 0,
			0,			0,		1,	 0,
			0,			0,		0,	 1 };

		vtkMatrix4x4* rotate_around_xz = vtkMatrix4x4::New();
		rotate_around_xz->DeepCopy(rxz_matrix);

		//rotate around Y to bring vector parallel to Z axis
		double beta = acos(pow(normal[2], 2) / sqrt(pow(normal[2], 2)) + sqrt(pow(cos_theta_xz, 2) + pow(normal[2], 2)));
		double cos_theta_y = cos(beta);
		double sin_theta_y = sin(beta);

		double ry_matrix[16] = { cos_theta_y,	0,	sin_theta_y,	0,
			0,			1,		0,			0,
			-sin_theta_y,	0,	cos_theta_y,	0,
			0,			0,		0,			1 };

		vtkMatrix4x4* rotate_around_y = vtkMatrix4x4::New();
		rotate_around_y->DeepCopy(ry_matrix);

		//rotate around Z by 180 degree - to bring object correct view
		double cos_theta_z = cos(vtkMath::Pi());
		double sin_theta_z = sin(vtkMath::Pi());

		double rz_matrix[16] = { cos_theta_z,	-sin_theta_z,	0,	0,
			sin_theta_z,	cos_theta_z,	0,	0,
			0,				0,			1,	0,
			0,				0,			0,	1 };

		vtkMatrix4x4* rotate_around_z = vtkMatrix4x4::New();
		rotate_around_z->DeepCopy(rz_matrix);

		//Move the origin to point Translation
		double OriginToPoint_matrix[16] = { 1, 0, 0, -point1[0],
			0, 1, 0, -point1[1],
			0, 0, 1, -point1[2],
			0, 0, 0, 1 };
		vtkMatrix4x4* OriginToPoint_Translation = vtkMatrix4x4::New();
		OriginToPoint_Translation->DeepCopy(OriginToPoint_matrix);

		///multiplication of transformation matics
		vtkMatrix4x4* Transformation_1 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(PointToOrigin_Translation, rotate_around_xz, Transformation_1);

		vtkMatrix4x4* Transformation_2 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_1, rotate_around_y, Transformation_2);

		vtkMatrix4x4* Transformation_3 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_2, rotate_around_z, Transformation_3);

		vtkMatrix4x4* Transformation_4 = vtkMatrix4x4::New();
		vtkMatrix4x4::Multiply4x4(Transformation_3, OriginToPoint_Translation, Transformation_4);

		final_transform->SetInput(Transformation_4);
		final_transform->Update();
	}

	slicer->channel(0)->setTransform(final_transform);
	QSignalBlocker block(slicer);
	slicer->setSliceNumber(point1[ptIndex]);
}

void MdiChild::setChannel(int c)
{
	disableRenderWindows(c);
	enableRenderWindows();
}

void MdiChild::slicerRotationChanged()
{
	m_renderer->setPlaneNormals(m_slicerTransform);
}

void MdiChild::linkViews(bool l)
{
	m_slicerSettings.LinkViews = l;
}

void MdiChild::linkMDIs(bool lm)
{
	m_slicerSettings.LinkMDIs = lm;
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		m_slicer[s]->setLinkedMdiChild(lm ? this : nullptr);
	}
}

void MdiChild::enableInteraction(bool b)
{
	for (int s = 0; s < 3; ++s)
	{
		if (b)
		{
			m_slicer[s]->enableInteractor();
		}
		else
		{
			m_slicer[s]->disableInteractor();
		}
	}
}

bool MdiChild::editPrefs(iAPreferences const& prefs)
{
	m_preferences = prefs;
	if (m_ioThread)	// don't do any updates if image still loading
	{
		return true;
	}
	setHistogramModality(m_currentModality);	// to update Histogram bin count
	m_histogram->setYMappingMode(prefs.HistogramLogarithmicYAxis ? iAChartWidget::Logarithmic : iAChartWidget::Linear);
	applyViewerPreferences();
	if (isMagicLens2DEnabled())
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
		m_slicer[s]->setMagicLensFrameWidth(m_preferences.MagicLensFrameWidth);
		m_slicer[s]->setMagicLensSize(m_preferences.MagicLensSize);
		m_slicer[s]->setStatisticalExtent(m_preferences.StatisticalExtent);
	}
	m_dwRenderer->vtkWidgetRC->setLensSize(m_preferences.MagicLensSize, m_preferences.MagicLensSize);
	m_renderer->setStatExt(m_preferences.StatisticalExtent);
}

void MdiChild::setRenderSettings(iARenderSettings const& rs, iAVolumeSettings const& vs)
{
	m_renderSettings = rs;
	m_volumeSettings = vs;
}

void MdiChild::applyVolumeSettings(const bool loadSavedVolumeSettings)
{
	for (int i = 0; i < 3; ++i)
	{
		m_dwSlicer[i]->showBorder(m_renderSettings.ShowSlicePlanes);
	}
	m_dwModalities->showSlicers(m_renderSettings.ShowSlicers && !m_snakeSlicer, m_renderer->plane1(), m_renderer->plane2(), m_renderer->plane3());
	m_dwModalities->changeRenderSettings(m_volumeSettings, loadSavedVolumeSettings);
}

QString MdiChild::layoutName() const
{
	return m_layout;
}

void MdiChild::updateLayout()
{
	m_mainWnd->loadLayout();
}

void MdiChild::loadLayout(QString const& layout)
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

int const* MdiChild::position() const
{
	return m_position;
}

void MdiChild::setupSlicers(iASlicerSettings const& ss, bool init)
{
	m_slicerSettings = ss;

	if (m_snakeSlicer)
	{
		// TODO: check why only XY slice here?
		int prevMax = m_dwSlicer[iASlicerMode::XY]->sbSlice->maximum();
		int prevValue = m_dwSlicer[iASlicerMode::XY]->sbSlice->value();
		m_dwSlicer[iASlicerMode::XY]->sbSlice->setRange(0, ss.SnakeSlices - 1);
		m_dwSlicer[iASlicerMode::XY]->sbSlice->setValue((double)prevValue / prevMax * (ss.SnakeSlices - 1));
	}

	linkViews(ss.LinkViews);
	linkMDIs(ss.LinkMDIs);

	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setup(ss.SingleSlicer);
	}

	if (init)
	{
		// connect signals for making slicers update other views in snake slicers mode:
		for (int i = 0; i < 3; ++i)
		{
			connect(m_slicer[i], &iASlicer::profilePointChanged, this, &MdiChild::updateProbe);
			connect(m_slicer[i], &iASlicer::profilePointChanged, m_renderer, &iARenderer::setProfilePoint);
			connect(m_slicer[i], &iASlicer::magicLensToggled, this, &MdiChild::toggleMagicLens2D);
			for (int j = 0; j < 3; ++j)
			{
				if (i != j)	// connect each slicer's signals to the other slicer's slots, except for its own:
				{
					connect(m_slicer[i], &iASlicer::addedPoint, m_slicer[j], &iASlicer::addPoint);
					connect(m_slicer[i], &iASlicer::movedPoint, m_slicer[j], &iASlicer::movePoint);
					connect(m_slicer[i], &iASlicer::profilePointChanged, m_slicer[j], &iASlicer::setProfilePoint);
					connect(m_slicer[i], &iASlicer::switchedMode, m_slicer[j], &iASlicer::switchInteractionMode);
					connect(m_slicer[i], &iASlicer::deletedSnakeLine, m_slicer[j], &iASlicer::deleteSnakeLine);
					connect(m_slicer[i], &iASlicer::deselectedPoint, m_slicer[j], &iASlicer::deselectPoint);
				}
			}
		}
	}
}

bool MdiChild::editRendererSettings(iARenderSettings const& rs, iAVolumeSettings const& vs)
{
	setRenderSettings(rs, vs);
	applyVolumeSettings(false);
	m_renderer->applySettings(renderSettings(), m_slicerVisibility);
	m_dwRenderer->vtkWidgetRC->show();
#if VTK_VERSION_NUMBER < VTK_VERSION_CHECK(9, 0, 0)
	m_dwRenderer->vtkWidgetRC->GetRenderWindow()->Render();
#else
	m_dwRenderer->vtkWidgetRC->renderWindow()->Render();
#endif
	emit renderSettingsChanged();
	return true;
}

iARenderSettings const& MdiChild::renderSettings() const
{
	return m_renderSettings;
}

iAVolumeSettings const& MdiChild::volumeSettings() const
{
	return m_volumeSettings;
}

iASlicerSettings const& MdiChild::slicerSettings() const
{
	return m_slicerSettings;
}

iAPreferences const& MdiChild::preferences() const
{
	return m_preferences;
}

bool MdiChild::editSlicerSettings(iASlicerSettings const& slicerSettings)
{
	setupSlicers(slicerSettings, false);
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->show();
	}
	emit slicerSettingsChanged();
	return true;
}

// Just proxies for histogram functions:
// {

bool MdiChild::loadTransferFunction()
{
	if (!m_histogram)
	{
		return false;
	}
	m_histogram->loadTransferFunction();
	return true;
}

bool MdiChild::saveTransferFunction()
{
	if (!m_histogram)
	{
		return false;
	}
	m_histogram->saveTransferFunction();
	return true;
}

int MdiChild::deletePoint()
{
	if (!m_histogram)
	{
		return -1;
	}
	return m_histogram->deletePoint();
}

void MdiChild::resetView()
{
	if (!m_histogram)
	{
		return;
	}
	m_histogram->resetView();
}

void MdiChild::changeColor()
{
	if (!m_histogram)
	{
		return;
	}
	m_histogram->changeColor();
}

int MdiChild::selectedFuncPoint()
{
	if (!m_histogram)
	{
		return -1;
	}
	return m_histogram->selectedFuncPoint();
}

int MdiChild::isFuncEndPoint(int index)
{
	if (!m_histogram)
	{
		return -1;
	}
	return m_histogram->isFuncEndPoint(index);
}

void MdiChild::setHistogramFocus()
{
	if (!m_histogram)
	{
		return;
	}
	m_histogram->setFocus(Qt::OtherFocusReason);
}

void MdiChild::redrawHistogram()
{
	if (m_histogram)
	{
		m_histogram->update();
	}
}

void MdiChild::resetTrf()
{
	if (!m_histogram)
	{
		return;
	}
	m_histogram->resetTrf();
	LOG(lvlInfo, tr("Resetting Transfer Functions."));
	LOG(lvlInfo, tr("  Adding transfer function point: %1.   Opacity: 0.0,   Color: 0, 0, 0")
		.arg(m_histogram->xBounds()[0]));
	LOG(lvlInfo, tr("  Adding transfer function point: %1.   Opacity: 1.0,   Color: 255, 255, 255")
		.arg(m_histogram->xBounds()[1]));
}

std::vector<iAChartFunction*>& MdiChild::functions()
{
	if (!m_histogram)
	{
		static std::vector<iAChartFunction*> nullVec;
		return nullVec;
	}
	return m_histogram->functions();
}

iAChartWithFunctionsWidget* MdiChild::histogram()
{
	return m_histogram;
}

// }

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

	QStringList modes = (QStringList() << tr("Rotate Z") << tr("Rotate X") << tr("Rotate Y"));
	QStringList inList = (QStringList() << tr("+Rotation mode"));
	QList<QVariant> inPara = (QList<QVariant>() << modes);
	dlg_commoninput dlg(this, "Save movie options", inList, inPara,
		"Creates a movie by rotating the object around a user-defined axis in the 3D renderer.");
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}

	mode = dlg.getComboBoxValue(0);
	imode = dlg.getComboBoxIndex(0);

	// Show standard save file dialog using available movie file types.
	raycaster.saveMovie(
		QFileDialog::getSaveFileName(
			this,
			tr("Export movie %1").arg(mode),
			m_fileInfo.absolutePath() + "/" + ((mode.isEmpty()) ? m_fileInfo.baseName() : m_fileInfo.baseName() + "_" + mode),
			movie_file_types),
		imode);
}

void MdiChild::toggleSnakeSlicer(bool isChecked)
{
	m_snakeSlicer = isChecked;

	if (m_snakeSlicer)
	{
		if (m_renderSettings.ShowSlicers)
		{
			m_dwModalities->showSlicers(false, nullptr, nullptr, nullptr);
		}

		// save the slicer transforms
		for (int s = 0; s < 3; ++s)
		{
			m_savedSlicerTransform[s] = m_slicer[s]->channel(0)->reslicer()->GetResliceTransform();
		}

		m_parametricSpline->Modified();
		double emptyper[3]; emptyper[0] = 0; emptyper[1] = 0; emptyper[2] = 0;
		double emptyp[3]; emptyp[0] = 0; emptyp[1] = 0; emptyp[2] = 0;
		m_parametricSpline->Evaluate(emptyper, emptyp, nullptr);

		// save the slicer transforms
		for (int s = 0; s < 3; ++s)
		{
			m_savedSlicerTransform[s] = m_slicer[s]->channel(0)->reslicer()->GetResliceTransform();
			m_slicer[s]->switchInteractionMode(iASlicer::SnakeShow);
			m_dwSlicer[s]->sbSlice->setValue(0);
		}
	}
	else
	{	// restore the slicer transforms
		m_slicer[iASlicerMode::YZ]->channel(0)->reslicer()->SetResliceAxesDirectionCosines(0, 1, 0, 0, 0, 1, 1, 0, 0);
		m_slicer[iASlicerMode::XZ]->channel(0)->reslicer()->SetResliceAxesDirectionCosines(1, 0, 0, 0, 0, 1, 0, -1, 0);
		m_slicer[iASlicerMode::XY]->channel(0)->reslicer()->SetResliceAxesDirectionCosines(1, 0, 0, 0, 1, 0, 0, 0, 1);

		for (int s = 0; s < 3; ++s)
		{
			m_dwSlicer[s]->sbSlice->setValue(m_imageData->GetDimensions()[mapSliceToGlobalAxis(s, iAAxisIndex::Z)] >> 1);
			m_slicer[s]->channel(0)->reslicer()->SetResliceTransform(m_savedSlicerTransform[s]);
			m_slicer[s]->channel(0)->reslicer()->SetOutputExtentToDefault();
			m_slicer[s]->resetCamera();
			m_slicer[s]->renderer()->Render();
			m_slicer[s]->switchInteractionMode(iASlicer::Normal);
		}
		if (m_renderSettings.ShowSlicers)
		{
			m_dwModalities->showSlicers(true, m_renderer->plane1(), m_renderer->plane2(), m_renderer->plane3());
		}
	}
}

void MdiChild::snakeNormal(int index, double point[3], double normal[3])
{
	int i1 = index;
	int i2 = index + 1;

	double spacing[3];
	m_imageData->GetSpacing(spacing);

	int snakeSlices = m_slicerSettings.SnakeSlices;
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
		m_parametricSpline->Evaluate(t1, p1, nullptr);
		m_parametricSpline->Evaluate(t2, p2, nullptr);

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
	return m_snakeSlicer;
}

void MdiChild::toggleSliceProfile(bool isChecked)
{
	m_isSliceProfileEnabled = isChecked;
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setSliceProfileOn(m_isSliceProfileEnabled);
	}
}

bool MdiChild::isSliceProfileToggled(void) const
{
	return m_isSliceProfileEnabled;
}

void MdiChild::toggleMagicLens2D(bool isEnabled)
{
	m_isMagicLensEnabled = isEnabled;

	if (isEnabled)
	{
		changeMagicLensModality(0);
	}
	setMagicLensEnabled(isEnabled);
	updateSlicers();
	m_mainWnd->updateMagicLens2DCheckState(isEnabled);
	emit magicLensToggled(m_isMagicLensEnabled);
}

void MdiChild::toggleMagicLens3D(bool isEnabled)
{
	if (isEnabled)
	{
		m_dwRenderer->vtkWidgetRC->magicLensOn();
	}
	else
	{
		m_dwRenderer->vtkWidgetRC->magicLensOff();
	}
}

bool MdiChild::isMagicLens2DEnabled() const
{
	return m_isMagicLensEnabled;
}

bool MdiChild::isMagicLens3DEnabled() const
{
	return m_dwRenderer->vtkWidgetRC->isMagicLensEnabled();
}

bool MdiChild::initView(QString const& title)
{
	if (!m_raycasterInitialized)
	{
		m_renderer->initialize(m_imageData, m_polyData);
		connect(m_renderer->getRenderObserver(), &iARenderObserver::keyPressed, this, &MdiChild::rendererKeyPressed);
		m_raycasterInitialized = true;
	}
	if (modalities()->size() == 0 && isVolumeDataLoaded())
	{
		// TODO: VOLUME: resolve duplication between here (called on loadFile) and adding modalities
		QString name;
		if (!m_curFile.isEmpty())
		{
			QFileInfo i(m_curFile);
			name = i.completeBaseName();
		}
		else
		{
			name = title;
		}
		// TODO: VOLUME: resolve indirect dependence of this call on the m_renderer->initialize method
		// before, which adds the renderers which this call will use
		QSharedPointer<iAModality> mod(new iAModality(name,
			currentFile(), -1, m_imageData, iAModality::MainRenderer + iAModality::Slicer));
		modalities()->add(mod);
		m_dwModalities->addListItem(mod);
	}
	if (m_channels.empty() && modalities()->size() > 0)
	{
		QSharedPointer<iAModalityTransfer> modTrans = modality(0)->transfer();
		uint channelID = createChannel();
		assert(channelID == 0); // first modality we create, there shouldn't be another channel yet!
		modality(0)->setChannelID(channelID);
		modality(0)->setRenderFlag(modality(0)->renderFlags() | iAModality::RenderFlag::Slicer);
		for (int s = 0; s < 3; ++s)
		{
			m_slicer[s]->addChannel(channelID, iAChannelData(modality(0)->name(), modality(0)->image(), modTrans->colorTF()), true);
			m_slicer[s]->resetCamera();
		}
		m_initVolumeRenderers = true;
	}
	m_dwRenderer->stackedWidgetRC->setCurrentIndex(0);

	if (isVolumeDataLoaded())
	{
		addImageProperty();
		if (m_imageData->GetNumberOfScalarComponents() == 1)
		{   // No histogram/profile for rgb, rgba or vector pixel type images
			tabifyDockWidget(m_dwModalities, m_dwHistogram);
			addProfile();
		}
	}
	else
	{	//Polygonal mesh is loaded
		showPoly();
		hideHistogram();
	}
	updateLayout();

	return true;
}

void MdiChild::hideHistogram()
{
	m_dwHistogram->hide();
}

void MdiChild::addImageProperty()
{
	if (m_dwImgProperty)
	{
		return;
	}
	m_dwImgProperty = new dlg_imageproperty(this);
	tabifyDockWidget(m_dwModalities, m_dwImgProperty);
}

void MdiChild::updateImageProperties()
{
	if (!m_dwImgProperty)
	{
		return;
	}
	m_dwImgProperty->Clear();
	for (int i = 0; i < modalities()->size(); ++i)
	{
		m_dwImgProperty->AddInfo(modality(i)->image(), modality(i)->info(), modality(i)->name(),
			(i == 0 &&
				modality(i)->componentCount() == 1 &&
				m_volumeStack->numberOfVolumes() > 1) ?
			m_volumeStack->numberOfVolumes() :
			modality(i)->componentCount()
		);
	}
}

bool MdiChild::addVolumePlayer()
{
	m_dwVolumePlayer = new dlg_volumePlayer(this, m_volumeStack.data());
	tabifyDockWidget(m_dwModalities, m_dwVolumePlayer);
	for (size_t id = 0; id < m_volumeStack->numberOfVolumes(); ++id)
	{
		m_checkedList.append(0);
	}
	connect(m_histogram, &iAChartWithFunctionsWidget::applyTFForAll, m_dwVolumePlayer, &dlg_volumePlayer::applyForAll);

	return true;
}

int MdiChild::evaluatePosition(int pos, int i, bool invert)
{
	if (pos < 0)
	{
		invert ? (pos = m_imageData->GetExtent()[i]) : (pos = 0);
	}
	if (pos > m_imageData->GetExtent()[i]) invert ? (pos = 0) : (pos = m_imageData->GetExtent()[i]);
	return pos;
}

void MdiChild::addStatusMsg(QString const & txt)
{
	m_mainWnd->statusBar()->showMessage(txt, 10000);
}

bool MdiChild::isMaximized()
{
	return m_visibility != MULTI;
}

void MdiChild::updateROI(int const roi[6])
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateROI(roi);
	}

	const double* spacing = modality(0)->spacing();
	m_renderer->setSlicingBounds(roi, spacing);
}

void MdiChild::setROIVisible(bool visible)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setROIVisible(visible);
	}
	m_renderer->setCubeVisible(visible);
}

QString MdiChild::userFriendlyCurrentFile() const
{
	return fileNameOnly(m_curFile);
}

QString MdiChild::currentFile() const
{
	return m_curFile;
}

QFileInfo MdiChild::fileInfo() const
{
	return m_fileInfo;
}

void MdiChild::closeEvent(QCloseEvent* event)
{
	if (m_ioThread)
	{
		LOG(lvlWarn, "Cannot close window while I/O operation is in progress!");
		addStatusMsg("Cannot close window while I/O operation is in progress!");
		event->ignore();
	}
	else
	{
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

void MdiChild::setCurrentFile(const QString& f)
{
	m_fileInfo.setFile(f);
	m_curFile = f;
	m_path = m_fileInfo.canonicalPath();
	if (isActiveWindow())
	{
		QDir::setCurrent(m_path);  // set current application working directory to the one where the file is in (as default directory, e.g. for file open)
	}
	m_isUntitled = f.isEmpty();
	setWindowTitle(userFriendlyCurrentFile() + "[*]");
}

// TODO: unify with setVisibility / check if one of the two calls redundant!
void MdiChild::changeVisibility(unsigned char mode)
{
	m_visibility = mode;

	bool  rc = (mode & RC) == RC;
	bool  xy = (mode & XY) == XY;
	bool  yz = (mode & YZ) == YZ;
	bool  xz = (mode & XZ) == XZ;
	bool tab = (mode & TAB) == TAB;
	m_dwRenderer->setVisible(rc);
	m_dwSlicer[iASlicerMode::XY]->setVisible(xy);
	m_dwSlicer[iASlicerMode::YZ]->setVisible(yz);
	m_dwSlicer[iASlicerMode::XZ]->setVisible(xz);

	if (isVolumeDataLoaded())
	{	// TODO: check redundancy with hideHistogram calls?
		m_dwHistogram->setVisible(tab);
	}
}

void MdiChild::multiview()
{
	changeVisibility(MULTI);
}

void MdiChild::hideVolumeWidgets()
{
	setVisibility(QList<QWidget*>() << m_dwSlicer[iASlicerMode::XY] << m_dwSlicer[iASlicerMode::XZ] << m_dwSlicer[iASlicerMode::YZ] << m_dwRenderer, false);
	update();
}

void MdiChild::setVisibility(QList<QWidget*> widgets, bool show)
{
	for (int i = 0; i < widgets.size(); ++i)
	{
		show ? widgets[i]->show() : widgets[i]->hide();
	}
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
	iAChannelData* chData = channelData(id);
	assert(chData);
	if (!chData)
	{
		return;
	}
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->addChannel(id, *chData, false);
	}

	if (use3D)
	{
		chData->set3D(true);
		// TODO: VOLUME: rewrite using separate volume /
		//    add capabilities of combining volumes from TripleHistogramTF module to renderer
		// m_renderer->addChannel(chData);
	}
	setChannelRenderingEnabled(id, enableChannel);
}

iAChannelData* MdiChild::channelData(uint id)
{
	auto it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return nullptr;
	}
	return it->data();
}

iAChannelData const* MdiChild::channelData(uint id) const
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
	{
		m_slicer[s]->update();
	}
}

void MdiChild::updateChannelOpacity(uint id, double opacity)
{
	if (!channelData(id))
	{
		return;
	}
	channelData(id)->setOpacity(opacity);
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setChannelOpacity(id, opacity);
	}
	updateSlicers();
}

void MdiChild::setChannelRenderingEnabled(uint id, bool enabled)
{
	iAChannelData* chData = channelData(id);
	if (!chData || chData->isEnabled() == enabled)
	{
		// the channel with the given ID doesn't exist or hasn't changed
		return;
	}
	chData->setEnabled(enabled);
	setSlicerChannelEnabled(id, enabled);
	/*
	// TODO: VOLUME: rewrite using volume manager:
	if (chData->Uses3D())
	{
		renderWidget()->updateChannelImages();
	}
	*/
	updateViews();
}

void MdiChild::setSlicerChannelEnabled(uint id, bool enabled)
{
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		slicer(i)->enableChannel(id, enabled);
	}
}

void MdiChild::removeChannel(uint id)
{
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (slicer(i)->hasChannel(id))
		{
			slicer(i)->removeChannel(id);
		}
	}
	m_channels.remove(id);
}

void MdiChild::removeFinishedAlgorithms()
{
	for (size_t i = 0; i < m_workingAlgorithms.size(); )
	{
		if (m_workingAlgorithms[i]->isFinished())
		{
			delete m_workingAlgorithms[i];
			m_workingAlgorithms.erase(m_workingAlgorithms.begin() + i);
		}
		else
		{
			++i;
		}
	}
}

void MdiChild::cleanWorkingAlgorithms()
{
	for (size_t i = 0; i < m_workingAlgorithms.size(); ++i)
	{
		if (m_workingAlgorithms[i]->isRunning())
		{
			m_workingAlgorithms[i]->SafeTerminate();
			delete m_workingAlgorithms[i];
		}
	}
	m_workingAlgorithms.clear();
}

void MdiChild::addProfile()
{
	m_profileProbe = QSharedPointer<iAProfileProbe>(new iAProfileProbe(m_imageData));
	double start[3];
	m_imageData->GetOrigin(start);
	int const* const dim = m_imageData->GetDimensions();
	double const* const spacing = m_imageData->GetSpacing();
	double end[3];
	for (int i = 0; i < 3; ++i)
	{
		end[i] = start[i] + (dim[i] - 1) * spacing[i];
	}
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setProfilePoint(0, start);
		m_slicer[s]->setProfilePoint(1, end);
	}
	m_renderer->setProfilePoint(0, start);
	m_renderer->setProfilePoint(1, end);
	m_profileProbe->updateProbe(0, start);
	m_profileProbe->updateProbe(1, end);
	m_profileProbe->updateData();
	m_dwProfile = new dlg_profile(this, m_profileProbe->m_profileData, m_profileProbe->rayLength());
	tabifyDockWidget(m_dwHistogram, m_dwProfile);
	connect(m_dwProfile->profileMode, &QCheckBox::toggled, this, &MdiChild::toggleProfileHandles);
}

void MdiChild::toggleProfileHandles(bool isChecked)
{
	m_profileHandlesEnabled = (bool)isChecked;
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setProfileHandlesOn(m_profileHandlesEnabled);
	}
	m_renderer->setProfileHandlesOn(m_profileHandlesEnabled);
}

void MdiChild::updateProbe(int ptIndex, double* newPos)
{
	if (m_imageData->GetNumberOfScalarComponents() != 1) //No profile for rgb, rgba or vector pixel type images
	{
		return;
	}
	m_profileProbe->updateProbe(ptIndex, newPos);
	updateProfile();
}

void MdiChild::updateProfile()
{
	m_profileProbe->updateData();
	m_dwProfile->profileWidget->initialize(m_profileProbe->m_profileData, m_profileProbe->rayLength());
	m_dwProfile->profileWidget->update();
}

int MdiChild::sliceNumber(int mode) const
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_slicer[mode]->sliceNumber();
}

void MdiChild::maximizeDockWidget(QDockWidget* dw)
{
	m_beforeMaximizeState = saveState();
	QList<QDockWidget*> dockWidgets = findChildren<QDockWidget*>();
	for (int i = 0; i < dockWidgets.size(); ++i)
	{
		QDockWidget* curDW = dockWidgets[i];
		if (curDW != dw)
		{
			curDW->setVisible(false);
		}
	}
	m_whatMaximized = dw;
	m_isSmthMaximized = true;
}

void MdiChild::demaximizeDockWidget(QDockWidget* /*dw*/)
{
	restoreState(m_beforeMaximizeState);
	m_isSmthMaximized = false;
}

void MdiChild::resizeDockWidget(QDockWidget* dw)
{
	if (m_isSmthMaximized)
	{
		if (m_whatMaximized == dw)
		{
			demaximizeDockWidget(dw);
		}
		else
		{
			demaximizeDockWidget(m_whatMaximized);
			maximizeDockWidget(dw);
		}
	}
	else
	{
		maximizeDockWidget(dw);
	}
}

void MdiChild::ioFinished()
{
	m_ioThread = nullptr;
	m_tmpSaveImg = nullptr;
}

iASlicer* MdiChild::slicer(int mode)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_slicer[mode];
}

dlg_slicer* MdiChild::slicerDockWidget(int mode)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_dwSlicer[mode];
}

dlg_renderer* MdiChild::renderDockWidget()
{
	return m_dwRenderer;
}

dlg_imageproperty* MdiChild::imagePropertyDockWidget()
{
	return m_dwImgProperty;
}

dlg_profile* MdiChild::profileDockWidget()
{
	return m_dwProfile;
}

iADockWidgetWrapper* MdiChild::histogramDockWidget()
{
	return m_dwHistogram;
}

void MdiChild::setReInitializeRenderWindows(bool reInit)
{
	m_reInitializeRenderWindows = reInit;
}

vtkTransform* MdiChild::slicerTransform()
{
	return m_slicerTransform;
}

bool MdiChild::resultInNewWindow() const
{
	return m_preferences.ResultInNewWindow;
}

bool MdiChild::linkedMDIs() const
{
	return m_slicerSettings.LinkMDIs;
}

bool MdiChild::linkedViews() const
{
	return m_slicerSettings.LinkViews;
}

void MdiChild::check2DMode()
{
	if (modalities()->size() == 0)
	{
		return;
	}
	// TODO: check over all modalities?
	int dim[3];
	modality(0)->image()->GetDimensions(dim);

	if (dim[0] == 1 && dim[1] > 1 && dim[2] > 1)
	{
		maximizeSlicer(iASlicerMode::YZ);
	}
	else if (dim[0] > 1 && dim[1] == 1 && dim[2] > 1)
	{
		maximizeSlicer(iASlicerMode::XZ);
	}
	else if (dim[0] > 1 && dim[1] > 1 && dim[2] == 1)
	{
		maximizeSlicer(iASlicerMode::XY);
	}
}

void MdiChild::setMagicLensInput(uint id)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensInput(id);
	}
}

void MdiChild::setMagicLensEnabled(bool isOn)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensEnabled(isOn);
	}
}

void MdiChild::updateChannel(uint id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf, bool enable)
{
	iAChannelData* chData = channelData(id);
	if (!chData)
	{
		return;
	}
	chData->setData(imgData, ctf, otf);
	for (uint s = 0; s < 3; ++s)
	{
		if (m_slicer[s]->hasChannel(id))
		{
			m_slicer[s]->updateChannel(id, *chData);
		}
		else
		{
			m_slicer[s]->addChannel(id, *chData, enable);
		}
	}
}

void MdiChild::reInitMagicLens(uint id, QString const& name, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf)
{
	if (!m_isMagicLensEnabled)
	{
		return;
	}

	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateChannel(id, iAChannelData(name, imgData, ctf));
	}
	setMagicLensInput(id);
	updateSlicers();
}

int MdiChild::magicLensSize() const
{
	return m_preferences.MagicLensSize;
}
int MdiChild::magicLensFrameWidth() const
{
	return m_preferences.MagicLensFrameWidth;
}

void MdiChild::updateChannelMappers()
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateChannelMappers();
	}
}

QString MdiChild::filePath() const
{
	return m_path;
}

iAVolumeStack* MdiChild::volumeStack()
{
	return m_volumeStack.data();
}

bool MdiChild::isVolumeDataLoaded() const
{
	QString suffix = fileInfo().suffix();
	int* extent = m_imageData->GetExtent();
	return QString::compare(suffix, "STL", Qt::CaseInsensitive) != 0 &&
		// need better way to check that! at this point, modalities not set up yet,
		// but .vtk files can contain both polydata and volumes!
		// Maybe extent check is enough?
		// QString::compare(suffix, "VTK", Qt::CaseInsensitive) != 0 &&
		QString::compare(suffix, "FEM", Qt::CaseInsensitive) != 0 &&
		extent[1] >= 0 && extent[3] >= 0 && extent[5] >= 0;
}

void MdiChild::changeMagicLensModality(int chg)
{
	if (!m_isMagicLensEnabled)
	{
		return;
	}
	m_currentComponent = (m_currentComponent + chg);
	if (m_currentComponent < 0 || static_cast<size_t>(m_currentComponent) >= modality(m_currentModality)->componentCount())
	{
		m_currentComponent = 0;
		m_currentModality = (m_currentModality + chg + modalities()->size()) % (modalities()->size());
	}
	if (m_currentModality < 0 || m_currentModality >= modalities()->size())
	{
		LOG(lvlWarn, "Invalid modality index!");
		m_currentModality = 0;
		return;
	}
	if (m_magicLensChannel == NotExistingChannel)
	{
		m_magicLensChannel = createChannel();
	}
	vtkSmartPointer<vtkImageData> img = modality(m_currentModality)->component(m_currentComponent);
	channelData(m_magicLensChannel)->setOpacity(0.5);
	QString name(modality(m_currentModality)->imageName(m_currentComponent));
	channelData(m_magicLensChannel)->setName(name);
	updateChannel(m_magicLensChannel, img, m_dwModalities->colorTF(m_currentModality), m_dwModalities->opacityTF(m_currentModality), false);
	setMagicLensInput(m_magicLensChannel);
	setHistogramModality(m_currentModality);	// TODO: don't change histogram, just read/create min/max and transfer function?
}

void MdiChild::changeMagicLensOpacity(int chg)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensOpacity(m_slicer[s]->magicLensOpacity() + (chg * 0.05));
	}
}

void MdiChild::changeMagicLensSize(int chg)
{
	if (!isMagicLens2DEnabled())
	{
		return;
	}
	double sizeFactor = 1.1 * (std::abs(chg));
	if (chg < 0)
	{
		sizeFactor = 1 / sizeFactor;
	}
	int newSize = std::max(MinimumMagicLensSize, static_cast<int>(m_preferences.MagicLensSize * sizeFactor));
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setMagicLensSize(newSize);
		newSize = std::min(m_slicer[s]->magicLensSize(), newSize);
	}
	m_preferences.MagicLensSize = newSize;
	updateSlicers();
}

int MdiChild::currentModality() const
{
	return m_currentModality;
}

void MdiChild::showModality(int modIdx)
{
	if (m_currentModality == modIdx)
	{
		return;
	}
	m_currentModality = modIdx;
	m_currentComponent = 0;
	setHistogramModality(modIdx);
}

void MdiChild::setModalities(QSharedPointer<iAModalityList> modList)
{
	bool noDataLoaded = modalities()->size() == 0;
	m_dwModalities->setModalities(modList);

	if (noDataLoaded && modalities()->size() > 0)
	{
		initModalities();
	}
}

dlg_modalities* MdiChild::dataDockWidget()
{
	return m_dwModalities;
}

QSharedPointer<iAModalityList> MdiChild::modalities()
{
	return m_dwModalities->modalities();
}

QSharedPointer<iAModality> MdiChild::modality(int idx)
{
	return modalities()->get(idx);
}

void MdiChild::initModalities()
{
	for (int i = 0; i < modalities()->size(); ++i)
	{
		m_dwModalities->addListItem(modality(i));
	}
	// TODO: VOLUME: rework - workaround: "initializes" renderer and slicers with modality 0
	m_initVolumeRenderers = true;
	setImageData(
		currentFile().isEmpty() ? modality(0)->fileName() : currentFile(),
		modality(0)->image()
	);
	m_dwModalities->selectRow(0);
}

void MdiChild::setHistogramModality(int modalityIdx)
{
	if (!m_histogram || modalities()->size() <= modalityIdx ||
		modality(modalityIdx)->image()->GetNumberOfScalarComponents() != 1) //No histogram/profile for rgb, rgba or vector pixel type images
	{
		return;
	}
	if (modality(modalityIdx)->transfer()->statisticsComputed())
	{
		displayHistogram(modalityIdx);
		return;
	}
	if (modality(modalityIdx)->info().isComputing()) // already computing currently...
	{
		return;
	}
	LOG(lvlDebug, QString("Computing statistics for modality %1...")
		.arg(modality(modalityIdx)->name()));
	modality(modalityIdx)->transfer()->info().setComputing();
	updateImageProperties();

	runAsync([this, modalityIdx]
		{
			modality(modalityIdx)->computeImageStatistics();
		},
		[this, modalityIdx]
		{
			statisticsAvailable(modalityIdx);
		});
}

void MdiChild::modalityAdded(int modalityIdx)
{
	if (modality(modalityIdx)->image()->GetNumberOfScalarComponents() == 1) //No histogram/profile for rgb, rgba or vector pixel type images
	{
		setHistogramModality(modalityIdx);
	}
	else
	{
		m_dwModalities->initDisplay(modality(modalityIdx));
		applyVolumeSettings(false);
	}
}

void MdiChild::histogramDataAvailable(int modalityIdx)
{
	QString modalityName = modality(modalityIdx)->name();
	m_currentHistogramModality = modalityIdx;
	LOG(lvlDebug, QString("Displaying histogram for modality %1.").arg(modalityName));
	m_histogram->removePlot(m_histogramPlot);
	m_histogramPlot = QSharedPointer<iAPlot>(new
		iABarGraphPlot(modality(modalityIdx)->histogramData(),
			QWidget::palette().color(QPalette::Shadow)));
	m_histogram->addPlot(m_histogramPlot);
	m_histogram->setXCaption("Histogram " + modalityName);
	m_histogram->setTransferFunctions(modality(modalityIdx)->transfer()->colorTF(),
		modality(modalityIdx)->transfer()->opacityTF());
	m_histogram->updateTrf();	// will also redraw() the histogram
	updateImageProperties();
	if (!findChild<iADockWidgetWrapper*>("Histogram"))
	{
		splitDockWidget(m_dwRenderer, m_dwHistogram, Qt::Vertical);
		addProfile();
	}
	emit histogramAvailable();
}

void MdiChild::displayHistogram(int modalityIdx)
{
	auto histData = modality(modalityIdx)->transfer()->histogramData();
	size_t newBinCount = m_preferences.HistogramBins;
	auto img = modality(modalityIdx)->image();
	auto scalarRange = img->GetScalarRange();
	if (isVtkIntegerType(modality(modalityIdx)->image()->GetScalarType()))
	{
		newBinCount = std::min(newBinCount, static_cast<size_t>(scalarRange[1] - scalarRange[0] + 1));
	}
	if (histData && histData->numBin() == newBinCount)
	{
		if (modalityIdx != m_currentHistogramModality)
		{
			histogramDataAvailable(modalityIdx);
		}
		return;
	}

	LOG(lvlDebug, QString("Computing histogram for modality %1...")
		.arg(modality(modalityIdx)->name()));
	runAsync([this, modalityIdx, newBinCount]
		{   // run computation of histogram...
			modality(modalityIdx)->computeHistogramData(newBinCount);
		},  // ... and on finished signal, trigger histogramDataAvailable
		[this, modalityIdx]
		{
			histogramDataAvailable(modalityIdx);
		});
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

void MdiChild::resetCamera(bool spacingChanged, double const* newSpacing)
{
	if (!spacingChanged)
	{
		return;
	}
	// reset camera when the spacing of modality was changed
	// TODO: maybe instead of reset, apply a zoom corresponding to the ratio of spacing change?
	m_renderer->updateSlicePlanes(newSpacing);
	m_renderer->renderer()->ResetCamera();
	m_renderer->update();
	for (int s = 0; s < 3; ++s)
	{
		set3DSlicePlanePos(s, sliceNumber(s));
		slicer(s)->renderer()->ResetCamera();
		slicer(s)->update();
	}
	emit viewsUpdated();
}

void MdiChild::initVolumeRenderers()
{
	if (!m_initVolumeRenderers)
	{
		for (int i = 0; i < modalities()->size(); ++i)
		{
			modality(i)->updateRenderer();
		}
		return;
	}
	m_initVolumeRenderers = false;
	for (int i = 0; i < modalities()->size(); ++i)
	{
		m_dwModalities->initDisplay(modality(i));
	}
	applyVolumeSettings(true);
	connect(modalities().data(), &iAModalityList::added, m_dwModalities, &dlg_modalities::modalityAdded);
	m_renderer->renderer()->ResetCamera();
}

void MdiChild::saveProject(QString const& fileName)
{
	m_ioThread = new iAIO(modalities(), m_renderer->renderer()->GetActiveCamera(), iALog::get());
	connectIOThreadSignals(m_ioThread);
	QFileInfo fileInfo(fileName);
	if (!m_ioThread->setupIO(PROJECT_WRITER, fileInfo.absoluteFilePath()))
	{
		ioFinished();
		return;
	}
	LOG(lvlInfo, tr("Saving file '%1'.").arg(fileName));
	m_ioThread->start();
	// TODO: only set new project file name if saving succeeded
	setCurrentFile(fileName);
}

bool MdiChild::doSaveProject(QString const & projectFileName)
{
	QVector<int> unsavedModalities;
	for (int i = 0; i < modalities()->size(); ++i)
	{
		if (modality(i)->fileName().isEmpty())
		{
			unsavedModalities.push_back(i);
		}
	}
	if (unsavedModalities.size() > 0)
	{
		if (QMessageBox::question(m_mainWnd, "Unsaved modalities",
			"This window has some unsaved modalities, you need to save them before you can store the project. Save them now?",
			QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) != QMessageBox::Yes)
		{
			return false;
		}
		for (int modNr : unsavedModalities)
		{
			if (!saveAs(modNr))
			{
				return false;
			}
		}
	}
	// TODO:
	//   - work in background
	saveProject(projectFileName);
	if (projectFileName.toLower().endsWith(iAIOProvider::NewProjectFileExtension))
	{
		QSettings projectFile(projectFileName, QSettings::IniFormat);
		projectFile.setIniCodec("UTF-8");
		projectFile.setValue("UseMdiChild", true);
		for (auto projectKey : m_projects.keys())
		{
			projectFile.beginGroup(projectKey);
			m_projects[projectKey]->saveProject(projectFile, projectFileName);
			projectFile.endGroup();
		}
	}
	return true;
}

void MdiChild::addProject(QString const& key, QSharedPointer<iAProjectBase> project)
{
	project->setChild(this);
	m_projects.insert(key, project);
}

QMap<QString, QSharedPointer<iAProjectBase>> const& MdiChild::projects()
{
	return m_projects;
}

MdiChild::iAInteractionMode MdiChild::interactionMode() const
{
	return m_interactionMode;
}

void MdiChild::setInteractionMode(iAInteractionMode mode)
{
	m_interactionMode = mode;
	m_mainWnd->updateInteractionModeControls(mode);
	m_dwModalities->setInteractionMode(mode == imRegistration);
}

bool MdiChild::meshDataMovable()
{
	return renderer()->polyActor()->GetDragable();
}

void MdiChild::setMeshDataMovable(bool movable)
{
	renderer()->polyActor()->SetPickable(movable);
	renderer()->polyActor()->SetDragable(movable);
}

MainWindow* MdiChild::mainWnd()
{
	return m_mainWnd;
}

vtkPiecewiseFunction* MdiChild::opacityTF()
{
	return modality(0)->transfer()->opacityTF();
}

vtkColorTransferFunction* MdiChild::colorTF()
{
	return modality(0)->transfer()->colorTF();
}

void MdiChild::saveFinished()
{
	if (m_storedModalityNr < modalities()->size() && m_ioThread->ioID() != STL_WRITER)
		m_dwModalities->setFileName(m_storedModalityNr, m_ioThread->fileName());
	m_mainWnd->setCurrentFile(m_ioThread->fileName());
	setWindowModified(modalities()->hasUnsavedModality());
}

bool MdiChild::isFullyLoaded() const
{
	int const* dim = m_imageData->GetDimensions();
	return dim[0] > 0 && dim[1] > 0 && dim[2] > 0;
}

void MdiChild::styleChanged()
{
	if (m_histogramPlot)
	{
		m_histogramPlot->setColor(QWidget::palette().color(QPalette::Shadow));
	}
}
