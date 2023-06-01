// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mainwindow.h"

// gui
#include "dlg_datatypeconversion.h"
#include "iACheckOpenGL.h"
#include "iAFileParamDlg.h"
#include "iALogWidget.h"
#include "iASystemThemeWatcher.h"
#include "iATLGICTLoader.h"
#include "mdichild.h"
#include "ui_Mainwindow.h"

// guibase
#include <iADefaultSettings.h>
#include <iAJobListView.h>
#include <iAModuleDispatcher.h>
#include <iAParameterDlg.h>
#include <iAQMenuHelper.h>
#include <iARawFileParamDlg.h>
#include <iARenderer.h>
#include <iASavableProject.h>
#include <iASlicer.h>        // for iASlicer and slicerModeString
#include <iAThemeHelper.h>

// io:
#include <iADataSet.h>
#include <iAFileStackParams.h>
#include <iAFileTypeRegistry.h>
#include <iARawFileIO.h>

// charts:
#include <iAChartWidget.h>

// qthelper
#include <iADockWidgetWrapper.h>

// base
#include <iAAttributes.h>    // for loading/storing default settings in XML
#include <iALog.h>
#include <iALogLevelMappings.h>
#include <iALUT.h>           // for iALUT::loadMaps
#include <iAMathUtility.h>
#include <iAProgress.h>
#include <iASettings.h>      // for loadSettings, storeSettings
#include <iAStringHelper.h>  // for iAConverter
#include <iAToolsVTK.h>
#include <iAXmlSettings.h>

#include <vtkCamera.h>
#include <vtkRenderer.h>

#include <QActionGroup>
#include <QCloseEvent>
#include <QDate>
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

namespace
{
	const QString DarkThemeQss(":/dark.qss");
	const QString BrightThemeQss(":/bright.qss");
	const QString SystemTheme("system");
	const QString qssNameFromSystem()
	{
		return iASystemThemeWatcher::isBrightTheme() ? BrightThemeQss : DarkThemeQss;
	}
	const QString ProjectFileExtension("iaproj"); // TODO: reuse in iAProjectFileIO

	constexpr const char RendererNiceName[] = "3D Renderer";
	constexpr const char RendererElemName[] = "renderer";
	constexpr const char PrefElemName[] = "preferences";
	constexpr const char PrefNiceName[] = "Preferences";
	constexpr const char SlicerElemName[] = "slicerSettings";
	constexpr const char SlicerNiceName[] = "Slicer Settings";
	constexpr const char XMLFileFilter[] = "XML (*.xml)";

	QString slicerNiceName(int m)
	{
		return QString("Slicer %1").arg(slicerModeString(m));
	}

	QString slicerElemName(int m)
	{
		return QString("slicer%1").arg(slicerModeString(m));
	}

	void saveCamera(QDomElement& element, vtkCamera* camera)
	{
		double position[3], focalPoint[3], viewUp[3];
		camera->GetPosition(position);
		camera->GetFocalPoint(focalPoint);
		camera->GetViewUp(viewUp);
		element.setAttribute("position", arrayToString(position, 3, ","));
		element.setAttribute("focalPoint", arrayToString(focalPoint, 3, ","));
		element.setAttribute("viewUp", arrayToString(viewUp, 3, ","));
		if (camera->GetParallelProjection())
		{
			double scale = camera->GetParallelScale();
			element.setAttribute("scale", scale);
		}
	}

	void loadCamera(QDomNode const& node, vtkRenderer* ren)
	{
		vtkCamera* camera = ren->GetActiveCamera();
		QDomNamedNodeMap attributes = node.attributes();
		double position[3], focalPoint[3], viewUp[3];
		stringToArray<double>(attributes.namedItem("viewUp").nodeValue(), viewUp, 3, ",");
		stringToArray<double>(attributes.namedItem("position").nodeValue(), position, 3, ",");
		stringToArray<double>(attributes.namedItem("focalPoint").nodeValue(), focalPoint, 3, ",");
		camera->SetViewUp(viewUp);
		camera->SetPosition(position);
		camera->SetFocalPoint(focalPoint);
		if (attributes.contains("scale"))
		{
			double scale = attributes.namedItem("scale").nodeValue().toDouble();
			camera->SetParallelScale(scale);
		}
		camera->SetParallelProjection(attributes.contains("scale"));
	}

	void savePreferences(iAXmlSettings& xml, iAPreferences const& prefs)
	{
		QDomElement preferencesElement = xml.createElement(PrefElemName);
		preferencesElement.setAttribute("limitForAuto3DRender", prefs.LimitForAuto3DRender);
		preferencesElement.setAttribute("positionMarkerSize", prefs.PositionMarkerSize);
		preferencesElement.setAttribute("printParameters", prefs.PrintParameters);
		preferencesElement.setAttribute("resultsInNewWindow", prefs.ResultInNewWindow);
		preferencesElement.setAttribute("fontSize", QString::number(prefs.FontSize));
		preferencesElement.setAttribute("logFileName", iALogWidget::get()->logFileName());
		preferencesElement.setAttribute("logToFile", iALogWidget::get()->isLogToFileOn());
		preferencesElement.setAttribute("logLevel", iALogWidget::get()->logLevel());
		preferencesElement.setAttribute("logFileLevel", iALogWidget::get()->fileLogLevel());
		preferencesElement.setAttribute("logVTK", iALogWidget::get()->logVTK());
		preferencesElement.setAttribute("logITK", iALogWidget::get()->logITK());
	}

	void loadPreferences(QDomNode preferencesNode, iAPreferences& preferences)
	{
		QDomNamedNodeMap attributes = preferencesNode.attributes();
		preferences.LimitForAuto3DRender = attributes.namedItem("limitForAuto3DRender").nodeValue().toInt();
		preferences.PositionMarkerSize = attributes.namedItem("positionMarkerSize").nodeValue().toDouble();
		preferences.PrintParameters = attributes.namedItem("printParameters").nodeValue() == "1";
		preferences.ResultInNewWindow = attributes.namedItem("resultsInNewWindow").nodeValue() == "1";
		preferences.FontSize = attributes.namedItem("fontSize").nodeValue().toInt();
		bool prefLogToFile = attributes.namedItem("logToFile").nodeValue() == "1";
		QString logFileName = attributes.namedItem("logFileName").nodeValue();
		iALogWidget::get()->setLogToFile(prefLogToFile, logFileName);
		iALogWidget::get()->setLogLevel(static_cast<iALogLevel>(attributes.namedItem("logLevel").nodeValue().toInt()));
		iALogWidget::get()->setFileLogLevel(static_cast<iALogLevel>(attributes.namedItem("logFileLevel").nodeValue().toInt()));
		iALogWidget::get()->setLogVTK(attributes.namedItem("logVTK").nodeValue() == "1");
		iALogWidget::get()->setLogITK(attributes.namedItem("logITK").nodeValue() == "1");
	}

	void saveSlicerSettings(iAXmlSettings& xml, iASlicerSettings const slicerSettings)
	{
		QDomElement slicerSettingsElement = xml.createElement(SlicerElemName);
		slicerSettingsElement.setAttribute("linkViews", slicerSettings.LinkViews);
		slicerSettingsElement.setAttribute("snakeSlices", slicerSettings.SnakeSlices);
		slicerSettingsElement.setAttribute("linkMDIs", slicerSettings.LinkMDIs);
	}

	void loadSlicerSettings(QDomNode slicerSettingsNode, iASlicerSettings& slicerSettings)
	{
		QDomNamedNodeMap attributes = slicerSettingsNode.attributes();
		slicerSettings.SnakeSlices = attributes.namedItem("snakeSlices").nodeValue().toDouble();
		slicerSettings.LinkMDIs = attributes.namedItem("linkMDIs").nodeValue() == "1";
		slicerSettings.LinkViews = attributes.namedItem("linkViews").nodeValue() == "1";
	}
}

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
	m_useSystemTheme(false),
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

	const QString OrganisationName("FHW");
	const QString ApplicationName("open_iA");
	QCoreApplication::setOrganizationName(OrganisationName);
	QCoreApplication::setOrganizationDomain("3dct.at");
	QCoreApplication::setApplicationName(ApplicationName);
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

	addActionIcon(m_ui->actionViewXDirectionInRaycaster, "px");
	addActionIcon(m_ui->actionViewYDirectionInRaycaster, "py");
	addActionIcon(m_ui->actionViewZDirectionInRaycaster, "pz");
	addActionIcon(m_ui->actionViewmXDirectionInRaycaster, "mx");
	addActionIcon(m_ui->actionViewmYDirectionInRaycaster, "my");
	addActionIcon(m_ui->actionViewmZDirectionInRaycaster, "mz");
	addActionIcon(m_ui->actionIsometricViewInRaycaster, "iso");
	addActionIcon(m_ui->actionMagicLens2D, "magic_lens_2d");
	addActionIcon(m_ui->actionMagicLens3D, "magic_lens_3d");
	addActionIcon(m_ui->actionSlicerSettings, "settings_slicer");
	addActionIcon(m_ui->actionPreferences, "settings");
	addActionIcon(m_ui->actionOpenWithDataTypeConversion, "open");
	addActionIcon(m_ui->actionOpenTLGICTData, "open");
	addActionIcon(m_ui->actionOpenRaw, "open");
	addActionIcon(m_ui->actionOpenDataSet, "open");
	addActionIcon(m_ui->actionOpenInNewWindow, "open");
	addActionIcon(m_ui->actionSaveDataSet, "save");
	addActionIcon(m_ui->actionSaveVolumeStack, "save-all");
	addActionIcon(m_ui->actionSaveProject, "save-all");
	addActionIcon(m_ui->actionEditProfilePoints, "profile-edit");
	addActionIcon(m_ui->actionRawProfile, "profile-raw");
	addActionIcon(m_ui->actionSnakeSlicer, "snakeslicer");
	addActionIcon(m_ui->actionSyncCamera, "camera-sync");
	addActionIcon(m_ui->actionInteractionModeCamera, "camera");
	addActionIcon(m_ui->actionLoadCameraSettings, "camera-load");
	addActionIcon(m_ui->actionSaveCameraSettings, "camera-save");
	addActionIcon(m_ui->actionSaveLayout, "layout_export");
	addActionIcon(m_ui->actionLoadLayout, "layout_load");
	addActionIcon(m_ui->actionResetLayout, "layout_reset");
	addActionIcon(m_ui->actionDeleteLayout, "layout_delete");
	addActionIcon(m_ui->actionInteractionModeRegistration, "transform-move");

	m_ui->menuEdit->addSeparator();
}

MainWindow::~MainWindow()
{
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	settings.setValue("state", saveState());
	for (auto a : m_actionIcons.keys())
	{   // to prevent invalid access when QActions are deleted
		disconnect(a, &QObject::destroyed, this, &MainWindow::removeActionIcon);
	}
	m_moduleDispatcher->SaveModulesSettings();
	iASettingsManager::store();
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

void MainWindow::closeEvent(QCloseEvent *e)
{
	if (keepOpen())
	{
		e->ignore();
		return;
	}
	m_ui->mdiArea->closeAllSubWindows();
	if (activeMdiChild())
	{
		e->ignore();
		return;
	}
	iASystemThemeWatcher::stop();
	writeSettings();
	iALogWidget::shutdown();
	e->accept();
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
		loadFile(url.toLocalFile(), nullptr);
	}
}

bool MainWindow::event(QEvent *e)
{
	// according to QWidget docs, StyleChange event should also trigger more specific changeEvent method,
	// but this does not seem to be the case (at least for Qt 6.5.0 beta2 on Ubuntu 22.04)
	// currently only called on Linux (tested on XFCE, Gnome and KDE), not on Windows (untested on Mac OS)
	if  (e->type() == QEvent::StyleChange
		//|| e->type() == QEvent::PaletteChange  // also triggered, but probably only as side-effect
		//|| e->type() == QEvent::ThemeChange    // might also be relevant, not sure about difference to StyleChange, both are triggered
	) {
		iASystemThemeWatcher::get()->checkForChange();
	}
	return QMainWindow::event(e);
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
	loadFile(fileName, askWhichChild(), std::make_shared<iARawFileIO>());
}

void MainWindow::openRecentFile()
{
	auto action = qobject_cast<QAction*>(sender());
	if (!action)
	{
		return;
	}
	QString fileName = action->data().toString();
	loadFile(fileName, askWhichChild());
}

iAMdiChild* MainWindow::askWhichChild()
{
	auto child = activeMdiChild();
	if (child)
	{
		auto result = QMessageBox::question(this, "", "Load data into the active window?", QMessageBox::Yes | QMessageBox::No);
		if (result == QMessageBox::No)
		{
			child = nullptr;
		}
	}
	return child;
}

void MainWindow::loadFile(QString const& fileName, iAMdiChild* child, std::shared_ptr<iAFileIO> io)
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
				targetChild->setWindowTitleAndFile(fileName);
			}
			addRecentFile(fileName);
			targetChild->addDataSet(dataSet);
		});
	QObject::connect(futureWatcher, &FutureWatcherType::finished, futureWatcher, &FutureWatcherType::deleteLater);
	auto future = QtConcurrent::run( [p, fileName, io, paramValues]() { return io->load(fileName, paramValues, *p.get()); });
	futureWatcher->setFuture(future);
	iAJobListView::get()->addJob(QString("Loading file '%1'").arg(fileName), p.get(), futureWatcher);
}

void MainWindow::loadFiles(QStringList fileNames, iAMdiChild* child)
{
	for (int i = 0; i < fileNames.length(); i++)
	{
		loadFile(fileNames[i], child);
	}
}

void MainWindow::saveSettings()
{
	auto SaveSettings = tr("Save Settings");
	QString fileName = QFileDialog::getSaveFileName(this, SaveSettings, path(), XMLFileFilter);
	if (fileName.isEmpty())
	{
		return;
	}
	iAAttributes params;
	addAttr(params, PrefNiceName, iAValueType::Boolean, m_loadSavePreferences);
	addAttr(params, SlicerNiceName, iAValueType::Boolean, m_loadSaveSlicerSettings);
	auto settingsMap = iASettingsManager::getMap();
	for (auto const& settingName: settingsMap.keys())
	{
		addAttr(params, settingName, iAValueType::Boolean, m_settingsToLoadSave.contains(configStorageName(settingName)));
	}
	iAParameterDlg dlg(this, SaveSettings, params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	m_loadSavePreferences = values[PrefNiceName].toBool();
	m_loadSaveSlicerSettings = values[SlicerNiceName].toBool();

	iAXmlSettings xml;
	if (m_loadSavePreferences)
	{
		savePreferences(xml, m_defaultPreferences);
	}
	if (m_loadSaveSlicerSettings)
	{
		saveSlicerSettings(xml, m_defaultSlicerSettings);
	}
	m_settingsToLoadSave.clear();
	for (auto const& settingName : settingsMap.keys())
	{
		if (values[settingName].toBool())
		{
			m_settingsToLoadSave.append(configStorageName(settingName));
			QDomElement element = xml.createElement(configStorageName(settingName));
			storeAttributeValues(element, *settingsMap[settingName]);
		}
	}
	xml.save(fileName);
	LOG(lvlInfo, QString("Saved settings to %1").arg(fileName));
}

void MainWindow::loadSettings()
{
	auto LoadSettings = tr("Load Settings");
	QString fileName = QFileDialog::getOpenFileName(this, LoadSettings, path(), XMLFileFilter);
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	if (!xml.read(fileName))
	{
		QMessageBox::warning(this, LoadSettings, "An error occurred during xml parsing!");
		return;
	}
	iAAttributes params;
	if (xml.hasElement(PrefElemName))   { addAttr(params, PrefNiceName, iAValueType::Boolean, m_loadSavePreferences); }
	if (xml.hasElement(SlicerElemName)) { addAttr(params, SlicerNiceName, iAValueType::Boolean, m_loadSaveSlicerSettings); }

	constexpr const char ApplyTo[] = "Apply above to";
	constexpr const char CurrentWindow[] = "Current window";
	constexpr const char AllOpenWindows[] = "All open windows";
	if (childList<MdiChild>().size() > 0 && xml.hasElement(PrefElemName) || xml.hasElement(SlicerElemName))
	{
		auto applyToOptions = QStringList() << "Only defaults";
		if (activeMDI())
		{
			applyToOptions << CurrentWindow;
		}
		if (childList<MdiChild>().size() > 1)
		{
			applyToOptions << AllOpenWindows;
		}
		addAttr(params, ApplyTo, iAValueType::Categorical, applyToOptions);
	}

	auto settingsMap = iASettingsManager::getMap();
	for (auto settingName : settingsMap.keys())
	{
		auto sanitizedName = configStorageName(settingName);
		if (xml.hasElement(sanitizedName))
		{
			addAttr(params, settingName, iAValueType::Boolean, m_settingsToLoadSave.contains(sanitizedName));
		}
	}
	iAParameterDlg dlg(this, LoadSettings + ": " + fileName, params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	if (xml.hasElement(PrefElemName))
	{
		m_loadSavePreferences = values[PrefNiceName].toBool();
		if (m_loadSavePreferences)
		{
			loadPreferences(xml.node(PrefElemName), m_defaultPreferences);
			if (values[ApplyTo].toString() == CurrentWindow && activeMDI())
			{
				activeMDI()->applyPreferences(m_defaultPreferences);
			}
			else if (values[ApplyTo].toString() == AllOpenWindows)
			{
				for (auto m : childList<MdiChild>())
				{
					m->applyPreferences(m_defaultPreferences);
				}
			}
		}
	}
	if (xml.hasElement(SlicerElemName))
	{
		m_loadSaveSlicerSettings = values[SlicerNiceName].toBool();
		if (m_loadSaveSlicerSettings)
		{
			loadSlicerSettings(xml.node(SlicerElemName), m_defaultSlicerSettings);
			if (values[ApplyTo].toString() == CurrentWindow && activeMDI())
			{
				activeMDI()->applySlicerSettings(m_defaultSlicerSettings);
			}
			else if (values[ApplyTo].toString() == AllOpenWindows)
			{
				for (auto m : childList<MdiChild>())
				{
					m->applySlicerSettings(m_defaultSlicerSettings);
				}
			}
		}
	}
	for (auto settingName : settingsMap.keys())
	{
		auto sanitizedName = configStorageName(settingName);
		if (xml.hasElement(sanitizedName))
		{
			if (values[settingName].toBool())
			{
				if (!m_settingsToLoadSave.contains(sanitizedName))
				{
					m_settingsToLoadSave.append(sanitizedName);
				}
				QDomNamedNodeMap attributes = xml.node(sanitizedName).attributes();
				loadAttributeValues(attributes, *settingsMap[settingName]);
			}
			else
			{
				m_settingsToLoadSave.removeAll(sanitizedName);
			}
		}
	}
	LOG(lvlInfo, QString("Loaded settings from %1").arg(fileName));
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
	if (!activeMdiChild())
	{
		return;
	}
	m_defaultSlicerSettings.LinkViews = m_ui->actionLinkViews->isChecked();
	activeMDI()->linkViews(m_defaultSlicerSettings.LinkViews);
	LOG(lvlInfo, QString("Link Views: ").arg(iAConverter<bool>::toString(m_defaultSlicerSettings.LinkViews)));
}

void MainWindow::linkMDIs()
{
	if (!activeMdiChild())
	{
		return;
	}
	m_defaultSlicerSettings.LinkMDIs = m_ui->actionLinkMdis->isChecked();
	activeMDI()->linkMDIs(m_defaultSlicerSettings.LinkMDIs);
	LOG(lvlInfo, QString("Link MDIs: ").arg(iAConverter<bool>::toString(m_defaultSlicerSettings.LinkMDIs)));
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
	QString const Sys("Adapt to system theme");
	styleNames.insert(Sys, SystemTheme);
	styleNames.insert("Dark", DarkThemeQss);
	styleNames.insert("Bright", BrightThemeQss);
	for (QString key: styleNames.keys())
	{
		looks.append(QString("%1%2")
			.arg((key == Sys && m_useSystemTheme) || (!m_useSystemTheme && m_qssName == styleNames[key]) ? "!" : "")
			.arg(key));
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
	addAttr(params, "Position marker size", iAValueType::Discrete, p.PositionMarkerSize, 1);
	addAttr(params, "Print Parameters", iAValueType::Boolean, p.PrintParameters);
	addAttr(params, "Results in new window", iAValueType::Boolean, p.ResultInNewWindow);
	addAttr(params, "Log Level", iAValueType::Categorical, logLevels);
	addAttr(params, "Log to file", iAValueType::Boolean, iALogWidget::get()->isLogToFileOn());
	addAttr(params, "Log File Name", iAValueType::FileNameSave, iALogWidget::get()->logFileName());
	addAttr(params, "File Log Level", iAValueType::Categorical, fileLogLevels);
	addAttr(params, "Looks", iAValueType::Categorical, looks);
	addAttr(params, "Font size", iAValueType::Discrete, QString::number(p.FontSize), 4, 120);
	addAttr(params, "Size limit for automatic 3D rendering (MB)", iAValueType::Discrete, p.LimitForAuto3DRender, 0);
	iAParameterDlg dlg(this, PrefNiceName, params, descr);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	m_defaultPreferences.PositionMarkerSize = values["Position marker size"].toInt();
	m_defaultPreferences.PrintParameters = values["Print Parameters"].toBool();
	m_defaultPreferences.ResultInNewWindow = values["Results in new window"].toBool();
	iALogWidget::get()->setLogLevel(static_cast<iALogLevel>(AvailableLogLevels().indexOf(values["Log Level"].toString()) + 1));
	bool logToFile = values["Log to file"].toBool();
	QString logFileName = values["Log File Name"].toString();
	iALogWidget::get()->setFileLogLevel(static_cast<iALogLevel>(AvailableLogLevels().indexOf(values["File Log Level"].toString()) + 1));
	QString looksStr = values["Looks"].toString();
	m_useSystemTheme = (looksStr == Sys);
	auto newQssName = m_useSystemTheme ? qssNameFromSystem() : styleNames[looksStr];
	if (m_qssName != newQssName)
	{
		m_qssName = newQssName;
		iAThemeHelper::setBrightMode(brightMode());
		applyQSS();
	}
	m_defaultPreferences.FontSize = values["Font size"].toInt();
	auto f = QApplication::font();
	f.setPointSize(m_defaultPreferences.FontSize);
	QApplication::setFont(f);
	m_defaultPreferences.LimitForAuto3DRender = values["Size limit for automatic 3D rendering (MB)"].toInt();
	if (activeMdiChild())
	{
		activeMDI()->applyPreferences(m_defaultPreferences);
	}
	iALogWidget::get()->setLogToFile(logToFile, logFileName, true);
}

void MainWindow::slicerSettings()
{
	QString dlgTitle = "Slicer settings";
	iAAttributes params;
	iASlicerSettings const& slicerSettings = activeMDI() ? activeMDI()->slicerSettings() : m_defaultSlicerSettings;
	addAttr(params, "Link Views", iAValueType::Boolean, slicerSettings.LinkViews);
	addAttr(params, "Link MDIs", iAValueType::Boolean, slicerSettings.LinkMDIs);
	addAttr(params, "Snake Slices", iAValueType::Discrete, slicerSettings.SnakeSlices);
	iAParameterDlg dlg(this, dlgTitle, params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	m_defaultSlicerSettings.LinkViews = values["Link Views"].toBool();
	m_defaultSlicerSettings.LinkMDIs = values["Link MDIs"].toBool();
	m_defaultSlicerSettings.SnakeSlices = values["Snake Slices"].toInt();
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
		tmpChild->setCamPosition(camOptions, /*m_defaultRenderSettings.ParallelProjection*/ true);
	}
}

void MainWindow::saveCameraSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	auto SaveCameraTitle = tr("Save Camera Settings");
	QString fileName = QFileDialog::getSaveFileName(this, SaveCameraTitle, filePath, XMLFileFilter);
	if (fileName.isEmpty())
	{
		return;
	}
	iAAttributes params;
	addAttr(params, RendererNiceName, iAValueType::Boolean, m_loadSaveCamRen3D);
	for (int m = 0; m < iASlicerMode::SlicerCount; ++m)
	{
		addAttr(params, slicerNiceName(m), iAValueType::Boolean, m_loadSaveSlice[m]);
	}
	iAParameterDlg dlg(this, SaveCameraTitle, params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	iAXmlSettings xml;
	m_loadSaveCamRen3D = values[RendererNiceName].toBool();
	if (m_loadSaveCamRen3D)
	{
		vtkCamera* camera = activeMdiChild()->renderer()->renderer()->GetActiveCamera();
		auto camElem = xml.createElement(RendererElemName);
		saveCamera(camElem, camera);
	}
	for (int m = 0; m < iASlicerMode::SlicerCount; ++m)
	{
		m_loadSaveSlice[m] = values[slicerNiceName(m)].toBool();
		if (m_loadSaveSlice[m])
		{
			auto camElem = xml.createElement(slicerElemName(m));
			saveCamera(camElem, activeMdiChild()->slicer(m)->camera());
		}
	}
	xml.save(fileName);
	LOG(lvlInfo, QString("Saved camera settings to %1").arg(fileName));
}

void MainWindow::loadCameraSettings()
{
	QString filePath = activeMdiChild()->currentFile();
	filePath.truncate(filePath.lastIndexOf('/'));
	auto LoadCameraTitle = tr("Load Camera Settings");
	QString fileName = QFileDialog::getOpenFileName(this, LoadCameraTitle, filePath, XMLFileFilter);
	if (fileName.isEmpty())
	{
		return;
	}
	iAXmlSettings xml;
	if (!xml.read(fileName))
	{
		QMessageBox::warning(this, LoadCameraTitle, "An error occurred during xml parsing!");
		return;
	}
	QDomElement root = xml.documentElement();
	iAAttributes params;
	if (xml.hasElement(RendererElemName))
	{
		addAttr(params, RendererNiceName, iAValueType::Boolean, m_loadSaveCamRen3D);
	}
	for (int m = 0; m < iASlicerMode::SlicerCount; ++m)
	{
		if (xml.hasElement(slicerElemName(m)))
		{
			addAttr(params, slicerNiceName(m), iAValueType::Boolean, m_loadSaveSlice[m]);
		}
	}
	constexpr const char ApplyToAllOpenWindows[] = "Apply to all open windows";
	addAttr(params, ApplyToAllOpenWindows, iAValueType::Boolean, m_loadSaveApplyToAllOpenWindows);
	iAParameterDlg dlg(this, LoadCameraTitle, params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	if (xml.hasElement(RendererElemName))
	{
		m_loadSaveCamRen3D = values[RendererNiceName].toBool();
		if (m_loadSaveCamRen3D)
		{
			loadCamera(xml.node(RendererElemName), activeMdiChild()->renderer()->renderer());
		}
	}
	for (int m = 0; m < iASlicerMode::SlicerCount; ++m)
	{
		if (xml.hasElement(slicerElemName(m)))
		{
			m_loadSaveSlice[m] = values[slicerNiceName(m)].toBool();
			if (m_loadSaveSlice[m])
			{
				loadCamera(xml.node(slicerElemName(m)), activeMdiChild()->slicer(m)->renderer());
			}
		}
	}
	activeMdiChild()->updateViews();
	m_loadSaveApplyToAllOpenWindows = values[ApplyToAllOpenWindows].toBool();
	if (m_loadSaveApplyToAllOpenWindows)
	{
		rendererSyncCamera();
	}
	LOG(lvlInfo, QString("Loaded camera settings from %1").arg(fileName));
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
	//child->applyRendererSettings(m_defaultRenderSettings);
	child->applySlicerSettings(m_defaultSlicerSettings);
	emit childCreated(child);
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
	auto getOpenFileNames = [this]() -> QStringList {
		return QFileDialog::getOpenFileNames(this, tr("Open Files (new)"), m_path, iAFileTypeRegistry::registeredFileTypes(iAFileIO::Load));
	};
	connect(m_ui->actionOpenDataSet, &QAction::triggered, this, [this, getOpenFileNames]
	{
		auto fileNames = getOpenFileNames();
		if (fileNames.isEmpty())
		{
			return;
		}
		auto child = activeMdiChild();
		if (!child) // difference between "Open dataset" and "Open in new window" (below) if no window is open: here, we only open a single window for all datasets!
		{
			child = createMdiChild(false);
		}
		loadFiles(fileNames, child);
	});
	connect(m_ui->actionOpenInNewWindow, &QAction::triggered, this, [this, getOpenFileNames] { loadFiles(getOpenFileNames(), nullptr); });
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
	connect(m_ui->actionViewXDirectionInRaycaster, &QAction::triggered,  this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::PX); });
	connect(m_ui->actionViewmXDirectionInRaycaster, &QAction::triggered, this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::MX); });
	connect(m_ui->actionViewYDirectionInRaycaster, &QAction::triggered,  this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::PY); });
	connect(m_ui->actionViewmYDirectionInRaycaster, &QAction::triggered, this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::MY); });
	connect(m_ui->actionViewZDirectionInRaycaster, &QAction::triggered,  this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::PZ); });
	connect(m_ui->actionViewmZDirectionInRaycaster, &QAction::triggered, this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::MZ); });
	connect(m_ui->actionIsometricViewInRaycaster, &QAction::triggered,   this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::Iso);});

	// Camera toolbar:
	connect(m_ui->actionSyncCamera,   &QAction::triggered, this, &MainWindow::rendererSyncCamera);
	connect(m_ui->actionSaveCameraSettings, &QAction::triggered, this, &MainWindow::saveCameraSettings);
	connect(m_ui->actionLoadCameraSettings, &QAction::triggered, this, &MainWindow::loadCameraSettings);

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

	// layout / look:
	m_qssName = settings.value("themeName", SystemTheme).toString();
	m_useSystemTheme = m_qssName == SystemTheme;
	if (m_useSystemTheme)
	{
		m_qssName = qssNameFromSystem();
	}
	iAThemeHelper::setBrightMode(brightMode());
	restoreGeometry(settings.value("geometry", saveGeometry()).toByteArray());
	restoreState(settings.value("state", saveState()).toByteArray());

	iAPreferences defaultPrefs;
	m_defaultLayout = settings.value("Preferences/defaultLayout", "").toString();
	m_defaultPreferences.FontSize = settings.value("Preferences/fontSize", defaultPrefs.FontSize).toInt();
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
	m_defaultPreferences.PositionMarkerSize = settings.value("Preferences/prefStatExt", defaultPrefs.PositionMarkerSize).toInt();
	
	// Logging-related:
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
	bool showLog = settings.value("Parameters/ShowLog", false).toBool();
	iALogWidget::get()->toggleViewAction()->setChecked(showLog);
	iALogWidget::get()->setVisible(showLog);
	m_ui->actionOpenLogOnNewMessage->setChecked(settings.value("Parameters/OpenLogOnNewMessages", true).toBool());
	toggleOpenLogOnNewMessage();

	// job/filter related:
	m_defaultPreferences.PrintParameters = settings.value("Preferences/prefPrintParameters", defaultPrefs.PrintParameters).toBool();
	m_defaultPreferences.ResultInNewWindow = settings.value("Preferences/prefResultInNewWindow", defaultPrefs.ResultInNewWindow).toBool();
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
	
	// performance:
	m_defaultPreferences.LimitForAuto3DRender = settings.value("Preferences/prefLimitForAuto3DRender", defaultPrefs.LimitForAuto3DRender).toInt();

	iASlicerSettings fallbackSS;
	m_defaultSlicerSettings.LinkViews = settings.value("Slicer/ssLinkViews", fallbackSS.LinkViews).toBool();
	m_defaultSlicerSettings.LinkMDIs = settings.value("Slicer/ssLinkMDIs", fallbackSS.LinkMDIs).toBool();
	m_defaultSlicerSettings.SnakeSlices = settings.value("Slicer/ssSnakeSlices", fallbackSS.SnakeSlices).toInt();

	m_loadSavePreferences = settings.value("Parameters/loadSavePreferences", true).toBool();
	m_loadSaveSlicerSettings = settings.value("Parameters/loadSaveSlicerSettings", true).toBool();
	m_settingsToLoadSave = settings.value("Parameters/settingsToLoadSave").toStringList();
	m_loadSaveCamRen3D = settings.value("Parameters/loadSaveCameraRenderer3D", true).toBool();
	for (int m = 0; m < iASlicerMode::SlicerCount; ++m)
	{
		m_loadSaveSlice[m] = settings.value(QString("Parameters/loadSaveCamera%1").arg(slicerElemName(m)), true).toBool();
	}
	m_loadSaveApplyToAllOpenWindows = settings.value("Parameters/loadSaveApplyToAllOpenWindows", false).toBool();

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
}

void MainWindow::writeSettings()
{
	QSettings settings;
	settings.setValue("Path", m_path);
	settings.setValue("themeName", m_useSystemTheme ? SystemTheme : m_qssName);

	settings.setValue("Preferences/defaultLayout", m_layout->currentText());
	settings.setValue("Preferences/prefLimitForAuto3DRender", m_defaultPreferences.LimitForAuto3DRender);
	settings.setValue("Preferences/prefStatExt", m_defaultPreferences.PositionMarkerSize);
	settings.setValue("Preferences/prefPrintParameters", m_defaultPreferences.PrintParameters);
	settings.setValue("Preferences/prefResultInNewWindow", m_defaultPreferences.ResultInNewWindow);
	settings.setValue("Preferences/fontSize", m_defaultPreferences.FontSize);
	settings.setValue("Preferences/prefLogToFile", iALogWidget::get()->isLogToFileOn());
	settings.setValue("Preferences/prefLogFile", iALogWidget::get()->logFileName());
	settings.setValue("Preferences/prefLogLevel", iALogWidget::get()->logLevel());
	settings.setValue("Preferences/prefLogVTK", iALogWidget::get()->logVTK());
	settings.setValue("Preferences/prefLogITK", iALogWidget::get()->logITK());
	settings.setValue("Preferences/prefFileLogLevel", iALogWidget::get()->fileLogLevel());

	settings.setValue("Slicer/ssLinkViews", m_defaultSlicerSettings.LinkViews);
	settings.setValue("Slicer/ssLinkMDIs", m_defaultSlicerSettings.LinkMDIs);
	settings.setValue("Slicer/ssSnakeSlices", m_defaultSlicerSettings.SnakeSlices);
	
	settings.setValue("Parameters/loadSavePreferences", m_loadSavePreferences);
	settings.setValue("Parameters/loadSaveSlicerSettings", m_loadSaveSlicerSettings);
	settings.setValue("Parameters/settingsToLoadSave", m_settingsToLoadSave);
	settings.setValue("Parameters/loadSaveCameraRenderer3D", m_loadSaveCamRen3D);
	for (int m = 0; m < iASlicerMode::SlicerCount; ++m)
	{
		settings.setValue(QString("Parameters/loadSaveCamera%1").arg(slicerElemName(m)), m_loadSaveSlice[m]);
	}
	settings.setValue("Parameters/loadSaveApplyToAllOpenWindows", m_loadSaveApplyToAllOpenWindows);
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
		p.setColor(QPalette::Window,          brightMode() ? QColor(255, 255, 255) : QColor(  0,   0,   0));
		p.setColor(QPalette::Base,            brightMode() ? QColor(255, 255, 255) : QColor(  0,   0,   0));
		p.setColor(QPalette::ToolTipBase,     brightMode() ? QColor(255, 255, 255) : QColor(  0,   0,   0));
		p.setColor(QPalette::Light,           brightMode() ? QColor(255, 255, 255) : QColor(  0,   0,   0));
		p.setColor(QPalette::Midlight,        brightMode() ? QColor(240, 240, 240) : QColor( 15,  15,  15));
		p.setColor(QPalette::AlternateBase,   brightMode() ? QColor(240, 240, 240) : QColor( 30,  30,  30));  // dark seems (to me, BF) to need a bit more contrast to be visible well
		p.setColor(QPalette::Button,          brightMode() ? QColor(215, 215, 215) : QColor( 40,  40,  40));
		p.setColor(QPalette::Mid,             brightMode() ? QColor(200, 200, 200) : QColor( 55,  55,  55));
		p.setColor(QPalette::Dark,            brightMode() ? QColor(180, 180, 180) : QColor( 75,  75,  75));
		p.setColor(QPalette::Shadow,          brightMode() ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		//p.setColor(QPalette::Highlight,       brightMode() ? QColor(  0,   0,   0) : QColor(255, 255, 255));  // TODO: determine proper highlight colors
		p.setColor(QPalette::HighlightedText, brightMode() ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		p.setColor(QPalette::Text,            brightMode() ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		p.setColor(QPalette::ToolTipText,     brightMode() ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		p.setColor(QPalette::PlaceholderText, brightMode() ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		p.setColor(QPalette::WindowText,      brightMode() ? QColor(  0,   0,   0) : QColor(255, 255, 255));
		QApplication::setPalette(p);

		for (auto a : m_actionIcons.keys())
		{
			a->setIcon(iAThemeHelper::icon(m_actionIcons[a]));
		}
		emit styleChanged();
	}
}

bool MainWindow::brightMode() const
{
	return m_qssName.contains("bright");
}

void MainWindow::addActionIcon(QAction* action, QString const& iconName)
{
	assert(action);
	m_actionIcons.insert(action, iconName);
	action->setIcon(iAThemeHelper::icon(iconName));
	connect(action, &QObject::destroyed, this, &MainWindow::removeActionIcon);
}

void MainWindow::removeActionIcon()
{
	m_actionIcons.remove(qobject_cast<QAction*>(sender()));
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

QMenu* MainWindow::editMenu()
{
	return m_ui->menuEdit;
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

//iARenderSettings const& MainWindow::defaultRenderSettings() const
//{
//	return m_defaultRenderSettings;
//}

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
		dlg_datatypeconversion conversionwidget(this, fileName, m_rawFileParams, m_owdtcs, convPara);
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
		loadFile(finalfilename, askWhichChild());
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

#include "iASCIFIOCheck.h"

#include <QPainter>
#include <QProxyStyle>

//! An application style used to override certain aspects of the user interface.
//! Specifically, to disable the tooltip delay in iAChartWidget and descendants,
//! and for drawing nice-looking MDI child control buttons in the menu bar on maximized child windows
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

	//! {
	//! For drawing the MDI controls (close, float, minimize buttons) in the menu bar
	//! when MDI children are maximized
	void drawSubControl(QStyleOptionComplex const * opt, QPainter* p, QWidget const* widget, SubControl subControl, QString const& iconName) const
	{
		// copied from QCommonStyle::drawComplexControl; removed duplication, modified icon retrieval and button drawing:
		if (opt->subControls & subControl) {
			QStyleOptionButton btnOpt;
			btnOpt.QStyleOption::operator=(*opt);
			btnOpt.state &= ~State_MouseOver;
			int bsx = 0;
			int bsy = 0;
			const int buttonIconMetric = proxy()->pixelMetric(PM_TitleBarButtonIconSize, &btnOpt, widget);
			const QSize buttonIconSize(buttonIconMetric, buttonIconMetric);
			btnOpt.rect = proxy()->subControlRect(CC_MdiControls, opt, subControl, widget);
			// the following code should probably adapt hover/non-hover state; the same icon seems to be used for both, but the background changes;
			// with our icons, there is no background change. There must be some behind the scenes logic going on in drawPrimitive, changing the
			// background on hover, but it doesn't work when we use our icon (even though our icon has transparent background)
			// workaround: switch icon depending on whether control is "active" (i.e. mouse hovering over it) or not
			if (opt->activeSubControls & subControl && (opt->state & State_Sunken))
			{
				btnOpt.state |= State_Sunken;
				btnOpt.state &= ~State_Raised;
				proxy()->drawPrimitive(PE_PanelButtonCommand, &btnOpt, p, widget);
			//	bsx = proxy()->pixelMetric(PM_ButtonShiftHorizontal, opt);  // always returns 0
			//	bsy = proxy()->pixelMetric(PM_ButtonShiftVertical, opt);    // always returns 0
			}
			//else // we want to only "sink" buttons that are pressed, but not "raise" buttons that are not.
			QPixmap pm = iAThemeHelper::icon(QString("%1%2").arg(iconName).arg((opt->activeSubControls & subControl) ? "-hover" : ""))
				.pixmap(buttonIconSize
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
				, p->device()->devicePixelRatio()
#endif
			);
			proxy()->drawItemPixmap(p, btnOpt.rect.translated(bsx, bsy), Qt::AlignCenter, pm);
		}
	}

	void drawComplexControl(QStyle::ComplexControl cc, QStyleOptionComplex const * opt, QPainter* p, QWidget const * widget = nullptr) const override
	{
		if (cc == CC_MdiControls)
		{
			drawSubControl(opt, p, widget, QStyle::SC_MdiCloseButton, "dockwidget-close");
			drawSubControl(opt, p, widget, QStyle::SC_MdiNormalButton, "dockwidget-float");
			drawSubControl(opt, p, widget, QStyle::SC_MdiMinButton, "dockwidget-minimize");
		}
		else
		{
			QProxyStyle::drawComplexControl(cc, opt, p, widget);
		}
	}
	//! @}
};

int MainWindow::runGUI(int argc, char * argv[], QString const & appName, QString const & version,
	QString const& buildInformation, QString const & splashPath, QString const & iconPath)
{
	iAFileParamDlg::setupDefaultFileParamDlgs();
	QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL, true);
	QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
	iAFileTypeRegistry::addDefaultExtension(iADataSetType::Volume, "mhd");
	iAFileTypeRegistry::addDefaultExtension(iADataSetType::Mesh, "stl");
	//iAFileTypeRegistry::addDefaultExtension(iADataSetType::Graph, ".txt");  -> graph storing not yet implemented!
	iAFileTypeRegistry::addDefaultExtension(iADataSetType::Collection, ProjectFileExtension);
#if defined(__APPLE__) && defined(__MACH__)
	QSurfaceFormat::setDefaultFormat(iAVtkWidget::defaultFormat());
#endif
	Q_INIT_RESOURCE(gui);
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
	connect(iASystemThemeWatcher::get(), &iASystemThemeWatcher::themeChanged, &mainWin,
		[&mainWin](bool brightTheme) {
			if (mainWin.m_useSystemTheme)
			{
				LOG(lvlDebug, QString("System theme changed and configured to automatically adapt to it, changing to %1 mode (you can override this in the Preferences)!").arg(brightTheme?"bright":"dark"));
				mainWin.m_qssName = qssNameFromSystem();
				iAThemeHelper::setBrightMode(mainWin.brightMode());
				mainWin.applyQSS();
			}
		});
	iASettingsManager::init();
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
