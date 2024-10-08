// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "mainwindow.h"

// gui
#include "iAAboutDlg.h"
#include "iACheckOpenGL.h"
#include "iAFileParamDlg.h"
#include "iALogWidget.h"
#include "iASystemThemeWatcher.h"
#include "iATLGICTLoader.h"
#include "mdichild.h"
#include "ui_Mainwindow.h"

// guibase
#include <iADefaultSettings.h>
#include <iADockWidgetWrapper.h>
#include <iAJobListView.h>
#include <iAModuleDispatcher.h>
#include <iAParameterDlg.h>
#include <iAQMenuHelper.h>
#include <iARenderer.h>
#include <iASavableProject.h>
#include <iASlicer.h>        // for iASlicer and slicerModeString
#include <iAThemeHelper.h>

// io:
#include <iADataSet.h>
#include <iAFileTypeRegistry.h>
#include <iARawFileIO.h>

// charts:
#include <iAChartWidget.h>

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
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QMdiSubWindow>
#include <QScreen>
#include <QSettings>
#include <QSpacerItem>
#include <QSplashScreen>
#include <QtConcurrent>
#include <QTextStream>
#include <QTimer>
#include <QtXml/QDomDocument>
#include <QWindow>

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
		slicerSettingsElement.setAttribute("linkMDIs", slicerSettings.LinkMDIs);
	}

	void loadSlicerSettings(QDomNode slicerSettingsNode, iASlicerSettings& slicerSettings)
	{
		QDomNamedNodeMap attributes = slicerSettingsNode.attributes();
		slicerSettings.LinkMDIs = attributes.namedItem("linkMDIs").nodeValue() == "1";
		slicerSettings.LinkViews = attributes.namedItem("linkViews").nodeValue() == "1";
	}

	void showSplashMsg(QSplashScreen& splash, QString const& msg)
	{
		splash.showMessage(QString("\n      %1").arg(msg), Qt::AlignTop, QColor(255, 255, 255));
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
	if (childList<T>().size() > 0)
	{
		return childList<T>(QMdiArea::ActivationHistoryOrder).last();
	}
	return nullptr;
}

MainWindow::MainWindow(QString const & appName, QString const & version, QString const & buildInformation,
	QString const & splashImage, QSplashScreen& splashScreen) :
	m_splashScreenImg(splashImage),
	m_useSystemTheme(false),
	m_moduleDispatcher( new iAModuleDispatcher( this ) ),
	m_gitVersion(version),
	m_buildInformation(buildInformation),
	m_ui(std::make_shared<Ui_MainWindow>()),
	m_dwJobs(new iADockWidgetWrapper(iAJobListView::get(), "Job List", "Jobs")),
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

	showSplashMsg(splashScreen, "Reading settings...");
	readSettings();

	showSplashMsg(splashScreen, "Setting up user interface...");
	m_ui->actionLinkSliceViews->setChecked(m_defaultSlicerSettings.LinkViews);//removed from readSettings, if is needed at all?
	m_ui->actionLinkMdis->setChecked(m_defaultSlicerSettings.LinkMDIs);
	setCentralWidget(m_ui->mdiArea);
	addDockWidget(Qt::RightDockWidgetArea, iALogWidget::get());
	splitDockWidget(iALogWidget::get(), m_dwJobs, Qt::Vertical);
	m_dwJobs->setFeatures(m_dwJobs->features() & ~QDockWidget::DockWidgetVerticalTitleBar);

	createRecentFileActions();
	connectSignalsToSlots();
	m_slicerToolsGroup = new QActionGroup(this);
	m_slicerToolsGroup->setExclusive(false);
	m_slicerToolsGroup->addAction(m_ui->actionRawProfile);
	m_slicerToolsGroup->addAction(m_ui->actionEditProfilePoints);

	m_ui->menuWindow->insertAction(m_ui->actionOpenLogOnNewMessage, iALogWidget::get()->toggleViewAction());
	m_ui->menuWindow->insertAction(m_ui->actionOpenListOnAddedJob, m_dwJobs->toggleViewAction());

	m_layout = new QComboBox(this);
	for (int i=0; i< m_layoutNames.size(); ++i)
	{
		m_layout->addItem(m_layoutNames[i]);
		if (m_layoutNames[i] == m_defaultLayout)
		{
			m_layout->setCurrentIndex(i);
		}
	}
	m_layout->resize(m_layout->geometry().width(), 100);
	m_layout->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	m_ui->layoutToolbar->insertWidget(m_ui->actionSaveLayout, m_layout);
	applyQSS();

	showSplashMsg(splashScreen, "Initializing modules...");
	m_moduleDispatcher->InitializeModules();

	showSplashMsg(splashScreen, "Finalizing user interface...");
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
	addActionIcon(m_ui->actionOpenTLGICTData, "open");
	addActionIcon(m_ui->actionOpenRaw, "open");
	addActionIcon(m_ui->actionOpenDataSet, "open");
	addActionIcon(m_ui->actionOpenInNewWindow, "open");
	addActionIcon(m_ui->actionSaveDataSet, "save");
	addActionIcon(m_ui->actionSaveVolumeStack, "save-all");
	addActionIcon(m_ui->actionSaveProject, "save-all");
	addActionIcon(m_ui->actionEditProfilePoints, "profile-edit");
	addActionIcon(m_ui->actionRawProfile, "profile-raw");
	addActionIcon(m_ui->actionLinkSliceViews, "slicer-sync");
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
	splashScreen.finish(this);
}

MainWindow::~MainWindow()
{
	QSettings settings;
	settings.setValue("geometry", saveGeometry());
	settings.setValue("state", saveState());
	m_moduleDispatcher->SaveModulesSettings();
	iASettingsManager::store();
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
	// TODO: Check whether any operation is still in progress, and cancel that (or wait for it to finish)
	/*
	{
		LOG(lvlWarn, "Cannot close application while operation is running!");
		event->ignore();
		return;
	}
	*/
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
		loadFile(url.toLocalFile(), iAChildSource::make(false));
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
	loadFile(fileName, iAChildSource::make(false, askWhichChild()), std::make_shared<iARawFileIO>());
}

void MainWindow::openRecentFile()
{
	auto action = qobject_cast<QAction*>(sender());
	if (!action)
	{
		return;
	}
	QString fileName = action->data().toString();
	loadFile(fileName, iAChildSource::make(false, askWhichChild()));
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

void MainWindow::loadFile(QString const& fileName, std::shared_ptr<iAChildSource> childSrc, std::shared_ptr<iAFileIO> io)
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
	if (!childSrc)
	{
		childSrc = iAChildSource::make(false);
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
		[this, childSrc, fileName]()
		{
			auto watcher = dynamic_cast<FutureWatcherType*>(sender());
			auto dataSet = watcher->result();
			if (!dataSet)
			{
				QMessageBox::warning(this, "Load: Error", QString("Loading %1 failed. See the log window for details.").arg(fileName));
				return;
			}
			iAMdiChild* targetChild = childSrc->child(this);
			targetChild->addDataSet(dataSet);
		});
	QObject::connect(futureWatcher, &FutureWatcherType::finished, futureWatcher, &FutureWatcherType::deleteLater);
	auto future = QtConcurrent::run( [p, fileName, io, paramValues]() { return io->load(fileName, paramValues, *p.get()); });
	futureWatcher->setFuture(future);
	iAJobListView::get()->addJob(QString("Loading file '%1'").arg(fileName), p.get(), futureWatcher);
}

void MainWindow::loadFiles(QStringList fileNames, std::shared_ptr<iAChildSource> childSrc)
{
	for (int i = 0; i < fileNames.length(); i++)
	{
		loadFile(fileNames[i], childSrc);
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
	if (childList<MdiChild>().size() > 0 && (xml.hasElement(PrefElemName) || xml.hasElement(SlicerElemName)))
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
	m_defaultSlicerSettings.LinkViews = m_ui->actionLinkSliceViews->isChecked();
	activeMDI()->linkSliceViews(m_defaultSlicerSettings.LinkViews);
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
	styleNames.insert("None", "");
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
	iAParameterDlg dlg(this, dlgTitle, params);
	if (dlg.exec() != QDialog::Accepted)
	{
		return;
	}
	auto values = dlg.parameterValues();
	m_defaultSlicerSettings.LinkViews = values["Link Views"].toBool();
	m_defaultSlicerSettings.LinkMDIs = values["Link MDIs"].toBool();
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
	auto mdiChilds = childList<MdiChild>();
	if (mdiChilds.size() <= 1)
	{
		return;
	}
	auto srcCam = activeMDI()->renderer()->camera();
	for (auto dstChild: mdiChilds)
	{
		if (dstChild == activeMDI())
		{
			continue;
		}
		copyCameraParams(dstChild->renderer()->camera(), srcCam);
		dstChild->renderer()->update();
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

void MainWindow::createRecentFileActions()
{
	m_separatorAct = m_ui->menuFile->addSeparator();
	for (qsizetype i = 0; i < MaxRecentFiles; ++i)
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
	QSignalBlocker blockSliceProfile(m_ui->actionRawProfile);
	m_ui->actionRawProfile->setChecked(child && child->isSliceProfileEnabled());
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
	updateWindowMenu();
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
	static QActionGroup* group = nullptr;
	delete group;
	group = new QActionGroup(this);
	m_ui->menuOpenWindows->clear();
	for (int i = 0; i < windows.size(); ++i)
	{
		iAMdiChild *child = windows.at(i);
		QString text = QString("%1%2 %3").arg(i < 9 ? "&" : "").arg(i + 1)
				.arg(child->fileInfo().fileName());
		QAction* action = m_ui->menuOpenWindows->addAction(text);
		action->setCheckable(true);
		action->setChecked(child == activeMdiChild());
		group->addAction(action);
		connect(action, &QAction::triggered, [this, child]
		{
			setActiveSubWindow(qobject_cast<QWidget*>(child->parent()) );
		});
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
	auto getOpenFileNames = [this](QString const & title) -> QStringList {
		return QFileDialog::getOpenFileNames(this, title, m_path, iAFileTypeRegistry::registeredFileTypes(iAFileIO::Load));
	};
	connect(m_ui->actionOpenDataSet, &QAction::triggered, this, [this, getOpenFileNames]
	{
		loadFiles(getOpenFileNames(tr("Open dataset")), iAChildSource::make(false, activeMdiChild()));
	});
	connect(m_ui->actionOpenInNewWindow, &QAction::triggered, this, [this, getOpenFileNames]
	{
		loadFiles(getOpenFileNames(tr("Open in new window")), iAChildSource::make(true));
	});
	connect(m_ui->actionOpenRaw, &QAction::triggered, this, &MainWindow::openRaw);
	connect(m_ui->actionOpenTLGICTData, &QAction::triggered, this, &MainWindow::openTLGICTData);
	connect(m_ui->actionSaveDataSet, &QAction::triggered, this, [childCall] { childCall(&MdiChild::save); });
	connect(m_ui->actionSaveProject, &QAction::triggered, this, &MainWindow::saveProject);
	connect(m_ui->actionSaveVolumeStack, &QAction::triggered, this, &MainWindow::saveVolumeStack);
	connect(m_ui->actionLoadSettings, &QAction::triggered, this, &MainWindow::loadSettings);
	connect(m_ui->actionSaveSettings, &QAction::triggered, this, &MainWindow::saveSettings);
	connect(m_ui->actionExit, &QAction::triggered, qApp, &QApplication::closeAllWindows);
	for (qsizetype i = 0; i < MaxRecentFiles; ++i)
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
	connect(m_ui->actionLinkSliceViews, &QAction::triggered, this, &MainWindow::linkViews);
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
	static std::map<QString, std::pair<QString, QString>> linkMap{
		{"Available releases", std::make_pair("https://github.com/3dct/open_iA/releases",
			"Opens a list of available open_iA releases in your default web browser")},
		{"Bug Tracker", std::make_pair("https://github.com/3dct/open_iA/issues",
			"Opens the open_iA bug tracker in your default web browser")},
		{"Core user guide", std::make_pair("https://github.com/3dct/open_iA/wiki/Core",
			"Opens the online open_iA core user guide (explaining the core user interface elements) in your default web browser")},
		{"Filters user guide", std::make_pair("https://github.com/3dct/open_iA/wiki/Filters",
			"Opens the online open_iA filters user guide (explaining dataset processing in your default web browser")},
		{"Tools user guide", std::make_pair("https://github.com/3dct/open_iA/wiki/Tools",
			"Opens the online open_iA tools user guide in your default web browser")},
	};
	for (auto l : linkMap)
	{
		auto a = new QAction(l.first, m_ui->menuHelp);
		a->setToolTip(linkMap[l.first].second);
		connect(a, &QAction::triggered, this, [this]()
		{
			auto act = qobject_cast<QAction*>(QObject::sender());
			QDesktopServices::openUrl(QUrl(linkMap[act->text()].first));
		});
		m_ui->menuHelp->insertAction(m_ui->actionAbout, a);
	}
	connect(m_ui->actionAbout, &QAction::triggered, this, [this]
		{ iAAboutDlg::show(this, m_splashScreenImg, m_buildInformation, m_gitVersion, screen()->geometry().height()); });

	// Camera-related actions:
	connect(m_ui->actionViewXDirectionInRaycaster, &QAction::triggered,  this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::PX); });
	connect(m_ui->actionViewmXDirectionInRaycaster, &QAction::triggered, this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::MX); });
	connect(m_ui->actionViewYDirectionInRaycaster, &QAction::triggered,  this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::PY); });
	connect(m_ui->actionViewmYDirectionInRaycaster, &QAction::triggered, this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::MY); });
	connect(m_ui->actionViewZDirectionInRaycaster, &QAction::triggered,  this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::PZ); });
	connect(m_ui->actionViewmZDirectionInRaycaster, &QAction::triggered, this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::MZ); });
	connect(m_ui->actionIsometricViewInRaycaster, &QAction::triggered,   this, [childCall] { childCall(&MdiChild::setPredefCamPos, iACameraPosition::Iso);});
	connect(m_ui->actionSyncCamera,   &QAction::triggered, this, &MainWindow::rendererSyncCamera);
	connect(m_ui->actionSaveCameraSettings, &QAction::triggered, this, &MainWindow::saveCameraSettings);
	connect(m_ui->actionLoadCameraSettings, &QAction::triggered, this, &MainWindow::loadCameraSettings);

	// Profile / Magic lens:
	connect(m_ui->actionRawProfile,   &QAction::toggled, this, [childCall](bool checked) { childCall(&MdiChild::toggleSliceProfile, checked); });
	connect(m_ui->actionEditProfilePoints, &QAction::toggled, this, [childCall](bool checked) { childCall(&MdiChild::toggleProfileHandles, checked); });
	connect(m_ui->actionMagicLens2D,  &QAction::toggled, this, [childCall](bool checked) { childCall(&MdiChild::toggleMagicLens2D, checked); });
	connect(m_ui->actionMagicLens3D,  &QAction::triggered, this, [childCall](bool checked) { childCall(&MdiChild::toggleMagicLens3D, checked); });

	// Layout actions:
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
	m_layoutNames.sort(Qt::CaseInsensitive);
	settings.endGroup();
	if (m_layoutNames.size() == 0)
	{
		m_layoutNames.push_back("none");
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
	if (!showLog)
	{
		iALogWidget::get()->hide();
	}
	iALogWidget::get()->toggleViewAction()->setChecked(showLog);
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

	m_loadSavePreferences = settings.value("Parameters/loadSavePreferences", true).toBool();
	m_loadSaveSlicerSettings = settings.value("Parameters/loadSaveSlicerSettings", true).toBool();
	m_settingsToLoadSave = settings.value("Parameters/settingsToLoadSave").toStringList();
	m_loadSaveCamRen3D = settings.value("Parameters/loadSaveCameraRenderer3D", true).toBool();
	for (int m = 0; m < iASlicerMode::SlicerCount; ++m)
	{
		m_loadSaveSlice[m] = settings.value(QString("Parameters/loadSaveCamera%1").arg(slicerElemName(m)), true).toBool();
	}
	m_loadSaveApplyToAllOpenWindows = settings.value("Parameters/loadSaveApplyToAllOpenWindows", false).toBool();
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
	updateWindowMenu();
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

	auto numRecentFiles = std::min(files.size(), MaxRecentFiles);

	for (qsizetype i = 0; i < numRecentFiles; ++i)
	{
		QString text = tr("&%1 %2").arg(i + 1).arg(QFileInfo(files[i]).fileName());
		m_recentFileActs[i]->setText(text);
		m_recentFileActs[i]->setData(files[i]);
		m_recentFileActs[i]->setVisible(true);
	}
	for (qsizetype j = numRecentFiles; j < MaxRecentFiles; ++j)
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
	QString style;
	if (!m_qssName.isEmpty())
	{
		QFile styleFile(m_qssName);
		if (!styleFile.open(QFile::ReadOnly))
		{
			return;
		}
		QTextStream styleIn(&styleFile);
		style = styleIn.readAll();
		styleFile.close();
	}
	QFile buttonIconFile(QString(":") + (brightMode() ? "bright" : "dark") + "-button-icons.qss");
	if (!buttonIconFile.open(QFile::ReadOnly))
	{
		return;
	}
	QTextStream buttonIconsIn(&buttonIconFile);
	style += buttonIconsIn.readAll();
	buttonIconFile.close();
	qApp->setStyleSheet(style);
#if (!__APPLE__)   // would prevent automatic recognition of bright/light mode on Mac OS, and doesn't change much there anyway:
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
#endif
	// TODO: remove items with unset QPointers? But m_actionIcons will probably never grow really large anyway
	for (auto a : m_actionIcons)
	{
		if (a.first)
		{
			a.first->setIcon(iAThemeHelper::icon(a.second));
		}
	}
	emit styleChanged();
}

bool MainWindow::brightMode() const
{
	return m_qssName.isEmpty() || m_qssName.contains("bright");
}

void MainWindow::addActionIcon(QAction* action, QString const& iconName)
{
	assert(action);
	m_actionIcons.push_back(std::make_pair(QPointer<QAction>(action), iconName));
	action->setIcon(iAThemeHelper::icon(iconName));
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
		if (m_layout->count() == 1 && m_layout->itemText(0) == "none")
		{
			m_layout->clear();
		}
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
	QStringList filesToLoad;
	bool doQuit = false;
	int quitMS = 0;
	bool separateWindows = false;
	bool screenshot = false;
	QString screenshotFileName;
	int w = -1, h = -1;
	for (int a = 1; a < argc; ++a)
	{
		if (QString(argv[a]).startsWith("--quit"))
		{
			++a;
			bool ok = a < argc;
			if (ok)
			{
				quitMS = QString(argv[a]).toInt(&ok);
				doQuit = ok;
			}
			if (!ok)
			{
				LOG(lvlWarn, "Command line arguments: Invalid --quit parameter; must be followed by an integer number (milliseconds) after which to quit, e.g. '--quit 1000'");
			}
		}
		else if (QString(argv[a]).startsWith("--separate"))
		{
			separateWindows = true;
		}
		else if (QString(argv[a]).startsWith("--screenshot"))
		{
			++a;
			if (a >= argc)
			{
				LOG(lvlWarn, "Command line arguments: Invalid --screenshot parameter; must be followed by a filename for the screenshot to be stored!");
			}
			screenshotFileName = QString(argv[a]);
			screenshot = true;
		}
		else if (QString(argv[a]).startsWith("--size"))
		{
			++a;
			bool ok = a < argc;
			if (ok)
			{
				auto size = QString(argv[a]).split("x");
				ok = size.size() == 2;
				if (ok)
				{
					bool ok1, ok2;
					w = size[0].toInt(&ok1);
					h = size[1].toInt(&ok2);
					ok = ok1 && ok2;
				}
			}
			if (!ok)
			{
				LOG(lvlWarn, "Command line arguments: Invalid --size parameter; must be followed by a size in the format WxH (where W and H are width and height)!");
			}
		}
		else
		{
			filesToLoad << QString::fromLocal8Bit(argv[a]);
		}
	}
	if (screenshot && !doQuit)
	{
		LOG(lvlWarn, "Command line arguments: --screenshot was specified, but that will not have any effect without --quit parameter!");
	}
	if (w != -1 && h != -1)
	{
		// using devicePixelRatio() to get actual pixel values
		setGeometry(0, 0, w / devicePixelRatio(), h / devicePixelRatio());
		QGuiApplication::processEvents();
	}
	loadFiles(filesToLoad, iAChildSource::make(separateWindows) );
	if (doQuit)
	{
		auto quitTimer = new QTimer();
		quitTimer->setSingleShot(true);
		auto quitFunc = [quitTimer, quitMS, screenshot, screenshotFileName, this]()
		{
			if (iAJobListView::get()->isAnyJobRunning())
			{
				quitTimer->start(quitMS);
				return;
			}
			if (screenshot)
			{
				QScreen* screen = QGuiApplication::primaryScreen();
				if (auto const window = windowHandle())
				{
					screen = windowHandle()->screen();
				}
				if (!screen)
				{
					LOG(lvlWarn, "Could not get current screen!");
				}
				auto pixmap = screen->grabWindow(winId());
				if (!pixmap.save(screenshotFileName))
				{
					LOG(lvlWarn, QString("Saving screenshot to file %1 failed!").arg(screenshotFileName));
				}
				else
				{
					LOG(lvlInfo, QString("Successfully stored screenshot to file %1.").arg(screenshotFileName));
				}
			}
			LOG(lvlInfo, "Closing application because of --quit parameter");
			quitTimer->deleteLater();
			QApplication::closeAllWindows();
		};
		connect(quitTimer, &QTimer::timeout, this, quitFunc);
		quitTimer->start(quitMS);
	}
}

iAPreferences const & MainWindow::defaultPreferences() const
{
	return m_defaultPreferences;
}

iAModuleDispatcher & MainWindow::moduleDispatcher() const
{
	return *this->m_moduleDispatcher.data();
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

#include <QPainter>
#include <QProxyStyle>

//! An application style used to override certain aspects of the user interface.
//! Specifically, to disable the tooltip delay in iAChartWidget and descendants,
//! and for drawing nice-looking MDI child control buttons in the menu bar on maximized child windows
class iAQProxyStyle : public QProxyStyle
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

	//! @{
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
				.pixmap(buttonIconSize, p->device()->devicePixelRatio());
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

#include "iASCIFIOCheck.h"

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
	QSplashScreen splashScreen{ QPixmap(splashPath) };
	//splashScreen.setWindowFlags(splashScreen.windowFlags() | Qt::WindowStaysOnTopHint);   // don't stay on top - otherwise it covers error messages, e.g. by Visual Studio!
	splashScreen.setWindowOpacity(0.8);
	splashScreen.show();
	iALog::setLogger(iALogWidget::get());
	QString msg;
	showSplashMsg(splashScreen, "Checking OpenGL version...");
	if (!checkOpenGLVersion(msg))
	{
		LOG(lvlWarn, msg);
		bool runningScripted = false;
		for (int a = 1; a < argc; ++a)
		{
			if (QString(argv[a]).startsWith("--quit"))
			{
				runningScripted = true;
				break;
			}
		}
		if (!runningScripted)
		{
			QMessageBox::warning(nullptr, appName, msg);
		}
		return 1;
	}
	app.setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
	iALUT::loadMaps(QCoreApplication::applicationDirPath() + "/colormaps");
	MainWindow mainWin(appName, version, buildInformation, splashPath, splashScreen);
	connect(iASystemThemeWatcher::get(), &iASystemThemeWatcher::themeChanged, &mainWin,
		[&mainWin](bool brightTheme)
		{
			if (mainWin.m_useSystemTheme)
			{
				LOG(lvlDebug, QString("System theme changed and configured to automatically adapt to it, changing to %1 mode (you can override this in the Preferences)!").arg(brightTheme?"bright":"dark"));
				mainWin.m_qssName = qssNameFromSystem();
				iAThemeHelper::setBrightMode(mainWin.brightMode());
				mainWin.applyQSS();
			}
		});
	iASettingsManager::init();
	CheckSCIFIO(QCoreApplication::applicationDirPath());
	app.setWindowIcon(QIcon(QPixmap(iconPath)));
	QApplication::setStyle(new iAQProxyStyle(QApplication::style()));
	mainWin.setWindowIcon(QIcon(QPixmap(iconPath)));
	if (QDate::currentDate().dayOfYear() >= 350)
	{
		mainWin.setWindowTitle("Merry Christmas and a Happy New Year!");
		mainWin.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
		app.setWindowIcon(QIcon(QPixmap(":/images/Xmas.png")));
	}
	mainWin.show();
	mainWin.loadArguments(argc, argv);
	return app.exec();
}
