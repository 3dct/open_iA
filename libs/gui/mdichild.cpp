// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mdichild.h"

#include "dlg_slicer.h"
#include "iADataSetViewer.h"
#include "iADataSetRenderer.h"
#include "iAVolumeViewer.h"    // TODO NEWIO: only required for changing magic lens input - move from here, e.g. to slicer
#include "iAFileParamDlg.h"
#include "iAFileUtils.h"    // for safeFileName
#include "iAParametricSpline.h"
#include "iAvtkInteractStyleActor.h"
#include "mainwindow.h"

// renderer
#include <iARendererImpl.h>
#include <iARenderObserver.h>

// slicer
#include <iASlicerImpl.h>

// guibase
#include <iAChannelData.h>
#include <iAChannelSlicerData.h>
#include <iADataSetListWidget.h>
#include <iAJobListView.h>
#include <iAMovieHelper.h>
#include <iAParameterDlg.h>
#include <iAPreferences.h>
#include <iATool.h>
#include <iAToolRegistry.h>
#include <iARunAsync.h>

// qthelper
#include <iADockWidgetWrapper.h>

// io
#include <iAFileStackParams.h>
#include <iAImageStackFileIO.h>
#include <iAProjectFileIO.h>
#include <iAVolStackFileIO.h>

// base
#include <iADataSet.h>
#include <iAFileTypeRegistry.h>
#include <iALog.h>
#include <iAProgress.h>
#include <iAStringHelper.h>
#include <iAToolsVTK.h>
#include <iATransferFunction.h>

#include <vtkColorTransferFunction.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageExtractComponents.h>
#include <vtkImageReslice.h>
#include <vtkMath.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkPolyData.h>
#include <vtkProperty.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkWindowToImageFilter.h>

// TODO: refactor methods using the following out of mdichild!
#include <vtkCamera.h>
#include <vtkTransform.h>

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
#include <QtGlobal> // for QT_VERSION


MdiChild::MdiChild(MainWindow* mainWnd, iAPreferences const& prefs, bool unsavedChanges) :
	m_mainWnd(mainWnd),
	m_preferences(prefs),
	m_isSmthMaximized(false),
	m_isUntitled(true),
	m_isSliceProfileEnabled(false),
	m_profileHandlesEnabled(false),
	m_isMagicLensEnabled(false),
	m_snakeSlicer(false),
	m_worldSnakePoints(vtkSmartPointer<vtkPoints>::New()),
	m_parametricSpline(vtkSmartPointer<iAParametricSpline>::New()),
	m_axesTransform(vtkTransform::New()),
	m_slicerTransform(vtkSmartPointer<vtkTransform>::New()),
	m_dataSetInfo(new QListWidget(this)),
	m_dataSetListWidget(new iADataSetListWidget()),
	m_dwInfo(new iADockWidgetWrapper(m_dataSetInfo, "Dataset Info", "DataInfo")),
	m_dwDataSets(new iADockWidgetWrapper(m_dataSetListWidget, "Datasets", "DataSets")),
	m_nextChannelID(0),
	m_magicLensChannel(NotExistingChannel),
	m_magicLensDataSet(0),
	m_interactionMode(imCamera),
	m_nextDataSetID(0)
{
	setAcceptDrops(true);
	setAttribute(Qt::WA_DeleteOnClose);
	setDockOptions(dockOptions() | QMainWindow::GroupedDragging);
	setWindowModified(unsavedChanges);
	setupUi(this);
	setCentralWidget(nullptr);
	setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
	m_dwRenderer = new dlg_renderer(this);
	addDockWidget(Qt::LeftDockWidgetArea, m_dwRenderer);
	m_initialLayoutState = saveState();
	for (int i = 0; i < 3; ++i)
	{
		m_slicer[i] = new iASlicerImpl(this, static_cast<iASlicerMode>(i), true, true, m_slicerTransform, m_worldSnakePoints);
		m_dwSlicer[i] = new dlg_slicer(m_slicer[i]);
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

	m_parametricSpline->SetPoints(m_worldSnakePoints);

	m_renderer = new iARendererImpl(this, dynamic_cast<vtkGenericOpenGLRenderWindow*>(m_dwRenderer->vtkWidgetRC->renderWindow()));
	m_renderer->setAxesTransform(m_axesTransform);

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

MdiChild::~MdiChild()
{
	m_axesTransform->Delete();
}

void MdiChild::connectSignalsToSlots()
{
	connect(m_mainWnd, &MainWindow::fullScreenToggled, this, &MdiChild::toggleFullScreen);

	connect(m_dataSetListWidget, &iADataSetListWidget::dataSetSelected, this, &iAMdiChild::dataSetSelected);

	connect(m_renderer, &iARendererImpl::bgColorChanged, m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::setLensBackground);
	connect(m_renderer, &iARendererImpl::interactionModeChanged, this, [this](bool camera)
	{
		setInteractionMode(camera ? imCamera : imRegistration);
	});
	auto adaptSlicerBorders = [this]()
	{
		for (int i = 0; i < 3; ++i)
		{
			m_dwSlicer[i]->showBorder(m_renderer->isShowSlicePlanes());
		}
	};
	connect(m_renderer, &iARendererImpl::settingsChanged, this, adaptSlicerBorders);
	adaptSlicerBorders();
	m_dwRenderer->vtkWidgetRC->setContextMenuEnabled(true);
	// TODO: merge iAFast3DMagicLensWidget (at least iAQVTKWidget part) with iARenderer, then this can be moved there:
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::editSettings, m_renderer, &iARendererImpl::editSettings);
	connect(m_dwRenderer->tbMax, &QPushButton::clicked, this, &MdiChild::maximizeRenderer);
	connect(m_dwRenderer->pbToggleInteraction, &QPushButton::clicked, this, [this]
	{
		enableRendererInteraction(!isRendererInteractionEnabled());
	});
	connect(m_dwRenderer->pushPX,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PX); });
	connect(m_dwRenderer->pushPY,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PY); });
	connect(m_dwRenderer->pushPZ,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::PZ); });
	connect(m_dwRenderer->pushMX,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MX); });
	connect(m_dwRenderer->pushMY,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MY); });
	connect(m_dwRenderer->pushMZ,  &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::MZ); });
	connect(m_dwRenderer->pushIso, &QPushButton::clicked, this, [this] { m_renderer->setCamPosition(iACameraPosition::Iso); });
	connect(m_dwRenderer->pbSaveScreen, &QPushButton::clicked, this, &MdiChild::saveRC);
	connect(m_dwRenderer->pbSaveMovie, &QPushButton::clicked, this, &MdiChild::saveMovRC);
	// { TODO: strange way to forward signals, find out why we need to do this and find better way:
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::rightButtonReleasedSignal, m_renderer, &iARendererImpl::mouseRightButtonReleasedSlot);
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::leftButtonReleasedSignal, m_renderer, &iARendererImpl::mouseLeftButtonReleasedSlot);
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::touchStart, m_renderer, &iARendererImpl::touchStart);
	connect(m_dwRenderer->vtkWidgetRC, &iAFast3DMagicLensWidget::touchScale, m_renderer, &iARendererImpl::touchScaleSlot);
	//! }

	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		connect(m_dwSlicer[s]->tbMax, &QPushButton::clicked, [this, s] { maximizeSlicer(s); });
		connect(m_dwSlicer[s], &QDockWidget::visibilityChanged, [this, s] { m_renderer->showSlicePlane(s, m_dwSlicer[s]->isVisible()); });
		m_manualMoveStyle[s] = vtkSmartPointer<iAvtkInteractStyleActor>::New();
		connect(m_manualMoveStyle[s].Get(), &iAvtkInteractStyleActor::actorsUpdated, this, &iAMdiChild::updateViews);
		connect(m_slicer[s], &iASlicer::shiftMouseWheel, this, &MdiChild::changeMagicLensDataSet);
		connect(m_slicer[s], &iASlicerImpl::sliceRotated, this, &MdiChild::slicerRotationChanged);
		connect(m_slicer[s], &iASlicer::sliceNumberChanged, this, &MdiChild::setSlice);
		connect(m_slicer[s], &iASlicer::mouseMoved, this, &MdiChild::updatePositionMarker);
		connect(m_slicer[s], &iASlicerImpl::regionSelected, this, [this](int minVal, int maxVal, uint channelID)
		{
			// TODO NEWIO: move to better place, e.g. dataset viewer for image data? slicer?
			if (minVal == maxVal)
			{
				return;
			}
			for (auto viewer: m_dataSetViewers)
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
				connect(m_slicer[s], &iASlicerImpl::addedPoint, m_slicer[j], &iASlicerImpl::addPoint);
				connect(m_slicer[s], &iASlicerImpl::movedPoint, m_slicer[j], &iASlicerImpl::movePoint);
				connect(m_slicer[s], &iASlicerImpl::profilePointChanged, m_slicer[j], &iASlicerImpl::setProfilePoint);
				connect(m_slicer[s], &iASlicerImpl::switchedMode, m_slicer[j], &iASlicerImpl::switchInteractionMode);
				connect(m_slicer[s], &iASlicerImpl::deletedSnakeLine, m_slicer[j], &iASlicerImpl::deleteSnakeLine);
				connect(m_slicer[s], &iASlicerImpl::deselectedPoint, m_slicer[j], &iASlicerImpl::deselectPoint);
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
		{   // TODO: set "slice" based on world coordinates directly instead of spacing?
			double spacing = m_slicer[i]->channel(0)->input()->GetSpacing()[i];
			m_dwSlicer[i]->sbSlice->setValue(pos[mapSliceToGlobalAxis(i, iAAxisIndex::Z)] / spacing);
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
		LOG(lvlDebug, "Developer Warning - consider calling setWindowTitleAndFile directly where you first call addDataSet");
		setWindowTitleAndFile(
			dataSet->hasMetaData(iADataSet::FileNameKey) ?
			dataSet->metaData(iADataSet::FileNameKey).toString() :
			dataSet->name());
	}
	auto p = std::make_shared<iAProgress>();
	auto viewer = createDataSetViewer(dataSet.get());
	if (!viewer)
	{
		LOG(lvlError, "No viewer associated with this dataset type!");
		return dataSetIdx;
	}
	connect(viewer.get(), &iADataSetViewer::dataSetChanged, this, [this, dataSetIdx](size_t dsIdx) {
		assert(dsIdx == dataSetIdx);
		updateDataSetInfo();
		emit dataSetChanged(dsIdx);
	});
	connect(viewer.get(), &iADataSetViewer::removeDataSet, this, &iAMdiChild::removeDataSet);
	m_dataSetViewers[dataSetIdx] = viewer;
	auto fw = runAsync(
		[this, viewer, dataSetIdx, p]()
		{
			viewer->prepare(p.get());
			emit dataSetPrepared(dataSetIdx);
		},
		[this, viewer, dataSetIdx]
		{
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
	if (m_dataSetViewers.find(dataSetIdx) == m_dataSetViewers.end())
	{
		LOG(lvlDebug, QString("Trying to remove dataset idx=%1 which does not exist anymore!").arg(dataSetIdx));
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
		auto msg = QString("The chosen file format (%1) does not support this kind of dataset!").arg(io->name());
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
	vtkSmartPointer<vtkWindowToImageFilter> filter = vtkSmartPointer<vtkWindowToImageFilter>::New();
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
	iAParameterDlg dlg(this, "Save movie options", params,
		"Creates a movie by rotating the object around a user-defined axis in the 3D renderer.");
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	QString mode = dlg.parameterValues()["Rotation mode"].toString();
	int imode = modes.indexOf(mode);

	// Show standard save file dialog using available movie file types.
	m_renderer->saveMovie(QFileDialog::getSaveFileName(this, tr("Export movie %1").arg(mode),
							m_fileInfo.absolutePath() + "/" +
								((mode.isEmpty()) ? m_fileInfo.baseName() : m_fileInfo.baseName() + "_" + mode),
							movie_file_types), imode);
}

void MdiChild::setPredefCamPos(int pos)
{
	m_renderer->setCamPosition(pos);
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
		if (m_dwSlicer[mode]->sbSlice->value() != s)
		{
			QSignalBlocker block(m_dwSlicer[mode]->sbSlice);
			m_dwSlicer[mode]->sbSlice->setValue(s);
		}
		if (m_dwSlicer[mode]->verticalScrollBar->value() != s)
		{
			QSignalBlocker block(m_dwSlicer[mode]->verticalScrollBar);
			m_dwSlicer[mode]->verticalScrollBar->setValue(s);
		}
		set3DSlicePlanePos(mode, s);
	}
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
	// + 0.5 to place slice plane in the middle of the sliced voxel:
	plane[sliceAxis] = (slice + 0.5) * spacing[sliceAxis];
	m_renderer->setSlicePlanePos(sliceAxis, plane[0], plane[1], plane[2]);
}

void MdiChild::updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s)
{
	if (!firstImageData())
	{
		return;
	}
	double spacing[3];
	firstImageData()->GetSpacing(spacing);

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
		double alpha = std::acos(std::pow(normal[0], 2) / (std::sqrt(std::pow(normal[0], 2)) * (std::sqrt(std::pow(normal[0], 2) + std::pow(normal[1], 2)))));
		double cos_theta_xz = std::cos(alpha);
		double sin_theta_xz = std::sin(alpha);

		double rxz_matrix[16] = { cos_theta_xz,	-sin_theta_xz,	0,	 0,
			sin_theta_xz,	cos_theta_xz,	0,	 0,
			0,			0,		1,	 0,
			0,			0,		0,	 1 };

		vtkMatrix4x4* rotate_around_xz = vtkMatrix4x4::New();
		rotate_around_xz->DeepCopy(rxz_matrix);

		//rotate around Y to bring vector parallel to Z axis
		double beta = std::acos(std::pow(normal[2], 2) / std::sqrt(std::pow(normal[2], 2)) + std::sqrt(std::pow(cos_theta_xz, 2) + std::pow(normal[2], 2)));
		double cos_theta_y = std::cos(beta);
		double sin_theta_y = std::sin(beta);

		double ry_matrix[16] = { cos_theta_y,	0,	sin_theta_y,	0,
			0,			1,		0,			0,
			-sin_theta_y,	0,	cos_theta_y,	0,
			0,			0,		0,			1 };

		vtkMatrix4x4* rotate_around_y = vtkMatrix4x4::New();
		rotate_around_y->DeepCopy(ry_matrix);

		//rotate around Z by 180 degree - to bring object correct view
		double cos_theta_z = std::cos(vtkMath::Pi());
		double sin_theta_z = std::sin(vtkMath::Pi());

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

void MdiChild::slicerRotationChanged(int mode, double angle)
{
	m_renderer->setPlaneNormals(m_slicerTransform);
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		m_slicer[s]->setAngle(mode, angle);
	}
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
			maxSpacing[c] = std::max(maxSpacing[c], unitDist[c] * m_preferences.PositionMarkerSize);
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

void MdiChild::toggleSnakeSlicer(bool isChecked)
{
	if (!firstImageData())
	{
		return;
	}
	m_snakeSlicer = isChecked;

	if (m_snakeSlicer)
	{
		for (auto v : m_dataSetViewers)
		{
			v.second->renderer()->removeCuttingPlanes();
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
			m_slicer[s]->switchInteractionMode(iASlicerImpl::SnakeShow);
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
			m_dwSlicer[s]->sbSlice->setValue(firstImageData()->GetDimensions()[mapSliceToGlobalAxis(s, iAAxisIndex::Z)] >> 1);
			m_slicer[s]->channel(0)->reslicer()->SetResliceTransform(m_savedSlicerTransform[s]);
			m_slicer[s]->channel(0)->reslicer()->SetOutputExtentToDefault();
			m_slicer[s]->resetCamera();
			m_slicer[s]->renderer()->Render();
			m_slicer[s]->switchInteractionMode(iASlicerImpl::Normal);
		}
		for (auto v : m_dataSetViewers)
		{
			if (v.second->renderFlagSet(iADataSetViewer::RenderCutPlane))
			{
				v.second->renderer()->setCuttingPlanes(m_renderer->slicePlanes());
			}
		}
	}
}

void MdiChild::snakeNormal(int index, double point[3], double normal[3])
{
	if (!firstImageData())
	{
		return;
	}
	int i1 = index;
	int i2 = index + 1;

	double spacing[3];
	firstImageData()->GetSpacing(spacing);

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

bool MdiChild::isSliceProfileEnabled() const
{
	return m_isSliceProfileEnabled;
}

void MdiChild::setProfilePoints(double const* start, double const* end)
{
	for (int s = 0; s < 3; ++s)
	{
		m_slicer[s]->setProfilePoint(0, start);
		m_slicer[s]->setProfilePoint(1, end);
	}
	m_renderer->setProfilePoint(0, start);
	m_renderer->setProfilePoint(1, end);
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
		m_dataSetInfo->addItem(dataSet.second->name() + "\n" + lines.join("\n"));
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
		m_mainWnd->loadFile(url.toLocalFile(), this);
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
	m_channels.insert(newChannelID, QSharedPointer<iAChannelData>::create());
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
	m_dwRenderer->vtkWidgetRC->update();
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
	return m_dwRenderer->vtkWidgetRC;
}

QSlider* MdiChild::slicerScrollBar(int mode)
{
	return m_dwSlicer[mode]->verticalScrollBar;
}

QHBoxLayout* MdiChild::slicerContainerLayout(int mode)
{
	return m_dwSlicer[mode]->horizontalLayout_2;
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
	return m_dwRenderer->vtkWidgetRC->getLensRenderer();
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
	m_dwRenderer->widget3D->setVisible(visible);
}

std::shared_ptr<iADataSet> MdiChild::dataSet(size_t dataSetIdx) const
{
	if (m_dataSets.find(dataSetIdx) != m_dataSets.end())
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
	LOG(lvlError, "No image/volume data loaded!");
	return nullptr;
}

iADataSetViewer* MdiChild::dataSetViewer(size_t idx) const
{
	return m_dataSetViewers.at(idx).get();
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
	QVector<size_t> unsavedDataSets;
	for (auto d: m_dataSets)
	{
		if (!d.second->hasMetaData(iADataSet::FileNameKey))
		{
			unsavedDataSets.push_back(d.first);
		}
	}
	if (!unsavedDataSets.isEmpty())
	{
		auto result = QMessageBox::question(m_mainWnd, "Unsaved datasets",
			"Before saving as a project, some unsaved datasets need to be saved first. Should I choose a filename automatically (in same folder as project file)?",
			QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Yes);
		if (result == QMessageBox::Cancel)
		{
			return false;
		}
		QFileInfo fi(projectFileName);
		for (size_t dataSetIdx : unsavedDataSets)
		{
			bool saveSuccess = true;
			bool saved = false;
			if (result == QMessageBox::Yes)
			{
				// try auto-creating; but if file exists, let user choose!
				QString fileName = fi.absoluteFilePath() + QString("dataSet%1.%2").arg(dataSetIdx).arg(iAFileTypeRegistry::defaultExtension(m_dataSets[dataSetIdx]->type()));
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
				return false;
			}
		}
	}
	auto s = std::make_shared<QSettings>(projectFileName, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(5, 99, 0)
	s->setIniCodec("UTF-8");
#endif
	s->setValue("UseMdiChild", true);
	saveSettings(*s.get());
	iAProjectFileIO io;
	auto dataSets = std::make_shared<iADataCollection>(m_dataSets.size(), s);
	for (auto v : m_dataSetViewers)
	{
		v.second->storeState();
	}
	for (auto d : m_dataSets)
	{
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
					m_dataSetViewers.find(dataSet.first) != m_dataSetViewers.end() &&
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
	assert(m_dataSetViewers.find(dataSetIdx) != m_dataSetViewers.end());
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
