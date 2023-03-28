// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAgui_export.h"

#include "iAMainWindow.h"
#include "iAPreferences.h"
#include "iARenderSettings.h"
#include "iASlicerSettings.h"
#include "iAVolumeSettings.h"
#include "iARawFileParameters.h"

#include <QMdiArea>
#include <QMdiSubWindow>

#include <memory>
#include <vector>

class QAction;
class QActionGroup;
class QComboBox;
class QDomDocument;
class QDomElement;
class QDomNode;
class QMenu;
class QLabel;
class QSplashScreen;

class vtkCamera;

class MdiChild;
class iADockWidgetWrapper;
class iAFileIO;
class iAModuleDispatcher;
class iATransferFunction;
class iAXmlSettings;

class Ui_MainWindow;

//! Application main window; implementation of iAMainWindow interface
class iAgui_API MainWindow : public iAMainWindow
{
	Q_OBJECT

public:
	MainWindow(QString const & appName, QString const & version, QString const& buildInformation, QString const & splashImage, iADockWidgetWrapper* dwJobs);
	~MainWindow() override;
	static int runGUI(int argc, char * argv[], QString const & appName, QString const & version, QString const& buildInformation,
		QString const & splashPath, QString const & iconPath);

	void setPath(QString const & p) override;
	QString const & path() const override;
	//! add a file to the list of recently loaded/saved files
	void addRecentFile(const QString &fileName);

	void loadFiles(QStringList fileNames);

	//! TODO NEWIO: create signal triggered on new child (fully) created
	void loadFileNew(QString const& fileName, iAMdiChild* child = nullptr, std::shared_ptr<iAFileIO> io = nullptr) override;

	//! Get the File menu (can be used by modules to append entries to it).
	QMenu * fileMenu() override;
	//! Get the Filters menu (can be used by modules to append entries to it).
	QMenu* filtersMenu() override;
	//! Get the Tools menu (can be used by modules to append entries to it).
	QMenu* toolsMenu() override;
	//! Get the Help menu (can be used by modules to append entries to it).
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
	iARenderSettings const& defaultRenderSettings() const override;
	iAVolumeSettings const& defaultVolumeSettings() const override;
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
	void renderSettings();
	void slicerSettings();

signals:
	void closing();
	void fullScreenToggled();

private slots:
	void quitTimerSlot();
	void hideSplashSlot();
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
	void rendererSaveCameraSettings();
	void rendererLoadCameraSettings();
	void openRecentFile();
	void childClosed();
	void listDockWidgetsInMenu();
	void toggleOpenLogOnNewMessage();
	void toggleOpenListOnAddedJob();
	void toggleToolbar();
	void about();
	void wiki();
	void saveLayout();
	void resetLayout();
	void deleteLayout();
	void updateMenus();
	void updateWindowMenu();
	void setActiveSubWindow(QWidget *window);
	void toggleMdiViewMode();
	void removeActionIcon();

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
	// TDOO NEWIO: currently unused, but functionality should be available for any filter (general runner options - re-use transfer function, result in new/existing window, copy non-TF functions
	void copyFunctions(MdiChild* oldChild, MdiChild* newChild);
	void loadTLGICTData(QString const & baseDirectory);
	iAMdiChild* askWhichChild();
	bool keepOpen();
	void loadArguments(int argc, char** argv);

	void loadCamera(QDomNode const & node, vtkCamera* camera);
	void saveCamera(QDomElement &cameraElement, vtkCamera* camera);
	void saveCamera(iAXmlSettings& xml);
	bool loadCamera(iAXmlSettings& xml);
	void saveSliceViews(iAXmlSettings& xml);
	void saveSliceView(QDomDocument& doc, QDomNode& sliceViewsNode, vtkCamera* ren, QString const& elemStr);
	void loadSliceViews(QDomNode sliceViewsNode);
	void savePreferences(iAXmlSettings& xml);
	void loadPreferences(QDomNode preferencesNode);
	void saveRenderSettings(iAXmlSettings& xml);
	void loadRenderSettings(QDomNode renderSettingsNode);
	void saveSlicerSettings(iAXmlSettings& xml);
	void loadSlicerSettings(QDomNode slicerSettingsNode);

	static const int MaxRecentFiles = 8;

	QSplashScreen *m_splashScreen;
	QPixmap m_splashScreenImg;
	QAction *m_separatorAct;
	QAction *m_recentFileActs[MaxRecentFiles];
	QActionGroup *m_slicerToolsGroup, *m_mdiViewModeGroup;
	//! resource path of currently loaded stylesheets (.qss)
	QString m_qssName;
	//! whether the option of bright/dark theme should adapt to the system theme
	bool m_useSystemTheme;
	iAVolumeSettings m_defaultVolumeSettings;
	iARenderSettings m_defaultRenderSettings;
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

	//! which settings to load from an XML settings file:
	bool m_lpCamera, m_lpSliceViews, m_lpPreferences, m_lpRenderSettings, m_lpSlicerSettings;
	//! which settings to save to an XML settings file:
	bool m_spCamera, m_spSliceViews, m_spPreferences, m_spRenderSettings, m_spSlicerSettings;

	QString m_defaultLayout;
	QString m_path;
	QTimer *m_splashTimer, *m_quitTimer;
	QComboBox * m_layout;
	QScopedPointer<iAModuleDispatcher> m_moduleDispatcher;
	//! actions from modules which should only be enabled when a child is active:
	QVector<QAction*> m_childDependentActions;
	QStringList m_layoutNames;
	QString m_gitVersion, m_buildInformation;

	QSharedPointer<Ui_MainWindow> m_ui;

	//! the job list dock widget
	iADockWidgetWrapper* m_dwJobs;
	//! whether the job list should be automatically shown when a new job is added to the list:
	bool m_openJobListOnNewJob;

	//! list of actions and their associated icon name:
	QMap<QAction*, QString> m_actionIcons;
};
