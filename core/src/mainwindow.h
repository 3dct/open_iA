/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
*          Stelzhamerstraße 23, 4600 Wels / Austria, Email:                           *
* ************************************************************************************/
 
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_Mainwindow.h"
#include "open_iA_Core_export.h"

#include <QMainWindow>

#include <string>

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

class MdiChild;
class vtkCamera;
class vtkImageData;
class vtkImageViewer2;
class vtkRenderer;

class iAModuleDispatcher;
class dlg_transfer;
class vtkCamera;

class open_iA_Core_API MainWindow : public QMainWindow, public Ui_MainWindow
{
	Q_OBJECT

public:
	MainWindow(QString const & appName, QString const & version, QString const & splashImage);
	~MainWindow();
	static void InitResources();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void timeout();
	void newFile();
	void open();
	void openImageStack();
	void openVolumeStack();
	void save();
	void saveAs();
	void saveAsImageStack();
	void saveScreen();
	bool loadSettings();
	bool saveSettings();
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
	void openRecentFile();
	void childClosed();
	void ToggleMainWindowStatusBar();
	void ToggleChildStatusBar();
	void OpenProject();
	void SaveProject();
	void OpenTLGICTData();

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
	MdiChild *createMdiChild();
	void switchLayoutDirection();
	void setActiveSubWindow(QWidget *window);
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void setHistogramFocus();
	void tabChanged(int index);

public:
	bool isStack;
	//void setFeaturesFromMasks(QStringList featuremask);
	//int updateVisibility(void);
	//int exportVisibility(QString filename);
	void setCurrentFile(const QString &fileName);
	void updateRecentFileActions();
	int getPrefHistoBinCnt() { return prefHistogramBins; };
	bool getPrefCompression() { return prefCompression; };
	bool getPrefMedianFilterFistogram() { return prefMedianFilterFistogram; };
	bool getPrefResultInNewWindow() { return prefResultInNewWindow; };
	
	bool getShowVolume() { return rsShowVolume; };
	bool getShowSlicers() { return rsShowSlicers; };
	bool getShowHelpers() { return rsShowHelpers; };
	bool getShowRPosition() { return rsShowRPosition; };
	bool getLinearInterpolation() { return rsLinearInterpolation; };
	bool getShading() { return rsShading; };
	bool getBoundingBox() { return rsBoundingBox; };
	bool getParallelProjection() { return rsParallelProjection; };
	
	double getImageSampleDistance() { return rsImageSampleDistance; };
	double getSampleDistance() { return rsSampleDistance; };
	double getAmbientLighting() { return rsAmbientLighting; };
	double getDiffuseLighting() { return rsDiffuseLighting; };
	double getSpecularLighting() { return rsSpecularLighting; };
	double getSpecularPower() { return rsSpecularPower; };
	QString getBackgroundTop() { return rsBackgroundTop; };
	QString getBackgroundBottom() { return rsBackgroundBottom; };
	QColor *getColors() { return colors; }

	void setImageSampleDistance( double d ) { rsImageSampleDistance = d; };
	void setSampleDistance( double d ) { rsSampleDistance = d; };
	void setPath(QString p) { path = p; };
	QString getPath() { return path; };

	void loadFile(QString fileName);
	void loadFiles(QStringList fileNames);

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
	MdiChild *activeMdiChild();
	QList<QMdiSubWindow*> MdiChildList(QMdiArea::WindowOrder order = QMdiArea::CreationOrder);
	int SelectInputs(QString winTitel, int n, QStringList inList, int * out_inputIndxs, bool modal = true);
	void addSubWindow(QWidget * child);
	QString getCurFile() { return curFile; }
	void loadLayout(MdiChild* child, QString const & layout);

private:
	void connectSignalsToSlots();
	void setupToolBars();
	void setupStatusBar();
	void readSettings();
	void writeSettings();
	void createRecentFileActions();
	void groupActions();
	void applyQSS();
	void SetModuleActionsEnabled( bool isEnabled );
	void loadFileInternal(QString fileName);
	void loadCamera(QDomNode const & node, vtkCamera* camera);
	void saveCamera(QDomElement &cameraElement, vtkCamera* camera);

	QSplashScreen *splashScreen;
		
	QMdiSubWindow *findMdiChild(const QString &fileName);
	QString strippedName(const QString &fullFileName);

	double neighborhood(vtkImageData *imageData, int x0, int y0, int z0);

	QAction *separatorAct;
	enum { MaxRecentFiles = 8 };
	QAction *recentFileActs[MaxRecentFiles];
	QActionGroup *slicerToolsGroup;

	QSignalMapper *windowMapper;
	
	int prefHistogramBins, prefStatExt;
	int prefMagicLensSize, prefMagicLensFrameWidth;
	bool prefCompression, prefResultInNewWindow, prefMedianFilterFistogram;
	
	QString qssName;
	bool rsShowVolume, rsShowSlicers, rsShowHelpers, rsShowRPosition, rsLinearInterpolation, rsShading, rsBoundingBox, rsParallelProjection;
	double rsImageSampleDistance, rsSampleDistance, rsAmbientLighting, rsDiffuseLighting, rsSpecularLighting, rsSpecularPower;
	double ssMinIsovalue, ssMaxIsovalue;
	int ssNumberOfIsolines, ssSnakeSlices;
	bool ssLinkViews, ssShowIsolines, ssShowPosition, ssImageActorUseInterpolation, ssInteractionEnabled, ssShowPorosityMaps, ssLinkMDIs;
	float dtcmin, dtcmax; double dtcoutmin, dtcoutmax; int dtcdov ;//MAE grayvalue filter
	int owdtcs, owdtcx,owdtcy,owdtcz, owdtcxori, owdtcyori, owdtczori, owdtcxsize, owdtcysize, owdtczsize; double owdtcsx, owdtcsy, owdtcsz;//openwithdatatypeconversion
	float owdtcmin, owdtcmax; double owdtcoutmin, owdtcoutmax; int owdtcdov ;//openwithdatatype

	QString rsBackgroundTop, rsBackgroundBottom;
	int rsRenderMode;

	bool lpCamera, lpSliceViews, lpTransferFunction, lpProbabilityFunctions, lpPreferences, lpRenderSettings, lpSlicerSettings;
	bool spCamera, spSliceViews, spTransferFunction, spProbabilityFunctions, spPreferences, spRenderSettings, spSlicerSettings;

	QString defaultLayout;

	QString movFileName; //mean object visualization parameter

	int fvDiscretizationFactor;
	QString fvFileName;

	QString curFile, path;
	QColor colors[7];

	QTimer *timer;
	//bool isStack;

	QComboBox * layout;

	QScopedPointer<iAModuleDispatcher> m_moduleDispatcher;
	QStringList layoutNames;
	QString m_gitVersion;
};

#endif
