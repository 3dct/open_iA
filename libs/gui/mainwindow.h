/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#pragma once

#include "iAgui_export.h"

#include "iAMainWindow.h"
#include "iAPreferences.h"
#include "iARenderSettings.h"
#include "iASlicerSettings.h"
#include "iAVolumeSettings.h"
#include "iARawFileParameters.h"

#include <vtkSmartPointer.h>

#include <QMdiArea>
#include <QMdiSubWindow>

#include <memory>
#include <vector>

class vtkPolyData;
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
class vtkImageData;

class MdiChild;
class iADockWidgetWrapper;
class iAFileIO;
class iAModalityList;
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
	static void initResources();

	void setPath(QString const & p) override;
	QString const & path() const override;
	//! add a file to the list of recently loaded/saved files
	void addRecentFile(const QString &fileName);

	void loadFile(QString const & fileName);
	void loadFile(QString fileName, bool isStack) override;
	void loadFiles(QStringList fileNames);

	void loadFileNew(QString const& fileName, bool newWindow, std::shared_ptr<iAFileIO> io = nullptr);

	void saveCamera(iAXmlSettings & xml);
	bool loadCamera(iAXmlSettings & xml);
	void saveSliceViews(iAXmlSettings & xml);
	void saveSliceView(QDomDocument &doc, QDomNode &sliceViewsNode, vtkCamera *ren, QString const & elemStr);
	void loadSliceViews(QDomNode sliceViewsNode);
	void saveTransferFunction(QDomDocument& doc, iATransferFunction* transferFunction);
	void savePreferences(iAXmlSettings &xml);
	void loadPreferences(QDomNode preferencesNode);
	void saveRenderSettings(iAXmlSettings &xml);
	void loadRenderSettings(QDomNode renderSettingsNode);
	void saveSlicerSettings(iAXmlSettings &xml);
	void loadSlicerSettings(QDomNode slicerSettingsNode);
	//! Get the File menu (can be used by modules to append entries to it).
	QMenu * fileMenu() override;
	//! Get the Filters menu (can be used by modules to append entries to it).
	QMenu* filtersMenu() override;
	//! Get the Tools menu (can be used by modules to append entries to it).
	QMenu* toolsMenu() override;
	//! Get the Help menu (can be used by modules to append entries to it).
	QMenu* helpMenu() override;
	//! @{ Get access to result child with the given title.
	//! (depending on preferences, this will either open a new mdi child window, or reuse the currently active one)
	//! @deprecated
	iAMdiChild * resultChild( QString const & title ) override;
	iAMdiChild * resultChild( int childInd, QString const & title ) override;
	iAMdiChild * resultChild( iAMdiChild* oldChild, QString const & title ) override;
	//! @}
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
	void loadArguments(int argc, char** argv);
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
	bool brightMode() const override;

public slots:
	void loadLayout();

signals:
	void closing();
	void fullScreenToggled();

private slots:
	void quitTimerSlot();
	void hideSplashSlot();
	void openRaw();
	void openVolumeStack();
	void openWithDataTypeConversion();
	void openTLGICTData();
	void save();
	void saveNew();
	void saveAs();
	void loadSettings();
	void saveSettings();
	void saveProject();
	void saveVolumeStack();
	void maxXY();
	void maxXZ();
	void maxYZ();
	void maxRC();
	void multi();
	void prefs();
	void linkViews();
	void linkMDIs();
	void enableInteraction();
	void toggleFullScreen();
	void toggleMenu();
	void renderSettings();
	void slicerSettings();
	void loadTransferFunction();
	void saveTransferFunctionSlot();
	void deletePoint();
	void changeColor();
	void resetView();
	void resetTrf();
	void changeInteractionMode(bool isChecked);
	void meshDataMovable(bool isChecked);
	void toggleSnakeSlicer(bool isChecked);
	void toggleMagicLens(bool isChecked);
	void toggleMagicLens3D(bool isChecked);
	void rendererCamPosition();
	void rendererSyncCamera();
	void rendererSaveCameraSettings();
	void rendererLoadCameraSettings();
	void openRecentFile();
	void childClosed();
	void listDockWidgetsInMenu();
	void toggleMainWindowStatusBar();
	void toggleOpenLogOnNewMessage();
	void toggleOpenListOnAddedJob();
	void toggleChildStatusBar();
	void toggleToolbar();
	void about();
	void wiki();
	void saveLayout();
	void resetLayout();
	void deleteLayout();
	void toggleSliceProfile(bool isChecked);
	void toggleEditProfilePoints(bool isChecked);
	void updateMenus();
	void updateWindowMenu();
	void setActiveSubWindow(QWidget *window);
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void toggleMdiViewMode();

private:
	//! internal retriever for MdiChild object (instead of iAMdiChild interface)
	MdiChild* activeMDI();
	void closeEvent(QCloseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent *e) override;
	void dropEvent(QDropEvent *e) override;
	void connectSignalsToSlots();
	void readSettings();
	void writeSettings();
	void createRecentFileActions();
	void updateRecentFileActions();
	void applyQSS();
	void setModuleActionsEnabled( bool isEnabled );
	void loadCamera(QDomNode const & node, vtkCamera* camera);
	void saveCamera(QDomElement &cameraElement, vtkCamera* camera);
	void copyFunctions(MdiChild* oldChild, MdiChild* newChild);
	void loadTLGICTData(QString const & baseDirectory);
	void loadFileAskNewWindow(QString const& fileName);
	bool keepOpen();

	static const int MaxRecentFiles = 8;

	QSplashScreen *m_splashScreen;
	QPixmap m_logoImg;
	QAction *m_separatorAct;
	QAction *m_recentFileActs[MaxRecentFiles];
	QActionGroup *m_slicerToolsGroup, *m_mdiViewModeGroup;
	QString m_qssName;
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
	bool m_lpCamera, m_lpSliceViews, m_lpTransferFunction, m_lpProbabilityFunctions, m_lpPreferences, m_lpRenderSettings, m_lpSlicerSettings;
	//! which settings to save to an XML settings file:
	bool m_spCamera, m_spSliceViews, m_spTransferFunction, m_spProbabilityFunctions, m_spPreferences, m_spRenderSettings, m_spSlicerSettings;

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
};
