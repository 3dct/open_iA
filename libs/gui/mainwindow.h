// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iagui_export.h"

#include "iAMainWindow.h"
#include "iAPreferences.h"
#include "iASlicerSettings.h"
#include "iARawFileParameters.h"

#include <QMdiArea>
#include <QPointer>

#include <memory>

class QAction;
class QActionGroup;
class QComboBox;
class QMdiSubWindow;
class QMenu;
class QSplashScreen;

class MdiChild;
class iADockWidgetWrapper;
class iAFileIO;
class iAModuleDispatcher;

class Ui_MainWindow;

//! Application main window; implementation of iAMainWindow interface
class iAgui_API MainWindow : public iAMainWindow
{
	Q_OBJECT

public:
	MainWindow(QString const & appName, QString const & version, QString const& buildInformation,
		QString const & splashImage, QSplashScreen& splashScreen, iADockWidgetWrapper* dwJobs);
	~MainWindow() override;
	static int runGUI(int argc, char * argv[], QString const & appName, QString const & version, QString const& buildInformation,
		QString const & splashPath, QString const & iconPath);

	void setPath(QString const & p) override;
	QString const & path() const override;
	//! add a file to the list of recently loaded/saved files
	void addRecentFile(const QString &fileName);
	//! Opens multiple files in an existing or new window
	//! @param fileNames the list of the file names of the files to load
	//! @param child the child window to load the files into. If nullptr, a new window is created **for each file**
	void loadFiles(QStringList fileNames, iAMdiChild* child = nullptr);
	//! see iAMainWindow::loadFile
	void loadFile(QString const& fileName, iAMdiChild* child = nullptr, std::shared_ptr<iAFileIO> io = nullptr) override;

	QMenu * fileMenu() override;
	QMenu* editMenu() override;
	QMenu* filtersMenu() override;
	QMenu* toolsMenu() override;
	QMenu* helpMenu() override;

	//! Provides access to the currently active mdi child, if such is available.
	//! @return pointer to the currently active mdi child, or nullptr if no child is currently open
	iAMdiChild * activeMdiChild() override;
	//! Provides access to a second loaded mdi child, if such is available.
	//! Will throw an error if none is available or more than two are loaded.
	//! @deprecated instead of this method, in filters, use the facilities
	//!     provided in iAFilter (via the requiredInputs parameter to the constructor) to specify multiple inputs
	iAMdiChild * secondNonActiveChild() override;
	//! Get list of the titles of currently open MdiChild windows.
	QList<QString> mdiWindowTitles();
	//! Get the list of current MdiChild windows.
	QList<iAMdiChild*> mdiChildList() override;
	//! Get the list of current child windows of type T.
	template <typename T> QList<T*> childList(QMdiArea::WindowOrder order = QMdiArea::CreationOrder);
	//! Get the active child window of type T.
	template <typename T> T * activeChild();
	QMdiSubWindow* activeChild() override;
	QMdiSubWindow* addSubWindow(QWidget * child) override;
	iAPreferences const& defaultPreferences() const override;
	iAModuleDispatcher& moduleDispatcher() const override;
	iAMdiChild* createMdiChild(bool unsavedChanges) override;
	void closeMdiChild(iAMdiChild* child) override;
	void closeAllSubWindows() override;
	void updateInteractionModeControls(int mode);
	void updateMagicLens2DCheckState(bool enabled);
	void makeActionChildDependent(QAction* action) override;

	//! whether the current qss theme is bright mode (true) or dark mode (false)
	bool brightMode() const;

	void addActionIcon(QAction* action, QString const& iconName) override;

public slots:
	void loadLayout();
	void slicerSettings();

signals:
	void closing();
	void fullScreenToggled();

private slots:
	void openRaw();
	void openWithDataTypeConversion();
	void openTLGICTData();
	void loadSettings();
	void saveSettings();
	void saveProject();
	void saveVolumeStack();
	void prefs();
	void linkViews();
	void linkMDIs();
	void toggleSlicerInteraction();
	void toggleFullScreen();
	void toggleMenu();
	void changeInteractionMode(bool isChecked);
	void rendererSyncCamera();
	void saveCameraSettings();
	void loadCameraSettings();
	void openRecentFile();
	void listDockWidgetsInMenu();
	void toggleOpenLogOnNewMessage();
	void toggleOpenListOnAddedJob();
	void toggleToolbar();
	void saveLayout();
	void resetLayout();
	void deleteLayout();
	void updateMenus();
	void updateWindowMenu();
	void setActiveSubWindow(QWidget *window);
	void toggleMdiViewMode();

private:
	//! internal retriever for MdiChild object (instead of iAMdiChild interface)
	MdiChild* activeMDI();
	void closeEvent(QCloseEvent *e) override;
	void dragEnterEvent(QDragEnterEvent *e) override;
	void dropEvent(QDropEvent *e) override;
	bool event(QEvent* e) override; //!< required to catch StyleChange event (changeEvent not triggered)
	void connectSignalsToSlots();
	void readSettings();
	void writeSettings();
	void createRecentFileActions();
	void updateRecentFileActions();
	void applyQSS();
	void setModuleActionsEnabled( bool isEnabled );
	void loadTLGICTData(QString const & baseDirectory);
	iAMdiChild* askWhichChild();
	bool keepOpen();
	void loadArguments(int argc, char** argv);

	static const int MaxRecentFiles = 8;

	QPixmap m_splashScreenImg;
	QAction *m_separatorAct;
	QAction *m_recentFileActs[MaxRecentFiles];
	QActionGroup *m_slicerToolsGroup, *m_mdiViewModeGroup;
	//! resource path of currently loaded stylesheets (.qss)
	QString m_qssName;
	//! whether the option of bright/dark theme should adapt to the system theme
	bool m_useSystemTheme;
	iASlicerSettings m_defaultSlicerSettings;
	iAPreferences m_defaultPreferences;

	//! @{ Open with DataType Conversion settings
	unsigned int m_owdtcs;
	int m_owdtcxori, m_owdtcyori, m_owdtczori,
		m_owdtcxsize, m_owdtcysize, m_owdtczsize,
		m_owdtcdov;
	iARawFileParameters m_rawFileParams;
	double m_owdtcoutmin, m_owdtcoutmax, m_owdtcmin, m_owdtcmax;
	//! @}

	QStringList m_settingsToLoadSave;  //!< list of default settings that by default are selected for loading from /saving to an XML settings file
	bool m_loadSavePreferences, m_loadSaveSlicerSettings;
	bool m_loadSaveCamRen3D, m_loadSaveSlice[3], m_loadSaveApplyToAllOpenWindows;   //!< camera settings: whether to save 3D renderer, slicer cameras, and whether to apply camera settings to all

	QString m_defaultLayout;
	QString m_path;
	QComboBox * m_layout;
	QScopedPointer<iAModuleDispatcher> m_moduleDispatcher;
	//! actions from modules which should only be enabled when a child is active:
	QVector<QAction*> m_childDependentActions;
	QStringList m_layoutNames;
	QString m_gitVersion, m_buildInformation;

	std::shared_ptr<Ui_MainWindow> m_ui;

	//! the job list dock widget
	iADockWidgetWrapper* m_dwJobs;
	//! whether the job list should be automatically shown when a new job is added to the list:
	bool m_openJobListOnNewJob;

	//! list of actions and their associated icon name
	std::vector<std::pair<QPointer<QAction>, QString>> m_actionIcons;
};
