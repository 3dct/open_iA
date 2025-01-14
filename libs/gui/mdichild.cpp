// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mdichild.h"

#include "iASlicerContainer.h"
#include "ui_renderer.h"

#include "iADataSetViewerImpl.h"
#include "iADataSetRenderer.h"
#include "iAVolumeViewer.h"    // TODO NEWIO: only required for changing magic lens input - move from here, e.g. to slicer
#include "iAFileParamDlg.h"
#include "iAFileUtils.h"    // for safeFileName
#include "iAvtkInteractStyleActor.h"
#include "mainwindow.h"

// renderer
#include <iARendererImpl.h>

// slicer
#include <iASlicerImpl.h>

// guibase
#include <iAChannelID.h>    // for NotExistingChannel
#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iADataSetListWidget.h>
#include <iADockWidgetWrapper.h>
#include <iAJobListView.h>
#include <iAMovieHelper.h>
#include <iAParameterDlg.h>
#include <iAPreferences.h>
#include <iATool.h>
#include <iARunAsync.h>

// io
#include <iAFileStackParams.h>
#include <iAImageStackFileIO.h>
#include <iAProjectFileIO.h>
#include <iAVolStackFileIO.h>

// base
#include <iAImageData.h>
#include <iAFileTypeRegistry.h>
#include <iALog.h>
#include <iAPolyData.h>
#include <iAProgress.h>
#include <iAStringHelper.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>

#include <vtkColorTransferFunction.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>

// TODO: refactor methods using the following out of mdichild!
#include <vtkCamera.h>
#include <vtkImageReslice.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkPoints.h>
#include <vtkTransform.h>
#include <vtkWindowToImageFilter.h>

#include <QByteArray>
#include <QCloseEvent>
#include <QFile>
#include <QFileDialog>
#include <QListWidget>
#include <QMainWindow>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>

#include <numbers>

//! avoid iAQTtoUIConnector to be able to forward-declare iARendererContainer
class iARendererContainer : public QWidget, public Ui_renderer
{
public:
	iARendererContainer(QWidget* parent) : QWidget(parent)
	{
		setupUi(this);
	}
	void showTitle(bool show) {
		rendererControls->setVisible(show);
	}
	bool isTitleShown() const {
		return rendererControls->isVisible();
	}
};

MdiChild::MdiChild(MainWindow* mainWnd, iAPreferences const& prefs, bool unsavedChanges) :
	m_mainWnd(mainWnd),
	m_preferences(prefs),
	m_isSmthMaximized(false),
	m_isUntitled(true),
	m_isSliceProfileEnabled(false),
	m_profileHandlesEnabled(false),
	m_isMagicLensEnabled(false),
	m_slicerTransform(vtkSmartPointer<vtkTransform>::New()),
	m_dataSetInfo(new QListWidget(this)),
	m_dataSetListWidget(new iADataSetListWidget()),
	m_rendererContainer(new iARendererContainer(this)),
	m_dwInfo(new iADockWidgetWrapper(m_dataSetInfo, "Dataset Info", "DataInfo", "https://github.com/3dct/open_iA/wiki/Core#widgets")),
	m_dwDataSets(new iADockWidgetWrapper(m_dataSetListWidget, "Datasets", "DataSets", "https://github.com/3dct/open_iA/wiki/Dataset-List")),
	m_dwRenderer(new iADockWidgetWrapper(m_rendererContainer, "3D Renderer", "renderer", "https://github.com/3dct/open_iA/wiki/3D-Renderer")),
	m_nextChannelID(0),
	m_magicLensChannel(NotExistingChannel),
	m_magicLensDataSet(0),
	m_interactionMode(imCamera),
	m_nextDataSetID(0)
{
	setAcceptDrops(true);
	setWindowIcon(QIcon(QPixmap(":/images/iA.svg")));
	setAttribute(Qt::WA_DeleteOnClose);
	setTabShape(QTabWidget::Triangular);
	setDockOptions(QMainWindow::AllowTabbedDocks | QMainWindow::AllowNestedDocks | QMainWindow::GroupedDragging);
	setWindowModified(unsavedChanges);
	setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
	addDockWidget(Qt::LeftDockWidgetArea, m_dwRenderer);
	m_initialLayoutState = saveState();
	for (int i = 0; i < 3; ++i)
	{
		auto sliceMode = static_cast<iASlicerMode>(i);
		m_slicer[i] = new iASlicerImpl(this, sliceMode, true, true, m_slicerTransform);
		auto sn = slicerModeString(sliceMode);
		m_slicerContainer[i] = new iASlicerContainer(m_slicer[i]);
		m_dwSlicer[i] = new iADockWidgetWrapper(m_slicerContainer[i], QString("Slice %1").arg(sn), QString("slice%1").arg(sn),
			"https://github.com/3dct/open_iA/wiki/2D-Slicers");
	}
	splitDockWidget(m_dwRenderer, m_dwSlicer[iASlicerMode::XY], Qt::Horizontal);
	splitDockWidget(m_dwSlicer[iASlicerMode::XY], m_dwSlicer[iASlicerMode::XZ], Qt::Vertical);
	splitDockWidget(m_dwSlicer[iASlicerMode::XZ], m_dwSlicer[iASlicerMode::YZ], Qt::Vertical);
	for (int i = 0; i < 3; ++i)
	{
		m_dwSlicer[i]->hide();
	}
	splitDockWidget(m_dwRenderer, m_dwDataSets, Qt::Vertical);
	splitDockWidget(m_dwDataSets, m_dwInfo, Qt::Horizontal);

	m_renderer = new iARendererImpl(this, dynamic_cast<vtkGenericOpenGLRenderWindow*>(m_rendererContainer->vtkWidgetRC->renderWindow()));

	connectSignalsToSlots();
}

void MdiChild::initializeViews()
{
	// avoid 3D renderer being resized to very small (resulting from splitDockWidget)
	float h = geometry().height();
	resizeDocks(QList<QDockWidget*>{ m_dwRenderer, m_dwDataSets, m_dwInfo },
		QList<int>{ static_cast<int>(0.7 * h), static_cast<int>(0.2 * h), static_cast<int>(0.2 * h) }, Qt::Vertical);
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

void MdiChild::connectSignalsToSlots()
{
	connect(m_mainWnd, &MainWindow::fullScreenToggled, this, &MdiChild::toggleFullScreen);

	connect(m_dataSetListWidget, &iADataSetListWidget::dataSetSelected, this, &iAMdiChild::dataSetSelected);

	connect(m_renderer, &iARendererImpl::bgColorChanged, m_rendererContainer->vtkWidgetRC, &iAFast3DMagicLensWidget::setLensBackground);
	connect(m_renderer, &iARendererImpl::interactionModeChanged, this, [this](bool camera)
	{
		setInteractionMode(camera ? imCamera : imRegistration);
	});
	auto adjustSliceShowBorders = [this]()
	{
		bool otherSlicePlaneShown = false;
		for (int i = 0; i < 3; ++i)
		{
			otherSlicePlaneShown |= m_slicer[i]->settings().value(iASlicerImpl::ShowOtherSlicePlanes).toBool();
		}
		for (int i = 0; i < 3; ++i)
		{
			m_slicerContainer[i]->showBorder(m_renderer->isShowSlicePlanes() || otherSlicePlaneShown);
		}
	};
	auto adoptChangedSlicerSettings = [this, adjustSliceShowBorders]()
	{
		adjustSliceShowBorders();
		auto s = m_renderer->settings()[iARendererImpl::MagicLensSize].toInt();
		auto fw = m_renderer->settings()[iARendererImpl::MagicLensFrameWidth].toInt();
		m_rendererContainer->vtkWidgetRC->setLensSize(s, s);
		m_rendererContainer->vtkWidgetRC->setFrameWidth(fw);
	};
	connect(m_renderer, &iARendererImpl::settingsChanged, this, adoptChangedSlicerSettings);
	adoptChangedSlicerSettings();
	m_rendererContainer->vtkWidgetRC->setContextMenuEnabled(true);
	// TODO: merge iAFast3DMagicLensWidget (at least iAQVTKWidget part) with iARenderer, then this can be moved there:
	connect(m_rendererContainer->vtkWidgetRC, &iAFast3DMagicLensWidget::editSettings, m_renderer, &iARendererImpl::editSettings);
	connect(m_rendererContainer->tbMax, &QPushButton::clicked, this, &MdiChild::maximizeRenderer);
	connect(m_rendererContainer->pbToggleInteraction, &QPushButton::clicked, this, [this]
	{
		enableRendererInteraction(!isRendererInteractionEnabled());
	});
	connect(m_rendererContainer->pushPX,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PX); });
	connect(m_rendererContainer->pushPY,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PY); });
	connect(m_rendererContainer->pushPZ,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PZ); });
	connect(m_rendererContainer->pushMX,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MX); });
	connect(m_rendererContainer->pushMY,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MY); });
	connect(m_rendererContainer->pushMZ,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MZ); });
	connect(m_rendererContainer->pushIso, &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::Iso); });
	connect(m_rendererContainer->pbSaveScreen, &QPushButton::clicked, this, &MdiChild::saveRC);
	connect(m_rendererContainer->pbSaveMovie, &QPushButton::clicked, this, &MdiChild::saveMovRC);
	// { TODO: strange way to forward signals, find out why we need to do this and find better way:
	connect(m_rendererContainer->vtkWidgetRC, &iAFast3DMagicLensWidget::rightButtonReleasedSignal, m_renderer, &iARendererImpl::mouseRightButtonReleasedSlot);
	connect(m_rendererContainer->vtkWidgetRC, &iAFast3DMagicLensWidget::leftButtonReleasedSignal, m_renderer, &iARendererImpl::mouseLeftButtonReleasedSlot);
	connect(m_rendererContainer->vtkWidgetRC, &iAFast3DMagicLensWidget::touchStart, m_renderer, &iARendererImpl::touchStart);
	connect(m_rendererContainer->vtkWidgetRC, &iAFast3DMagicLensWidget::touchScale, m_renderer, &iARendererImpl::touchScaleSlot);
	//! }

	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		connect(m_slicerContainer[s]->tbMax, &QPushButton::clicked, [this, s] { maximizeSlicer(s); });
		connect(m_dwSlicer[s], &QDockWidget::visibilityChanged, [this, s] { m_renderer->showSlicePlane(s, m_dwSlicer[s]->isVisible()); });
		m_manualMoveStyle[s] = vtkSmartPointer<iAvtkInteractStyleActor>::New();
		connect(m_manualMoveStyle[s].Get(), &iAvtkInteractStyleActor::actorsUpdated, this, &iAMdiChild::updateViews);
		connect(m_slicer[s], &iASlicer::shiftMouseWheel, this, &MdiChild::changeMagicLensDataSet);
		connect(m_slicer[s], &iASlicerImpl::sliceRotated, this, &MdiChild::slicerRotationChanged);
		connect(m_slicer[s], &iASlicer::sliceNumberChanged, this, &MdiChild::setSlice);
		connect(m_slicer[s], &iASlicer::mouseMoved, this, &MdiChild::updatePositionMarker);
		connect(m_slicer[s], &iASlicerImpl::otherSlicePlaneVisbilityChanged, this, adjustSliceShowBorders);
		connect(m_slicer[s], &iASlicerImpl::regionSelected, this, [this](int minVal, int maxVal, uint channelID)
		{
			// TODO NEWIO: move to better place, e.g. dataset viewer for image data? slicer?
			if (minVal == maxVal)
			{
				return;
			}
			for (auto const & viewer: m_dataSetViewers)
			{
				viewer.second->slicerRegionSelected(minVal, maxVal, channelID);
			}
			updateViews();
		});
		connect(m_slicer[s], &iASlicerImpl::profilePointChanged, this, &iAMdiChild::profilePointChanged);
		connect(m_slicer[s], &iASlicerImpl::profilePointChanged, m_renderer, &iARendererImpl::setProfilePoint);
		connect(m_slicer[s], &iASlicer::magicLensToggled, this, &MdiChild::toggleMagicLens2D);
		for (int j = 0; j < 3; ++j)
		{
			if (s != j)	// connect each slicer's signals to the other slicer's slots, except for its own:
			{
				connect(m_slicer[s], &iASlicerImpl::profilePointChanged, m_slicer[j], &iASlicerImpl::setProfilePoint);
			}
		}
	}
	m_manualMoveStyle[3] = vtkSmartPointer<iAvtkInteractStyleActor>::New();    // for renderer
}

void MdiChild::updatePositionMarker(double x, double y, double z, int mode)
{
	double pos[3] = { x, y, z };
	m_renderer->setPositionMarkerCenter(x, y, z);
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (mode == i)  // only update other slicers
		{
			continue;
		}
		if (m_slicerSettings.LinkViews && m_slicer[i]->hasChannel(0))  // TODO: check for whether dataset is shown in slicer?
		{
			m_slicer[i]->setSlicePosition(pos[mapSliceToGlobalAxis(i, iAAxisIndex::Z)]);
		}
		if (m_slicer[i]->settings()[iASlicerImpl::ShowPosition].toBool())
		{
			int slicerXAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::X);
			int slicerYAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::Y);
			int slicerZAxisIdx = mapSliceToGlobalAxis(i, iAAxisIndex::Z);
			m_slicer[i]->setPositionMarkerCenter(
				pos[slicerXAxisIdx],
				pos[slicerYAxisIdx],
				pos[slicerZAxisIdx]);
		}
	}
}

size_t MdiChild::addDataSet(std::shared_ptr<iADataSet> dataSet)
{
	size_t dataSetIdx;
	{
		QMutexLocker locker(&m_dataSetMutex);
		dataSetIdx = m_nextDataSetID;
		++m_nextDataSetID;
	}
	m_dataSets[dataSetIdx] = dataSet;
	if (m_curFile.isEmpty())
	{
		//LOG(lvlDebug, "Developer Warning - consider calling setWindowTitleAndFile directly where you first call addDataSet");
		setWindowTitleAndFile(
			dataSet->hasMetaData(iADataSet::FileNameKey) ?
			dataSet->metaData(iADataSet::FileNameKey).toString() :
			dataSet->name());
	}
	else if (dataSet->hasMetaData(iADataSet::FileNameKey))
	{
		m_mainWnd->addRecentFile(dataSet->metaData(iADataSet::FileNameKey).toString());
	}
	auto p = std::make_shared<iAProgress>();
	auto viewer = createDataSetViewer(dataSet.get());
	if (!viewer)
	{
		LOG(lvlError, "No viewer associated with this dataset type!");
		return dataSetIdx;
	}
	connect(viewer.get(), &iADataSetViewer::dataSetChanged, this, [this](size_t dsIdx)
	{
		updateDataSetInfo();
		emit dataSetChanged(dsIdx);
	});
	connect(viewer.get(), &iADataSetViewer::removeDataSet, this, &iAMdiChild::removeDataSet);
	m_dataSetViewers[dataSetIdx] = viewer;
	auto fw = runAsync(
		[viewer, p]()
		{
			viewer->prepare(p.get());
		},
		[this, viewer, dataSetIdx]
		{
			emit dataSetPrepared(dataSetIdx);
			viewer->createGUI(this, dataSetIdx);
			updateDataSetInfo();
			updatePositionMarkerSize();
			updateViews();
			emit dataSetRendered(dataSetIdx);
		},
		this);
	iAJobListView::get()->addJob(QString("Computing display data for %1").arg(dataSet->name()), p.get(), fw);
	return dataSetIdx;
}

void MdiChild::removeDataSet(size_t dataSetIdx)
{
	if (!m_dataSetViewers.contains(dataSetIdx))
	{
		LOG(lvlDebug, QString("Trying to remove dataset idx=%1 which does not exist (anymore?)!").arg(dataSetIdx));
		return;
	}
	LOG(lvlDebug, QString("Removing dataset idx = %1.").arg(dataSetIdx));
	m_dataSetViewers.erase(dataSetIdx);
	m_dataSets.erase(dataSetIdx);
	if (m_isMagicLensEnabled && m_magicLensDataSet == dataSetIdx)
	{
		changeMagicLensDataSet(0);
	}
	updateViews();
	updateDataSetInfo();
	emit dataSetRemoved(dataSetIdx);
}

void MdiChild::clearDataSets()
{
	std::vector<size_t> dataSetKeys;
	dataSetKeys.reserve(m_dataSets.size());
	std::transform(m_dataSets.begin(), m_dataSets.end(), std::back_inserter(dataSetKeys), [](auto const& p) { return p.first; });
	for (auto dataSetIdx : dataSetKeys)
	{
		removeDataSet(dataSetIdx);
	}
}

namespace
{
	const QString CameraPositionKey("CameraPosition");
	const QString CameraFocalPointKey("CameraFocalPoint");
	const QString CameraViewUpKey("CameraViewUp");
	const QString CameraParallelProjection("CameraParallelProjection");
	const QString CameraParallelScale("CameraParallelScale");
	const QString LayoutKey("Layout");
}

iARenderer* MdiChild::renderer()
{
	return m_renderer;
}

/*
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
	QStringList components;
	for (int i = 0; i < nrOfComponents; ++i)
	{
		components << QString::number(i);
	}
	components << "All components";
	iAAttributes params;
	addAttr(params, "Component", iAValueType::Categorical, components);
	iAParameterDlg componentChoice(this, "Choose Component", params);
	if (componentChoice.exec() != QDialog::Accepted)
	{
		return -1;
	}
	return components.indexOf(componentChoice.parameterValues()["Component"].toString());
}
*/

std::shared_ptr<iADataSet> MdiChild::chooseDataSet(QString const & title)
{
	if (m_dataSets.size() == 1)
	{
		return m_dataSets.begin()->second;
	}
	iAAttributes params;
	QStringList dataSetNames;
	for (auto dataSet : dataSetMap())
	{
		dataSetNames << dataSet.second->name();
	}
	const QString DataSetStr("Dataset");
	addAttr(params, DataSetStr, iAValueType::Categorical, dataSetNames);
	iAParameterDlg dataSetChoice(this, title, params);
	if (dataSetChoice.exec() == QDialog::Accepted)
	{
		auto dataSetName = dataSetChoice.parameterValues()[DataSetStr].toString();
		for (auto dataSet : m_dataSets)
		{
			if (dataSet.second->name() == dataSetName)
			{
				return dataSet.second;
			}
		}
	}
	return nullptr;
}

iADataSetListWidget* MdiChild::dataSetListWidget()
{
	return m_dataSetListWidget;
}

void MdiChild::saveVolumeStack()
{
	QString fileName = QFileDialog::getSaveFileName(
		QApplication::activeWindow(),
		tr("Save File"),
		QDir::currentPath(),
		tr("Volstack files (*.volstack);;All files (*)")
	);
	if (fileName.isEmpty())
	{
		return;
	}
	auto imgDataSets = std::make_shared<iADataCollection>(dataSetMap().size(), std::shared_ptr<QSettings>());
	for(auto d: dataSetMap())
	{
		if (dynamic_cast<iAImageData*>(d.second.get()))
		{
			imgDataSets->addDataSet(d.second);
		}
	}

	QFileInfo fi(fileName);
	QVariantMap paramValues;
	paramValues[iAFileStackParams::FileNameBase] = fi.completeBaseName();
	paramValues[iAFileStackParams::Extension] = ".mhd";
	paramValues[iAFileStackParams::NumDigits] = static_cast<int>(std::log10(static_cast<double>(imgDataSets->dataSets().size())) + 1);
	paramValues[iAFileStackParams::MinimumIndex] = 0;
	//paramValues[iAFileStackParams::MaximumIndex] = imgDataSets->size() - 1;

	auto io = std::make_shared<iAVolStackFileIO>();
	if (!iAFileParamDlg::getParameters(this, io.get(), iAFileIO::Save, fileName, paramValues))
	{
		return;
	}
	auto p = std::make_shared<iAProgress>();
	auto fw = runAsync([=]()
		{
			io->save(fileName,imgDataSets, paramValues, *p.get());
		},
		[]() {},
		this);
	iAJobListView::get()->addJob(QString("Saving volume stack %1").arg(fileName), p.get(), fw);
}

bool MdiChild::save()
{
	return saveDataSet(chooseDataSet());
}

bool MdiChild::saveDataSet(std::shared_ptr<iADataSet> dataSet)
{
	if (!dataSet)
	{
		return false;
	}
	QString path = m_path.isEmpty() ? m_mainWnd->path() : m_path;
	QString defaultFilter = iAFileTypeRegistry::defaultExtFilterString(dataSet->type());
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
		path + "/" + safeFileName(dataSet->name()),
		iAFileTypeRegistry::registeredFileTypes(iAFileIO::Save, dataSet->type()),
		 &defaultFilter);
	return saveDataSet(dataSet, fileName);
}

bool MdiChild::saveDataSet(std::shared_ptr<iADataSet> dataSet, QString const& fileName)
{
	if (fileName.isEmpty())
	{
		return false;
	}
	auto io = iAFileTypeRegistry::createIO(fileName, iAFileIO::Save);
	if (!io || !io->isDataSetSupported(dataSet, fileName, iAFileIO::Save))
	{
		auto msg = QString("No I/O found for this dataset, or the chosen file format (%1) does not support this kind of dataset!").arg(io ? io->name(): "none");
		QMessageBox::warning(this, "Save: Error", msg);
		LOG(lvlError, msg);
		return false;
	}
	QVariantMap paramValues;
	if (!iAFileParamDlg::getParameters(this, io.get(), iAFileIO::Save, fileName, paramValues, dataSet.get()))
	{
		return false;
	}
	auto p = std::make_shared<iAProgress>();
	auto futureWatcher = new QFutureWatcher<bool>(this);
	QObject::connect(futureWatcher, &QFutureWatcher<bool>::finished, this, [this, dataSet, fileName, futureWatcher]()
		{
			if (futureWatcher->result())
			{
				dataSet->setMetaData(iADataSet::FileNameKey, fileName);
				if (m_dataSets.size() == 1)
				{
					setWindowTitleAndFile(fileName);
				}
				else
				{
					m_mainWnd->addRecentFile(fileName);
					setWindowModified(hasUnsavedData());
				}
			}
			else
			{
				QMessageBox::warning(this, "Save: Error", QString("Saving %1 failed. See the log window for details.").arg(fileName));
			}
			delete futureWatcher;
		});
	auto future = QtConcurrent::run([fileName, p, io, dataSet, paramValues]()
		{
			return io->save(fileName, dataSet, paramValues, *p.get());
		});
	futureWatcher->setFuture(future);
	iAJobListView::get()->addJob("Save File", p.get(), futureWatcher);
	return true;
}

void MdiChild::updateViews()
{
	updateSlicers();
	updateRenderer();
	emit viewsUpdated();
}

void MdiChild::maximizeSlicer(int mode)
{
	resizeDockWidget(m_dwSlicer[mode]);
}

void MdiChild::maximizeRenderer()
{
	resizeDockWidget(m_dwRenderer);
}

void MdiChild::saveRC()
{
	iAImageStackFileIO io;
	QString file = QFileDialog::getSaveFileName(this, tr("Save Image"),
		"",
		io.filterString());
	if (file.isEmpty())
	{
		return;
	}
	auto filter = vtkSmartPointer<vtkWindowToImageFilter>::New();
	filter->SetInput(m_renderer->renderWindow());
	filter->Update();
	writeSingleSliceImage(file, filter->GetOutput());
}

void MdiChild::saveMovRC()
{
	QString movie_file_types = GetAvailableMovieFormats();

	// If VTK was built without video support, display error message and quit.
	if (movie_file_types.isEmpty())
	{
		QMessageBox::information(this, "Movie Export", "Sorry, but movie export support is disabled.");
		return;
	}

	QStringList modes = (QStringList() << tr("Rotate Z") << tr("Rotate X") << tr("Rotate Y"));
	iAAttributes params;
	addAttr(params, "Rotation mode", iAValueType::Categorical, modes);
	addAttr(params, "Steps", iAValueType::Discrete, 360, 2, 36000);
	addAttr(params, "Video quality", iAValueType::Discrete, 2, 0, 2);
	addAttr(params, "Frame rate", iAValueType::Discrete, 25, 1, 1000);
	iAParameterDlg dlg(this, "Save movie options", params,
		"Creates a movie by rotating the object around a user-defined axis in the 3D renderer.<br>"
		"The <em>rotation mode</em> defines the axis around which the object will be rotated in the exported video. "
		"The <em>steps</em> parameter defines into how many angle steps the rotation will be split, "
			"and therefore also determines the number of images in the resulting video (default: 360). "
		"The <em>video quality</em> specifies the quality of the output video "
			"(range: 0..2, 0 - worst, 2 - best; default: 2). "
		"The <em>frame rate</em> specifies the frames per second (default: 25). ");
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto pVals = dlg.parameterValues();
	QString mode = pVals["Rotation mode"].toString();
	auto imode = static_cast<int>(modes.indexOf(mode));
	auto quality = pVals["Video quality"].toInt();
	auto fps = pVals["Frame rate"].toInt();
	auto steps = pVals["Steps"].toInt();

	// Show standard save file dialog using available movie file types.
	auto fileName = QFileDialog::getSaveFileName(this, tr("Export movie %1").arg(mode),
		m_fileInfo.absolutePath() + "/" +
		((mode.isEmpty()) ? m_fileInfo.baseName() : m_fileInfo.baseName() + "_" + mode),
		movie_file_types);
	if (fileName.isEmpty())
	{
		return;
	}

	m_renderer->saveMovie(fileName, imode, quality, fps, steps);
}

void MdiChild::setPredefCamPos(int pos)
{
	m_renderer->setCamPosition(pos);
}

void MdiChild::setSlice(int mode, int s)
{
	set3DSlicePlanePos(mode, s);
}

void MdiChild::set3DSlicePlanePos(int mode, int slice)
{
	if (!firstImageData())
	{
		return;
	}
	int sliceAxis = mapSliceToGlobalAxis(mode, iAAxisIndex::Z);
	double plane[3];
	std::fill(plane, plane + 3, 0);
	auto const spacing = firstImageData()->GetSpacing();
	plane[sliceAxis] = slice * spacing[sliceAxis];
	m_renderer->setSlicePlanePos(sliceAxis, plane[0], plane[1], plane[2]);
	m_slicer[mapSliceToGlobalAxis(mode, iAAxisIndex::X)]->setOtherSlicePlanePos(mode, plane[sliceAxis]);
	m_slicer[mapSliceToGlobalAxis(mode, iAAxisIndex::Y)]->setOtherSlicePlanePos(mode, plane[sliceAxis]);
}

void MdiChild::slicerRotationChanged(int mode, double angle)
{
	m_renderer->setPlaneNormals(m_slicerTransform);
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		m_slicer[s]->setAngle(mode, angle);
	}
}

void MdiChild::linkSliceViews(bool l)
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

void MdiChild::enableSlicerInteraction(bool b)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->enableInteractor(b);
	}
}

void MdiChild::enableRendererInteraction(bool b)
{
	m_renderer->enableInteractor(b);
}

bool MdiChild::isRendererInteractionEnabled() const
{
	return m_renderer->isInteractorEnabled();
}

bool MdiChild::isSlicerInteractionEnabled() const
{
	for (auto s : m_slicer)
	{
		if (!s->isInteractorEnabled())
		{
			return false;
		}
	}
	return true;
}

void MdiChild::applyPreferences(iAPreferences const& prefs)
{
	m_preferences = prefs;
	updatePositionMarkerSize();
}

void MdiChild::updatePositionMarkerSize()
{
	const double MinSpacing = 0.00000001;
	std::array<double, 3> maxSpacing{ MinSpacing, MinSpacing, MinSpacing };
	for (auto dataSet: m_dataSets)
	{
		auto unitDist = dataSet.second->unitDistance();
		for (int c = 0; c < 3; ++c)
		{
			maxSpacing[c] = std::max(maxSpacing[c], unitDist[c]);
		}
	}
	m_renderer->setUnitSize(maxSpacing);
	// TODO NewIO: make slicer also use a size in physical units instead of voxels
	//auto maxSpc = std::max(std::max(maxSpacing[0], maxSpacing[1]), maxSpacing[2]);
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setPositionMarkerSize(m_preferences.PositionMarkerSize);
	}
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

void MdiChild::applySlicerSettings(iASlicerSettings const& ss)
{
	m_slicerSettings = ss;
	linkSliceViews(ss.LinkViews);
	linkMDIs(ss.LinkMDIs);
	emit slicerSettingsChanged();
}

iASlicerSettings const& MdiChild::slicerSettings() const
{
	return m_slicerSettings;
}

iAPreferences const& MdiChild::preferences() const
{
	return m_preferences;
}

void MdiChild::toggleSliceProfile(bool isChecked)
{
	m_isSliceProfileEnabled = isChecked;
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setSliceProfileOn(m_isSliceProfileEnabled);
	}
}

bool MdiChild::isSliceProfileEnabled() const
{
	return m_isSliceProfileEnabled;
}

void MdiChild::initProfilePoints(double const* start, double const* end)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setProfilePoint(0, start);
		m_slicer[s]->setProfilePoint(1, end);
	}
	m_renderer->initProfilePoints(start, end);
}

void MdiChild::toggleMagicLens2D(bool isEnabled)
{
	m_isMagicLensEnabled = isEnabled;
	if (isEnabled)
	{
		changeMagicLensDataSet(0);
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
		if (m_renderer->camera()->GetParallelProjection())
		{
			auto msg = "The 3D magic lens currently does not support parallel projection properly! You can disable parallel projection in renderer settings!";
			QMessageBox::warning(this, "3D magic lens", msg);
			LOG(lvlWarn, msg);
		}
		m_rendererContainer->vtkWidgetRC->magicLensOn();
	}
	else
	{
		m_rendererContainer->vtkWidgetRC->magicLensOff();
	}
}

bool MdiChild::isMagicLens2DEnabled() const
{
	return m_isMagicLensEnabled;
}

bool MdiChild::isMagicLens3DEnabled() const
{
	return m_rendererContainer->vtkWidgetRC->isMagicLensEnabled();
}

void MdiChild::updateDataSetInfo()
{   // TODO: optimize - don't fully recreate each time, just do necessary adjustments?
	m_dataSetInfo->clear();
	for (auto dataSet: m_dataSets)
	{
		if (!m_dataSetViewers[dataSet.first])    // probably not computed yet...
		{
			LOG(lvlWarn, QString("No viewer for dataset %1").arg(dataSet.second->name()));
			continue;
		}
		auto lines = m_dataSetViewers[dataSet.first]->information().split("\n", Qt::SkipEmptyParts);
		if (dataSet.second->hasMetaData(iADataSet::FileNameKey))
		{
			lines.prepend(iADataSet::FileNameKey + ": " + dataSet.second->metaData(iADataSet::FileNameKey).toString());
		}
		std::for_each(lines.begin(), lines.end(), [](QString& s) { s = "    " + s; });
		m_dataSetInfo->addItem(dataSet.second->name() +
			" (" + datasetTypeString(dataSet.second->type()) + ")\n" + lines.join("\n"));
	}
}

void MdiChild::updateROI(int const roi[6])
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->updateROI(roi);
	}
	auto img = firstImageData();
	if (!img)
	{
		return;
	}
	m_renderer->setSlicingBounds(roi, img->GetSpacing());
}

void MdiChild::setROIVisible(bool visible)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setROIVisible(visible);
	}
	m_renderer->setROIVisible(visible);
	updateViews();
}

QString MdiChild::currentFile() const
{
	return m_curFile;
}

QFileInfo const & MdiChild::fileInfo() const
{
	return m_fileInfo;
}

void MdiChild::closeEvent(QCloseEvent* event)
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
	LOG(lvlDebug, QString("Closing window %1.").arg(m_curFile));
	emit closed();
	event->accept();
}

void MdiChild::dragEnterEvent(QDragEnterEvent* e)
{
	if (e->mimeData()->hasUrls())
	{
		e->acceptProposedAction();
	}
}

void MdiChild::dropEvent(QDropEvent* e)
{
	for (const QUrl& url : e->mimeData()->urls())
	{
		m_mainWnd->loadFile(url.toLocalFile(), iAChildSource::make(false, this));
	}
}

void MdiChild::setWindowTitleAndFile(const QString& f)
{
	QString title = f;
	if (QFile::exists(f))
	{
		m_fileInfo.setFile(f);
		m_curFile = f;
		m_path = m_fileInfo.canonicalPath();
		m_isUntitled = f.isEmpty();
		m_mainWnd->addRecentFile(f);
		title = m_fileInfo.fileName();
	}
	setWindowTitle(title + "[*]");
	setWindowModified(hasUnsavedData());
}

void MdiChild::multiview()
{
	m_dwRenderer->setVisible(true);
	m_dwSlicer[iASlicerMode::XY]->setVisible(true);
	m_dwSlicer[iASlicerMode::YZ]->setVisible(true);
	m_dwSlicer[iASlicerMode::XZ]->setVisible(true);
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
	return it->get();
}

iAChannelData const* MdiChild::channelData(uint id) const
{
	auto it = m_channels.find(id);
	if (it == m_channels.end())
	{
		return nullptr;
	}
	return it->get();
}

uint MdiChild::createChannel()
{
	uint newChannelID = m_nextChannelID;
	++m_nextChannelID;
	m_channels.insert(newChannelID, std::make_shared<iAChannelData>());
	return newChannelID;
}

void MdiChild::updateSlicers()
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->update();
	}
}

void MdiChild::updateRenderer()
{
	m_renderer->update();
	m_renderer->renderWindow()->GetInteractor()->Modified();
	m_renderer->renderWindow()->GetInteractor()->Render();
	m_rendererContainer->vtkWidgetRC->update();
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

void MdiChild::toggleProfileHandles(bool isChecked)
{
	m_profileHandlesEnabled = isChecked;
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setProfileHandlesOn(m_profileHandlesEnabled);
	}
	m_renderer->setProfileHandlesOn(m_profileHandlesEnabled);
	updateViews();
}

bool MdiChild::profileHandlesEnabled() const
{
	return m_profileHandlesEnabled;
}

void MdiChild::showSlicerTitle(bool show)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicerContainer[s]->showTitle(show);
	}
}

bool MdiChild::isSlicerTitleShown() const
{
	for (int s = 0; s < 3; ++s)
	{
		if (m_slicerContainer[s]->isTitleShown()) {
			return true;
		}
	}
	return false;
}

void MdiChild::showRendererTitle(bool show)
{
	m_rendererContainer->showTitle(show);
}

bool MdiChild::isRendererTitleShown() const
{
	return m_rendererContainer->isTitleShown();
}

int MdiChild::sliceNumber(int mode) const
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_slicer[mode]->sliceNumber();
}

void MdiChild::maximizeDockWidget(QDockWidget* dw)
{
	// TODO: not ideal - user could have changed something in layout since last maximization -> confusion!
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

iASlicer* MdiChild::slicer(int mode)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_slicer[mode];
}

QWidget* MdiChild::rendererWidget()
{
	return m_rendererContainer->vtkWidgetRC;
}

QSlider* MdiChild::slicerScrollBar(int mode)
{
	return m_slicerContainer[mode]->verticalScrollBar;
}

QHBoxLayout* MdiChild::slicerContainerLayout(int mode)
{
	return m_slicerContainer[mode]->slicerContainerLayout;
}

QDockWidget* MdiChild::slicerDockWidget(int mode)
{
	assert(0 <= mode && mode < iASlicerMode::SlicerCount);
	return m_dwSlicer[mode];
}

QDockWidget* MdiChild::renderDockWidget()
{
	return m_dwRenderer;
}

QDockWidget* MdiChild::dataInfoDockWidget()
{
	return m_dwInfo;
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
/*
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
*/

vtkRenderer* MdiChild::magicLens3DRenderer() const
{
	return m_rendererContainer->vtkWidgetRC->getLensRenderer();
}

QString MdiChild::filePath() const
{
	return m_path;
}

bool MdiChild::isVolumeDataLoaded() const
{
	return firstImageDataSetIdx() != NoDataSet;
}

void MdiChild::changeMagicLensDataSet(int chg)
{
	// maybe move to slicer?
	if (!m_isMagicLensEnabled)
	{
		return;
	}
	std::vector<size_t> imageDataSets;
	for (auto dataSet : m_dataSets)
	{
		if (dataSet.second->type() == iADataSetType::Volume)
		{
			imageDataSets.push_back(dataSet.first);
		}
	}
	if (imageDataSets.empty())
	{
		setMagicLensEnabled(false);
		return;
	}
	if (chg == 0)   // initialization
	{
		m_magicLensDataSet = imageDataSets[0];
	}
	else            // we need to switch datasets
	{
		if (imageDataSets.size() <= 1)
		{   // only 1 dataset, nothing to change
			return;
		}
		auto it = std::find(imageDataSets.begin(), imageDataSets.end(), m_magicLensDataSet);
		if (it == imageDataSets.end())
		{
			m_magicLensDataSet = imageDataSets[0];
		}
		else
		{
			auto idx = it - imageDataSets.begin();
			auto newIdx = (idx + imageDataSets.size() + chg) % imageDataSets.size();
			m_magicLensDataSet = imageDataSets[newIdx];
		}
	}
	// To check: support for multiple components in a vtk image? or separating those components?
	auto imgData = dynamic_cast<iAImageData*>(m_dataSets[m_magicLensDataSet].get());
	auto viewer = dynamic_cast<iAVolumeViewer*>(m_dataSetViewers[m_magicLensDataSet].get());
	if (m_magicLensChannel == NotExistingChannel)
	{
		m_magicLensChannel = createChannel();
	}
	channelData(m_magicLensChannel)->setOpacity(0.5);
	QString name(imgData->name());
	channelData(m_magicLensChannel)->setName(name);
	updateChannel(m_magicLensChannel, imgData->vtkImage(), viewer->transfer()->colorTF(), viewer->transfer()->opacityTF(), false);
	setMagicLensInput(m_magicLensChannel);
}

void MdiChild::set3DControlVisibility(bool visible)
{
	m_rendererContainer->rendererControls->setVisible(visible);
}

std::shared_ptr<iADataSet> MdiChild::dataSet(size_t dataSetIdx) const
{
	if (m_dataSets.contains(dataSetIdx))
	{
		return m_dataSets.at(dataSetIdx);
	}
	return {};
}

size_t MdiChild::dataSetIndex(iADataSet const* dataSet) const
{
	auto it = std::find_if(m_dataSets.begin(), m_dataSets.end(), [dataSet](auto const & ds) { return ds.second.get() == dataSet; });
	if (it != m_dataSets.end())
	{
		return it->first;
	}
	return NoDataSet;
}

std::map<size_t, std::shared_ptr<iADataSet>> const& MdiChild::dataSetMap() const
{
	return m_dataSets;
}

size_t MdiChild::firstImageDataSetIdx() const
{
	for (auto dataSet : m_dataSets)
	{
		auto imgData = dynamic_cast<iAImageData*>(dataSet.second.get());
		if (imgData)
		{
			return dataSet.first;
		}
	}
	return NoDataSet;
}

vtkSmartPointer<vtkImageData> MdiChild::firstImageData() const
{
	for (auto dataSet : m_dataSets)
	{
		auto imgData = dynamic_cast<iAImageData*>(dataSet.second.get());
		if (imgData)
		{
			return imgData->vtkImage();
		}
	}
	LOG(lvlDebug, "No image/volume data loaded!");
	return nullptr;
}

iADataSetViewer* MdiChild::dataSetViewer(size_t idx) const
{
	return m_dataSetViewers.contains(idx) ? m_dataSetViewers.at(idx).get(): nullptr;
}

bool MdiChild::hasUnsavedData() const
{
	for (auto dataSet: m_dataSets)
	{
		QString fn = dataSet.second->metaData(iADataSet::FileNameKey).toString();
		if (fn.isEmpty() || !QFileInfo(fn).exists())
		{
			return true;
		}
	}
	return false;
}

void MdiChild::saveSettings(QSettings& settings)
{
	// move to iARenderer, create saveSettings method there?
	// {
	auto cam = renderer()->renderer()->GetActiveCamera();
	settings.setValue(CameraPositionKey, arrayToString(cam->GetPosition(), 3));
	settings.setValue(CameraFocalPointKey, arrayToString(cam->GetFocalPoint(), 3));
	settings.setValue(CameraViewUpKey, arrayToString(cam->GetViewUp(), 3));
	settings.setValue(CameraParallelScale, cam->GetParallelScale());
	settings.setValue(CameraParallelProjection, cam->GetParallelProjection());
	settings.setValue(LayoutKey, saveState());
	// }
}

void MdiChild::loadSettings(QSettings const& settings)
{
	// move to iARenderer, create loadSettings method there?
	// {
	double camPos[3], camFocalPt[3], camViewUp[3];
	if (!stringToArray<double>(settings.value(CameraPositionKey).toString(), camPos, 3) ||
		!stringToArray<double>(settings.value(CameraFocalPointKey).toString(), camFocalPt, 3) ||
		!stringToArray<double>(settings.value(CameraViewUpKey).toString(), camViewUp, 3))
	{
		LOG(lvlWarn, QString("Invalid or missing camera information."));
	}
	else
	{
		renderer()->renderer()->ResetCamera();
		auto cam = renderer()->renderer()->GetActiveCamera();
		bool parProj = settings.value(CameraParallelProjection, cam->GetParallelProjection()).toBool();
		double parScale = settings.value(CameraParallelScale, cam->GetParallelScale()).toDouble();
		cam->SetParallelProjection(parProj);
		if (parProj)
		{
			cam->SetParallelScale(parScale);
		}
		cam->SetPosition(camPos);
		cam->SetViewUp(camViewUp);
		cam->SetPosition(camPos);
	}

	if (settings.contains(LayoutKey))
	{
		restoreState(settings.value(LayoutKey).toByteArray());
	}
	// }
}

bool MdiChild::doSaveProject(QString const & projectFileName)
{
	QVector<size_t> unsavedDataSets;  // dataset currently unsaved
	for (auto d: m_dataSets)
	{
		if (!d.second->hasMetaData(iADataSet::SkipSaveKey) &&
		    !d.second->hasMetaData(iADataSet::FileNameKey))
		{
			unsavedDataSets.push_back(d.first);
		}
	}
	if (!unsavedDataSets.isEmpty())
	{
		bool defaultExtAvailable = true;
		for (size_t dataSetIdx : unsavedDataSets)
		{
			if (iAFileTypeRegistry::defaultExtAvailable(m_dataSets[dataSetIdx]->type()))
			{
				defaultExtAvailable = false;
				break;
			}
		}
		auto autoNameUnsaved = defaultExtAvailable ?
			QMessageBox::question(m_mainWnd, "Unsaved datasets",
			"Before saving as a project, some unsaved datasets need to be saved first. Should I choose a filename automatically (in same folder as project file)?",
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes) : QMessageBox::No;
		if (autoNameUnsaved == QMessageBox::Cancel)
		{
			return false;
		}
		QFileInfo fi(projectFileName);
		for (size_t dataSetIdx : unsavedDataSets)
		{
			bool saveSuccess = true;
			bool saved = false;
			if (autoNameUnsaved == QMessageBox::Yes)
			{
				// try auto-creating; but if file exists, let user choose!
				auto defaultExt = iAFileTypeRegistry::defaultExtension(m_dataSets[dataSetIdx]->type());
				QString fileName = fi.absoluteFilePath() + QString("dataSet%1.%2").arg(dataSetIdx).arg(defaultExt);
				if (!QFileInfo::exists(fileName))
				{
					saveSuccess = saveDataSet(m_dataSets[dataSetIdx], fileName);
					saved = true;
				}
			}
			if (!saved)
			{
				saveSuccess = saveDataSet(m_dataSets[dataSetIdx]);
			}
			if (!saveSuccess)
			{
				LOG(lvlWarn, QString("Storing dataset %1 failed. To save the project, please remove this dataset!"));
				return false;
			}
		}
	}
	auto s = std::make_shared<QSettings>(projectFileName, QSettings::IniFormat);
	s->clear();   // clear out existing entries (Qt initializes from a potentially existing file) to avoid stale information
	s->setValue("UseMdiChild", true);
	saveSettings(*s.get());
	iAProjectFileIO io;
	auto dataSets = std::make_shared<iADataCollection>(m_dataSets.size(), s);
	for (auto v : m_dataSetViewers)
	{
		if (m_dataSets[v.first]->hasMetaData(iADataSet::SkipSaveKey))
		{
			continue;
		}
		v.second->storeState();
	}
	for (auto d : m_dataSets)
	{
		if (d.second->hasMetaData(iADataSet::SkipSaveKey))
		{
			continue;
		}
		dataSets->addDataSet(d.second);
	}
	io.save(projectFileName, dataSets, QVariantMap());
	setWindowTitleAndFile(projectFileName);

	// store settings of current tools:
	for (auto toolKey : m_tools.keys())
	{
		s->beginGroup(toolKey);
		m_tools[toolKey]->saveState(*s.get(), projectFileName);
		s->endGroup();
	}
	return true;
}

void MdiChild::addTool(QString const& key, std::shared_ptr<iATool> tool)
{
	m_tools.insert(key, tool);
}

void MdiChild::removeTool(QString const& key)
{
	m_tools.remove(key);
	emit toolRemoved(key);
}

QMap<QString, std::shared_ptr<iATool>> const& MdiChild::tools()
{
	return m_tools;
}

MdiChild::iAInteractionMode MdiChild::interactionMode() const
{
	return m_interactionMode;
}

void MdiChild::setInteractionMode(iAInteractionMode mode)
{
	m_interactionMode = mode;
	m_mainWnd->updateInteractionModeControls(mode);
	for (auto v : m_dataSetViewers)
	{
		v.second->setPickActionVisible(mode == imRegistration);
	}
	try
	{
		if (m_interactionMode == imRegistration)
		{
			size_t dataSetIdx = NoDataSet;
			for (auto dataSet: m_dataSets)
			{
				if (dynamic_cast<iAImageData*>(dataSet.second.get()) &&
					m_dataSetViewers.contains(dataSet.first) &&
					m_dataSetViewers[dataSet.first]->renderer()->isPickable())
				{
					dataSetIdx = dataSet.first;
				}
			}
			if (dataSetIdx == NoDataSet)
			{
				LOG(lvlError, QString("No valid dataset loaded for moving (%1).").arg(dataSetIdx));
			}
			else
			{
				auto editDataSet = m_dataSets[dataSetIdx];
				setDataSetMovable(dataSetIdx);
			}
			renderer()->interactor()->SetInteractorStyle(m_manualMoveStyle[iASlicerMode::SlicerCount]);
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
			{
				slicer(i)->interactor()->SetInteractorStyle(m_manualMoveStyle[i]);
			}
		}
		else
		{
			renderer()->setDefaultInteractor();
			for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
			{
				slicer(i)->setDefaultInteractor();
			}
		}
	}
	catch (std::invalid_argument& ivae)
	{
		LOG(lvlError, ivae.what());
	}
}

void MdiChild::setDataSetMovable(size_t dataSetIdx)
{
	for (auto ds: m_dataSetViewers)
	{
		bool pickable = (ds.first == dataSetIdx);
		ds.second->setPickable(pickable);
	}

	// below required for synchronized slicers
	auto imgData = dynamic_cast<iAImageData*>(m_dataSets[dataSetIdx].get());
	if (!imgData)
	{
		LOG(lvlError, "Selected dataset is not an image.");
		return;
	}
	assert(m_dataSetViewers.contains(dataSetIdx));
	auto img = imgData->vtkImage();
	uint chID = m_dataSetViewers[dataSetIdx]->slicerChannelID();
	iAChannelSlicerData* props[3];
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		if (!slicer(i)->hasChannel(chID))
		{
			LOG(lvlWarn, "This modality cannot be moved as it isn't active in slicer, please select another one!");
			return;
		}
		else
		{
			props[i] = slicer(i)->channel(chID);
		}
	}
	for (int i = 0; i <= iASlicerMode::SlicerCount; ++i)
	{
		m_manualMoveStyle[i]->initialize(img, m_dataSetViewers[dataSetIdx]->renderer(), props, i);
	}
}
