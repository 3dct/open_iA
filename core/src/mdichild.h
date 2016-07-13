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
 
#ifndef MDICHILD_H
#define MDICHILD_H

#include "defines.h"
#include "iAChannelVisualizationData.h"
#include "iAChannelID.h"
#include "iARenderSettings.h"
#include "iAVolumeSettings.h"
#include "iAVolumeStack.h"
#include "open_iA_Core_export.h"

#include <vtkSmartPointer.h>
#include <vtkTable.h>

#include <QFileInfo>
#include <QMainWindow>
#include <QMap>
#include <QString>
#include <QSharedPointer>

#include <vector>

#include "ui_logs.h"
#include "ui_Mdichild.h"
#include "ui_renderer.h"
#include "ui_sliceXY.h"
#include "ui_sliceXZ.h"
#include "ui_sliceYZ.h"

#include "iAQTtoUIConnector.h"

class QProgressBar;

class vtkAbstractTransform;
class vtkActor;
class vtkColorTransferFunction;
class vtkCornerAnnotation;
class vtkImageAccumulate;
class vtkImageCast;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkPoints;
class vtkPolyData;
class vtkRenderWindow;
class vtkTable;
class vtkTransform;

struct CBCTReconstructionSettings;
class dlg_charts;
class dlg_renderer;
class dlg_function;
class dlg_histogram;
class dlg_imageproperty;
class dlg_modalities;
class dlg_periodicTable;
class dlg_profile;
class dlg_volumePlayer;
class iAAlgorithms;
class iAHistogramWidget;
class iAIO;
class iALogger;
class iAModalityList;
class iAParametricSpline;
struct iAProfileProbe;
class iARenderer;
class iASimReader;
class iASlicer;
class iASlicerData;
class MainWindow;

typedef iAQTtoUIConnector<QDockWidget, Ui_sliceXY>   dlg_sliceXY;
typedef iAQTtoUIConnector<QDockWidget, Ui_sliceXZ>   dlg_sliceXZ;
typedef iAQTtoUIConnector<QDockWidget, Ui_sliceYZ>   dlg_sliceYZ;
typedef iAQTtoUIConnector<QDockWidget, Ui_logs>   dlg_logs;

class open_iA_Core_API MdiChild : public QMainWindow, public Ui_Mdichild
{
	Q_OBJECT
public:
	dlg_renderer * r;
	dlg_sliceXY * sXY;
	dlg_sliceXZ * sXZ;
	dlg_sliceYZ * sYZ;
	dlg_logs * logs;
	QProgressBar * pbar;
private:
	QByteArray m_beforeMaximizeState;
	bool m_isSmthMaximized;
	QDockWidget * m_whatMaximized;
	int m_pbarMaxVal;
	void maximizeDockWidget(QDockWidget * dw);
	void demaximizeDockWidget(QDockWidget * dw);
	void resizeDockWidget(QDockWidget * dw);
public:
	enum ConnectionState {cs_NONE, cs_ROI};

	/** waits for the IO thread to finish in case any I/O operation is running; otherwise it will immediately exit */
	void waitForPreviousIO();

	MdiChild(MainWindow * mainWnd);
	~MdiChild();

	void newFile();
	void showPoly();
	bool loadFile(const QString &f, bool isStack);
	bool displayResult(QString const & title, vtkImageData* image = NULL, vtkPolyData* poly = NULL);
	bool save();
	bool saveAs();
	bool saveAsImageStack();
	bool saveFile(const QString &f);
	void setUpdateSliceIndicator(bool updateSI) {updateSliceIndicator = updateSI;}
	void updateLayout();

	bool multiview() { changeVisibility(MULTI); return true; };
	bool xyview() { maximizeXY(); return true; };
	bool xzview() { maximizeXZ(); return true; };
	bool yzview() { maximizeYZ(); return true; };
	bool rcview() { maximizeRC(); return true; };
	bool linkViews( bool l ) { link(l); return true; }
	bool linkMDIs( bool l ) { linkM(l); return true; }
	bool editPrefs( int h, int mls, int mlfw, int e, bool c, bool m, bool r, bool init );
	bool editRendererSettings(iARenderSettings const & rs, iAVolumeSettings const & vs);
	void applyCurrentSettingsToRaycaster(iARenderer * raycaster);
	bool editSlicerSettings( bool lv, bool sil, bool sp, int no, double min, double max, bool li, int ss, bool lm);
	bool loadTransferFunction();
	bool saveTransferFunction();
	void saveRenderWindow(vtkRenderWindow *renderWindow);

	/**
	* Provides the possibility to save a slice movie of the given slice view.
	*
	* \param [in]	slicer	the VTK slicer the moview shall be exported from.
	*/
	void saveMovie(iASlicer * slicer);

	/**
	* Provides the possibility to save a raycaster movie of the given raycaster view.
	*
	* \param [in]	raycaster the VTK raycaster, the movie shall be exported from.
	*/
	void saveMovie(iARenderer& raycaster);
	int deletePoint();
	void changeColor();
	void autoUpdate(bool toggled);
	void resetView();
	void resetTrf();
	void toggleSnakeSlicer(bool isEnabled);
	bool isSnakeSlicerToggled() const;
	void toggleSliceProfile(bool isEnabled);
	bool isSliceProfileToggled(void) const;
	void enableInteraction(bool b);
	void setupRaycaster(iARenderSettings const & rs, iAVolumeSettings const & vs, bool init );
	void setupSlicers(bool lv, bool sil, bool sp, int no, double min, double max, bool li, int ss, bool lm, bool init);
	void check2DMode();
	iALogger * getLogger();
	iARenderSettings const & GetRenderSettings() const;
	iAVolumeSettings const & GetVolumeSettings() const;
	iARenderer* getRaycaster() { return Raycaster; }
	iAVolumeStack * getVolumeStack();
	void connectThreadSignalsToChildSlots(iAAlgorithms* thread, bool providesProgress = true, bool usesDoneSignal = false);
	bool isHessianComputed() { return hessianComputed; }
	void setHessianComputed( bool isComputed ) { hessianComputed = isComputed; }
	vtkPiecewiseFunction * getPiecewiseFunction() { return piecewiseFunction; }
	vtkColorTransferFunction * getColorTransferFunction() { return colorTransferFunction; }
	void setReInitializeRenderWindows( bool reInit ) { reInitializeRenderWindows = reInit; }

	bool LoadCsvFile(FilterID fid, const QString &fileName);
	vtkTable * getMdCsvTable() { return mdCsvTable.GetPointer(); }

	//! deprecated; use getImagePointer instead!
	vtkImageData* getImageData() { return imageData; }
	//! function to retrieve main image data object of this MDI child
	vtkSmartPointer<vtkImageData> getImagePointer() { return imageData; }
	void setImageData(vtkImageData * iData) { imageData = iData; }
	void setImageData(QString const & filename, vtkSmartPointer<vtkImageData> imgData);
	vtkPolyData* getPolyData() { return polyData; };
	iARenderer* getRenderer() { return Raycaster; };
	iASlicerData* getSlicerDataXZ();
	iASlicerData* getSlicerDataXY();
	iASlicerData* getSlicerDataYZ();
	iASlicer* getSlicerXZ();
	iASlicer* getSlicerXY();
	iASlicer* getSlicerYZ();
	dlg_sliceXY * getSlicerDlgXY();
	dlg_sliceXZ	* getSlicerDlgXZ();
	dlg_sliceYZ	* getSlicerDlgYZ();
	dlg_imageproperty * getImagePropertyDlg();
	dlg_histogram * getHistogramDlg();
	vtkTransform* getSlicerTransform();
	bool getResultInNewWindow() const { return resultInNewWindow; }
	bool getCompression() const { return compression; }
	bool getShowPosition() const { return showPosition; }
	bool getMedianFilterHistogram() const { return filterHistogram; }
	int getNumberOfHistogramBins() const { return histogramBins; }
	int getStatExtent() const { return statExt; }

	bool getLinkedViews() const { return linkviews; }
	bool getLinkedMDIs() const { return linkmdis; }
	bool getShowIsolines() const { return showIsolines; }
	int getNumberOfIsolines() const { return numberOfIsolines; }
	double getMinIsovalue() const { return minIsovalue; }
	double getMaxIsovalue() const { return maxIsovalue; }
	bool getImageActorUseInterpolation() const { return imageActorUseInterpolation; }
	int getSnakeSlices() const { return snakeSlices; }
	std::vector<dlg_function*> &getFunctions();
	void redrawHistogram();
	dlg_profile *getProfile() { return imgProfile; }
	iAHistogramWidget * getHistogram();

	int getSelectedFuncPoint();
	int isFuncEndPoint(int index);
	bool isUpdateAutomatically();
	void setHistogramFocus();
	bool isMaximized();
	void setROI(int indexX, int indexY, int indexZ, int sizeX, int sizeY, int sizeZ);
	void hideROI();
	void showROI();
	void setCurrentFile(const QString &f);

	QString userFriendlyCurrentFile();
	QString currentFile() const { return curFile; }
	QFileInfo getFileInfo() const { return fileInfo; }

	void activate(ConnectionState state) { connectionState = state; }
	void deactivate() { connectionState = cs_NONE; }

	int getSliceXY();
	int getSliceYZ();
	int getSliceXZ();
	QSpinBox * getSpinBoxXY();
	QSpinBox * getSpinBoxYZ();
	QSpinBox * getSpinBoxXZ();

	vtkImageAccumulate * getImageAccumulate() { return imageAccumulate; }

	void SetChannelRenderingEnabled(iAChannelID, bool enabled);

	void InsertChannelData(iAChannelID id, iAChannelVisualizationData * channelData);

	iAChannelVisualizationData * GetChannelData(iAChannelID id);
	iAChannelVisualizationData const * GetChannelData(iAChannelID id) const;
	void UpdateChannelSlicerOpacity(iAChannelID id, double opacity);
	void InitChannelRenderer(iAChannelID id, bool use3D, bool enableChannel = true);

	void SetSlicerPieGlyphsEnabled(bool isOn);
	void SetPieGlyphParameters(double opacity, double spacing, double magFactor);

	void updateChannelMappers();
	QString getFilePath() const;

	// Magic Lens
	void toggleMagicLens(bool isEnabled);
	bool isMagicLensToggled(void) const;
	void SetMagicLensInput(iAChannelID id, bool initReslicer);
	void SetMagicLensEnabled(bool isOn);
	void SetMagicLensCaption(std::string caption);
	void reInitMagicLens(iAChannelID id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf, std::string const & caption = "");
	int  GetMagicLensSize() const { return magicLensSize; }
	int  GetMagicLensFrameWidth() const { return magicLensFrameWidth; }

	void reInitChannel(iAChannelID id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);

	int GetRenderMode();

	int getXCoord() const { return xCoord; }
	int getYCoord() const { return yCoord; }
	int getZCoord() const { return zCoord; }

	MainWindow* getM_mainWnd();

Q_SIGNALS:
	void rendererDeactivated(int c);
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void active();
	void autoUpdateChanged(bool toogled);
	void currentChanged(int index);
	void magicLensToggled( bool isToggled );
	void closed();
	void updatedViews();
	void renderSettingsChanged();
	void preferencesChanged();
	void viewInitialized();
	void ioSuccesful();

private slots:
	void maximizeRC();
	void maximizeXY();
	void maximizeXZ();
	void maximizeYZ();
	void saveRC();
	void saveXY();
	void saveXZ();
	void saveYZ();
	void saveStackXY();
	void saveStackXZ();
	void saveStackYZ();
	void saveMovXY();
	void saveMovXZ();
	void saveMovYZ();
	void saveMovRC();
	void triggerInteractionXY();
	void triggerInteractionXZ();
	void triggerInteractionYZ();
	void triggerInteractionRaycaster();
	void link( bool l );
	void linkM( bool lm );
	void setSliceXY(int s);
	void setSliceYZ(int s);
	void setSliceXZ(int s);
	void setSliceXYScrollBar(int s);
	void setSliceYZScrollBar(int s);
	void setSliceXZScrollBar(int s);
	void setSliceXYSpinBox(int s);
	void setSliceYZSpinBox(int s);
	void setSliceXZSpinBox(int s);
	void setChannel(int ch);
	void setRotationXY(double a);
	void setRotationYZ(double a);
	void setRotationXZ(double a);
	void updateRenderWindows(int channels);
	void updateRenderers(int x, int y, int z, int mode);
	void paintEvent(QPaintEvent * );
	void updated(QString text);
	void updated(int state);
	void updated(int state, QString text);
	void toggleArbitraryProfile(bool isChecked);
	void ioFinished();

public slots:
	void updateProgressBar(int i);
	void hideProgressBar();
	void initProgressBar();
	void disableRenderWindows(int ch);
	void enableRenderWindows();
	void updateSlicer(int index);
	void updateSlicers();
	void updateViews();
	void addMsg(QString txt);
	void addStatusMsg(QString txt);
	bool setupView(bool active = false);
	bool setupStackView(bool active = false);
	bool updateVolumePlayerView(int updateIndex, bool isApplyForAll);
	void removeFinishedAlgorithms();
	void camPX();
	void camPY();
	void camPZ();
	void camMX();
	void camMY();
	void camMZ();
	void camIso();

	/**
	* Calls the getCamPosition function of iARenderer (described there in more detail).
	* \param	camOptions	All informations of the camera stored in a double array
	*/
	void getCamPosition(double * camOptions);
	/**
	* Calls the setCamPosition function of iARenderer (described there in more detail).
	*
	* \param	camOptions	All informations of the camera stored in a double array
	* \param	rsParallelProjection	boolean variable to determine if parallel projection option on.
	*/
	void setCamPosition(double * camOptions, bool rsParallelProjection);
	void UpdateProbe(int ptIndex, double * newPos);
	void resetLayout();

protected:
	void closeEvent(QCloseEvent *event);

	bool calculateHistogram( );
	bool medianFilterHistogram( vtkImageAccumulate* imgA );
	bool addImageProperty( );
	bool addVolumePlayer(iAVolumeStack *volumeStack);
	bool addHistogram( );
	bool addProfile( );
	void showProfile( bool isVisible );
	int profileWidgetIndex;

	bool LoadCsvFile(vtkTable *table, FilterID fid);
	bool LoadCsvFile(vtkTable *table, FilterID fid, const QString &fileName);


	bool initView( );
	bool initTransferfunctions( );
	int EvaluatePosition(int pos, int i, bool invert = false);
	//void printFileInfos();
	//void printSTLFileInfos();

	/**
	* Changes the display of views from full to multi screen or multi screen to fullscreen.
	*
	* \param	mode	how the views should be arranged \return.
	*/
	void changeVisibility(unsigned char mode);
	int getVisibility() const;
	void widgetsVisible(bool b);
	void visibilityBlock(QList<QSpacerItem*> spacerItems, QList<QWidget*> widgets, bool show);
	void cleanWorkingAlgorithms();
	virtual void resizeEvent ( QResizeEvent * event );

private:
	iAHistogramWidget * getHistogram(dlg_histogram * h);
	void connectSignalsToSlots();
	void SetRenderWindows();
	void getSnakeNormal(int index, double point[3], double normal[3]);
	void updateReslicer(double point[3], double normal[3], int mode);
	void updateSliceIndicators();
	QString strippedName(const QString &f);
	
	/**
	* sets up the IO thread for saving the correct file type for the given filename.
	* @return	true if it succeeds, false if it fails.
	*/
	bool setupSaveIO(QString const & f);

	/**
	* sets up the IO thread for loading the correct file type according to the given filename.
	* @return	true if it succeeds, false if it fails.
	*/
	bool setupLoadIO(QString const & f, bool isStack);

	QFileInfo fileInfo;

	vtkPoints *worldProfilePoints;
	vtkPoints *worldSnakePoints;
	iAParametricSpline *parametricSpline;
	MainWindow * m_mainWnd;

	static const unsigned char RC = 0x01;
	static const unsigned char XY = 0x02;
	static const unsigned char YZ = 0x04;
	static const unsigned char XZ = 0x08;
	static const unsigned char TAB = 0x10;
	static const unsigned char MULTI = 0x1F;

	QString curFile, path;
	QPoint lastPoint;
	bool isUntitled, tabsVisible;
	int xCoord, yCoord, zCoord;
	int histogramBins, statExt, magicLensSize, magicLensFrameWidth;
	bool compression, showPosition, resultInNewWindow, filterHistogram, linkSlicers, logarithmicHistogram;

	iARenderSettings renderSettings;
	iAVolumeSettings volumeSettings;

	double minIsovalue, maxIsovalue;
	int numberOfIsolines, snakeSlices;
	bool linkviews, showIsolines, imageActorUseInterpolation, interactorsEnabled, linkmdis;
	unsigned char visibility;

	ConnectionState connectionState;
	int roi[6];

	bool snakeSlicer;
	bool isSliceProfileEnabled;	//!< slice profile, shown in slices
	bool isArbProfileEnabled;	//!< arbitrary profile, shown in profile widget
	bool isMagicLensEnabled;	//!< magic lens exploration
	
	void updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s);
	void setupViewInternal(bool active);
	bool IsOnlyPolyDataLoaded();

	vtkSmartPointer<vtkImageData> imageData;
	vtkPolyData* polyData;
	vtkImageAccumulate* imageAccumulate;
	vtkPiecewiseFunction* piecewiseFunction;
	vtkColorTransferFunction* colorTransferFunction;
	vtkTransform* axesTransform;
	vtkTransform* slicerTransform;
	vtkAbstractTransform *SlicerYZ_Transform, *SlicerXY_Transform, *SlicerXZ_Transform;
	iARenderer* Raycaster;
	iASlicer * slicerYZ, * slicerXY, * slicerXZ;
	QSharedPointer<iAProfileProbe> profileProbe;
	QScopedPointer<iAVolumeStack> volumeStack;
	iAIO* ioThread;

	dlg_histogram* imgHistogram;
	dlg_imageproperty* imgProperty;
	dlg_volumePlayer* volumePlayer;
	dlg_profile* imgProfile;

	dlg_charts* charts;
	dlg_charts* charts2;
	dlg_charts* charts3;


	// csv file to table
	vtkSmartPointer<vtkTable> mdCsvTable;


	bool saveNative;
	std::vector<iAAlgorithms*> workingAlgorithms;

	QMap<iAChannelID, QSharedPointer<iAChannelVisualizationData> > m_channels;

	bool hessianComputed;
	bool updateSliceIndicator;
	int numberOfVolumes;
	int previousIndexOfVolume;

	QList <int> CheckedList;

	bool reInitializeRenderWindows;
	bool raycasterInitialized;
	iALogger* m_logger;
	QByteArray m_initialLayoutState;

	// previously "Modality Explorer":
	dlg_modalities * m_dlgModalities;
	int m_currentModality;
private slots:
	void ChangeModality(int chg);
	void ChangeMagicLensOpacity(int chg);
private:
	int GetCurrentModality() const;
	void SetCurrentModality(int modality);
	void ChangeImage(vtkSmartPointer<vtkImageData> img);
	void ChangeImage(vtkSmartPointer<vtkImageData> img, std::string const & caption);
public:
	void SetModalities(QSharedPointer<iAModalityList> modList);
	dlg_modalities* GetModalitiesDlg();
	void LoadModalities();
	void StoreModalities();
};

#endif
