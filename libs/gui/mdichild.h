/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2021  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "ui_Mdichild.h"
#include "ui_renderer.h"
#include "iAgui_export.h"

// core
#include "qthelper/iAQTtoUIConnector.h"
#include "iAMdiChild.h"
#include "iAPreferences.h"
#include "iARenderSettings.h"
#include "iASavableProject.h"
#include "iASlicerSettings.h"
#include "iAVolumeSettings.h"

#include <vtkSmartPointer.h>

#include <QFileInfo>
#include <QMap>
#include <QString>
#include <QSharedPointer>

#include <vector>
#include <functional>

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

class dlg_imageproperty;
class dlg_slicer;
class dlg_volumePlayer;
class iAParametricSpline;
struct iAProfileProbe;
class iAProfileWidget;
class MainWindow;

// renderer
class iARendererImpl;

// slicer
class iASlicerImpl;

// charts
class iAChartWithFunctionsWidget;
class iAPlot;

// core
class iAAbortListener;
class dlg_modalities;
class iAAlgorithm;
class iAChannelData;
class iADockWidgetWrapper;
class iAIO;
class iAModality;
class iAStatisticsUpdater;
class iAModalityList;
class iAProjectBase;
class iAVolumeStack;

typedef iAQTtoUIConnector<QDockWidget, Ui_renderer>  dlg_renderer;

//! Child window of MainWindow's mdi area for showing a volume or mesh dataset.
//! Some tools in the modules attach to MdiChild's to enhance their functionality.
class iAgui_API MdiChild : public iAMdiChild, public Ui_Mdichild, public iASavableProject
{
	Q_OBJECT
public:
	MdiChild(MainWindow* mainWnd, iAPreferences const& preferences, bool unsavedChanges);
	~MdiChild();

	void showPoly();
	bool loadFile(const QString &f, bool isStack) override;
	bool loadRaw(const QString &f);
	bool displayResult(QString const & title, vtkImageData* image = nullptr, vtkPolyData* poly = nullptr) override;
	void prepareForResult();
	bool save();
	bool saveAs();
	bool saveFile(const QString &f, int modalityNr, int componentNr);
	void updateLayout() override;

	//! waits for the IO thread to finish in case any I/O operation is running; otherwise it will immediately exit
	void waitForPreviousIO();

	void multiview() override;
	bool editPrefs(iAPreferences const & p);
	void applyViewerPreferences();
	bool editRendererSettings(iARenderSettings const & rs, iAVolumeSettings const & vs) override;
	bool editSlicerSettings(iASlicerSettings const & slicerSettings);
	bool loadTransferFunction();
	bool saveTransferFunction();

	int  deletePoint();
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
	iARenderSettings const & renderSettings() const override;
	iAVolumeSettings const & volumeSettings() const override;
	iASlicerSettings const & slicerSettings() const override;
	iAPreferences    const & preferences()    const override;
	iAVolumeStack * volumeStack() override;
	void connectThreadSignalsToChildSlots(iAAlgorithm* thread) override;
	void connectIOThreadSignals(iAIO* thread) override;
	void connectAlgorithmSignalsToChildSlots(iAAlgorithm* thread);

	//! Access the opacity function of the "main image"
	//! @deprecated all access to images should proceed via modalities (modality(int) / setModalities /...)
	//vtkPiecewiseFunction * opacityTF();
	//! Access the color transfer function of the "main image"
	//! @deprecated all access to images should proceed via modalities (modality(int) / setModalities /...)
	//vtkColorTransferFunction * colorTF();
	//! Access to the "main image"
	//! @deprecated retrieve images via the modalities (modality(int) etc.) instead!
	vtkImageData* imageData() override;
	//! Access to the "main image"
	//! @deprecated retrieve images via the modalities (modality(int) etc.) instead!
	vtkSmartPointer<vtkImageData> imagePointer() override;
	//! Set "main image" - does not update views (see displayResult for a method that does)!
	//! @deprecated all access to images should proceed via modalities (modality(int) / setModalities /...) or channels (createChannel/updateChannel)
	void setImageData(vtkImageData* iData) override;
	//! @deprecated all access to images should proceed via modalities (modality(int) / setModalities /...) or channels (createChannel/updateChannel)
	void setImageData(QString const & filename, vtkSmartPointer<vtkImageData> imgData);
	//! Access to "main" polydata object (if any)
	//! @deprecated move out of mdi child, into something like an iAModality
	vtkPolyData* polyData() override;
	//! Access to the 3D renderer widget
	iARenderer* renderer() override;
	//! Access to the 3D renderer vtk widget
	iAVtkWidget* renderVtkWidget() override;
	//! Access slicer for given mode (use iASlicerMode enum for mode values)
	iASlicer* slicer(int mode) override;
	//! Access to the scroll bar next to a slicer
	QSlider* slicerScrollBar(int mode) override;
	//! Access to the layout in the slicer dockwidget containing the actual iASlicer
	QHBoxLayout* slicerContainerLayout(int mode) override;
	//! Get current slice number in the respective slicer
	int sliceNumber(int mode) const;
	//! Access to slicer dock widget for the given mode
	//! @param mode slicer to access - use constants from iASlicerMode enum
	QDockWidget* slicerDockWidget(int mode) override;
	//! Access to 3D renderer dock widget
	QDockWidget* renderDockWidget() override;
	//! Access to image property dock widget
	QDockWidget* imagePropertyDockWidget() override;
	//! Access to histogram dock widget
	QDockWidget* histogramDockWidget() override;
	//! Access to modalities dock widget
	dlg_modalities* dataDockWidget() override;

	void setReInitializeRenderWindows(bool reInit) override;
	vtkTransform* slicerTransform() override;
	bool resultInNewWindow() const;
	//! Whether this child has the linked MDIs feature enabled
	bool linkedMDIs() const;
	//! Whether this child has the linked views feature enabled
	bool linkedViews() const override;
	iAChartWithFunctionsWidget* histogram() override;

	int selectedFuncPoint();
	int isFuncEndPoint(int index);
	void setHistogramFocus();
	bool isMaximized();

	void updateROI(int const roi[6]) override;
	void setROIVisible(bool visible) override;

	void setCurrentFile(const QString &f);
	QString userFriendlyCurrentFile() const;
	QString currentFile() const override;
	QFileInfo const & fileInfo() const override;
	QString filePath() const override;

	//! @{ Multi-Channel rendering
	//! Create a new channel, return its ID.
	uint createChannel() override;
	//! Update the data of the given channel ID.
	void updateChannel(uint id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf, vtkPiecewiseFunction* otf, bool enable) override;
	//! Update opacity of the given channel ID.
	void updateChannelOpacity(uint id, double opacity) override;

	void setChannelRenderingEnabled(uint, bool enabled) override;

	//! Enable / disable a channel in all slicers.
	void setSlicerChannelEnabled(uint id, bool enabled);

	//! Remove channel in all slicers.
	void removeChannel(uint id) override;

	iAChannelData * channelData(uint id) override;
	iAChannelData const * channelData(uint id) const override;
	void initChannelRenderer(uint id, bool use3D, bool enableChannel = true) override;
	//! @}

	//! @{ Magic Lens
	void toggleMagicLens2D(bool isEnabled);
	void toggleMagicLens3D(bool isEnabled);
	bool isMagicLens2DEnabled() const override;
	bool isMagicLens3DEnabled() const;
	void setMagicLensInput(uint id) override;
	void setMagicLensEnabled(bool isOn) override;
	void reInitMagicLens(uint id, QString const & name, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf) override;
	int  magicLensSize() const;
	int  magicLensFrameWidth() const;
	//! @}

	//! Current coordinate position (defined by the respective slice number in the slice views).
	int const * position() const override;

	iAMainWindow* mainWnd() override;
	//! Apply current volume settings to all modalities in the current list in dlg_modalities.
	void applyVolumeSettings(const bool loadSavedVolumeSettings);
	//! Returns the name of the layout currently applied to this child window.
	QString layoutName() const override;
	//! Loads the layout with the given name from the settings store, and tries to restore the according dockwidgets configuration
	void loadLayout(QString const & layout) override;

	//! If more than one modality loaded, ask user to choose one of them.
	//! (currently used for determining which modality to save)
	int chooseModalityNr(QString const & caption = "Choose Channel") override;
	//! If given modality has more than one component, ask user to choose one of them.
	//! (currently used for determining which modality to save)
	int chooseComponentNr(int modalityNr);

	//! Checks whether the main image data is fully loaded.
	bool isFullyLoaded() const override;
	//! Store current situation in the given project file:
	//!    - loaded files and their transfer functions, when old project file (.mod) is chosen
	//!    - configuration of opened tools (which support it), when new project file (.iaproj) is chosen
	//!      (to be extended to modalities and TFs soon)
	bool doSaveProject(QString const& projectFileName) override;
	//! Save all currently loaded files into a project with the given file name.
	void saveProject(QString const & fileName) override;
	//! Whether volume data is loaded (only checks filename and volume dimensions).
	bool isVolumeDataLoaded() const override;
	//! Enable or disable linked slicers and 3D renderer.
	void linkViews(bool l);
	//! Enable or disable linked MDI windows for this MDI child.
	void linkMDIs(bool lm);
	//! Clear current histogram (i.e. don't show it anymore)
	void clearHistogram() override;
	//! Set the list of modalities for this window.
	void setModalities(QSharedPointer<iAModalityList> modList);
	//! Retrieve the list of all currently loaded modalities.
	QSharedPointer<iAModalityList> modalities() override;
	//! Retrieve data for modality with given index.
	QSharedPointer<iAModality> modality(int idx) override;
	//! add project
	void addProject(QString const & key, QSharedPointer<iAProjectBase> project) override;
	QMap<QString, QSharedPointer<iAProjectBase> > const & projects();

	iAInteractionMode interactionMode() const override;
	void setInteractionMode(iAInteractionMode mode);

	bool meshDataMovable();
	void setMeshDataMovable(bool movable);
	//! maximize slicer dockwidget with the given mode
	void maximizeSlicer(int mode);

	//! whether profile handles are currently shown (i.e. "Edit profile points" mode is enabled)
	bool profileHandlesEnabled() const;
	//! whether this child has a profile plot (only has one if "normal" volume data loaded)
	bool hasProfilePlot() const;
	
	//! @{ 
	bool statisticsComputed(QSharedPointer<iAModality>);
	bool statisticsComputable(QSharedPointer<iAModality>, int modalityIdx = -1);
	void computeStatisticsAsync(std::function<void()> callbackSlot, QSharedPointer<iAModality> modality);
	//! @}

	//! @{
	size_t histogramNewBinCount(QSharedPointer<iAModality>) override;
	bool histogramComputed(size_t newBinCount, QSharedPointer<iAModality>) override;
	void computeHistogramAsync(std::function<void()> callbackSlot, size_t newBinCount, QSharedPointer<iAModality>) override;
	//! @}
//signals:
//	void preferencesChanged();


public slots:
	void maximizeRC();
	void disableRenderWindows(int ch) override;
	void enableRenderWindows() override;
	void updateSlicer(int index);
	void updateSlicers() override;
	void updateViews() override;
	void addStatusMsg(QString const& txt) override;
	void setupView(bool active = false);
	void setupStackView(bool active = false);
	void setupProject(bool active = false);
	bool updateVolumePlayerView(int updateIndex, bool isApplyForAll);
	void removeFinishedAlgorithms();

	//! Calls the camPosition function of iARenderer (described there in more detail).
	//! @param camOptions All informations of the camera stored in a double array
	void camPosition(double * camOptions);
	//! Calls the setCamPosition function of iARenderer (described there in more detail).
	//! @param pos set one of the predefined camera positions
	void setCamPosition(int pos);
	//! Calls the setCamPosition function of iARenderer (described there in more detail).
	//! @param camOptions All informations of the camera stored in a double array
	//! @param rsParallelProjection boolean variable to determine if parallel projection option on.
	void setCamPosition(double * camOptions, bool rsParallelProjection);
	void updateProbe(int ptIndex, double * newPos);
	void resetLayout();
	void toggleProfileHandles(bool isChecked);

private slots:
	void saveRC();
	void saveMovRC();
	void triggerInteractionRaycaster();
	void setSlice(int mode, int s);
	void slicerRotationChanged();
	void setChannel(int ch);
	void updateRenderWindows(int channels);
	void updatePositionMarker(int x, int y, int z, int mode);
	void ioFinished();
	void updateImageProperties();
	void modalityTFChanged();
	void histogramDataAvailable(int modalityIdx);
	void statisticsAvailable(int modalityIdx);
	void changeMagicLensModality(int chg);
	void changeMagicLensOpacity(int chg);
	void changeMagicLensSize(int chg);
	void showModality(int modIdx);
	void saveFinished();
	void modalityAdded(int modalityIdx);
	void resetCamera(bool spacingChanged, double const * newSpacing);
	void toggleFullScreen();
	void rendererKeyPressed(int keyCode);
	void styleChanged();

private:
	void closeEvent(QCloseEvent *event) override;
	void addImageProperty( );
	bool addVolumePlayer();
	void addProfile();
	void updateProfile();
	bool saveAs(int modalityNr);
	bool initView(QString const & title);
	int  evaluatePosition(int pos, int i, bool invert = false);
	void set3DSlicePlanePos(int mode, int slice);

	//! Changes the display of views from full to multi screen or multi screen to fullscreen.
	//! @param mode how the views should be arranged.
	void changeVisibility(unsigned char mode);
	int  visibility() const;
	void hideVolumeWidgets();
	void setVisibility(QList<QWidget*> widgets, bool show);
	void cleanWorkingAlgorithms();
	void maximizeDockWidget(QDockWidget * dw);
	void demaximizeDockWidget(QDockWidget * dw);
	void resizeDockWidget(QDockWidget * dw);

	void connectSignalsToSlots();
	void updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s);
	void snakeNormal(int index, double point[3], double normal[3]);

	//! sets up the IO thread for saving the correct file type for the given filename.
	//! @return	true if it succeeds, false if it fails.
	bool setupSaveIO(QString const & f);

	//! sets up the IO thread for loading the correct file type according to the given filename.
	//! @return	true if it succeeds, false if it fails.
	bool setupLoadIO(QString const & f, bool isStack);

	//! adds an algorithm to the list of currently running jobs
	void addAlgorithm(iAAlgorithm* thread);

	void setupViewInternal(bool active);

	void setHistogramModality(int modalityIdx);
	void displayHistogram(int modalityIdx);
	int  currentModality() const;
	void initModalities();
	void initVolumeRenderers();
	void slicerVisibilityChanged(int mode);

	static const unsigned char RC = 0x01;
	static const unsigned char XY = 0x02;
	static const unsigned char YZ = 0x04;
	static const unsigned char XZ = 0x08;
	static const unsigned char TAB = 0x10;
	static const unsigned char MULTI = 0x1F;

	MainWindow * m_mainWnd;
	QFileInfo m_fileInfo;
	QString m_curFile, m_path;
	//! current "position" in image (in voxel indices).
	//! @deprecated use global coordinates instead of voxel indices
	int m_position[3];

	QByteArray m_beforeMaximizeState;
	QDockWidget* m_whatMaximized;

	iARenderSettings m_renderSettings;
	iAVolumeSettings m_volumeSettings;
	iASlicerSettings m_slicerSettings;
	iAPreferences m_preferences;

	unsigned char m_visibility;

	bool m_isSmthMaximized;       //!< whether a single dock widget is currently maximized
	bool m_isUntitled;            //!< whether current content is saved as a file already
	bool m_isSliceProfileEnabled; //!< whether slice profile, shown in slices, is enabled
	bool m_profileHandlesEnabled; //!< whether profile handles (profile points) in renderer/slicer are enabled
	bool m_isMagicLensEnabled;    //!< whether magic lens in slicers is enabled
	bool m_reInitializeRenderWindows; //! whether render windows need to be reinitialized
	bool m_raycasterInitialized;  //!< whether renderer is already initialized

	//! @{ snake slicer related:
	bool m_snakeSlicer;           //!< whether snake slicer is enabled
	vtkAbstractTransform *m_savedSlicerTransform[3];
	vtkPoints *m_worldProfilePoints;
	vtkPoints *m_worldSnakePoints;
	iAParametricSpline *m_parametricSpline;
	//! @}

	//! smart pointer to first image data shown in mdiChild.
	//! @deprecated use modality data instead, will be removed
	vtkSmartPointer<vtkImageData> m_imageData;
	vtkPolyData * m_polyData;
	vtkTransform * m_axesTransform;
	vtkTransform * m_slicerTransform;
	iARendererImpl * m_renderer;
	iASlicerImpl * m_slicer[3];
	QSharedPointer<iAProfileProbe> m_profileProbe;
	QScopedPointer<iAVolumeStack> m_volumeStack;
	QList<int> m_checkedList;
	iAIO* m_ioThread;

	iAChartWithFunctionsWidget * m_histogram;
	iAProfileWidget* m_profile;
	QSharedPointer<iAPlot> m_histogramPlot;

	//! @{ dock widgets
	iADockWidgetWrapper* m_dwHistogram;
	iADockWidgetWrapper* m_dwProfile;
	dlg_imageproperty * m_dwImgProperty;
	dlg_volumePlayer * m_dwVolumePlayer;
	dlg_slicer * m_dwSlicer[3];
	dlg_modalities * m_dwModalities;
	dlg_renderer * m_dwRenderer;
	//! @}

	std::vector<iAAlgorithm*> m_workingAlgorithms;

	QMap<uint, QSharedPointer<iAChannelData> > m_channels;
	uint m_nextChannelID;
	uint m_magicLensChannel;

	int m_previousIndexOfVolume;
	
	QByteArray m_initialLayoutState;
	QString m_layout;
	//! temporary smart pointer to image currently being saved
	//! @deprecated should be referenced in wherever image is stored, e.g. in iAIO)
	vtkSmartPointer<vtkImageData> m_tmpSaveImg;

	int m_currentModality;
	int m_currentComponent;
	int m_currentHistogramModality;
	bool m_initVolumeRenderers;
	int m_storedModalityNr;		                              //!< modality nr being stored
	QMap<QString, QSharedPointer<iAProjectBase>> m_projects;  //!< list of currently active "projects" (i.e. Tools)
	iAInteractionMode m_interactionMode;                      //!< current interaction mode in slicers/renderer (see iAInteractionMode)
	bool m_slicerVisibility[3];
};
