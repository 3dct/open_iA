/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "ui_Mainwindow.h"
#include "open_iA_Core_export.h"

#include "iAPreferences.h"
#include "iARenderSettings.h"
#include "iASlicerSettings.h"
#include "iAVolumeSettings.h"

#include <QMainWindow>
#include <QSharedPointer>

QT_BEGIN_NAMESPACE
class QAction;
class QActionGroup;
class QComboBox;
class QDomDocument;
class QDomElement;
class QDomNode;
class QMenu;
class QMdiArea;
class QMdiSubWindow;
class QSignalMapper;
class QLabel;
class QSplashScreen;
QT_END_NAMESPACE

class vtkCamera;
class vtkImageData;
class vtkRenderer;

class dlg_transfer;
class iAModalityList;
class iAModuleDispatcher;
class MdiChild;

class open_iA_Core_API MainWindow : public QMainWindow, public Ui_MainWindow
{
	Q_OBJECT

public:
	MainWindow(QString const & appName, QString const & version, QString const & splashImage);
	~MainWindow();
	static int RunGUI(int argc, char * argv[], QString const & appName, QString const & version,
		QString const & splashPath, QString const & iconPath);
	static void InitResources();
	void setCurrentFile(const QString &fileName);
	void updateRecentFileActions();

	void setPath(QString p) { path = p; };
	QString getPath() { return path; };

	void LoadFile(QString const & fileName);
	void LoadFile(QString fileName, bool isStack);
	void LoadFiles(QStringList fileNames);

	QDomDocument loadSettingsFile(QString filename);
	void saveSettingsFile(QDomDocument &doc, QString filename);
	void saveCamera(QDomDocument &doc);
	void loadCamera(QDomNode &cameraNode);
	void saveSliceViews(QDomDocument &doc);
	void saveSliceView(QDomDocument &doc, QDomNode &sliceViewsNode, vtkRenderer *ren, char const *elemStr);
	void loadSliceViews(QDomNode &sliceViewsNode);
	void saveTransferFunction(QDomDocument &doc, dlg_transfer* transferFunction);
	void saveProbabilityFunctions(QDomDocument &doc);
	void loadProbabilityFunctions(QDomNode &functionsNode);
	void savePreferences(QDomDocument &doc);
	void loadPreferences(QDomNode &preferencesNode);
	void saveRenderSettings(QDomDocument &doc);
	void loadRenderSettings(QDomNode &renderSettingsNode);
	void saveSlicerSettings(QDomDocument &doc);
	void loadSlicerSettings(QDomNode &slicerSettingsNode);

	void removeNode(QDomNode &node, char const *str);

	QList<QString> mdiWindowTitles();

	QMenu * getToolsMenu();
	QMenu * getFiltersMenu();
	QMenu * getHelpMenu();
	QMenu * getFileMenu();
	MdiChild *GetResultChild( QString const & title );
	MdiChild *GetResultChild( int childInd, QString const & title );
	MdiChild *GetResultChild( MdiChild* oldChild, QString const & title );
	MdiChild *activeMdiChild();
	QList<MdiChild*> MdiChildList(QMdiArea::WindowOrder order = QMdiArea::CreationOrder);
	QMdiSubWindow* addSubWindow(QWidget * child);
	QString getCurFile() { return curFile; }	//!< deprecated. Use a specific mdichilds or even an mdichilds dlg_modalities methods instead!
	void LoadArguments(int argc, char** argv);
	iAPreferences const & GetDefaultPreferences() const;
protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void timeout();
	void Open();
	void OpenRaw();
	void OpenImageStack();
	void OpenVolumeStack();
	void OpenTLGICTData();
	void save();
	void saveAs();
	bool loadSettings();
	bool saveSettings();
	void LoadProject();
	void SaveProject();
	void maxXY();
	void maxXZ();
	void maxYZ();
	void maxRC();
	void multi();
	void prefs();
	void linkViews();
	void linkMDIs();
	void enableInteraction();
	void renderSettings();
	void slicerSettings();
	void loadTransferFunction();
	void saveTransferFunction();
	void deletePoint();
	void changeColor();
	void autoUpdate(bool toggled);
	void updateViews();
	void resetView();
	void resetTrf();
	void toggleSnakeSlicer(bool isChecked);
	void toggleMagicLens(bool isChecked);
	void raycasterCamPX();
	void raycasterCamPY();
	void raycasterCamPZ();
	void raycasterCamMX();
	void raycasterCamMY();
	void raycasterCamMZ();
	void raycasterCamIso();
	void raycasterAssignIso();
	void raycasterSaveCameraSettings();
	void raycasterLoadCameraSettings();
	void OpenRecentFile();
	void childClosed();
	void ToggleMainWindowStatusBar();
	void ToggleChildStatusBar();
	void ToggleToolbar();
public slots:
	void saveLayout();
	void loadLayout();
	void resetLayout();
	void deleteLayout();
	void toggleSliceProfile(bool isChecked);
	void childActivatedSlot(QMdiSubWindow *wnd);
	void OpenWithDataTypeConversion();
	void about();
	void updateMenus();
	void updateWindowMenu();
	MdiChild *createMdiChild(bool unsavedChanges);
	void setActiveSubWindow(QWidget *window);
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void setHistogramFocus();
	void CloseAllSubWindows();

signals:
	void StyleChanged();
private:
	void connectSignalsToSlots();
	void readSettings();
	void writeSettings();
	void createRecentFileActions();
	void applyQSS();
	void SetModuleActionsEnabled( bool isEnabled );
	void loadCamera(QDomNode const & node, vtkCamera* camera);
	void saveCamera(QDomElement &cameraElement, vtkCamera* camera);
	void copyFunctions(MdiChild* oldChild, MdiChild* newChild);
	void LoadProject(QString const & fileName);
	void LoadTLGICTData(QString const & baseDirectory);
	bool KeepOpen();
	MdiChild* findMdiChild(const QString &fileName);
	QString strippedName(const QString &fullFileName);

	QSplashScreen *splashScreen;
	QAction *separatorAct;
	enum { MaxRecentFiles = 8 };
	QAction *recentFileActs[MaxRecentFiles];
	QActionGroup *slicerToolsGroup;
	QSignalMapper *windowMapper;
	QString qssName;
	iAVolumeSettings defaultVolumeSettings;	
	iARenderSettings defaultRenderSettings;
	iASlicerSettings defaultSlicerSettings;
	iAPreferences defaultPreferences;

	//! @{ Open with DataType Conversion settings
	int owdtcs,
		owdtcx, owdtcy, owdtcz,
		owdtcxori, owdtcyori, owdtczori,
		owdtcxsize, owdtcysize, owdtczsize,
		owdtcdov;
	double owdtcsx, owdtcsy, owdtcsz,
		owdtcoutmin, owdtcoutmax;
	float owdtcmin, owdtcmax;
	//! @}

	bool lpCamera, lpSliceViews, lpTransferFunction, lpProbabilityFunctions, lpPreferences, lpRenderSettings, lpSlicerSettings;
	bool spCamera, spSliceViews, spTransferFunction, spProbabilityFunctions, spPreferences, spRenderSettings, spSlicerSettings;

	QString defaultLayout;
	QString curFile, path;
	QTimer *timer;
	QComboBox * layout;
	QScopedPointer<iAModuleDispatcher> m_moduleDispatcher;
	QStringList layoutNames;
	QString m_gitVersion;
};
