/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
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

#include "defines.h"
#include "iAChannelID.h"
#include "iAQTtoUIConnector.h"
#include "iAPreferences.h"
#include "iARenderSettings.h"
#include "iASlicerSettings.h"
#include "iAVolumeSettings.h"
#include "open_iA_Core_export.h"
#include "ui_logs.h"
#include "ui_Mdichild.h"
#include "ui_renderer.h"
#include "ui_sliceXY.h"
#include "ui_sliceXZ.h"
#include "ui_sliceYZ.h"

#include <vtkSmartPointer.h>

#include <QFileInfo>
#include <QMainWindow>
#include <QMap>
#include <QString>
#include <QSharedPointer>

#include <vector>

class QProgressBar;

class vtkAbstractTransform;
class vtkActor;
class vtkColorTransferFunction;
class vtkCornerAnnotation;
class vtkImageCast;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkPoints;
class vtkPolyData;
class vtkScalarsToColors;
class vtkTransform;

class dlg_function;
class dlg_imageproperty;
class dlg_modalities;
class dlg_periodicTable;
class dlg_profile;
class dlg_volumePlayer;
class iAAlgorithm;
class iAChannelVisualizationData;
class iADiagramFctWidget;
class iADockWidgetWrapper;
class iAIO;
class iALogger;
class iAModality;
class iAModalityList;
class iAParametricSpline;
class iAPlot;
struct iAProfileProbe;
class iARenderer;
class iASlicer;
class iASlicerData;
class iAVolumeStack;
class MainWindow;

typedef iAQTtoUIConnector<QDockWidget, Ui_sliceXY>   dlg_sliceXY;
typedef iAQTtoUIConnector<QDockWidget, Ui_sliceXZ>   dlg_sliceXZ;
typedef iAQTtoUIConnector<QDockWidget, Ui_sliceYZ>   dlg_sliceYZ;
typedef iAQTtoUIConnector<QDockWidget, Ui_renderer>  dlg_renderer;
typedef iAQTtoUIConnector<QDockWidget, Ui_logs>   dlg_logs;

class open_iA_Core_API MdiChild : public QMainWindow, public Ui_Mdichild
{
	Q_OBJECT
public:
	dlg_renderer * renderer;
	dlg_sliceXY * sXY;
	dlg_sliceXZ * sXZ;
	dlg_sliceYZ * sYZ;
	dlg_logs * logs;
	QProgressBar * pbar;

	/** waits for the IO thread to finish in case any I/O operation is running; otherwise it will immediately exit */
	void waitForPreviousIO();

	MdiChild(MainWindow * mainWnd, iAPreferences const & preferences, bool unsavedChanges);
	~MdiChild();

	void showPoly();
	bool loadFile(const QString &f, bool isStack);
	bool loadRaw(const QString &f);
	bool displayResult(QString const & title, vtkImageData* image = nullptr, vtkPolyData* poly = nullptr);
	void PrepareForResult();
	bool save();
	bool saveAs();
	bool saveFile(const QString &f, int modalityNr, int componentNr);
	void updateLayout();

	bool multiview() { changeVisibility(MULTI); return true; };
	bool xyview() { maximizeXY(); return true; };
	bool xzview() { maximizeXZ(); return true; };
	bool yzview() { maximizeYZ(); return true; };
	bool rcview() { maximizeRC(); return true; };
	bool linkViews( bool l ) { link(l); return true; }
	bool linkMDIs( bool l ) { linkM(l); return true; }
	bool editPrefs(iAPreferences const & p);
	void ApplyViewerPreferences();
	bool editRendererSettings(iARenderSettings const & rs, iAVolumeSettings const & vs);
	bool editSlicerSettings(iASlicerSettings const & slicerSettings);
	bool loadTransferFunction();
	bool saveTransferFunction();

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
	void resetView();
	void resetTrf();
	void toggleSnakeSlicer(bool isEnabled);
	bool isSnakeSlicerToggled() const;
	void toggleSliceProfile(bool isEnabled);
	bool isSliceProfileToggled(void) const;
	void enableInteraction(bool b);
	void setRenderSettings(iARenderSettings const & rs, iAVolumeSettings const & vs);
	void setupSlicers(iASlicerSettings const & ss, bool init);
	void check2DMode();
	iALogger * getLogger();
	iARenderSettings const & GetRenderSettings() const;
	iAVolumeSettings const & GetVolumeSettings() const;
	iASlicerSettings const & GetSlicerSettings() const;
	iAPreferences    const & GetPreferences()    const;
	iAVolumeStack * getVolumeStack();
	void connectThreadSignalsToChildSlots(iAAlgorithm* thread);
	void connectIOThreadSignals(iAIO* thread);
	void connectAlgorithmSignalsToChildSlots(iAAlgorithm* thread);
	vtkPiecewiseFunction * getPiecewiseFunction();
	vtkColorTransferFunction * getColorTransferFunction();
	void setReInitializeRenderWindows( bool reInit ) { reInitializeRenderWindows = reInit; }

	//! deprecated; use getImagePointer instead!
	vtkImageData* getImageData();
	//! function to retrieve main image data object of this MDI child
	vtkSmartPointer<vtkImageData> getImagePointer() { return imageData; }
	//! deprecated; use the version with smart pointer instead!
	void setImageData(vtkImageData * iData);
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
	dlg_renderer * getRendererDlg();
	dlg_imageproperty * getImagePropertyDlg();
	vtkTransform* getSlicerTransform();
	bool getResultInNewWindow() const { return preferences.ResultInNewWindow; }
	bool getLinkedMDIs() const { return slicerSettings.LinkMDIs; }
	bool getLinkedViews() const { return slicerSettings.LinkViews; }
	std::vector<dlg_function*> &getFunctions();
	void redrawHistogram();
	dlg_profile *getProfile() { return imgProfile; }
	iADiagramFctWidget* getHistogram();
	iADockWidgetWrapper* getHistogramDockWidget();

	int getSelectedFuncPoint();
	int isFuncEndPoint(int index);
	void setHistogramFocus();
	bool isMaximized();

	void UpdateROI(int const roi[6]);
	void SetROIVisible(bool visible);

	void setCurrentFile(const QString &f);
	QString userFriendlyCurrentFile();
	QString currentFile() const { return curFile; }
	QFileInfo getFileInfo() const { return fileInfo; }

	int getSliceXY();
	int getSliceYZ();
	int getSliceXZ();
	QSpinBox * getSpinBoxXY();
	QSpinBox * getSpinBoxYZ();
	QSpinBox * getSpinBoxXZ();

	//! @{ Multi-Channel rendering
	void SetChannelRenderingEnabled(iAChannelID, bool enabled);
	void InsertChannelData(iAChannelID id, iAChannelVisualizationData * channelData);
	iAChannelVisualizationData * GetChannelData(iAChannelID id);
	iAChannelVisualizationData const * GetChannelData(iAChannelID id) const;
	void UpdateChannelSlicerOpacity(iAChannelID id, double opacity);
	void InitChannelRenderer(iAChannelID id, bool use3D, bool enableChannel = true);
	void reInitChannel(iAChannelID id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);
	void updateChannelMappers();
	//! @}

	//! @{ slicer pie glyphs - move to XRF module!
	void SetSlicerPieGlyphsEnabled(bool isOn);
	void SetPieGlyphParameters(double opacity, double spacing, double magFactor);
	//! @}

	QString getFilePath() const;

	//! @{ Magic Lens
	void toggleMagicLens(bool isEnabled);
	bool isMagicLensToggled(void) const;
	void SetMagicLensInput(iAChannelID id, bool initReslicer);
	void SetMagicLensEnabled(bool isOn);
	void reInitMagicLens(iAChannelID id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf);
	int  GetMagicLensSize() const { return preferences.MagicLensSize; }
	int  GetMagicLensFrameWidth() const { return preferences.MagicLensFrameWidth; }
	//! @}

	int GetRenderMode();

	int getXCoord() const { return xCoord; }
	int getYCoord() const { return yCoord; }
	int getZCoord() const { return zCoord; }

	MainWindow* getMainWnd();
	void HideHistogram();
	//! apply current rendering settings of this mdi child to the given iARenderer
	void ApplyRenderSettings(iARenderer* raycaster);
	//! apply current volume settings of this mdi child to all modalities in the current list in dlg_modalities
	void ApplyVolumeSettings(const bool loadSavedVolumeSettings);
	QString GetLayoutName() const;
	void LoadLayout(QString const & layout);

	//! if more than one modality loaded, ask user to choose one of them
	//! (currently used for determining which modality to save)
	int chooseModalityNr(QString const & caption = "Choose Channel");
	//! if given modality has more than one component, ask user to choose one of them
	//! (currently used for determining which modality to save)
	int chooseComponentNr(int modalityNr);

	//! workaround for bug in splitDockWidget (see https://bugreports.qt.io/browse/QTBUG-60093)
	//! splitDockWidget would makes ref and newWidget disappear if ref is tabbed at the moment
	void SplitDockWidget(QDockWidget* ref, QDockWidget* newWidget, Qt::Orientation orientation);

	//! checks whether the main image data in this child is fully loaded
	bool IsFullyLoaded() const;

	//! save all currently loaded files into a project with the given file name
	void saveProject(QString const & fileName);

	//! whether volume data is loaded (only checks filename and volume dimensions)
	bool IsVolumeDataLoaded() const;

Q_SIGNALS:
	void rendererDeactivated(int c);
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	void active();
	void magicLensToggled( bool isToggled );
	void closed();
	void updatedViews();
	void renderSettingsChanged();
	void preferencesChanged();
	void viewInitialized();
	void TransferFunctionChanged();
	void fileLoaded();

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
	void setSlabModeXY(bool slabMode);
	void setSlabModeYZ(bool slabMode);
	void setSlabModeXZ(bool slabMode);
	void updateSlabThicknessXY(int thickness);
	void updateSlabThicknessYZ(int thickness);
	void updateSlabThicknessXZ(int thickness);
	void updateSlabCompositeModeXY(int compositeMode);
	void updateSlabCompositeModeXZ(int compositeMode);
	void updateSlabCompositeModeYZ(int compositeMode);
	void updateRenderWindows(int channels);
	void updateRenderers(int x, int y, int z, int mode);
	void toggleArbitraryProfile(bool isChecked);
	void ioFinished();
	void updateImageProperties();
	void clearLogs();
	void ModalityTFChanged();
	void HistogramDataAvailable(int modalityIdx);
	void StatisticsAvailable(int modalityIdx);

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
	void setupView(bool active = false);
	void setupStackView(bool active = false);
	void setupProject(bool active = false);
	bool updateVolumePlayerView(int updateIndex, bool isApplyForAll);
	void removeFinishedAlgorithms();
	void camPX();
	void camPY();
	void camPZ();
	void camMX();
	void camMY();
	void camMZ();
	void camIso();

	//! Calls the getCamPosition function of iARenderer (described there in more detail).
	//!
	//! \param camOptions	All informations of the camera stored in a double array
	void getCamPosition(double * camOptions);

	//! Calls the setCamPosition function of iARenderer (described there in more detail).
	//!
	//! \param camOptions	All informations of the camera stored in a double array
	//! \param rsParallelProjection	boolean variable to determine if parallel projection option on.
	void setCamPosition(double * camOptions, bool rsParallelProjection);
	void UpdateProbe(int ptIndex, double * newPos);
	void resetLayout();
private:
	void closeEvent(QCloseEvent *event);
	void addImageProperty( );
	bool addVolumePlayer(iAVolumeStack *volumeStack);
	void addProfile();
	void UpdateProfile();
	bool saveAs(int modalityNr);
	bool initView(QString const & title);
	int EvaluatePosition(int pos, int i, bool invert = false);

	//! Changes the display of views from full to multi screen or multi screen to fullscreen.
	//!
	//! \param mode	how the views should be arranged.
	void changeVisibility(unsigned char mode);
	int getVisibility() const;
	void hideVolumeWidgets();
	void setVisibility(QList<QWidget*> widgets, bool show);
	void cleanWorkingAlgorithms();
	int profileWidgetIndex;
	QByteArray m_beforeMaximizeState;
	bool m_isSmthMaximized;
	QDockWidget * m_whatMaximized;
	int m_pbarMaxVal;
	void maximizeDockWidget(QDockWidget * dw);
	void demaximizeDockWidget(QDockWidget * dw);
	void resizeDockWidget(QDockWidget * dw);

	void connectSignalsToSlots();
	void SetRenderWindows();
	void getSnakeNormal(int index, double point[3], double normal[3]);
	void updateReslicer(double point[3], double normal[3], int mode);
	void updateSliceIndicators();
	QString strippedName(const QString &f);

	//! sets up the IO thread for saving the correct file type for the given filename.
	//! \return	true if it succeeds, false if it fails.
	bool setupSaveIO(QString const & f);

	//! sets up the IO thread for loading the correct file type according to the given filename.
	//! \return	true if it succeeds, false if it fails.
	bool setupLoadIO(QString const & f, bool isStack);

	// adds an algorithm to the list of currently running jobs
	void addAlgorithm(iAAlgorithm* thread);

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
	bool isUntitled;
	int xCoord, yCoord, zCoord;

	iARenderSettings renderSettings;
	iAVolumeSettings volumeSettings;
	iASlicerSettings slicerSettings;
	iAPreferences preferences;

	unsigned char visibility;

	bool snakeSlicer;           //!< whether snake slicer is enabled
	bool isSliceProfileEnabled; //!< slice profile, shown in slices
	bool isArbProfileEnabled;   //!< arbitrary profile, shown in profile widget
	bool isMagicLensEnabled;    //!< magic lens exploration

	void updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s);
	void setupViewInternal(bool active);

	vtkSmartPointer<vtkImageData> imageData;		// TODO: remove - use modality data instead!
	vtkPolyData* polyData;
	vtkTransform* axesTransform;
	vtkTransform* slicerTransform;
	vtkAbstractTransform *SlicerYZ_Transform, *SlicerXY_Transform, *SlicerXZ_Transform;
	iARenderer* Raycaster;
	iASlicer * slicer[3];
	QSharedPointer<iAProfileProbe> profileProbe;
	QScopedPointer<iAVolumeStack> volumeStack;
	iAIO* ioThread;

	iADiagramFctWidget* m_histogram;
	iADockWidgetWrapper* m_histogramContainer;
	QSharedPointer<iAPlot> m_histogramPlot;

	dlg_imageproperty* imgProperty;
	dlg_volumePlayer* volumePlayer;
	dlg_profile* imgProfile;

	std::vector<iAAlgorithm*> workingAlgorithms;

	QMap<iAChannelID, QSharedPointer<iAChannelVisualizationData> > m_channels;

	bool updateSliceIndicator;
	int numberOfVolumes;
	int previousIndexOfVolume;

	QList <int> CheckedList;

	bool reInitializeRenderWindows;
	bool raycasterInitialized;
	iALogger* m_logger;
	QByteArray m_initialLayoutState;
	QString m_layout;
	vtkSmartPointer<vtkImageData> tmpSaveImg;	//< TODO: get rid of this (by introducing smart pointer in iAIO/ iAlgorithm?

	//! @{ previously "Modality Explorer":
	dlg_modalities * m_dlgModalities;
	int m_currentModality;
	int m_currentComponent;
	int m_currentHistogramModality;
	bool m_initVolumeRenderers;
	int m_storedModalityNr;		// modality nr being stored
private slots:
	void ChangeMagicLensModality(int chg);
	void ChangeMagicLensOpacity(int chg);
	void ChangeMagicLensSize(int chg);
	void ShowModality(int modIdx);
	void SaveFinished();
	void ModalityAdded(int modalityIdx);
private:
	void SetHistogramModality(int modalityIdx);
	int GetCurrentModality() const;
	void InitModalities();
	void InitVolumeRenderers();
public:
	void SetModalities(QSharedPointer<iAModalityList> modList);
	QSharedPointer<iAModalityList> GetModalities();
	QSharedPointer<iAModality> GetModality(int idx);
	dlg_modalities* GetModalitiesDlg();
	void StoreProject();
	//! @}
};
