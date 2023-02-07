// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mainwindow.h"

#include "defines.h"
#include "iAProgress.h"

#include "dlg_datatypeconversion.h"
#include "iACheckOpenGL.h"
#include "iAFileParamDlg.h"
#include "iAJobListView.h"
#include "iALogWidget.h"
#include "iAModuleDispatcher.h"
#include "iAParameterDlg.h"
#include "iATool.h"
#include "iAToolRegistry.h"
#include "iAQMenuHelper.h"
#include "iARawFileParamDlg.h"
#include "iARenderer.h"
#include "iASavableProject.h"
#include "iASlicerImpl.h"      // for slicerModeToString
#include "iAStringHelper.h"    // for iAConverter
#include "iATLGICTLoader.h"
#include "mdichild.h"
#include "ui_Mainwindow.h"

// io:
#include <iADataSet.h>
#include <iAFileStackParams.h>
#include <iAFileTypeRegistry.h>
#include <iARawFileIO.h>

// charts:
#include <iAChartWidget.h>

// qthelper
#include "iADockWidgetWrapper.h"

// base
#include "iALog.h"
#include "iALogLevelMappings.h"
#include "iALUT.h"
#include "iAMathUtility.h"
#include "iASettings.h"    // for loadSettings, storeSettings, initializeSettingTypes
#include "iAToolsVTK.h"
#include "iAXmlSettings.h"

#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkPolyData.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>

#include <QActionGroup>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QMimeData>
#include <QMdiSubWindow>
#include <QScreen>
#include <QSettings>
#include <QSpacerItem>
#include <QSplashScreen>
#include <QTableWidget>
#include <QtConcurrent>
#include <QTextStream>
#include <QTimer>
#include <QtXml/QDomDocument>
#include <QDesktopServices>

const int MainWindow::MaxRecentFiles;

template <typename T>
QList<T*> MainWindow::childList(QMdiArea::WindowOrder order)
{
	QList<T*> res;
	for (QMdiSubWindow* window : m_ui->mdiArea->subWindowList(order))
	{
		T* child = dynamic_cast<T*>(window->widget());
		if (child)
		{
			res.append(child);
		}
	}
	return res;
}

template <typename T>
T* MainWindow::activeChild()
{
	int subWndCnt = childList<T>().size();
	if (subWndCnt > 0)
	{
		return childList<T>(QMdiArea::ActivationHistoryOrder).last();
	}
	return nullptr;
}

MainWindow::MainWindow(QString const & appName, QString const & version, QString const & buildInformation, QString const & splashImage, iADockWidgetWrapper* dwJobs) :
	m_splashScreenImg(splashImage),
	m_moduleDispatcher( new iAModuleDispatcher( this ) ),
	m_gitVersion(version),
	m_buildInformation(buildInformation),
	m_ui(new Ui_MainWindow()),
	m_dwJobs(dwJobs),
	m_openJobListOnNewJob(false)
{
	assert(!m_mainWnd);
	m_mainWnd = this;

	m_ui->setupUi(this);
	setAcceptDrops(true);

	m_mdiViewModeGroup = new QActionGroup(this);
	m_mdiViewModeGroup->addAction(m_ui->actionTabbed);
	m_mdiViewModeGroup->addAction(m_ui->actionSubWindows);
	m_mdiViewModeGroup->setExclusive(true);

	// restore geometry and state
	QCoreApplication::setOrganizationName("FHW");
	QCoreApplication::setOrganizationDomain("3dct.at");
	QCoreApplication::setApplicationName(appName);
	setWindowTitle(appName + " " + m_gitVersion);

	m_splashScreen = new QSplashScreen(m_splashScreenImg);
	m_splashScreen->setWindowFlags(m_splashScreen->windowFlags() | Qt::WindowStaysOnTopHint);
	m_splashScreen->show();
	m_splashScreen->showMessage("\n      Reading settings...", Qt::AlignTop, QColor(255, 255, 255));

	readSettings();

	m_splashTimer = new QTimer();
	m_splashTimer->setSingleShot(true);
	connect(m_splashTimer, &QTimer::timeout, this, &MainWindow::hideSplashSlot);
	m_splashTimer->start(2000);

	m_splashScreen->showMessage("\n      Setup UI...", Qt::AlignTop, QColor(255, 255, 255));
	applyQSS();
	m_ui->actionLinkViews->setChecked(m_defaultSlicerSettings.LinkViews);//removed from readSettings, if is needed at all?
	m_ui->actionLinkMdis->setChecked(m_defaultSlicerSettings.LinkMDIs);
	setCentralWidget(m_ui->mdiArea);

	createRecentFileActions();
	connectSignalsToSlots();
	m_slicerToolsGroup = new QActionGroup(this);
	m_slicerToolsGroup->setExclusive(false);
	m_slicerToolsGroup->addAction(m_ui->actionSnakeSlicer);
	m_slicerToolsGroup->addAction(m_ui->actionRawProfile);
	m_slicerToolsGroup->addAction(m_ui->actionEditProfilePoints);

	m_ui->menuWindow->insertAction(m_ui->actionOpenLogOnNewMessage, iALogWidget::get()->toggleViewAction());
	m_ui->menuWindow->insertAction(m_ui->actionOpenListOnAddedJob, m_dwJobs->toggleViewAction());

	m_splashScreen->showMessage(tr("\n      Version: %1").arg (m_gitVersion), Qt::AlignTop, QColor(255, 255, 255));

	m_layout = new QComboBox(this);
	for (int i=0; i< m_layoutNames.size(); ++i)
	{
		m_layout->addItem(m_layoutNames[i]);
		if (m_layoutNames[i] == m_defaultLayout)
		{
			m_layout->setCurrentIndex(i);
		}
	}
	m_layout->setStyleSheet("padding: 0");
	m_layout->resize(m_layout->geometry().width(), 100);
	m_layout->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_ui->layoutToolbar->insertWidget(m_ui->actionSaveLayout, m_layout);

	m_moduleDispatcher->InitializeModules(iALogWidget::get());
	updateMenus();
}

MainWindow::~MainWindow()
{
	// save geometry and state
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	settings.setValue("state", saveState());

	m_moduleDispatcher->SaveModulesSettings();
}

void MainWindow::hideSplashSlot()
{
	m_splashScreen->finish(this);
	delete m_splashTimer;
}

void MainWindow::quitTimerSlot()
{
	if (iAJobListView::get()->isAnyJobRunning())
	{
		constexpr int RecheckTimeMS = 1000;
		m_quitTimer->start(RecheckTimeMS);
		return;
	}
	delete m_quitTimer;
	QApplication::closeAllWindows();
}

bool MainWindow::keepOpen()
{
	bool childHasChanges = false;
	for (iAMdiChild* mdiChild : mdiChildList())
	{
		childHasChanges |= mdiChild->isWindowModified();
	}
	if (childHasChanges)
	{
		auto reply = QMessageBox::question(this, "Unsaved changes",
			"One or more windows have unsaved changes. Are you sure you want to close?",
			QMessageBox::Yes | QMessageBox::No);
		if (reply != QMessageBox::Yes)
		{
			return true;
		}
		else
		{ // avoid individual questions for each window
			for (iAMdiChild* mdiChild : mdiChildList())
			{
				mdiChild->setWindowModified(false);
			}
		}
	}
	return false;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	if (keepOpen())
	{
		event->ignore();
		return;
	}
	m_ui->mdiArea->closeAllSubWindows();
	if (activeMdiChild())
	{
		event->ignore();
		return;
	}
	writeSettings();
	iALogWidget::shutdown();
	event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
	if (e->mimeData()->hasUrls())
	{
		e->acceptProposedAction();
	}
}

void MainWindow::dropEvent(QDropEvent *e)
{
	for(const QUrl &url: e->mimeData()->urls())
	{
		loadFileAskNewWindow(url.toLocalFile());
	}
}

void MainWindow::closeAllSubWindows()
{
	if (!keepOpen())
	{
		m_ui->mdiArea->closeAllSubWindows();
	}
}

void MainWindow::openRaw()
{
	QString fileName = QFileDialog::getOpenFileName(
		this,
		tr("Open Raw File"),
		m_path,
		"Raw File (*)"
	);
	// TODO NEWIO: ask for whether to load in new window here?
	loadFileNew(fileName, nullptr, std::make_shared<iARawFileIO>());
}

void MainWindow::openRecentFile()
{
	auto action = qobject_cast<QAction*>(sender());
	if (!action)
	{
		return;
	}
	QString fileName = action->data().toString();
	loadFileAskNewWindow(fileName);
}

void MainWindow::loadFileAskNewWindow(QString const & fileName)
{
	auto child = activeMdiChild();
	if (child)
	{
		auto result = QMessageBox::question(
			this, "", "Load data into the active window?", QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if (result == QMessageBox::Cancel)
		{
			return;
		}
		if (result == QMessageBox::No)
		{
			child = nullptr;
		}
	}
	loadFileNew(fileName, child);
}

void MainWindow::loadFileNew(QString const& fileName, iAMdiChild* child, std::shared_ptr<iAFileIO> io)
{
	if (fileName.isEmpty())
	{
		return;
	}
	if (!io)
	{
		io = iAFileTypeRegistry::createIO(fileName, iAFileIO::Load);
	}
	if (!io)
	{   // did not find an appropriate file IO; createIO already outputs a warning in that case; maybe a QMessageBox?
		return;
	}
	QVariantMap paramValues;
	if (!iAFileParamDlg::getParameters(this, io.get(), iAFileIO::Load, fileName, paramValues))
	{
		return;
	}
	auto p = std::make_shared<iAProgress>();
	using FutureWatcherType = QFutureWatcher<std::shared_ptr<iADataSet>>;
	auto futureWatcher = new FutureWatcherType(this);
	QObject::connect(futureWatcher, &FutureWatcherType::finished, this,
		[this, child, fileName]()
		{
			auto watcher = dynamic_cast<FutureWatcherType*>(sender());
			auto dataSet = watcher->result();
			if (!dataSet)
			{
				LOG(lvlError, QString("No data loaded!"));
				return;
			}
			iAMdiChild* targetChild = child;
			if (!targetChild)
			{
				targetChild = createMdiChild(false);
				dynamic_cast<MdiChild*>(targetChild)->setWindowTitleAndFile(fileName);
			}
			addRecentFile(fileName);
			targetChild->addDataSet(dataSet);
		});
	QObject::connect(futureWatcher, &FutureWatcherType::finished, futureWatcher, &FutureWatcherType::deleteLater);
	auto future = QtConcurrent::run( [p, fileName, io, paramValues]() { return io->load(fileName, paramValues, *p.get()); });
	futureWatcher->setFuture(future);
	iAJobListView::get()->addJob(QString("Loading file '%1'").arg(fileName), p.get(), futureWatcher);
}

void MainWindow::loadFiles(QStringList fileNames)
{
	for (int i = 0; i < fileNames.length(); i++)
	{
		loadFileNew(fileNames[i], activeMdiChild());
	}
}

// TODO NEWIO: separate program settings from current view state
//     - view state -> project file
//     - settings -> keep here
void MainWindow::saveSettings()
{
	if (!activeMdiChild())
	{
		return;
	}
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAAttributes params;
	addAttr(params, "Camera", iAValueType::Boolean, m_spCamera);
	addAttr(params, "Slice Views", iAValueType::Boolean, m_spSliceViews);
	addAttr(params, "Preferences", iAValueType::Boolean, m_spPreferences);
	addAttr(params, "Render Settings", iAValueType::Boolean, m_spRenderSettings);
	addAttr(params, "Slice Settings", iAValueType::Boolean, m_spSlicerSettings);
	iAParameterDlg dlg(this, "Save Settings", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	m_spCamera = values["Camera"].toBool();
	m_spSliceViews = values["Slice Views"].toBool();
	m_spPreferences = values["Preferences"].toBool();
	m_spRenderSettings = values["Render Settings"].toBool();
	m_spSlicerSettings = values["Slice Settings"].toBool();

	iAXmlSettings xml;
	if (m_spCamera)
	{
		saveCamera(xml);
	}
	if (m_spSliceViews)
	{
		saveSliceViews(xml);
	}
	if (m_spPreferences)
	{
		savePreferences(xml);
	}
	if (m_spRenderSettings)
	{
		saveRenderSettings(xml);
	}
	if (m_spSlicerSettings)
	{
		saveSlicerSettings(xml);
	}
	xml.save(fileName);
}

void MainWindow::loadSettings()
{
	if (!activeMdiChild())
	{
		return;
	}
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath, tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	if (!xml.read(fileName))
	{
		QMessageBox::warning(this, "Loading settings", "An error occurred during xml parsing!");
		return;
	}
	bool camera = false, sliceViews = false, preferences = false, renderSettings = false, slicerSettings = false;

	QDomElement root = xml.documentElement();
	QDomNodeList list = root.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		QString nodeName = node.nodeName();
		if (nodeName == "camera") camera = true;
		else if (nodeName == "sliceViews") sliceViews = true;
		else if (nodeName == "functions")
		{
			LOG(lvlWarn, "This file contains (transfer or probability) functions. "
				"Note that saving and loading such functions through the settings has been discontinued, "
				"since we can load multiple datasets now and wouldn't know for which dataset these functions should apply. "
				"You can still load the functions in the histogram of a specific volume dataset!");
		}
		else if (nodeName == "preferences") preferences = true;
		else if (nodeName == "renderSettings") renderSettings = true;
		else if (nodeName == "slicerSettings") slicerSettings = true;
	}
	iAAttributes params;
	if (camera)               { addAttr(params, "Camera", iAValueType::Boolean, m_lpCamera ); }
	if (sliceViews)           { addAttr(params, "Slice Views", iAValueType::Boolean, m_lpSliceViews); }
	if (preferences)          { addAttr(params, "Preferences", iAValueType::Boolean, m_lpPreferences); }
	if (renderSettings)       { addAttr(params, "Render Settings", iAValueType::Boolean, m_lpRenderSettings); }
	if (slicerSettings)       { addAttr(params, "Slice Settings", iAValueType::Boolean, m_lpSlicerSettings); }
	iAParameterDlg dlg(this, "Load Settings", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	if (camera)               { m_lpCamera               = values["Camera"].toBool(); }
	if (sliceViews)           { m_lpSliceViews           = values["Slice Views"].toBool(); }
	if (preferences)          { m_lpPreferences          = values["Preferences"].toBool(); }
	if (renderSettings)       { m_lpRenderSettings       = values["Render Settings"].toBool(); }
	if (slicerSettings)       { m_lpSlicerSettings       = values["Slice Settings"].toBool(); }

	if (m_lpCamera)
	{
		loadCamera(xml);
	}
	if (m_lpSliceViews && xml.hasElement("sliceViews"))
	{
		loadSliceViews(xml.node("sliceViews"));
	}
	if (m_lpPreferences && xml.hasElement("preferences"))
	{
		loadPreferences(xml.node("preferences"));
	}
	if (m_lpRenderSettings && xml.hasElement("renderSettings"))
	{
		loadRenderSettings(xml.node("renderSettings"));
	}
	if (m_lpSlicerSettings && xml.hasElement("slicerSettings"))
	{
		loadSlicerSettings(xml.node("slicerSettings"));
	}
}

void MainWindow::saveCamera(iAXmlSettings & xml)
{
	vtkCamera *camera = activeMdiChild()->renderer()->renderer()->GetActiveCamera();
	QDomElement cameraElement = xml.createElement("camera");
	saveCamera(cameraElement, camera);
}

bool MainWindow::loadCamera(iAXmlSettings & xml)
{
	vtkCamera *camera = activeMdiChild()->renderer()->renderer()->GetActiveCamera();
	if (!xml.hasElement("camera"))
	{
		return false;
	}
	loadCamera(xml.node("camera"), camera);

	double allBounds[6];
	activeMdiChild()->renderer()->renderer()->ComputeVisiblePropBounds( allBounds );
	activeMdiChild()->renderer()->renderer()->ResetCameraClippingRange( allBounds );
	return true;
}

void MainWindow::saveSliceViews(iAXmlSettings & xml)
{
	QDomNode sliceViewsNode = xml.createElement("sliceViews");
	for (int i = 0; i < iASlicerMode::SlicerCount; ++i)
	{
		saveSliceView(xml.document(), sliceViewsNode, activeMdiChild()->slicer(i)->camera(), slicerModeString(i));
	}
}

void MainWindow::saveSliceView(QDomDocument &doc, QDomNode &sliceViewsNode, vtkCamera *cam, QString const & elemStr)
{
	// add new slice view node
	QDomElement cameraElement = doc.createElement(elemStr);

	saveCamera(cameraElement, cam);

	sliceViewsNode.appendChild(cameraElement);
}

void MainWindow::loadCamera(QDomNode const & node, vtkCamera* camera)
{
	QDomNamedNodeMap attributes = node.attributes();
	double position[4], focalPoint[4], viewUp[4];
	position[0] = attributes.namedItem("positionX").nodeValue().toDouble();
	position[1] = attributes.namedItem("positionY").nodeValue().toDouble();
	position[2] = attributes.namedItem("positionZ").nodeValue().toDouble();
	focalPoint[0] = attributes.namedItem("focalPointX").nodeValue().toDouble();
	focalPoint[1] = attributes.namedItem("focalPointY").nodeValue().toDouble();
	focalPoint[2] = attributes.namedItem("focalPointZ").nodeValue().toDouble();
	viewUp[0] = attributes.namedItem("viewUpX").nodeValue().toDouble();
	viewUp[1] = attributes.namedItem("viewUpY").nodeValue().toDouble();
	viewUp[2] = attributes.namedItem("viewUpZ").nodeValue().toDouble();

	camera->SetPosition(position);
	camera->SetFocalPoint(focalPoint);
	camera->SetViewUp(viewUp);
	if (attributes.contains("scale"))
	{
		double scale = attributes.namedItem("scale").nodeValue().toDouble();
		camera->SetParallelScale(scale);
	}
}

void MainWindow::saveCamera(QDomElement &cameraElement, vtkCamera* camera)
{
	double position[4], focalPoint[4], viewUp[4];
	camera->GetPosition(position);
	camera->GetFocalPoint(focalPoint);
	camera->GetViewUp(viewUp);
	position[3] = 1.0; focalPoint[3] = 1.0; viewUp[3] = 1.0;

	cameraElement.setAttribute("positionX", tr("%1").arg(position[0]));
	cameraElement.setAttribute("positionY", tr("%1").arg(position[1]));
	cameraElement.setAttribute("positionZ", tr("%1").arg(position[2]));
	cameraElement.setAttribute("focalPointX", tr("%1").arg(focalPoint[0]));
	cameraElement.setAttribute("focalPointY", tr("%1").arg(focalPoint[1]));
	cameraElement.setAttribute("focalPointZ", tr("%1").arg(focalPoint[2]));
	cameraElement.setAttribute("viewUpX", tr("%1").arg(viewUp[0]));
	cameraElement.setAttribute("viewUpY", tr("%1").arg(viewUp[1]));
	cameraElement.setAttribute("viewUpZ", tr("%1").arg(viewUp[2]));

	if (camera->GetParallelProjection())
	{
		double scale = camera->GetParallelScale();
		cameraElement.setAttribute("scale", tr("%1").arg(scale));
	}
}

void MainWindow::loadSliceViews(QDomNode sliceViewsNode)
{
	QDomNodeList list = sliceViewsNode.childNodes();
	for (int n = 0; n < int(list.length()); n++)
	{
		QDomNode node = list.item(n);
		vtkCamera *camera;
		if      (node.nodeName() == "XY") camera = activeMdiChild()->slicer(iASlicerMode::XY)->camera();
		else if (node.nodeName() == "YZ") camera = activeMdiChild()->slicer(iASlicerMode::YZ)->camera();
		else                              camera = activeMdiChild()->slicer(iASlicerMode::XZ)->camera();
		loadCamera(node, camera);
	}
}

void MainWindow::savePreferences(iAXmlSettings &xml)
{
	QDomElement preferencesElement = xml.createElement("preferences");
	preferencesElement.setAttribute("histogramBins", tr("%1").arg(m_defaultPreferences.HistogramBins));
	preferencesElement.setAttribute("histogramLogarithmicYAxis", tr("%1").arg(m_defaultPreferences.HistogramLogarithmicYAxis));
	preferencesElement.setAttribute("limitForAuto3DRender", tr("%1").arg(m_defaultPreferences.LimitForAuto3DRender));
	preferencesElement.setAttribute("statisticalExtent", tr("%1").arg(m_defaultPreferences.StatisticalExtent));
	preferencesElement.setAttribute("compression", tr("%1").arg(m_defaultPreferences.Compression));
	preferencesElement.setAttribute("printParameters", tr("%1").arg(m_defaultPreferences.PrintParameters));
	preferencesElement.setAttribute("resultsInNewWindow", tr("%1").arg(m_defaultPreferences.ResultInNewWindow));
	preferencesElement.setAttribute("magicLensSize", tr("%1").arg(m_defaultPreferences.MagicLensSize));
	preferencesElement.setAttribute("magicLensFrameWidth", tr("%1").arg(m_defaultPreferences.MagicLensFrameWidth));
	preferencesElement.setAttribute("fontSize", QString::number(m_defaultPreferences.FontSize));
	preferencesElement.setAttribute("logToFile", tr("%1").arg(iALogWidget::get()->isLogToFileOn()));
}

void MainWindow::loadPreferences(QDomNode preferencesNode)
{
	QDomNamedNodeMap attributes = preferencesNode.attributes();
	m_defaultPreferences.HistogramBins = attributes.namedItem("histogramBins").nodeValue().toInt();
	m_defaultPreferences.HistogramLogarithmicYAxis = attributes.namedItem("histogramLogarithmicYAxis").nodeValue().toInt();
	m_defaultPreferences.LimitForAuto3DRender = attributes.namedItem("limitForAuto3DRender").nodeValue().toInt();
	m_defaultPreferences.StatisticalExtent = attributes.namedItem("statisticalExtent").nodeValue().toDouble();
	m_defaultPreferences.Compression = attributes.namedItem("compression").nodeValue() == "1";
	m_defaultPreferences.PrintParameters = attributes.namedItem("printParameters").nodeValue() == "1";
	m_defaultPreferences.ResultInNewWindow = attributes.namedItem("resultsInNewWindow").nodeValue() == "1";
	m_defaultPreferences.MagicLensSize = attributes.namedItem("magicLensSize").nodeValue().toInt();
	m_defaultPreferences.MagicLensFrameWidth = attributes.namedItem("magicLensFrameWidth").nodeValue().toInt();
	m_defaultPreferences.FontSize = attributes.namedItem("fontSize").nodeValue().toInt();
	bool prefLogToFile = attributes.namedItem("logToFile").nodeValue() == "1";
	QString logFileName = attributes.namedItem("logFile").nodeValue();

	iALogWidget::get()->setLogToFile(prefLogToFile, logFileName);

	activeMDI()->applyPreferences(m_defaultPreferences);
}

void MainWindow::saveRenderSettings(iAXmlSettings &xml)
{
	QDomElement renderSettingsElement = xml.createElement("renderSettings");
	renderSettingsElement.setAttribute("showSlicers", m_defaultRenderSettings.ShowSlicers);
	renderSettingsElement.setAttribute("showSlicePlanes", m_defaultRenderSettings.ShowSlicePlanes);
	renderSettingsElement.setAttribute("showHelpers", m_defaultRenderSettings.ShowHelpers);
	renderSettingsElement.setAttribute("showRPosition", m_defaultRenderSettings.ShowRPosition);
	renderSettingsElement.setAttribute("parallelProjection", m_defaultRenderSettings.ParallelProjection);
	renderSettingsElement.setAttribute("useStyleBGColor", m_defaultRenderSettings.UseStyleBGColor);
	renderSettingsElement.setAttribute("backgroundTop", m_defaultRenderSettings.BackgroundTop);
	renderSettingsElement.setAttribute("backgroundBottom", m_defaultRenderSettings.BackgroundBottom);
	renderSettingsElement.setAttribute("planeOpacity", m_defaultRenderSettings.PlaneOpacity);
	renderSettingsElement.setAttribute("useFXAA", m_defaultRenderSettings.UseFXAA);
	renderSettingsElement.setAttribute("multiSamples", m_defaultRenderSettings.MultiSamples);
	renderSettingsElement.setAttribute("useDepthPeeling", m_defaultRenderSettings.UseDepthPeeling);
	renderSettingsElement.setAttribute("depthPeels", m_defaultRenderSettings.DepthPeels);

	renderSettingsElement.setAttribute("linearInterpolation", m_defaultVolumeSettings.LinearInterpolation);
	renderSettingsElement.setAttribute("shading", m_defaultVolumeSettings.Shading);
	renderSettingsElement.setAttribute("sampleDistance", m_defaultVolumeSettings.SampleDistance);
	renderSettingsElement.setAttribute("ambientLighting", m_defaultVolumeSettings.AmbientLighting);
	renderSettingsElement.setAttribute("diffuseLighting", m_defaultVolumeSettings.DiffuseLighting);
	renderSettingsElement.setAttribute("specularLighting", m_defaultVolumeSettings.SpecularLighting);
	renderSettingsElement.setAttribute("specularPower", m_defaultVolumeSettings.SpecularPower);
	renderSettingsElement.setAttribute("renderMode", m_defaultVolumeSettings.RenderMode);
}

void MainWindow::loadRenderSettings(QDomNode renderSettingsNode)
{
	QDomNamedNodeMap attributes = renderSettingsNode.attributes();

	m_defaultRenderSettings.ShowSlicers = attributes.namedItem("showSlicers").nodeValue() == "1";
	m_defaultRenderSettings.ShowSlicePlanes = attributes.namedItem("showSlicePlanes").nodeValue() == "1";
	m_defaultRenderSettings.ShowHelpers = attributes.namedItem("showHelpers").nodeValue() == "1";
	m_defaultRenderSettings.ShowRPosition = attributes.namedItem("showRPosition").nodeValue() == "1";
	m_defaultRenderSettings.ParallelProjection = attributes.namedItem("parallelProjection").nodeValue() == "1";
	m_defaultRenderSettings.UseStyleBGColor = attributes.namedItem("useStyleBGColor").nodeValue() == "1";
	m_defaultRenderSettings.BackgroundTop = attributes.namedItem("backgroundTop").nodeValue();
	m_defaultRenderSettings.BackgroundBottom = attributes.namedItem("backgroundBottom").nodeValue();
	m_defaultRenderSettings.PlaneOpacity = attributes.namedItem("planeOpacity").nodeValue().toDouble();
	m_defaultRenderSettings.UseFXAA = attributes.namedItem("useFXAA").nodeValue() == "1";
	m_defaultRenderSettings.MultiSamples = attributes.namedItem("multiSamples").nodeValue().toInt();
	m_defaultRenderSettings.UseDepthPeeling = attributes.namedItem("useDepthPeeling").nodeValue() == "1";
	m_defaultRenderSettings.DepthPeels = attributes.namedItem("depthPeels").nodeValue().toInt();

	m_defaultVolumeSettings.LinearInterpolation = attributes.namedItem("linearInterpolation").nodeValue() == "1";
	m_defaultVolumeSettings.Shading = attributes.namedItem("shading").nodeValue() == "1";
	m_defaultVolumeSettings.SampleDistance = attributes.namedItem("sampleDistance").nodeValue().toDouble();
	m_defaultVolumeSettings.AmbientLighting = attributes.namedItem("ambientLighting").nodeValue().toDouble();
	m_defaultVolumeSettings.DiffuseLighting = attributes.namedItem("diffuseLighting").nodeValue().toDouble();
	m_defaultVolumeSettings.SpecularLighting = attributes.namedItem("specularLighting").nodeValue().toDouble();
	m_defaultVolumeSettings.SpecularPower = attributes.namedItem("specularPower").nodeValue().toDouble();
	m_defaultVolumeSettings.RenderMode = attributes.namedItem("renderMode").nodeValue().toInt();

	activeMDI()->applyRendererSettings(m_defaultRenderSettings, m_defaultVolumeSettings);
}

void MainWindow::saveSlicerSettings(iAXmlSettings &xml)
{
	QDomElement slicerSettingsElement = xml.createElement("slicerSettings");
	slicerSettingsElement.setAttribute("linkViews", m_defaultSlicerSettings.LinkViews);
	slicerSettingsElement.setAttribute("showIsolines", m_defaultSlicerSettings.SingleSlicer.ShowIsoLines);
	slicerSettingsElement.setAttribute("showPosition", m_defaultSlicerSettings.SingleSlicer.ShowPosition);
	slicerSettingsElement.setAttribute("showAxesCaption", m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption);
	slicerSettingsElement.setAttribute("numberOfIsolines", m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines);
	slicerSettingsElement.setAttribute("minIsovalue", m_defaultSlicerSettings.SingleSlicer.MinIsoValue);
	slicerSettingsElement.setAttribute("maxIsovalue", m_defaultSlicerSettings.SingleSlicer.MaxIsoValue);
	slicerSettingsElement.setAttribute("linearInterpolation", m_defaultSlicerSettings.SingleSlicer.LinearInterpolation);
	slicerSettingsElement.setAttribute("adjustWindowLevelEnabled", m_defaultSlicerSettings.SingleSlicer.AdjustWindowLevelEnabled);
	slicerSettingsElement.setAttribute("snakeSlices", m_defaultSlicerSettings.SnakeSlices);
	slicerSettingsElement.setAttribute("linkMDIs", m_defaultSlicerSettings.LinkMDIs);
	slicerSettingsElement.setAttribute("cursorMode", m_defaultSlicerSettings.SingleSlicer.CursorMode);
	slicerSettingsElement.setAttribute("toolTipFontSize", m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize);
	for (int s=0; s<iASlicerMode::SlicerCount; ++s)
	{
		slicerSettingsElement.setAttribute(QString("slicerBgColor%1").arg(s), m_defaultSlicerSettings.BackgroundColor[s]);
	}
}

void MainWindow::loadSlicerSettings(QDomNode slicerSettingsNode)
{
	QDomNamedNodeMap attributes = slicerSettingsNode.attributes();

	m_defaultSlicerSettings.LinkViews = attributes.namedItem("linkViews").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.ShowIsoLines = attributes.namedItem("showIsolines").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.ShowPosition = attributes.namedItem("showPosition").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption = attributes.namedItem("showAxesCaption").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = attributes.namedItem("numberOfIsolines").nodeValue().toInt();
	m_defaultSlicerSettings.SingleSlicer.MinIsoValue = attributes.namedItem("minIsovalue").nodeValue().toDouble();
	m_defaultSlicerSettings.SingleSlicer.MaxIsoValue = attributes.namedItem("maxIsovalue").nodeValue().toDouble();
	m_defaultSlicerSettings.SingleSlicer.LinearInterpolation = attributes.namedItem("linearInterpolation").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.AdjustWindowLevelEnabled = attributes.namedItem("adjustWindowLevelEnabled").nodeValue() == "1";
	m_defaultSlicerSettings.SnakeSlices = attributes.namedItem("snakeSlices").nodeValue().toDouble();
	m_defaultSlicerSettings.LinkMDIs = attributes.namedItem("linkMDIs").nodeValue() == "1";
	m_defaultSlicerSettings.SingleSlicer.CursorMode = attributes.namedItem("cursorMode").nodeValue();
	m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize = attributes.namedItem("toolTipFontSize").nodeValue().toInt();
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		m_defaultSlicerSettings.BackgroundColor[s] = attributes.namedItem(QString("slicerBgColor%1").arg(s)).nodeValue();
	}
	activeMDI()->applySlicerSettings(m_defaultSlicerSettings);
}

QList<QString> MainWindow::mdiWindowTitles()
{
	QList<QString> windowTitles;
	for (iAMdiChild* mdiChild : mdiChildList())
	{
		windowTitles.append(mdiChild->windowTitle());
	}
	return windowTitles;
}

void MainWindow::linkViews()
{
	if (activeMdiChild())
	{
		m_defaultSlicerSettings.LinkViews = m_ui->actionLinkViews->isChecked();
		activeMDI()->linkViews(m_defaultSlicerSettings.LinkViews);
		LOG(lvlInfo, QString("Link Views: ").arg(iAConverter<bool>::toString(m_defaultSlicerSettings.LinkViews)));
	}
}

void MainWindow::linkMDIs()
{
	if (activeMdiChild())
	{
		m_defaultSlicerSettings.LinkMDIs = m_ui->actionLinkMdis->isChecked();
		activeMDI()->linkMDIs(m_defaultSlicerSettings.LinkMDIs);
		LOG(lvlInfo, QString("Link MDIs: ").arg(iAConverter<bool>::toString(m_defaultSlicerSettings.LinkMDIs)));
	}
}

void MainWindow::toggleSlicerInteraction()
{
	if (!activeMdiChild())
	{
		return;
	}
	m_defaultSlicerSettings.InteractorsEnabled = m_ui->actionToggleSlicerInteraction->isChecked();
	activeMDI()->enableSlicerInteraction(m_defaultSlicerSettings.InteractorsEnabled);
}

void MainWindow::toggleFullScreen()
{
	bool fullScreen = m_ui->actionFullScreenMode->isChecked();
	if (fullScreen)
	{
		showFullScreen();
	}
	else
	{
		showNormal();
	}
	emit fullScreenToggled();
}

void MainWindow::toggleMenu()
{
	bool showMenu = m_ui->actionShowMenu->isChecked();
	if (showMenu)
	{
		m_ui->menubar->show();
	}
	else
	{
		m_ui->menubar->hide();
	}
}

void MainWindow::prefs()
{
	iAMdiChild *child = activeMdiChild();

	QStringList looks;
	QMap<QString, QString> styleNames;
	styleNames.insert(tr("Dark")      , ":/dark.qss");
	styleNames.insert(tr("Bright")    , ":/bright.qss");
	for (QString key: styleNames.keys())
	{
		if (m_qssName == styleNames[key])
		{
			looks.append(QString("!") + key);
		}
		else
		{
			looks.append(key);
		}
	}
	iAPreferences p = child ? child->preferences() : m_defaultPreferences;
	QString descr;
	if (iALogWidget::get()->isFileLogError())
	{
		descr = "Could not write to the specified logfile, logging to file was therefore disabled."
			" Please check file permissions and/or whether the path to the file exists, before re-enabling the option!.";
	}
	QStringList logLevels(AvailableLogLevels()), fileLogLevels(AvailableLogLevels());
	logLevels[iALogWidget::get()->logLevel()-1] = "!" + logLevels[iALogWidget::get()->logLevel()-1];
	fileLogLevels[iALogWidget::get()->fileLogLevel() - 1] = "!" + fileLogLevels[iALogWidget::get()->fileLogLevel() - 1];
	iAAttributes params;
	addAttr(params, "Histogram Bins", iAValueType::Discrete, p.HistogramBins, 2);
	addAttr(params, "Statistical extent", iAValueType::Discrete, p.StatisticalExtent, 1);
	addAttr(params, "Use Compression when storing .mhd files", iAValueType::Boolean, p.Compression);
	addAttr(params, "Print Parameters", iAValueType::Boolean, p.PrintParameters);
	addAttr(params, "Results in new window", iAValueType::Boolean, p.ResultInNewWindow);
	addAttr(params, "Log Level", iAValueType::Categorical, logLevels);
	addAttr(params, "Log to file", iAValueType::Boolean, iALogWidget::get()->isLogToFileOn());
	addAttr(params, "Log File Name", iAValueType::FileNameSave, iALogWidget::get()->logFileName());
	addAttr(params, "File Log Level", iAValueType::Categorical, fileLogLevels);
	addAttr(params, "Looks", iAValueType::Categorical, looks);
	addAttr(params, "Font size", iAValueType::Discrete, QString::number(p.FontSize), 4, 120);
	addAttr(params, "Magic lens size", iAValueType::Discrete, p.MagicLensSize, MinimumMagicLensSize, MaximumMagicLensSize);
	addAttr(params, "Magic lens frame width", iAValueType::Discrete, p.MagicLensFrameWidth, 0);
	addAttr(params, "Logarithmic Histogram y axis", iAValueType::Boolean, p.HistogramLogarithmicYAxis);
	addAttr(params, "Size limit for automatic 3D rendering (MB)", iAValueType::Discrete, p.LimitForAuto3DRender, 0);
	iAParameterDlg dlg(this, "Preferences", params, descr);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	m_defaultPreferences.HistogramBins = values["Histogram Bins"].toInt();
	m_defaultPreferences.StatisticalExtent = values["Statistical extent"].toInt();
	m_defaultPreferences.Compression = values["Use Compression when storing .mhd files"].toBool();
	m_defaultPreferences.PrintParameters = values["Print Parameters"].toBool();
	m_defaultPreferences.ResultInNewWindow = values["Results in new window"].toBool();
	iALogWidget::get()->setLogLevel(static_cast<iALogLevel>(AvailableLogLevels().indexOf(values["Log Level"].toString()) + 1));
	bool logToFile = values["Log to file"].toBool();
	QString logFileName = values["Log File Name"].toString();
	iALogWidget::get()->setFileLogLevel(static_cast<iALogLevel>(AvailableLogLevels().indexOf(values["File Log Level"].toString()) + 1));
	QString looksStr = values["Looks"].toString();
	if (m_qssName != styleNames[looksStr])
	{
		m_qssName = styleNames[looksStr];
		applyQSS();
	}
	m_defaultPreferences.FontSize = values["Font size"].toInt();
	auto f = QApplication::font();
	f.setPointSize(m_defaultPreferences.FontSize);
	QApplication::setFont(f);
	m_defaultPreferences.MagicLensSize = clamp(MinimumMagicLensSize, MaximumMagicLensSize, values["Magic lens size"].toInt());
	m_defaultPreferences.MagicLensFrameWidth = std::max(0, static_cast<int>(values["Magic lens frame width"].toInt()));
	m_defaultPreferences.HistogramLogarithmicYAxis = values["Logarithmic Histogram y axis"].toBool();
	m_defaultPreferences.LimitForAuto3DRender = values["Size limit for automatic 3D rendering (MB)"].toInt();
	if (activeMdiChild())
	{
		activeMDI()->applyPreferences(m_defaultPreferences);
	}
	iALogWidget::get()->setLogToFile(logToFile, logFileName, true);
}

void MainWindow::renderSettings()
{
	QString dlgTitle = activeMdiChild()? (activeMdiChild()->windowTitle() + " - renderer setings") : "Default renderer settings";
	iARenderSettings renderSettings = activeMdiChild() ? activeMDI()->renderSettings() : m_defaultRenderSettings;
	iAVolumeSettings volumeSettings = activeMdiChild() ? activeMdiChild()->volumeSettings() : m_defaultVolumeSettings;
	QStringList renderTypes = RenderModeMap().values();
	selectOption(renderTypes, renderTypes[volumeSettings.RenderMode]);
	iAAttributes params;
	addAttr(params, "Show slicers", iAValueType::Boolean, renderSettings.ShowSlicers);
	addAttr(params, "Show slice planes", iAValueType::Boolean, renderSettings.ShowSlicePlanes);
	addAttr(params, "Slice plane opacity", iAValueType::Continuous, renderSettings.PlaneOpacity, 0, 1);
	addAttr(params, "Show helpers", iAValueType::Boolean, renderSettings.ShowHelpers);
	addAttr(params, "Show position", iAValueType::Boolean, renderSettings.ShowRPosition);
	addAttr(params, "Parallel projection", iAValueType::Boolean, renderSettings.ParallelProjection);
	addAttr(params, "Use style background color", iAValueType::Boolean, renderSettings.UseStyleBGColor);
	addAttr(params, "Background top", iAValueType::Color, renderSettings.BackgroundTop);
	addAttr(params, "Background bottom", iAValueType::Color, renderSettings.BackgroundBottom);
	addAttr(params, "Use FXAA", iAValueType::Boolean, renderSettings.UseFXAA);
	addAttr(params, "MultiSamples", iAValueType::Discrete, renderSettings.MultiSamples);
	addAttr(params, "Use Depth Peeling", iAValueType::Boolean, renderSettings.UseDepthPeeling);
	addAttr(params, "Occlusion Ratio", iAValueType::Continuous, renderSettings.OcclusionRatio);
	addAttr(params, "Use Screen Space Ambient Occlusion", iAValueType::Boolean, renderSettings.UseSSAO);
	addAttr(params, "Maximum Depth Peels", iAValueType::Discrete, renderSettings.DepthPeels);

	addAttr(params, "Linear interpolation", iAValueType::Boolean, volumeSettings.LinearInterpolation);
	addAttr(params, "Shading", iAValueType::Boolean, volumeSettings.Shading);
	addAttr(params, "Sample distance", iAValueType::Continuous, volumeSettings.SampleDistance);
	addAttr(params, "Ambient lighting", iAValueType::Continuous, volumeSettings.AmbientLighting);
	addAttr(params, "Diffuse lighting", iAValueType::Continuous, volumeSettings.DiffuseLighting);
	addAttr(params, "Specular lighting", iAValueType::Continuous, volumeSettings.SpecularLighting);
	addAttr(params, "Specular power", iAValueType::Continuous, volumeSettings.SpecularPower);
	addAttr(params, "Renderer type", iAValueType::Categorical, renderTypes);
	iAParameterDlg dlg(this, dlgTitle, params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	m_defaultRenderSettings.ShowSlicers = values["Show slicers"].toBool();
	m_defaultRenderSettings.ShowSlicePlanes = values["Show slice planes"].toBool();
	m_defaultRenderSettings.ShowHelpers = values["Show helpers"].toBool();
	m_defaultRenderSettings.ShowRPosition = values["Show position"].toBool();
	m_defaultRenderSettings.ParallelProjection = values["Parallel projection"].toBool();
	m_defaultRenderSettings.UseStyleBGColor = values["Use style background color"].toBool();
	m_defaultRenderSettings.BackgroundTop = values["Background top"].toString();
	m_defaultRenderSettings.BackgroundBottom = values["Background bottom"].toString();
	m_defaultRenderSettings.UseFXAA = values["Use FXAA"].toBool();
	m_defaultRenderSettings.UseSSAO = values["Use Screen Space Ambient Occlusion"].toBool();
	// available sub-options:
	//      radius, bias, kernel size, blur
	m_defaultRenderSettings.MultiSamples = values["MultiSamples"].toInt();
	m_defaultRenderSettings.UseDepthPeeling = values["Use Depth Peeling"].toBool();
	m_defaultRenderSettings.DepthPeels = values["Maximum Depth Peels"].toInt();
	m_defaultRenderSettings.OcclusionRatio = values["Occlusion Ratio"].toDouble();

	QColor bgTop(m_defaultRenderSettings.BackgroundTop);
	QColor bgBottom(m_defaultRenderSettings.BackgroundBottom);
	if (!bgTop.isValid())
	{
		bgTop.setRgbF(0.5, 0.666666666666666667, 1.0);
		m_defaultRenderSettings.BackgroundTop = bgTop.name();
	}
	if (!bgBottom.isValid())
	{
		bgBottom.setRgbF(1.0, 1.0, 1.0);
		m_defaultRenderSettings.BackgroundBottom = bgTop.name();
	}

	m_defaultVolumeSettings.LinearInterpolation = values["Linear interpolation"].toBool();
	m_defaultVolumeSettings.Shading = values["Shading"].toBool();
	m_defaultVolumeSettings.SampleDistance = values["Sample distance"].toDouble();
	m_defaultVolumeSettings.AmbientLighting = values["Ambient lighting"].toDouble();
	m_defaultVolumeSettings.DiffuseLighting = values["Diffuse lighting"].toDouble();
	m_defaultVolumeSettings.SpecularLighting = values["Specular lighting"].toDouble();
	m_defaultVolumeSettings.SpecularPower = values["Specular power"].toDouble();
	m_defaultVolumeSettings.RenderMode = mapRenderModeToEnum(values["Renderer type"].toString());

	m_defaultRenderSettings.PlaneOpacity = values["Slice plane opacity"].toDouble();

	if (activeMdiChild())
	{
		activeMDI()->applyRendererSettings(m_defaultRenderSettings, m_defaultVolumeSettings);
	}
	LOG(lvlInfo, "Changed renderer settings");
}

namespace
{
	QString slicerBGColorSetting(int slicerMode)
	{
		return QString("Background Color %1 Slicer").arg(slicerModeString(slicerMode));
	}
}

void MainWindow::slicerSettings()
{
	QStringList mouseCursorOptions = QStringList()
		<< "Crosshair default"
		<< "Crosshair thick red"	<< "Crosshair thin red"
		<< "Crosshair thick orange"	<< "Crosshair thin orange"
		<< "Crosshair thick yellow"	<< "Crosshair thin yellow"
		<< "Crosshair thick blue"	<< "Crosshair thin blue"
		<< "Crosshair thick cyan"	<< "Crosshair thin cyan";
	QString dlgTitle = activeMdiChild() ? (activeMdiChild()->windowTitle() + " - slicer setings") : "Default slicer settings";
	iASlicerSettings const& slicerSettings = activeMDI() ? activeMDI()->slicerSettings() : m_defaultSlicerSettings;
	selectOption(mouseCursorOptions, slicerSettings.SingleSlicer.CursorMode);
	iAAttributes params;
	addAttr(params, "Link Views", iAValueType::Boolean, slicerSettings.LinkViews);
	addAttr(params, "Show Position", iAValueType::Boolean, slicerSettings.SingleSlicer.ShowPosition);
	addAttr(params, "Show Isolines", iAValueType::Boolean, slicerSettings.SingleSlicer.ShowIsoLines);
	addAttr(params, "Linear Interpolation", iAValueType::Boolean, slicerSettings.SingleSlicer.LinearInterpolation);
	addAttr(params, "Adjust Window/Level via Mouse Click+Drag", iAValueType::Boolean, slicerSettings.SingleSlicer.AdjustWindowLevelEnabled);
	addAttr(params, "Number of Isolines", iAValueType::Discrete, slicerSettings.SingleSlicer.NumberOfIsoLines);
	addAttr(params, "Min Isovalue", iAValueType::Continuous, slicerSettings.SingleSlicer.MinIsoValue);
	addAttr(params, "Max Isovalue", iAValueType::Continuous, slicerSettings.SingleSlicer.MaxIsoValue);
	addAttr(params, "Snake Slices", iAValueType::Discrete, slicerSettings.SnakeSlices);
	addAttr(params, "Link MDIs", iAValueType::Boolean, slicerSettings.LinkMDIs);
	addAttr(params, "Mouse Cursor", iAValueType::Categorical, mouseCursorOptions);
	addAttr(params, "Show Axes Caption", iAValueType::Boolean, slicerSettings.SingleSlicer.ShowAxesCaption);
	addAttr(params, "Tooltip Font Size (pt)", iAValueType::Discrete, slicerSettings.SingleSlicer.ToolTipFontSize);
	addAttr(params, "Show Tooltip", iAValueType::Boolean, slicerSettings.SingleSlicer.ShowTooltip);
	for (int s=0; s<iASlicerMode::SlicerCount; ++s)
	{
		addAttr(params, slicerBGColorSetting(s), iAValueType::Color, slicerSettings.BackgroundColor[s]);
	}
	iAParameterDlg dlg(this, dlgTitle, params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	m_defaultSlicerSettings.LinkViews = values["Link Views"].toBool();
	m_defaultSlicerSettings.SnakeSlices = values["Snake Slices"].toInt();
	m_defaultSlicerSettings.LinkMDIs = values["Link MDIs"].toBool();
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		m_defaultSlicerSettings.BackgroundColor[s] = values[slicerBGColorSetting(s)].toString();
	}
	m_defaultSlicerSettings.SingleSlicer.ShowPosition = values["Show Position"].toBool();
	m_defaultSlicerSettings.SingleSlicer.ShowIsoLines = values["Show Isolines"].toBool();
	m_defaultSlicerSettings.SingleSlicer.LinearInterpolation = values["Linear Interpolation"].toBool();
	m_defaultSlicerSettings.SingleSlicer.AdjustWindowLevelEnabled = values["Adjust Window/Level via Mouse Click+Drag"].toBool();
	m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = values["Number of Isolines"].toInt();
	m_defaultSlicerSettings.SingleSlicer.MinIsoValue = values["Min Isovalue"].toDouble();
	m_defaultSlicerSettings.SingleSlicer.MaxIsoValue = values["Max Isovalue"].toDouble();
	m_defaultSlicerSettings.SingleSlicer.CursorMode = values["Mouse Coursor Types"].toString();
	m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption = values["Show Axes Caption"].toBool();
	m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize = values["Tooltip Font Size (pt)"].toInt();
	m_defaultSlicerSettings.SingleSlicer.ShowTooltip = values["Show Tooltip"].toBool();

	if (activeMdiChild())
	{
		activeMDI()->applySlicerSettings(m_defaultSlicerSettings);
	}
	LOG(lvlInfo, "Changed slicer settings");
}

void MainWindow::changeInteractionMode(bool isChecked)
{
	if (activeMdiChild())
	{
		auto a = qobject_cast<QAction*>(sender());
		auto mode = (a == m_ui->actionInteractionModeCamera) ?
			(isChecked ? MdiChild::imCamera : MdiChild::imRegistration) :
			(isChecked ? MdiChild::imRegistration : MdiChild::imCamera);
		activeMDI()->setInteractionMode(mode);
	}
}

void MainWindow::updateInteractionModeControls(int mode)
{
	QSignalBlocker b1(m_ui->actionInteractionModeRegistration), b2(m_ui->actionInteractionModeCamera);
	m_ui->actionInteractionModeCamera->setChecked(mode == MdiChild::imCamera);
	m_ui->actionInteractionModeRegistration->setChecked(mode == MdiChild::imRegistration);
}

void MainWindow::rendererCamPosition()
{
	if (activeMdiChild())
	{
		int pos = sender()->property("camPosition").toInt();
		activeMDI()->setCamPosition(pos);
	}
}

void MainWindow::rendererSyncCamera()
{
	QList<iAMdiChild *> mdiwindows = mdiChildList();
	int sizeMdi = mdiwindows.size();
	if (sizeMdi <= 1)
	{
		return;
	}
	double camOptions[10] = {0};
	activeMDI()->camPosition(camOptions);
	for (int i = 0; i < sizeMdi; i++)
	{
		MdiChild *tmpChild = dynamic_cast<MdiChild*>(mdiwindows.at(i));
		if (tmpChild == activeMDI())
		{
			continue;
		}
		tmpChild->setCamPosition(camOptions, m_defaultRenderSettings.ParallelProjection);
	}
}

void MainWindow::rendererSaveCameraSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), filePath ,tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	saveCamera(xml);
	xml.save(fileName);
}

void MainWindow::rendererLoadCameraSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), filePath, tr("XML (*.xml)"));
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	if (!xml.read(fileName) ||
		!loadCamera(xml))
	{
		return;
	}
	// apply this camera settings to all the MdiChild.
	rendererSyncCamera();
}

iAMdiChild* MainWindow::resultChild(iAMdiChild* iaOldChild, QString const & title)
{
	auto oldChild = dynamic_cast<MdiChild*>(iaOldChild);
	if (!oldChild || oldChild->resultInNewWindow())
	{
		// TODO: copy all dataset images, or don't copy anything here and use image from old child directly,
		// or nothing at all until new image available!
		// Note that filters currently get their input from this child already!
		MdiChild* newChild = dynamic_cast<MdiChild*>(createMdiChild(true));
		if (oldChild)
		{
			//auto img = oldChild->firstImageData();
			//if (img)
			//{
			//	newChild->displayResult(title, img);
			//}
			copyFunctions(oldChild, newChild);
		}
		return newChild;
	}
	oldChild->setWindowModified(true);
	return oldChild;
}

iAMdiChild * MainWindow::resultChild(QString const & title)
{
	return resultChild(activeMdiChild(), title);
}

iAMdiChild * MainWindow::resultChild(int childInd, QString const & f)
{
	return resultChild(mdiChildList().at(childInd), f);
}

// TODO NEWIO: probably has outlived its usefulness, but check!
void MainWindow::copyFunctions(MdiChild* oldChild, MdiChild* newChild)
{
	//std::vector<iAChartFunction*> const & oldChildFunctions = oldChild->histogram()->functions();
	//for (unsigned int i = 1; i < oldChildFunctions.size(); ++i)
	//{
	//	// TODO: implement a "clone" function to avoid dynamic_cast here?
	//	iAChartFunction *curFunc = oldChildFunctions[i];
	//	if (dynamic_cast<iAChartFunctionGaussian*>(curFunc))
	//	{
	//		auto oldGaussian = dynamic_cast<iAChartFunctionGaussian*>(curFunc);
	//		auto newGaussian = new iAChartFunctionGaussian(newChild->histogram(), FunctionColors()[i % 7]);
	//		newGaussian->setMean(oldGaussian->getMean());
	//		newGaussian->setMultiplier(oldGaussian->getMultiplier());
	//		newGaussian->setSigma(oldGaussian->getSigma());
	//		newChild->histogram()->functions().push_back(newGaussian);
	//	}
	//	else if (dynamic_cast<iAChartFunctionBezier*>(curFunc))
	//	{
	//		auto oldBezier = dynamic_cast<iAChartFunctionBezier*>(curFunc);
	//		auto newBezier = new iAChartFunctionBezier(newChild->histogram(), FunctionColors()[i % 7]);
	//		for (unsigned int j = 0; j < oldBezier->getPoints().size(); ++j)
	//		{
	//			newBezier->addPoint(oldBezier->getPoints()[j].x(), oldBezier->getPoints()[j].y());
	//		}
	//		newChild->histogram()->functions().push_back(newBezier);
	//	}
	//	// otherwise: unknown function type, do nothing
	//}
}

void MainWindow::about()
{
	QDialog dlg(this);
	dlg.setWindowTitle("About open_iA");
	dlg.setLayout(new QVBoxLayout());
	
	auto imgLabel = new QLabel();
	imgLabel->setPixmap(m_splashScreenImg);
	// to center image:
	auto imgWidget = new QWidget();
	imgWidget->setLayout(new QHBoxLayout());
	imgWidget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));
	imgWidget->layout()->addWidget(imgLabel);
	imgWidget->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed));

	dlg.layout()->addWidget(imgWidget);

	auto linkLabel = new QLabel("<a href=\"https://3dct.github.io/open_iA/\">3dct.github.io/open_iA</a>");
	linkLabel->setTextFormat(Qt::RichText);
	linkLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
	linkLabel->setOpenExternalLinks(true);
	linkLabel->setAlignment(Qt::AlignRight);
	dlg.layout()->addWidget(linkLabel);

	auto buildInfoLabel = new QLabel("Build information:");
	buildInfoLabel->setIndent(0);
	buildInfoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	dlg.layout()->addWidget(buildInfoLabel);
	
	auto rows = m_buildInformation.count('\n') + 1;
	auto table = new QTableWidget(rows, 2, this);
	table->setEditTriggers(QAbstractItemView::NoEditTriggers);
	table->setItem(0, 0, new QTableWidgetItem("Version"));
	table->setItem(0, 1, new QTableWidgetItem(m_gitVersion));
	auto lines = m_buildInformation.split("\n");
	int row = 1;
	for (auto line: lines)
	{
		auto tokens = line.split("\t");
		if (tokens.size() != 2)
		{
			continue;
		}
		table->setItem(row, 0, new QTableWidgetItem(tokens[0]));
		table->setItem(row, 1, new QTableWidgetItem(tokens[1]));
		++row;
	}
	table->resizeColumnsToContents();
	table->verticalHeader()->hide();
	table->horizontalHeader()->hide();
	// set fixed table height:
	auto tableHeight = 0;
	for (int r = 0; r < table->rowCount(); ++r)
	{
		tableHeight += table->rowHeight(r);
	}
	// +2 to avoid minor scrolling when clicking on the left/right- up/bottom-most cell in the table:
	tableHeight += 2;
	auto tableWidth = 0;
	for (int c=0; c < table->columnCount(); ++c)
	{
		tableWidth += table->columnWidth(c);
	}
	auto screenHeightThird = screen()->geometry().height() / 3;
	if (imgLabel->height() > screenHeightThird)
	{
		imgLabel->setFixedSize(
			screenHeightThird * static_cast<double>(imgLabel->width()) / imgLabel->height(),
			screenHeightThird);
	}

	imgLabel->setScaledContents(true);
	// make sure about dialog isn't higher than roughly 2/3 the screen size:
	tableWidth = std::max(tableWidth, imgLabel->width());
	const int MinTableHeight = 50;
	auto newTableHeight = std::max(MinTableHeight, std::min(tableHeight, screenHeightThird));
	table->setMinimumWidth(tableWidth + 20); // + 20 for approximation of scrollbar width; verticalScrollBar()->height is wildly inaccurate before first show (100 or so)
	table->setMinimumHeight(newTableHeight);
	//table->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	table->setAlternatingRowColors(true);
	table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	table->horizontalHeader()->setStretchLastSection(true);
	dlg.layout()->addWidget(table);

	auto okBtn = new QPushButton("Ok");
	connect(okBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
	dlg.layout()->addWidget(okBtn);

	dlg.exec();
}

void MainWindow::wiki()
{
	auto act = qobject_cast<QAction*>(QObject::sender());
	if (act->text().contains("Core"))
	{
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/wiki/Core"));
	}
	else if (act->text().contains("Filters"))
	{
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/wiki/Filters"));
	}
	else if (act->text().contains("Tools"))
	{
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/wiki/Tools"));
	}
	else if (act->text().contains("releases"))
	{
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/releases"));
	}
	else if (act->text().contains("bug"))
	{
		QDesktopServices::openUrl(QUrl("https://github.com/3dct/open_iA/issues"));
	}
}

void MainWindow::createRecentFileActions()
{
	m_separatorAct = m_ui->menuFile->addSeparator();
	for (int i = 0; i < MaxRecentFiles; ++i)
	{
		m_recentFileActs[i] = new QAction(this);
		m_recentFileActs[i]->setVisible(false);
		m_ui->menuFile->addAction(m_recentFileActs[i]);
	}
	updateRecentFileActions();
}

void MainWindow::updateMenus()  // (and toolbars)
{
	bool hasMdiChild = activeMdiChild();
	auto child = activeMDI();
	
	// File Menu: / toolbar
	m_ui->actionSaveDataSet->setEnabled(hasMdiChild);
	m_ui->actionSaveProject->setEnabled(activeChild<iASavableProject>());
	m_ui->actionSaveVolumeStack->setEnabled(hasMdiChild);
	m_ui->actionLoadSettings->setEnabled(hasMdiChild);
	m_ui->actionSaveSettings->setEnabled(hasMdiChild);
	m_ui->actionClose->setEnabled(hasMdiChild);
	m_ui->actionCloseAll->setEnabled(hasMdiChild);

	// Edit Menu / toolbar
	m_ui->actionEditProfilePoints->setEnabled(hasMdiChild);
	QSignalBlocker blockEditProfile(m_ui->actionEditProfilePoints);
	m_ui->actionEditProfilePoints->setChecked(child && child->profileHandlesEnabled());
	m_ui->menuInteractionMode->setEnabled(hasMdiChild);
	QSignalBlocker interactionModeCameraBlock(m_ui->actionInteractionModeCamera);
	m_ui->actionInteractionModeCamera->setChecked(child && child->interactionMode() == MdiChild::imCamera);
	m_ui->actionInteractionModeCamera->setEnabled(hasMdiChild);
	QSignalBlocker interactionModeRegistrationBlock(m_ui->actionInteractionModeRegistration);
	m_ui->actionInteractionModeRegistration->setChecked(child && child->interactionMode() == MdiChild::imRegistration);
	m_ui->actionInteractionModeRegistration->setEnabled(hasMdiChild);
	m_ui->actionRendererSettings->setEnabled(hasMdiChild);
	m_ui->actionSlicerSettings->setEnabled(hasMdiChild);
	m_ui->menuInteractionMode->setEnabled(hasMdiChild);

	// Views Menu:
	// layout submenu / toolbar
	m_ui->menuViews->setEnabled(hasMdiChild);
	m_ui->actionLoadLayout->setEnabled(hasMdiChild);
	m_ui->actionSaveLayout->setEnabled(hasMdiChild);
	m_ui->actionDeleteLayout->setEnabled(hasMdiChild);
	m_ui->actionResetLayout->setEnabled(hasMdiChild);
	m_layout->setEnabled(hasMdiChild);
	// slicer submenu / toolbar
	QSignalBlocker sliceInteractBlock(m_ui->actionToggleSlicerInteraction);
	m_ui->actionToggleSlicerInteraction->setChecked(activeMDI() && activeMDI()->isSlicerInteractionEnabled());
	m_ui->actionMagicLens2D->setEnabled(hasMdiChild);
	m_ui->actionRawProfile->setEnabled(hasMdiChild);
	m_ui->actionSnakeSlicer->setEnabled(hasMdiChild);
	QSignalBlocker blockSliceProfile(m_ui->actionRawProfile);
	m_ui->actionRawProfile->setChecked(child && child->isSliceProfileEnabled());
	QSignalBlocker blockSnakeSlicer(m_ui->actionSnakeSlicer);
	m_ui->actionSnakeSlicer->setChecked(child && child->isSnakeSlicerToggled());
	updateMagicLens2DCheckState(child && child->isMagicLens2DEnabled());
	// renderer submenu / toolbar
	QSignalBlocker renderInteractBlock(m_ui->actionToggleRendererInteraction);
	m_ui->actionToggleRendererInteraction->setChecked(activeMDI() && activeMDI()->isRendererInteractionEnabled());
	QSignalBlocker blockMagicLens3D(m_ui->actionMagicLens3D);
	m_ui->actionMagicLens3D->setChecked(child && child->isMagicLens3DEnabled());
	m_ui->actionMagicLens3D->setEnabled(hasMdiChild);
	m_ui->actionViewXDirectionInRaycaster->setEnabled(hasMdiChild);
	m_ui->actionViewYDirectionInRaycaster->setEnabled(hasMdiChild);
	m_ui->actionViewZDirectionInRaycaster->setEnabled(hasMdiChild);
	m_ui->actionViewmXDirectionInRaycaster->setEnabled(hasMdiChild);
	m_ui->actionViewmYDirectionInRaycaster->setEnabled(hasMdiChild);
	m_ui->actionViewmZDirectionInRaycaster->setEnabled(hasMdiChild);
	m_ui->actionIsometricViewInRaycaster->setEnabled(hasMdiChild);
	m_ui->actionSaveCameraSettings->setEnabled(hasMdiChild);
	m_ui->actionLoadCameraSettings->setEnabled(hasMdiChild);
	m_ui->actionSyncCamera->setEnabled(hasMdiChild);

	// Window Menu:
	m_ui->actionTile->setEnabled(hasMdiChild && m_ui->actionSubWindows->isChecked());
	m_ui->actionCascade->setEnabled(hasMdiChild && m_ui->actionSubWindows->isChecked());
	m_ui->actionNextWindow->setEnabled(hasMdiChild);
	m_ui->actionPrevWindow->setEnabled(hasMdiChild);

	updateRecentFileActions();

	setModuleActionsEnabled(hasMdiChild);
}

void MainWindow::updateMagicLens2DCheckState(bool enabled)
{
	QSignalBlocker blockMagicLens2D(m_ui->actionMagicLens2D);
	m_ui->actionMagicLens2D->setChecked(enabled);
}

void MainWindow::updateWindowMenu()
{
	auto windows = mdiChildList();
	for (int i = 0; i < windows.size(); ++i)
	{
		iAMdiChild *child = windows.at(i);
		QString text = QString("%1%2 %3").arg(i < 9 ? "&" : "").arg(i + 1)
				.arg(child->fileInfo().fileName());
		QAction* action = m_ui->menuWindow->addAction(text);
		action->setCheckable(true);
		action->setChecked(child == activeMdiChild());
		connect(action, &QAction::triggered, [&] { setActiveSubWindow(windows.at(i)); });
	}
}

iAMdiChild* MainWindow::createMdiChild(bool unsavedChanges)
{
	MdiChild *child = new MdiChild(this, m_defaultPreferences, unsavedChanges);
	QMdiSubWindow* subWin = m_ui->mdiArea->addSubWindow(child);
	subWin->setOption(QMdiSubWindow::RubberBandResize);
	subWin->setOption(QMdiSubWindow::RubberBandMove);

	if (m_ui->mdiArea->subWindowList().size() < 2)
	{
		child->showMaximized();
	}
	else
	{
		child->show();
	}
	child->initializeViews();
	child->applyRendererSettings(m_defaultRenderSettings, m_defaultVolumeSettings);
	child->applySlicerSettings(m_defaultSlicerSettings);
	connect(child, &MdiChild::closed, this, &MainWindow::childClosed);
	return child;
}

void MainWindow::closeMdiChild(iAMdiChild* child)
{
	if (!child)
	{
		return;
	}
	QMdiSubWindow* subWin = qobject_cast<QMdiSubWindow*>(child->parent());
	delete subWin;
}

void MainWindow::connectSignalsToSlots()
{
	// de-duplicate standard-pattern of forwarding events to the currently active mdi child:
	auto childCall = [this](auto method, auto... params) {
		if (activeMDI())
		{
			(activeMDI()->*method)(params...);
		}
	};
	// "File" menu entries:
	auto getOpenFileName = [this]() -> QString {
		return QFileDialog::getOpenFileName(this, tr("Open Files (new)"), m_path, iAFileTypeRegistry::registeredFileTypes(iAFileIO::Load));
	};
	connect(m_ui->actionOpenDataSet, &QAction::triggered, this, [this, getOpenFileName] { loadFileNew(getOpenFileName(), activeMdiChild()); });
	connect(m_ui->actionOpenInNewWindow, &QAction::triggered, this, [this, getOpenFileName] { loadFileNew(getOpenFileName(), nullptr); });
	connect(m_ui->actionOpenRaw, &QAction::triggered, this, &MainWindow::openRaw);
	connect(m_ui->actionOpenWithDataTypeConversion, &QAction::triggered, this, &MainWindow::openWithDataTypeConversion);
	connect(m_ui->actionOpenTLGICTData, &QAction::triggered, this, &MainWindow::openTLGICTData);
	connect(m_ui->actionSaveDataSet, &QAction::triggered, this, [childCall] { childCall(&MdiChild::save); });
	connect(m_ui->actionSaveProject, &QAction::triggered, this, &MainWindow::saveProject);
	connect(m_ui->actionSaveVolumeStack, &QAction::triggered, this, &MainWindow::saveVolumeStack);
	connect(m_ui->actionLoadSettings, &QAction::triggered, this, &MainWindow::loadSettings);
	connect(m_ui->actionSaveSettings, &QAction::triggered, this, &MainWindow::saveSettings);
	connect(m_ui->actionExit, &QAction::triggered, qApp, &QApplication::closeAllWindows);
	for (int i = 0; i < MaxRecentFiles; ++i)
	{
		connect(m_recentFileActs[i], &QAction::triggered, this, &MainWindow::openRecentFile);
	}

	// "Edit" menu entries:
	connect(m_ui->actionPreferences, &QAction::triggered, this, &MainWindow::prefs);
	connect(m_ui->actionRendererSettings, &QAction::triggered, this, &MainWindow::renderSettings);
	connect(m_ui->actionSlicerSettings, &QAction::triggered, this, &MainWindow::slicerSettings);
	connect(m_ui->actionInteractionModeRegistration, &QAction::triggered, this, &MainWindow::changeInteractionMode);
	connect(m_ui->actionInteractionModeCamera, &QAction::triggered, this, &MainWindow::changeInteractionMode);

	// "Views" menu entries:
	connect(m_ui->actionXY, &QAction::triggered, this, [childCall] { childCall(&MdiChild::maximizeSlicer, iASlicerMode::XY); });
	connect(m_ui->actionXZ, &QAction::triggered, this, [childCall] { childCall(&MdiChild::maximizeSlicer, iASlicerMode::XZ); });
	connect(m_ui->actionYZ, &QAction::triggered, this, [childCall] { childCall(&MdiChild::maximizeSlicer, iASlicerMode::YZ); });
	connect(m_ui->action3D, &QAction::triggered, this, [childCall] { childCall(&MdiChild::maximizeRenderer); } );
	connect(m_ui->actionMultiViews, &QAction::triggered, this, [childCall] { childCall(&MdiChild::multiview); });
	connect(m_ui->actionLinkViews, &QAction::triggered, this, &MainWindow::linkViews);
	connect(m_ui->actionLinkMdis, &QAction::triggered, this, &MainWindow::linkMDIs);
	connect(m_ui->actionToggleSlicerInteraction, &QAction::triggered, this, &MainWindow::toggleSlicerInteraction);
	connect(m_ui->actionToggleRendererInteraction, &QAction::triggered, this, [this, childCall] {
		childCall(&MdiChild::enableRendererInteraction, m_ui->actionToggleRendererInteraction->isChecked());
	});
	connect(m_ui->actionFullScreenMode, &QAction::triggered, this, &MainWindow::toggleFullScreen);
	connect(m_ui->actionShowMenu, &QAction::triggered, this, &MainWindow::toggleMenu);
	connect(m_ui->actionShowToolbar, &QAction::triggered, this, &MainWindow::toggleToolbar);
	connect(m_ui->menuDockWidgets, &QMenu::aboutToShow, this, &MainWindow::listDockWidgetsInMenu);
	// Enable these actions also when menu not visible:
	addAction(m_ui->actionFullScreenMode);
	addAction(m_ui->actionShowMenu);
	addAction(m_ui->actionShowToolbar);

	// "Window" menu entries:
	connect(m_ui->actionClose, &QAction::triggered, m_ui->mdiArea, &QMdiArea::closeActiveSubWindow);
	connect(m_ui->actionCloseAll, &QAction::triggered, this, &MainWindow::closeAllSubWindows);
	connect(m_ui->actionTile, &QAction::triggered, m_ui->mdiArea, &QMdiArea::tileSubWindows);
	connect(m_ui->actionCascade, &QAction::triggered, m_ui->mdiArea, &QMdiArea::cascadeSubWindows);
	connect(m_ui->actionNextWindow, &QAction::triggered, m_ui->mdiArea, &QMdiArea::activateNextSubWindow);
	connect(m_ui->actionPrevWindow, &QAction::triggered, m_ui->mdiArea, &QMdiArea::activatePreviousSubWindow);
	connect(m_ui->actionOpenLogOnNewMessage, &QAction::triggered, this, &MainWindow::toggleOpenLogOnNewMessage);
	connect(m_ui->actionOpenListOnAddedJob, &QAction::triggered, this, &MainWindow::toggleOpenListOnAddedJob);

	connect(m_ui->actionTabbed, &QAction::triggered, this, &MainWindow::toggleMdiViewMode);
	connect(m_ui->actionSubWindows, &QAction::triggered, this, &MainWindow::toggleMdiViewMode);

	// "Help" menu entries:
	connect(m_ui->actionUserGuideCore, &QAction::triggered, this, &MainWindow::wiki);
	connect(m_ui->actionUserGuideFilters, &QAction::triggered, this, &MainWindow::wiki);
	connect(m_ui->actionUserGuideTools, &QAction::triggered, this, &MainWindow::wiki);
	connect(m_ui->actionReleases, &QAction::triggered, this, &MainWindow::wiki);
	connect(m_ui->actionBug, &QAction::triggered, this, &MainWindow::wiki);
	connect(m_ui->actionAbout, &QAction::triggered, this, &MainWindow::about);

	// Renderer toolbar:
	connect(m_ui->actionViewXDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	m_ui->actionViewXDirectionInRaycaster->setProperty("camPosition", iACameraPosition::PX);
	connect(m_ui->actionViewmXDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	m_ui->actionViewmXDirectionInRaycaster->setProperty("camPosition", iACameraPosition::MX);
	connect(m_ui->actionViewYDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	m_ui->actionViewYDirectionInRaycaster->setProperty("camPosition", iACameraPosition::PY);
	connect(m_ui->actionViewmYDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	m_ui->actionViewmYDirectionInRaycaster->setProperty("camPosition", iACameraPosition::MY);
	connect(m_ui->actionViewZDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	m_ui->actionViewZDirectionInRaycaster->setProperty("camPosition", iACameraPosition::PZ);
	connect(m_ui->actionViewmZDirectionInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	m_ui->actionViewmZDirectionInRaycaster->setProperty("camPosition", iACameraPosition::MZ);
	connect(m_ui->actionIsometricViewInRaycaster, &QAction::triggered, this, &MainWindow::rendererCamPosition);
	m_ui->actionIsometricViewInRaycaster->setProperty("camPosition", iACameraPosition::Iso);

	// Camera toolbar:
	connect(m_ui->actionSyncCamera,   &QAction::triggered, this, &MainWindow::rendererSyncCamera);
	connect(m_ui->actionSaveCameraSettings, &QAction::triggered, this, &MainWindow::rendererSaveCameraSettings);
	connect(m_ui->actionLoadCameraSettings, &QAction::triggered, this, &MainWindow::rendererLoadCameraSettings);

	// Snake slicer toolbar
	connect(m_ui->actionSnakeSlicer, &QAction::toggled, this, [childCall](bool checked) { childCall(&MdiChild::toggleSnakeSlicer, checked); });
	connect(m_ui->actionRawProfile,   &QAction::toggled, this, [childCall](bool checked) { childCall(&MdiChild::toggleSliceProfile, checked); });
	connect(m_ui->actionEditProfilePoints, &QAction::toggled, this, [childCall](bool checked) { childCall(&MdiChild::toggleProfileHandles, checked); });
	connect(m_ui->actionMagicLens2D,  &QAction::toggled, this, [childCall](bool checked) { childCall(&MdiChild::toggleMagicLens2D, checked); });
	connect(m_ui->actionMagicLens3D,  &QAction::triggered, this, [childCall](bool checked) { childCall(&MdiChild::toggleMagicLens3D, checked); });

	// Layout toolbar menu entries
	connect(m_ui->actionSaveLayout,   &QAction::triggered, this, &MainWindow::saveLayout);
	connect(m_ui->actionLoadLayout,   &QAction::triggered, this, &MainWindow::loadLayout);
	connect(m_ui->actionDeleteLayout, &QAction::triggered, this, &MainWindow::deleteLayout);
	connect(m_ui->actionResetLayout,  &QAction::triggered, this, &MainWindow::resetLayout);

	connect(m_ui->mdiArea, &QMdiArea::subWindowActivated, this, &MainWindow::updateMenus);
	connect(m_ui->mdiArea, &QMdiArea::subWindowActivated, this, &iAMainWindow::childChanged);
}

void MainWindow::readSettings()
{
	QSettings settings;

	m_path = settings.value("Path").toString();
	m_qssName = settings.value("qssName", ":/bright.qss").toString();
	restoreGeometry(settings.value("geometry", saveGeometry()).toByteArray());
	restoreState(settings.value("state", saveState()).toByteArray());

	iAPreferences defaultPrefs;
	m_defaultLayout = settings.value("Preferences/defaultLayout", "").toString();
	m_defaultPreferences.HistogramBins = settings.value("Preferences/prefHistogramBins", defaultPrefs.HistogramBins).toInt();
	m_defaultPreferences.HistogramLogarithmicYAxis = settings.value("Preferences/prefHistogramLogarithmicYAxis", defaultPrefs.HistogramLogarithmicYAxis).toBool();
	m_defaultPreferences.LimitForAuto3DRender = settings.value("Preferences/prefLimitForAuto3DRender", defaultPrefs.LimitForAuto3DRender).toInt();
	m_defaultPreferences.StatisticalExtent = settings.value("Preferences/prefStatExt", defaultPrefs.StatisticalExtent).toInt();
	m_defaultPreferences.Compression = settings.value("Preferences/prefCompression", defaultPrefs.Compression).toBool();
	m_defaultPreferences.PrintParameters = settings.value("Preferences/prefPrintParameters", defaultPrefs.PrintParameters).toBool();
	m_defaultPreferences.ResultInNewWindow = settings.value("Preferences/prefResultInNewWindow", defaultPrefs.ResultInNewWindow).toBool();
	m_defaultPreferences.MagicLensSize = settings.value("Preferences/prefMagicLensSize", defaultPrefs.MagicLensSize).toInt();
	m_defaultPreferences.MagicLensFrameWidth = settings.value("Preferences/prefMagicLensFrameWidth", defaultPrefs.MagicLensFrameWidth).toInt();
	m_defaultPreferences.FontSize = settings.value("Preferences/fontSize", defaultPrefs.FontSize).toInt();
	bool prefLogToFile = settings.value("Preferences/prefLogToFile", false).toBool();
	QString logFileName = settings.value("Preferences/prefLogFile", "debug.log").toString();
	iALogWidget::get()->setLogToFile(prefLogToFile, logFileName);
	iALogWidget::get()->setLogLevel(static_cast<iALogLevel>(settings.value("Preferences/prefLogLevel", lvlInfo).toInt()));
	iALogWidget::get()->setFileLogLevel(static_cast<iALogLevel>(settings.value("Preferences/prefFileLogLevel", lvlWarn).toInt()));
	iALogWidget::get()->setLogVTK(settings.value("Preferences/prefLogVTK", true).toBool());
	iALogWidget::get()->setLogITK(settings.value("Preferences/prefLogITK", true).toBool());
	auto f = QApplication::font();
	f.setPointSize(m_defaultPreferences.FontSize);
	QApplication::setFont(f);

	iARenderSettings fallbackRS;
	m_defaultRenderSettings.ShowSlicers = settings.value("Renderer/rsShowSlicers", fallbackRS.ShowSlicers).toBool();
	m_defaultRenderSettings.ShowSlicePlanes = settings.value("Renderer/rsShowSlicePlanes", fallbackRS.ShowSlicePlanes).toBool();
	m_defaultRenderSettings.ShowHelpers = settings.value("Renderer/rsShowHelpers", fallbackRS.ShowHelpers).toBool();
	m_defaultRenderSettings.ShowRPosition = settings.value("Renderer/rsShowRPosition", fallbackRS.ShowRPosition).toBool();
	m_defaultRenderSettings.ParallelProjection = settings.value("Renderer/rsParallelProjection", fallbackRS.ParallelProjection).toBool();
	m_defaultRenderSettings.UseStyleBGColor = settings.value("Renderer/rsUseStyleBGColor", fallbackRS.UseStyleBGColor).toBool();
	m_defaultRenderSettings.UseFXAA = settings.value("Renderer/rsUseFXAA", fallbackRS.UseFXAA).toBool();
	m_defaultRenderSettings.MultiSamples = settings.value("Renderer/rsMultiSamples", fallbackRS.MultiSamples).toInt();
	m_defaultRenderSettings.PlaneOpacity = settings.value("Renderer/rsPlaneOpacity", fallbackRS.PlaneOpacity).toDouble();
	m_defaultRenderSettings.BackgroundTop = settings.value("Renderer/rsBackgroundTop", fallbackRS.BackgroundTop).toString();
	m_defaultRenderSettings.BackgroundBottom = settings.value("Renderer/rsBackgroundBottom", fallbackRS.BackgroundBottom).toString();
	m_defaultRenderSettings.UseDepthPeeling = settings.value("Renderer/rsUseDepthPeeling", fallbackRS.UseDepthPeeling).toBool();
	m_defaultRenderSettings.DepthPeels = settings.value("Renderer/rsDepthPeels", fallbackRS.DepthPeels).toInt();

	iAVolumeSettings fallbackVS;
	m_defaultVolumeSettings.LinearInterpolation = settings.value("Renderer/rsLinearInterpolation", fallbackVS.LinearInterpolation).toBool();
	m_defaultVolumeSettings.Shading = settings.value("Renderer/rsShading", fallbackVS.Shading).toBool();
	m_defaultVolumeSettings.SampleDistance = settings.value("Renderer/rsSampleDistance", fallbackVS.SampleDistance).toDouble();
	m_defaultVolumeSettings.AmbientLighting = settings.value("Renderer/rsAmbientLighting", fallbackVS.AmbientLighting).toDouble();
	m_defaultVolumeSettings.DiffuseLighting = settings.value("Renderer/rsDiffuseLighting", fallbackVS.DiffuseLighting).toDouble();
	m_defaultVolumeSettings.SpecularLighting = settings.value("Renderer/rsSpecularLighting", fallbackVS.SpecularLighting).toDouble();
	m_defaultVolumeSettings.SpecularPower = settings.value("Renderer/rsSpecularPower", fallbackVS.SpecularPower).toDouble();
	m_defaultVolumeSettings.RenderMode = settings.value("Renderer/rsRenderMode", fallbackVS.RenderMode).toInt();

	iASlicerSettings fallbackSS;
	m_defaultSlicerSettings.LinkViews = settings.value("Slicer/ssLinkViews", fallbackSS.LinkViews).toBool();
	m_defaultSlicerSettings.LinkMDIs = settings.value("Slicer/ssLinkMDIs", fallbackSS.LinkMDIs).toBool();
	m_defaultSlicerSettings.SnakeSlices = settings.value("Slicer/ssSnakeSlices", fallbackSS.SnakeSlices).toInt();
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		m_defaultSlicerSettings.BackgroundColor[s] = settings.value(QString("ssBgColor%1").arg(s), "").toString();
	}
	m_defaultSlicerSettings.SingleSlicer.ShowPosition = settings.value("Slicer/ssShowPosition", fallbackSS.SingleSlicer.ShowPosition).toBool();
	m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption = settings.value("Slicer/ssShowAxesCaption", fallbackSS.SingleSlicer.ShowAxesCaption).toBool();
	m_defaultSlicerSettings.SingleSlicer.ShowIsoLines = settings.value("Slicer/ssShowIsolines", fallbackSS.SingleSlicer.ShowIsoLines).toBool();
	m_defaultSlicerSettings.SingleSlicer.ShowTooltip = settings.value("Slicer/ssShowTooltip", fallbackSS.SingleSlicer.ShowTooltip).toBool();
	m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines = settings.value("Slicer/ssNumberOfIsolines", fallbackSS.SingleSlicer.NumberOfIsoLines).toDouble();
	m_defaultSlicerSettings.SingleSlicer.MinIsoValue = settings.value("Slicer/ssMinIsovalue", fallbackSS.SingleSlicer.MinIsoValue).toDouble();
	m_defaultSlicerSettings.SingleSlicer.MaxIsoValue = settings.value("Slicer/ssMaxIsovalue", fallbackSS.SingleSlicer.MaxIsoValue).toDouble();
	m_defaultSlicerSettings.SingleSlicer.LinearInterpolation = settings.value("Slicer/ssImageActorUseInterpolation", fallbackSS.SingleSlicer.LinearInterpolation).toBool();
	m_defaultSlicerSettings.SingleSlicer.AdjustWindowLevelEnabled = settings.value("Slicer/ssAdjustWindowLevelEnabled", fallbackSS.SingleSlicer.AdjustWindowLevelEnabled).toBool();
	m_defaultSlicerSettings.SingleSlicer.CursorMode = settings.value( "Slicer/ssCursorMode", fallbackSS.SingleSlicer.CursorMode).toString();
	m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize = settings.value("Slicer/toolTipFontSize", fallbackSS.SingleSlicer.ToolTipFontSize).toInt();

	m_lpCamera = settings.value("Parameters/lpCamera").toBool();
	m_lpSliceViews = settings.value("Parameters/lpSliceViews").toBool();
	m_lpPreferences = settings.value("Parameters/lpPreferences").toBool();
	m_lpRenderSettings = settings.value("Parameters/lpRenderSettings").toBool();
	m_lpSlicerSettings = settings.value("Parameters/lpSlicerSettings").toBool();

	m_spCamera = settings.value("Parameters/spCamera").toBool();
	m_spSliceViews = settings.value("Parameters/spSliceViews").toBool();
	m_spPreferences = settings.value("Parameters/spPreferences").toBool();
	m_spRenderSettings = settings.value("Parameters/spRenderSettings").toBool();
	m_spSlicerSettings = settings.value("Parameters/spSlicerSettings").toBool();

	bool showLog = settings.value("Parameters/ShowLog", false).toBool();
	iALogWidget::get()->toggleViewAction()->setChecked(showLog);
	iALogWidget::get()->setVisible(showLog);
	m_ui->actionOpenLogOnNewMessage->setChecked(settings.value("Parameters/OpenLogOnNewMessages", true).toBool());
	toggleOpenLogOnNewMessage();
	bool showJobs = settings.value("Parameters/ShowJobs", false).toBool();
	m_dwJobs->toggleViewAction()->setChecked(showJobs);
	m_dwJobs->setVisible(showJobs);
	m_ui->actionOpenListOnAddedJob->setChecked(settings.value("Parameters/OpenListOnAddedJob", true).toBool());
	toggleOpenListOnAddedJob();
	m_ui->actionShowToolbar->setChecked(settings.value("Parameters/ShowToolbar", true).toBool());
	toggleToolbar();
	auto viewMode = static_cast<QMdiArea::ViewMode>(settings.value("Parameters/ViewMode", QMdiArea::SubWindowView).toInt());
	m_ui->mdiArea->setViewMode(viewMode);
	if (viewMode == QMdiArea::SubWindowView)
	{
		m_ui->actionSubWindows->setChecked(true);
	}
	else
	{
		m_ui->actionTabbed->setChecked(true);
	}

	m_owdtcs = settings.value("OpenWithDataTypeConversion/owdtcs", 1).toInt();
	m_rawFileParams.m_size[0] = settings.value("OpenWithDataTypeConversion/owdtcx", 1).toInt();
	m_rawFileParams.m_size[1] = settings.value("OpenWithDataTypeConversion/owdtcy", 1).toInt();
	m_rawFileParams.m_size[2] = settings.value("OpenWithDataTypeConversion/owdtcz", 1).toInt();
	m_rawFileParams.m_spacing[0] = settings.value("OpenWithDataTypeConversion/owdtcsx", 1.0).toDouble();
	m_rawFileParams.m_spacing[1] = settings.value("OpenWithDataTypeConversion/owdtcsy", 1.0).toDouble();
	m_rawFileParams.m_spacing[2] = settings.value("OpenWithDataTypeConversion/owdtcsz", 1.0).toDouble();
	m_owdtcmin = settings.value("OpenWithDataTypeConversion/owdtcmin").toDouble();
	m_owdtcmax = settings.value("OpenWithDataTypeConversion/owdtcmax").toDouble();
	m_owdtcoutmin = settings.value("OpenWithDataTypeConversion/owdtcoutmin").toDouble();
	m_owdtcoutmax = settings.value("OpenWithDataTypeConversion/owdtcoutmax").toDouble();
	m_owdtcdov = settings.value("OpenWithDataTypeConversion/owdtcdov").toInt();
	m_owdtcxori = settings.value("OpenWithDataTypeConversion/owdtcxori").toInt();
	m_owdtcyori = settings.value("OpenWithDataTypeConversion/owdtcyori").toInt();
	m_owdtczori = settings.value("OpenWithDataTypeConversion/owdtczori").toInt();
	m_owdtcxsize = settings.value("OpenWithDataTypeConversion/owdtcxsize").toInt();
	m_owdtcysize = settings.value("OpenWithDataTypeConversion/owdtcysize").toInt();
	m_owdtczsize = settings.value("OpenWithDataTypeConversion/owdtczsize").toInt();

	settings.beginGroup("Layout");
	m_layoutNames = settings.allKeys();
	m_layoutNames = m_layoutNames.filter(QRegularExpression("^state"));
	m_layoutNames.replaceInStrings(QRegularExpression("^state"), "");
	settings.endGroup();
	if (m_layoutNames.size() == 0)
	{
		m_layoutNames.push_back("1");
		m_layoutNames.push_back("2");
		m_layoutNames.push_back("3");
	}
}

void MainWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("Path", m_path);
	settings.setValue("qssName", m_qssName);

	settings.setValue("Preferences/defaultLayout", m_layout->currentText());
	settings.setValue("Preferences/prefHistogramBins", m_defaultPreferences.HistogramBins);
	settings.setValue("Preferences/prefHistogramLogarithmicYAxis", m_defaultPreferences.HistogramLogarithmicYAxis);
	settings.setValue("Preferences/prefLimitForAuto3DRender", m_defaultPreferences.LimitForAuto3DRender);
	settings.setValue("Preferences/prefStatExt", m_defaultPreferences.StatisticalExtent);
	settings.setValue("Preferences/prefCompression", m_defaultPreferences.Compression);
	settings.setValue("Preferences/prefPrintParameters", m_defaultPreferences.PrintParameters);
	settings.setValue("Preferences/prefResultInNewWindow", m_defaultPreferences.ResultInNewWindow);
	settings.setValue("Preferences/prefMagicLensSize", m_defaultPreferences.MagicLensSize);
	settings.setValue("Preferences/prefMagicLensFrameWidth", m_defaultPreferences.MagicLensFrameWidth);
	settings.setValue("Preferences/fontSize", m_defaultPreferences.FontSize);

	settings.setValue("Preferences/prefLogToFile", iALogWidget::get()->isLogToFileOn());
	settings.setValue("Preferences/prefLogFile", iALogWidget::get()->logFileName());
	settings.setValue("Preferences/prefLogLevel", iALogWidget::get()->logLevel());
	settings.setValue("Preferences/prefLogVTK", iALogWidget::get()->logVTK());
	settings.setValue("Preferences/prefLogITK", iALogWidget::get()->logITK());
	settings.setValue("Preferences/prefFileLogLevel", iALogWidget::get()->fileLogLevel());

	settings.setValue("Renderer/rsShowSlicers", m_defaultRenderSettings.ShowSlicers);
	settings.setValue("Renderer/rsShowSlicePlanes", m_defaultRenderSettings.ShowSlicePlanes);
	settings.setValue("Renderer/rsParallelProjection", m_defaultRenderSettings.ParallelProjection);
	settings.setValue("Renderer/rsUseStyleBGColor", m_defaultRenderSettings.UseStyleBGColor);
	settings.setValue("Renderer/rsBackgroundTop", m_defaultRenderSettings.BackgroundTop);
	settings.setValue("Renderer/rsBackgroundBottom", m_defaultRenderSettings.BackgroundBottom);
	settings.setValue("Renderer/rsShowHelpers", m_defaultRenderSettings.ShowHelpers);
	settings.setValue("Renderer/rsShowRPosition", m_defaultRenderSettings.ShowRPosition);
	settings.setValue("Renderer/rsUseFXAA", m_defaultRenderSettings.UseFXAA);
	settings.setValue("Renderer/rsMultiSamples", m_defaultRenderSettings.MultiSamples);
	settings.setValue("Renderer/rsPlaneOpacity", m_defaultRenderSettings.PlaneOpacity);
	settings.setValue("Renderer/rsUseDepthPeeling", m_defaultRenderSettings.UseDepthPeeling);
	settings.setValue("Renderer/rsDepthPeels", m_defaultRenderSettings.DepthPeels);

	settings.setValue("Renderer/rsLinearInterpolation", m_defaultVolumeSettings.LinearInterpolation);
	settings.setValue("Renderer/rsShading", m_defaultVolumeSettings.Shading);
	settings.setValue("Renderer/rsSampleDistance", m_defaultVolumeSettings.SampleDistance);
	settings.setValue("Renderer/rsAmbientLighting", m_defaultVolumeSettings.AmbientLighting);
	settings.setValue("Renderer/rsDiffuseLighting", m_defaultVolumeSettings.DiffuseLighting);
	settings.setValue("Renderer/rsSpecularLighting", m_defaultVolumeSettings.SpecularLighting);
	settings.setValue("Renderer/rsSpecularPower", m_defaultVolumeSettings.SpecularPower);
	settings.setValue("Renderer/rsRenderMode", m_defaultVolumeSettings.RenderMode);

	settings.setValue("Slicer/ssLinkViews", m_defaultSlicerSettings.LinkViews);
	settings.setValue("Slicer/ssLinkMDIs", m_defaultSlicerSettings.LinkMDIs);
	settings.setValue("Slicer/ssSnakeSlices", m_defaultSlicerSettings.SnakeSlices);
	for (int s = 0; s < iASlicerMode::SlicerCount; ++s)
	{
		settings.setValue(QString("ssBgColor%1").arg(s), m_defaultSlicerSettings.BackgroundColor[s]);
	}
	settings.setValue("Slicer/ssShowPosition", m_defaultSlicerSettings.SingleSlicer.ShowPosition);
	settings.setValue("Slicer/ssShowAxesCaption", m_defaultSlicerSettings.SingleSlicer.ShowAxesCaption);
	settings.setValue("Slicer/ssShowIsolines", m_defaultSlicerSettings.SingleSlicer.ShowIsoLines);
	settings.setValue("Slicer/ssShowTooltip", m_defaultSlicerSettings.SingleSlicer.ShowTooltip);
	settings.setValue("Slicer/ssNumberOfIsolines", m_defaultSlicerSettings.SingleSlicer.NumberOfIsoLines);
	settings.setValue("Slicer/ssMinIsovalue", m_defaultSlicerSettings.SingleSlicer.MinIsoValue);
	settings.setValue("Slicer/ssMaxIsovalue", m_defaultSlicerSettings.SingleSlicer.MaxIsoValue);
	settings.setValue("Slicer/ssImageActorUseInterpolation", m_defaultSlicerSettings.SingleSlicer.LinearInterpolation);
	settings.setValue("Slicer/ssAdjustWindowLevelEnabled", m_defaultSlicerSettings.SingleSlicer.AdjustWindowLevelEnabled);
	settings.setValue("Slicer/ssCursorMode", m_defaultSlicerSettings.SingleSlicer.CursorMode);
	settings.setValue("Slicer/toolTipFontSize", m_defaultSlicerSettings.SingleSlicer.ToolTipFontSize);

	settings.setValue("Parameters/lpCamera", m_lpCamera);
	settings.setValue("Parameters/lpSliceViews", m_lpSliceViews);
	settings.setValue("Parameters/lpPreferences", m_lpPreferences);
	settings.setValue("Parameters/lpRenderSettings", m_lpRenderSettings);
	settings.setValue("Parameters/lpSlicerSettings", m_lpSlicerSettings);

	settings.setValue("Parameters/spCamera", m_spCamera);
	settings.setValue("Parameters/spSliceViews", m_spSliceViews);
	settings.setValue("Parameters/spPreferences", m_spPreferences);
	settings.setValue("Parameters/spRenderSettings", m_spRenderSettings);
	settings.setValue("Parameters/spSlicerSettings", m_spSlicerSettings);

	settings.setValue("Parameters/ShowLog", iALogWidget::get()->toggleViewAction()->isChecked());
	settings.setValue("Parameters/ShowJobs", m_dwJobs->toggleViewAction()->isChecked());
	settings.setValue("Parameters/ViewMode", m_ui->mdiArea->viewMode());
	settings.setValue("Parameters/ShowToolbar", m_ui->actionShowToolbar->isChecked());
	settings.setValue("Parameters/OpenLogOnNewMessages", m_ui->actionOpenLogOnNewMessage->isChecked());
	settings.setValue("Parameters/OpenListOnAddedJob", m_ui->actionOpenListOnAddedJob->isChecked());

	settings.setValue("OpenWithDataTypeConversion/owdtcs", m_owdtcs);
	settings.setValue("OpenWithDataTypeConversion/owdtcx", m_rawFileParams.m_size[0]);
	settings.setValue("OpenWithDataTypeConversion/owdtcy", m_rawFileParams.m_size[1]);
	settings.setValue("OpenWithDataTypeConversion/owdtcz", m_rawFileParams.m_size[2]);
	settings.setValue("OpenWithDataTypeConversion/owdtcsx", m_rawFileParams.m_spacing[0]);
	settings.setValue("OpenWithDataTypeConversion/owdtcsy", m_rawFileParams.m_spacing[1]);
	settings.setValue("OpenWithDataTypeConversion/owdtcsz", m_rawFileParams.m_spacing[2]);
	settings.setValue("OpenWithDataTypeConversion/owdtcmin", m_owdtcmin);
	settings.setValue("OpenWithDataTypeConversion/owdtcmax", m_owdtcmax);
	settings.setValue("OpenWithDataTypeConversion/owdtcoutmin", m_owdtcoutmin);
	settings.setValue("OpenWithDataTypeConversion/owdtcoutmax", m_owdtcoutmax);
	settings.setValue("OpenWithDataTypeConversion/owdtcdov", m_owdtcdov);
	settings.setValue("OpenWithDataTypeConversion/owdtcxori", m_owdtcxori);
	settings.setValue("OpenWithDataTypeConversion/owdtcyori", m_owdtcyori);
	settings.setValue("OpenWithDataTypeConversion/owdtczori", m_owdtczori);
	settings.setValue("OpenWithDataTypeConversion/owdtcxsize", m_owdtcxsize);
	settings.setValue("OpenWithDataTypeConversion/owdtcysize", m_owdtcysize);
	settings.setValue("OpenWithDataTypeConversion/owdtczsize", m_owdtczsize);
}

void MainWindow::addRecentFile(const QString &fileName)
{
	if (fileName.isEmpty())
	{
		LOG(lvlWarn, "Can't add empty filename as recent!");
		return;
	}
	setPath(QFileInfo(fileName).canonicalPath());

	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);

	while (files.size() > MaxRecentFiles)
	{
		files.removeLast();
	}
	settings.setValue("recentFileList", files);

	updateRecentFileActions();
}

void MainWindow::setPath(QString const & p)
{
	m_path = p;
	QDir::setCurrent(m_path);
}

QString const& MainWindow::path() const
{
	return m_path;
}

void MainWindow::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	QMutableStringListIterator it(files);
	while (it.hasNext())
	{
		QString fileName = it.next();
		QFileInfo fi(fileName);
		if (!fi.exists())
		{
			it.remove();
		}
	}
	settings.setValue("recentFileList", files);

	int numRecentFiles = qMin(files.size(), MaxRecentFiles);

	for (int i = 0; i < numRecentFiles; ++i)
	{
		QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
		m_recentFileActs[i]->setText(text);
		m_recentFileActs[i]->setData(files[i]);
		m_recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
	{
		m_recentFileActs[j]->setVisible(false);
	}

	m_separatorAct->setVisible(numRecentFiles > 0);
}

iAMdiChild* MainWindow::activeMdiChild()
{
	return activeMDI();
}

MdiChild* MainWindow::activeMDI()
{
	return activeChild<MdiChild>();
}

iAMdiChild * MainWindow::secondNonActiveChild()
{
	QList<iAMdiChild *> mdiwindows = mdiChildList();
	if (mdiwindows.size() > 2)
	{
		QMessageBox::warning(this, tr("Warning"),
			tr("Only two datasets can be processed at a time! Please close %1 datasets")
			.arg(mdiwindows.size() - 2));
		return nullptr;
	}
	else if (mdiwindows.size() < 2)
	{
		QMessageBox::warning(this, tr("Warning"),
			tr("Not enough datasets available! Please open exactly two datasets"));
		return nullptr;
	}
	return activeMdiChild() == mdiwindows.at(0) ?
		mdiwindows.at(1) : mdiwindows.at(0);
}

void MainWindow::setActiveSubWindow(QWidget *window)
{
	if (!window)
	{
		return;
	}
	m_ui->mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow*>(window));
}

void MainWindow::toggleMdiViewMode()
{
	auto action = qobject_cast<QAction*>(sender());
	auto viewMode = action == m_ui->actionTabbed ? QMdiArea::TabbedView : QMdiArea::SubWindowView;
	m_ui->actionTile->setEnabled(m_ui->actionSubWindows->isChecked());
	m_ui->actionCascade->setEnabled(m_ui->actionSubWindows->isChecked());
	m_ui->mdiArea->setViewMode(viewMode);
}

QList<iAMdiChild*> MainWindow::mdiChildList()
{
	return childList<iAMdiChild>(QMdiArea::CreationOrder);
}

void MainWindow::applyQSS()
{
	// Load an application style
	QFile styleFile(m_qssName);
	if (styleFile.open( QFile::ReadOnly ))
	{
		QTextStream styleIn(&styleFile);
		QString style = styleIn.readAll();
		styleFile.close();
		qApp->setStyleSheet(style);

		QPalette p = QApplication::palette();
		p.setColor(QPalette::Window,          m_qssName.contains("bright") ? QColor(255, 255, 255) : QColor(  0,   0,   0));
		p.setColor(QPalette::Base,            m_qssName.contains("bright") ? QColor(255, 255, 255) : QColor(  0,   0,   0));
		p.setColor(QPalette::ToolTipBase,     m_qssName.contains("bright") ? QColor(255, 255, 255) : QColor(  0,   0,   0));
		p.setColor(QPalette::Light,           m_qssName.contains("bright") ? QColor(255, 255, 255) : QColor(  0,   0,   0));
		p.setColor(QPalette::Midlight,        m_qssName.contains("bright") ? QColor(240, 240, 240) : QColor( 15,  15,  15));
		p.setColor(QPalette::AlternateBase,   m_qssName.contains("bright") ? QColor(240, 240, 240) : QColor( 30,  30,  30)); // dark seems (to me, BF) to need a bit more contrast to be visible well
		p.setColor(QPalette::Button,          m_qssName.contains("bright") ? QColor(215, 215, 215) : QColor( 40,  40,  40));
		p.setColor(QPalette::Mid,             m_qssName.contains("bright") ? QColor(200, 200, 200) : QColor( 55,  55,  55));
		p.setColor(QPalette::Dark,            m_qssName.contains("bright") ? QColor(180, 180, 180) : QColor( 75,  75,  75));
		p.setColor(QPalette::Shadow,          m_qssName.contains("bright") ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		//p.setColor(QPalette::Highlight,       m_qssName.contains("bright") ? QColor(  0,   0,   0) : QColor(255, 255, 255));		// TODO: determine proper highlight colors
		p.setColor(QPalette::HighlightedText, m_qssName.contains("bright") ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		p.setColor(QPalette::Text,            m_qssName.contains("bright") ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		p.setColor(QPalette::ToolTipText,     m_qssName.contains("bright") ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		p.setColor(QPalette::PlaceholderText, m_qssName.contains("bright") ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		p.setColor(QPalette::WindowText,      m_qssName.contains("bright") ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		QApplication::setPalette(p);
		
		// adapt action icons:
		static std::vector<std::pair<QAction*, QString>> actionIcons = {
			{ m_ui->actionViewXDirectionInRaycaster, "px" },
			{ m_ui->actionViewYDirectionInRaycaster, "py" },
			{ m_ui->actionViewZDirectionInRaycaster, "pz" },
			{ m_ui->actionViewmXDirectionInRaycaster, "mx" },
			{ m_ui->actionViewmYDirectionInRaycaster, "my" },
			{ m_ui->actionViewmZDirectionInRaycaster, "mz" },
			{ m_ui->actionIsometricViewInRaycaster, "iso" },
			{ m_ui->actionMagicLens2D, "magic_lens_2d" },
			{ m_ui->actionMagicLens3D, "magic_lens_3d" },
			{ m_ui->actionRendererSettings, "settings_renderer" },
			{ m_ui->actionSlicerSettings, "settings_slicer" },
			{ m_ui->actionPreferences, "settings" },
			{ m_ui->actionOpenWithDataTypeConversion, "open" },
			{ m_ui->actionOpenVolumeStack, "open" },
			{ m_ui->actionOpenTLGICTData, "open" },
			{ m_ui->actionOpenRaw, "open" },
			{ m_ui->actionOpenDataSet, "open" },
			{ m_ui->actionOpenInNewWindow, "open" },
			{ m_ui->actionSaveDataSet, "save" },
			{ m_ui->actionSaveVolumeStack, "save-all" },
			{ m_ui->actionSaveProject, "save-all" },
			{ m_ui->actionEditProfilePoints, "profile-edit" },
			{ m_ui->actionRawProfile, "profile-raw" },
			{ m_ui->actionSnakeSlicer, "snakeslicer" },
			{ m_ui->actionSyncCamera, "camera-sync" },
			{ m_ui->actionInteractionModeCamera, "camera" },
			{ m_ui->actionLoadCameraSettings, "camera-load" },
			{ m_ui->actionSaveCameraSettings, "camera-save" },
			{ m_ui->actionSaveLayout, "layout_export" },
			{ m_ui->actionLoadLayout, "layout_load" },
			{ m_ui->actionResetLayout, "layout_reset" },
			{ m_ui->actionDeleteLayout, "layout_delete" },
			{ m_ui->actionInteractionModeRegistration, "transform-move" }

		};
		for (auto a : actionIcons)
		{
			a.first->setIcon(resourceIcon(a.second));
		}
		emit styleChanged();
	}
}

bool MainWindow::brightMode() const
{
	return m_qssName.contains("bright");
}

void MainWindow::saveLayout()
{
	iAMdiChild *child = activeMdiChild();
	if (!child)
	{
		return;
	}
	QByteArray state = child->saveState(0);
	QSettings settings;
	QString layoutName(m_layout->currentText());
	iAAttributes params;
	addAttr(params, "Layout Name:", iAValueType::String, layoutName);
	iAParameterDlg dlg(this, "Layout Name", params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	layoutName = dlg.parameterValues()["Layout Name:"].toString();
	if (layoutName == "")
	{
		QMessageBox::warning(this, "Save Layout", "Layout Name cannot be empty!");
		return;
	}
	if (m_layout->findText(layoutName) == -1)
	{
		m_layout->addItem(layoutName);
	}
	else
	{
		if (QMessageBox::question(this, "Save Layout",
				"Do you want to overwrite the existing layout with this name?") != QMessageBox::Yes)
		{
			return;
		}
	}
	settings.setValue( "Layout/state" + layoutName, state );
	m_layout->setCurrentIndex(m_layout->findText(layoutName));
}

void MainWindow::loadLayout()
{
	MdiChild* child = activeMDI();
	assert(child);
	if (!child)
	{
		return;
	}
	child->loadLayout(m_layout->currentText());
}

void MainWindow::deleteLayout()
{
	if (QMessageBox::question(this, "Delete Layout",
		QString("Do you want to delete the layout '")+ m_layout->currentText()+"'?")
		== QMessageBox::Yes)
	{
		QSettings settings;
		settings.remove("Layout/state" + m_layout->currentText());
		m_layout->removeItem(m_layout->currentIndex());
	}
}

void MainWindow::resetLayout()
{
	activeMDI()->resetLayout();
}

QMenu * MainWindow::fileMenu()
{
	return m_ui->menuFile;
}

QMenu * MainWindow::filtersMenu()
{
	return m_ui->menuFilters;
}

QMenu * MainWindow::toolsMenu()
{
	return m_ui->menuTools;
}

QMenu * MainWindow::helpMenu()
{
	return m_ui->menuHelp;
}

void MainWindow::toggleOpenLogOnNewMessage()
{
	iALogWidget::get()->setOpenOnNewMessage(m_ui->actionOpenLogOnNewMessage->isChecked());
}

void MainWindow::toggleOpenListOnAddedJob()
{
	if (m_openJobListOnNewJob == m_ui->actionOpenListOnAddedJob->isChecked())
	{	// no change
		return;
	}
	m_openJobListOnNewJob = m_ui->actionOpenListOnAddedJob->isChecked();
	if (m_openJobListOnNewJob)
	{
		connect(iAJobListView::get(), &iAJobListView::jobAdded, m_dwJobs, &QWidget::show);
		connect(iAJobListView::get(), &iAJobListView::allJobsDone, m_dwJobs, &QWidget::hide);
	}
	else
	{
		disconnect(iAJobListView::get(), &iAJobListView::jobAdded, m_dwJobs, nullptr);
	}
}

void MainWindow::listDockWidgetsInMenu()
{
	m_ui->menuDockWidgets->clear();
	if (activeChild())
	{
		for (auto dockWidget : activeChild()->findChildren<QDockWidget*>())
		{
			addToMenuSorted(m_ui->menuDockWidgets, dockWidget->toggleViewAction());
		}
	}
}

void MainWindow::toggleToolbar()
{
	bool visible = m_ui->actionShowToolbar->isChecked();
	QList<QToolBar *> toolbars = findChildren<QToolBar *>();
	for (auto toolbar : toolbars)
	{
		toolbar->setVisible(visible);
	}
}

QMdiSubWindow* MainWindow::activeChild()
{
	return m_ui->mdiArea->currentSubWindow();
}

QMdiSubWindow* MainWindow::addSubWindow( QWidget * child )
{
	return m_ui->mdiArea->addSubWindow(child);
}

void MainWindow::makeActionChildDependent(QAction* action)
{
	m_childDependentActions.push_back(action);
}

void MainWindow::setModuleActionsEnabled( bool isEnabled )
{
	for (int i = 0; i < m_childDependentActions.size(); ++i)
	{
		m_childDependentActions[i]->setEnabled(isEnabled);
		QMenu* actMenu = m_childDependentActions[i]->menu();
		if (actMenu)
		{
			actMenu->setEnabled(isEnabled);
		}
	}
}

void MainWindow::childClosed()
{
	auto sender = dynamic_cast<MdiChild*>(QObject::sender());
	if (!sender)
	{
		return;
	}
	// magic lens size can be modified in the slicers as well; make sure to store this change:
	m_defaultPreferences.MagicLensSize = sender->magicLensSize();
	/*
	if( mdiArea->subWindowList().size() == 1 )
	{
		MdiChild * child = dynamic_cast<MdiChild*> ( mdiArea->subWindowList().at( 0 )->widget() );
		if (!child)
		{
			return;
		}
		if (child == sender)
		{
			setModuleActionsEnabled(false);
		}
	}
	*/
}

void MainWindow::saveProject()
{
	iASavableProject * child = activeChild<iASavableProject>();
	if (!child)
	{
		return;
	}
	if (!child->saveProject(m_path))
	{
		QMessageBox::warning(this, "Save Project", "Saving project was aborted!");
	}
	else
	{
		addRecentFile(child->fileName());
	}
}

void MainWindow::saveVolumeStack()
{
	if (!activeMDI())
	{
		return;
	}
	activeMDI()->saveVolumeStack();
}

void MainWindow::loadArguments(int argc, char** argv)
{
	QStringList files;
	for (int a = 1; a < argc; ++a)
	{
		if (QString(argv[a]).startsWith("--quit"))
		{
			++a;
			bool ok;
			quint64 ms = QString(argv[a]).toULongLong(&ok);
			if (ok)
			{
				m_quitTimer = new QTimer();
				m_quitTimer->setSingleShot(true);
				connect(m_quitTimer, &QTimer::timeout, this, &MainWindow::quitTimerSlot);
				m_quitTimer->start(ms);
			}
			else
			{
				LOG(lvlWarn, "Invalid --quit parameter; must be followed by an integer number (milliseconds) after which to quit, e.g. '--quit 1000'");
			}
		}
		else
		{
			files << QString::fromLocal8Bit(argv[a]);
		}
	}
	loadFiles(files);
}

iAPreferences const & MainWindow::defaultPreferences() const
{
	return m_defaultPreferences;
}

iARenderSettings const& MainWindow::defaultRenderSettings() const
{
	return m_defaultRenderSettings;
}

iAVolumeSettings const& MainWindow::defaultVolumeSettings() const
{
	return m_defaultVolumeSettings;
}

iAModuleDispatcher & MainWindow::moduleDispatcher() const
{
	return *this->m_moduleDispatcher.data();
}


// Move to other places (modules?):

void MainWindow::openWithDataTypeConversion()
{
	iARawFileIO io;
	QString fileName = QFileDialog::getOpenFileName(this,
		tr("Open File"),
		m_path,
		io.filterString()
	);
	if (fileName.isEmpty())
	{
		return;
	}
	iAAttributes params;
	addAttr(params, "Slice sample rate", iAValueType::Discrete, m_owdtcs, 1);
	auto map = rawParamsToMap(m_rawFileParams);
	iARawFileParamDlg dlg(fileName, this, "Open With DataType Conversion", params, map, brightMode());
	if (!dlg.accepted())
	{
		return;
	}
	m_rawFileParams = rawParamsFromMap(dlg.parameterValues());
	m_owdtcs = clamp(1u, m_rawFileParams.m_size[2], static_cast<unsigned int>(dlg.parameterValues()["Slice sample rate"].toUInt()));

	double convPara[11];
	convPara[0] = m_owdtcmin;   convPara[1] = m_owdtcmax;  convPara[2] = m_owdtcoutmin; convPara[3] = m_owdtcoutmax; convPara[4] =  m_owdtcdov; convPara[5] = m_owdtcxori;
	convPara[6] = m_owdtcxsize; convPara[7] = m_owdtcyori; convPara[8] = m_owdtcysize;  convPara[9] = m_owdtczori;   convPara[10] = m_owdtczsize;
	try
	{
		dlg_datatypeconversion conversionwidget(this, fileName, m_rawFileParams,
			m_owdtcs, m_defaultPreferences.HistogramBins, convPara);
		if (conversionwidget.exec() != QDialog::Accepted)
		{
			return;
		}

		QString outDataType = conversionwidget.getDataType();
		m_owdtcmin = conversionwidget.getRangeLower();   m_owdtcmax = conversionwidget.getRangeUpper();
		m_owdtcoutmin = conversionwidget.getOutputMin(); m_owdtcoutmax = conversionwidget.getOutputMax();
		m_owdtcdov = conversionwidget.getConvertROI();
		m_owdtcxori = conversionwidget.getXOrigin(); m_owdtcxsize = conversionwidget.getXSize();
		m_owdtcyori = conversionwidget.getYOrigin(); m_owdtcysize = conversionwidget.getYSize();
		m_owdtczori = conversionwidget.getZOrigin(); m_owdtczsize = conversionwidget.getZSize();

		double roi[6];
		roi[0] = m_owdtcxori; roi[1] = m_owdtcxsize;
		roi[2] = m_owdtcyori; roi[3] = m_owdtcysize;
		roi[4] = m_owdtczori; roi[5] = m_owdtczsize;

		QString finalfilename;
		if (m_owdtcdov == 0)
		{
			finalfilename = conversionwidget.convert(fileName,
				mapReadableDataTypeToVTKType(outDataType),
				m_owdtcmin, m_owdtcmax, m_owdtcoutmin, m_owdtcoutmax);
		}
		else
		{
			finalfilename = conversionwidget.convertROI(fileName, m_rawFileParams,
				mapReadableDataTypeToVTKType(outDataType),
				m_owdtcmin, m_owdtcmax, m_owdtcoutmin, m_owdtcoutmax, roi);
		}
		loadFileAskNewWindow(finalfilename);
	}
	catch (std::exception & e)
	{
		LOG(lvlError, QString("Open with datatype conversion: %1").arg(e.what()));
	}
}

void MainWindow::openTLGICTData()
{
	QString baseDirectory = QFileDialog::getExistingDirectory(
		this,
		tr("Open Talbot-Lau Grating Interferometer CT Dataset"),
		path(),
		QFileDialog::ShowDirsOnly);
	loadTLGICTData(baseDirectory);
}

void MainWindow::loadTLGICTData(QString const & baseDirectory)
{
	iATLGICTLoader* tlgictLoader = new iATLGICTLoader();
	if (!tlgictLoader->setup(baseDirectory, this))
	{
		return;
	}
	tlgictLoader->start(dynamic_cast<MdiChild*>(createMdiChild(false)));
	// tlgictLoader will delete itself when finished!
}

#include "iALog.h"
#include "iASCIFIOCheck.h"

#include <QApplication>
#include <QDate>
#include <QProxyStyle>

class iAProxyStyle : public QProxyStyle
{
public:
	using QProxyStyle::QProxyStyle;

	int styleHint(StyleHint hint, const QStyleOption* option = nullptr, const QWidget* widget = nullptr, QStyleHintReturn* returnData = nullptr) const override
	{
		// disable tooltip delay for iAChartWidget and descendants:
		if (hint == QStyle::SH_ToolTip_WakeUpDelay && widget && widget->inherits(iAChartWidget::staticMetaObject.className()))
		{
			return 0;
		}
		return QProxyStyle::styleHint(hint, option, widget, returnData);
	}
};

void MainWindow::initResources()
{
	Q_INIT_RESOURCE(gui);
}

int MainWindow::runGUI(int argc, char * argv[], QString const & appName, QString const & version,
	QString const& buildInformation, QString const & splashPath, QString const & iconPath)
{
	iAFileParamDlg::setupDefaultFileParamDlgs();
	initializeSettingTypes();
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	iAFileTypeRegistry::addDefaultExtension(iADataSetType::Volume, "mhd");
	iAFileTypeRegistry::addDefaultExtension(iADataSetType::Mesh, "stl");
	//iAFileTypeRegistry::addDefaultExtension(iADataSetType::Graph, ".txt");  -> graph storing not yet implemented!
	iAFileTypeRegistry::addDefaultExtension(iADataSetType::Collection, "iaproj");
#if defined(__APPLE__) && defined(__MACH__)
	QSurfaceFormat::setDefaultFormat(iAVtkWidget::defaultFormat());
#endif
	MainWindow::initResources();
	QApplication app(argc, argv);
	QString msg;
	if (!checkOpenGLVersion(msg))
	{
		bool runningScripted = false;
		for (int a = 1; a < argc; ++a)
		{
			if (QString(argv[a]).startsWith("--quit"))
			{
				runningScripted = true;
				break;
			}
		}
		if (runningScripted)
		{
			LOG(lvlWarn, msg);
		}
		else
		{
			QMessageBox::warning(nullptr, appName, msg);
		}
		return 1;
	}
	app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
	iALog::setLogger(iALogWidget::get());
	iALUT::loadMaps(QCoreApplication::applicationDirPath() + "/colormaps");
	auto dwJobs = new iADockWidgetWrapper(iAJobListView::get(), "Job List", "Jobs");
	MainWindow mainWin(appName, version, buildInformation, splashPath, dwJobs);
	mainWin.addDockWidget(Qt::RightDockWidgetArea, iALogWidget::get());
	mainWin.splitDockWidget(iALogWidget::get(), dwJobs, Qt::Vertical);
	dwJobs->setFeatures(dwJobs->features() & ~QDockWidget::DockWidgetVerticalTitleBar);
	CheckSCIFIO(QCoreApplication::applicationDirPath());
	mainWin.loadArguments(argc, argv);
	app.setWindowIcon(QIcon(QPixmap(iconPath)));
	QApplication::setStyle(new iAProxyStyle(QApplication::style()));
	mainWin.setWindowIcon(QIcon(QPixmap(iconPath)));
	if (QDate::currentDate().dayOfYear() >= 350)
	{
		mainWin.setWindowTitle("Merry Christmas and a Happy New Year!");
		mainWin.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
		app.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
	}
	mainWin.show();
	return app.exec();
}
