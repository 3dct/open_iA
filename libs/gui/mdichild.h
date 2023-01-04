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

// get rid of ui_ includes (use ui internally only, see )
#include "ui_Mdichild.h"
#include "ui_renderer.h"

// guibase
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
#include <QMutex>
#include <QString>
#include <QSharedPointer>

#include <array>
#include <functional>
#include <vector>

class QListWidget;

class vtkAbstractTransform;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkPoints;
class vtkPolyData;
class vtkScalarsToColors;
class vtkTransform;

// gui
class dlg_volumePlayer;
class iADataForDisplay;
class iADataSetListWidget;
class iADataSetViewer;
class iAParametricSpline;
struct iAProfileProbe;
class iASliceRenderer;
class iAvtkInteractStyleActor;
class MainWindow;

// guibase
class dlg_modalities;
class iAAlgorithm;
class iAChannelData;
class iAIO;
class iAModality;
class iAModalityList;
class iATool;
class iAVolumeStack;

// slicer / renderer
class dlg_slicer;
class iARendererImpl;
class iASlicerImpl;

// charts
class iAChartWithFunctionsWidget;
class iAPlot;
class iAProfileWidget;

// base
class iADockWidgetWrapper;

typedef iAQTtoUIConnector<QDockWidget, Ui_renderer>  dlg_renderer;

//! Child window of MainWindow's mdi area for showing a volume or mesh dataset.
//! Some tools in the modules attach to MdiChild's to enhance their functionality.
class iAgui_API MdiChild : public iAMdiChild, public Ui_Mdichild, public iASavableProject
{
	Q_OBJECT
public:

	MdiChild(MainWindow* mainWnd, iAPreferences const& preferences, bool unsavedChanges);
	~MdiChild();

	//! performs initialization that needs to be done after the widget is being displayed
	void initializeViews();

	void showPoly();
	bool loadFile(const QString& f, bool isStack) override;
	void setSTLParameter();
	bool displayResult(QString const & title, vtkImageData* image = nullptr, vtkPolyData* poly = nullptr) override;
	void prepareForResult();
	bool save();
	void saveNew();
	bool saveAs();
	bool saveFile(const QString &f, int modalityNr, int componentNr);
	void saveVolumeStack();
	void updateLayout() override;

	//! waits for the IO thread to finish in case any I/O operation is running; otherwise it will immediately exit
	void waitForPreviousIO();

	void multiview() override;
	bool applyPreferences(iAPreferences const & p);
	void applyViewerPreferences();
	bool applyRendererSettings(iARenderSettings const & rs, iAVolumeSettings const & vs) override;
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
	//! @{
	//! @deprecated iAAlgorithm/iAIO will be removed soon. use iAFilter / iAFileTypeRegistry instead
	void connectThreadSignalsToChildSlots(iAAlgorithm* thread) override;
	void connectIOThreadSignals(iAIO* thread) override;
	void connectAlgorithmSignalsToChildSlots(iAAlgorithm* thread);
	//! @}

	//! Set "main image" - does not update views
	//! @deprecated use addDataSet instead!
	void setImageData(vtkImageData* iData) override;

	//! Access to "main" polydata object (if any)
	//! @deprecated move out of mdi child, into something like an iAModality
	vtkPolyData* polyData() override;

	//! Access to the 3D renderer widget
	iARenderer* renderer() override;
	//! Access slicer for given mode (use iASlicerMode enum for mode values)
	iASlicer* slicer(int mode) override;
	//! Access to 3D renderer widget
	QWidget* rendererWidget() override;
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
	//! Access to dataset information dock widget
	QDockWidget* dataInfoDockWidget() override;
	//! Access to histogram dock widget
	QDockWidget* histogramDockWidget() override;
	//! Access to modalities dock widget
	dlg_modalities* dataDockWidget() override;

	void setReInitializeRenderWindows(bool reInit) override;
	vtkTransform* slicerTransform() override;

	//! Whether results should be opened in a new window; if false, they replace the content of the current window instead
	bool resultInNewWindow() const;
	//! Whether this child has the linked MDIs feature enabled
	bool linkedMDIs() const;
	//! Whether this child has the linked views feature enabled
	bool linkedViews() const override;

	//! Access the histogram widget
	iAChartWithFunctionsWidget* histogram() override;

	int selectedFuncPoint();
	int isFuncEndPoint(int index);
	bool isMaximized();

	// TODO: use world coordinates here
	void updateROI(int const roi[6]) override;
	void setROIVisible(bool visible) override;

	void setWindowTitleAndFile(const QString &f);
	QString currentFile() const override;
	QFileInfo const & fileInfo() const override;
	QString filePath() const override;

	//! @{ Multi-Channel rendering
	//! TODO: check if we still need it with dataSets!
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

	iAMainWindow* mainWnd() override;
	//! Apply current volume settings to all modalities in the current list in dlg_modalities.
	void applyVolumeSettings(const bool loadSavedVolumeSettings);
	//! Returns the name of the layout currently applied to this child window.
	QString layoutName() const override;
	//! Loads the layout with the given name from the settings store, and tries to restore the according dockwidgets configuration
	void loadLayout(QString const & layout) override;

	int chooseModalityNr(QString const & caption = "Choose Channel") override;
	//! If given modality has more than one component, ask user to choose one of them.
	//! (currently used for determining which modality to save)
	int chooseComponentNr(int modalityNr);

	std::shared_ptr<iADataSet> chooseDataSet(QString const& title = "Choose dataset") override;

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

	void addTool(QString const & key, std::shared_ptr<iATool> tool) override;
	void removeTool(QString const& key) override;
	QMap<QString, std::shared_ptr<iATool> > const & tools() override;

	iAInteractionMode interactionMode() const override;
	void setInteractionMode(iAInteractionMode mode);

	//! maximize slicer dockwidget with the given mode
	void maximizeSlicer(int mode);

	//! whether profile handles are currently shown (i.e. "Edit profile points" mode is enabled)
	bool profileHandlesEnabled() const;
	//! whether this child has a profile plot (only has one if "normal" volume data loaded)
	bool hasProfilePlot() const;
	
	//! @{ deprecated
	//! Clear current histogram (i.e. don't show it anymore)
	void clearHistogram() override;
	//! Set the list of modalities for this window.
	void setModalities(QSharedPointer<iAModalityList> modList) override;
	//! Retrieve the list of all currently loaded modalities.
	QSharedPointer<iAModalityList> modalities() override;
	//! Retrieve data for modality with given index.
	QSharedPointer<iAModality> modality(int idx) override;
	bool meshDataMovable();
	void setMeshDataMovable(bool movable);
	bool statisticsComputed(QSharedPointer<iAModality>);
	bool statisticsComputable(QSharedPointer<iAModality>, int modalityIdx = -1);
	void computeStatisticsAsync(std::function<void()> callbackSlot, QSharedPointer<iAModality> modality);
	//! @}

	//! @{ deprecated
	bool histogramComputed(size_t newBinCount, QSharedPointer<iAModality>) override;
	void computeHistogramAsync(std::function<void()> callbackSlot, size_t newBinCount, QSharedPointer<iAModality>) override;
	//! @}

	void set3DControlVisibility(bool visible) override;

	size_t addDataSet(std::shared_ptr<iADataSet> dataSet) override;
	void removeDataSet(size_t dataSetIdx) override;
	void clearDataSets() override;
	std::shared_ptr<iADataSet> dataSet(size_t dataSetIdx) const override;
	std::vector<std::shared_ptr<iADataSet>> dataSets() const override;
	std::vector<size_t> dataSetIndices() const override;

	size_t firstImageDataSetIdx() const override;
	vtkSmartPointer<vtkImageData> firstImageData() const override;
	iAModalityTransfer* dataSetTransfer(size_t idx) const override;
	iADataSetRenderer* dataSetRenderer(size_t idx) const override;
	void applyRenderSettings(size_t dataSetIdx, QVariantMap const& renderSettings) override;

	bool hasUnsavedData() const;

public slots:
	void maximizeRC();

	//! update a specific slicer (specified through slicer mode, @see iASlicerMode)
	void updateSlicer(int index);
	//! update all 3 axis-aligned slicer
	void updateSlicers() override;
	//! update 3D renderer
	void updateRenderer() override;
	//! update all dataset views (3D renderer + all 3 axis-aligned slicers)
	void updateViews() override;

	//! called whenever the transfer function is changed and the views using it need updating
	void changeTransferFunction() override;

	//! @deprecated use logging or global status bar (iAMainWindow::statusBar) instead
	void addStatusMsg(QString const& txt) override;

	//! @{ @deprecated not required
	void disableRenderWindows(int ch) override;
	void enableRenderWindows() override;
	//! @}
	//! @{ @deprecated will be removed soon, see addDataset instead
	void setupView(bool active = false);
	void setupStackView(bool active = false);
	void setupProject(bool active = false);
	void removeFinishedAlgorithms();
	//! @}

	bool updateVolumePlayerView(int updateIndex, bool isApplyForAll);

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
	//! called when the user wants to display the profile handles inside the views
	//! showing the dataset (3D renderer and slicers)
	void toggleProfileHandles(bool isChecked);

private slots:
	void saveRC();
	void saveMovRC();
	void triggerInteractionRaycaster();
	void setSlice(int mode, int s);
	void slicerRotationChanged();
	void setChannel(int ch);
	void updateRenderWindows(int channels);
	void updatePositionMarker(double x, double y, double z, int mode);
	void ioFinished();
	void updateDataSetInfo();
	void histogramDataAvailable(int modalityIdx);
	void statisticsAvailable(int modalityIdx);
	void changeMagicLensDataSet(int chg);
	void changeMagicLensOpacity(int chg);
	void changeMagicLensSize(int chg);
	void saveFinished();
	void modalityAdded(int modalityIdx);
	void toggleFullScreen();
	void styleChanged();

private:
	void closeEvent(QCloseEvent *event) override;
	bool addVolumePlayer();
	void addProfile();
	void updateProfile();
	bool saveAs(int modalityNr);
	void set3DSlicePlanePos(int mode, int slice);

	//! Changes the display of views from full to multi screen or multi screen to fullscreen.
	//! @param mode how the views should be arranged.
	void changeVisibility(unsigned char mode);
	int  visibility() const;
	void hideVolumeWidgets();
	void setVisibility(QList<QWidget*> widgets, bool show);
	void maximizeDockWidget(QDockWidget * dw);
	void demaximizeDockWidget(QDockWidget * dw);
	void resizeDockWidget(QDockWidget * dw);

	void updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s);
	void snakeNormal(int index, double point[3], double normal[3]);

	// DEPRECATED {

	bool initView(QString const& title);

	//! sets up the IO thread for saving the correct file type for the given filename.
	//! @return	true if it succeeds, false if it fails.
	bool setupSaveIO(QString const& f);
	//! sets up the IO thread for loading the correct file type according to the given filename.
	//! @return	true if it succeeds, false if it fails.
	bool setupLoadIO(QString const& f, bool isStack);

	void connectSignalsToSlots();

	void setHistogramModality(int modalityIdx) override;
	//! display histogram - if not computed yet, trigger computation
	void displayHistogram(int modalityIdx);
	//! if available, show histogram (i.e. does not trigger computation, as displayHistogram does)
	void showHistogram(int modalityIdx);
	
	//! adds an algorithm to the list of currently running jobs
	void addAlgorithm(iAAlgorithm* thread);
	void cleanWorkingAlgorithms();

	void setupViewInternal(bool active);
	void initModalities();
	void initVolumeRenderers();

	// DEPRECATED }

	void slicerVisibilityChanged(int mode);
	void updatePositionMarkerSize();

	static const unsigned char RC = 0x01;
	static const unsigned char XY = 0x02;
	static const unsigned char YZ = 0x04;
	static const unsigned char XZ = 0x08;
	static const unsigned char TAB = 0x10;
	static const unsigned char MULTI = 0x1F;

	MainWindow * m_mainWnd;
	QFileInfo m_fileInfo;
	QString m_curFile, m_path;

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
	vtkSmartPointer<vtkPoints> m_worldSnakePoints;
	vtkSmartPointer<iAParametricSpline> m_parametricSpline;
	//! @}

	//! smart pointer to first image data shown in mdiChild.
	//! @deprecated use dataSets instead, will be removed soon
	//! @{
	vtkSmartPointer<vtkImageData> m_imageData;
	vtkPolyData * m_polyData;
	vtkTransform * m_axesTransform;
	//! @}

	vtkTransform * m_slicerTransform;
	iARendererImpl * m_renderer;
	std::array<iASlicerImpl*, 3> m_slicer;
	QSharedPointer<iAProfileProbe> m_profileProbe;
	QScopedPointer<iAVolumeStack> m_volumeStack;
	QList<int> m_checkedList;
	iAIO* m_ioThread;    //!< @deprecated use iAFileTypeRegistry / iAFileIO instead!

	iAChartWithFunctionsWidget * m_histogram;
	iAProfileWidget* m_profile;
	QSharedPointer<iAPlot> m_histogramPlot;

	//! dataset info:
	QListWidget* m_dataSetInfo;

	//! @{ dock widgets
	iADockWidgetWrapper* m_dwHistogram;
	iADockWidgetWrapper* m_dwProfile;
	iADockWidgetWrapper* m_dwInfo;
	iADataSetListWidget* m_dataSetListWidget;                          //!< display widget for list of currently loaded datasets
	iADockWidgetWrapper* m_dwDataSets;
	dlg_volumePlayer * m_dwVolumePlayer;
	dlg_slicer * m_dwSlicer[3];
	dlg_modalities * m_dwModalities;
	dlg_renderer * m_dwRenderer;
	//! @}

	//! @deprecated use jobs instead (iARunAsync + iAJobListView)
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

	size_t m_magicLensDataSet;
	bool m_initVolumeRenderers;
	int m_storedModalityNr;		                                          //!< modality nr being stored
	QMap<QString, std::shared_ptr<iATool>> m_tools;                       //!< list of currently active tools
	iAInteractionMode m_interactionMode;                                  //!< current interaction mode in slicers/renderer (see iAInteractionMode)
	bool m_slicerVisibility[3];

	size_t m_nextDataSetID;                                               //!< holds ID for next dataSet (in order to provide a unique ID to each loaded dataset)
	QMutex m_dataSetMutex;                                                //!< used to guarantee that m_nextDataSetID can only be read and modified together
	std::map<size_t, std::shared_ptr<iADataSet>> m_dataSets;              //!< list of all currently loaded datasets.

	// todo: find better way to handle this
	std::map<size_t, std::shared_ptr<iADataForDisplay>> m_dataForDisplay; //!< optional additional data required for displaying a dataset
	std::map<size_t, std::shared_ptr<iADataSetRenderer>> m_dataRenderers; //!< 3D renderers (one per dataset in m_datasets)
	std::map<size_t, std::shared_ptr<iADataSetRenderer>> m_3dMagicLensRenderers; //!< 3D renderers for magic lens (one per dataset in m_datasets)
	std::map<size_t, std::shared_ptr<iASliceRenderer>> m_sliceRenderers;  //!< slice renderers (one per dataset in m_datsets)
	// should become the replacement for the four maps above
	std::map<size_t, std::shared_ptr<iADataSetViewer>> m_dataSetViewers;

	void setDataSetMovable(size_t dataSetIdx);
	vtkSmartPointer<iAvtkInteractStyleActor> m_manualMoveStyle[4];        //!< for syncing the manual registration between views
};
