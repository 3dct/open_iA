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

// TODO: get rid of ui_ includes (use ui internally only, see MainWindow::m_ui)
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
class QSpinBox;

class vtkAbstractTransform;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkPoints;
class vtkPolyData;
class vtkScalarsToColors;
class vtkTransform;

// gui
class dlg_volumePlayer;
class iADataSetListWidget;
class iADataSetViewer;
class iAParametricSpline;
class iAvtkInteractStyleActor;
class MainWindow;

// guibase
class dlg_modalities;
class iAAlgorithm;
class iAChannelData;
class iATool;
// TODO NEWIO: move to new tool
class iAVolumeStack;

// slicer / renderer
class dlg_slicer;
class iARendererImpl;
class iASlicerImpl;

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

	bool saveNew();
	void saveVolumeStack();

	void applyPreferences(iAPreferences const & p);
	void applyViewerPreferences();
	void applyRendererSettings(iARenderSettings const & rs, iAVolumeSettings const & vs) override;
	void applySlicerSettings(iASlicerSettings const & ss);

	void toggleSnakeSlicer(bool isEnabled);
	bool isSnakeSlicerToggled() const;
	void toggleSliceProfile(bool isEnabled);
	bool isSliceProfileEnabled() const;
	void setProfilePoints(double const* start, double const* end) override;
	void setRenderSettings(iARenderSettings const & rs, iAVolumeSettings const & vs);
	void adapt3DViewDisplay();
	iARenderSettings const & renderSettings() const override;
	iAVolumeSettings const & volumeSettings() const override;
	iASlicerSettings const & slicerSettings() const override;
	iAPreferences    const & preferences()    const override;
	//! @deprecated TODO NEWIO: move to separate tool
	iAVolumeStack * volumeStack() override;
	//! @{
	//! @deprecated iAAlgorithm will be removed soon. use iAFilter / iAFileTypeRegistry instead
	void connectThreadSignalsToChildSlots(iAAlgorithm* thread) override;
	void connectAlgorithmSignalsToChildSlots(iAAlgorithm* thread);
	//! @}

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

	vtkTransform* slicerTransform() override;

	//! return true if interaction is enabled  in renderer, false otherwise
	bool isRendererInteractionEnabled() const;
	//! return true if interaction is enabled in all slicers, false otherwise (i.e. if disabled in at least one)
	bool isSlicerInteractionEnabled() const;

	//! Whether results should be opened in a new window; if false, they replace the content of the current window instead
	bool resultInNewWindow() const;
	//! Whether this child has the linked MDIs feature enabled
	bool linkedMDIs() const;
	//! Whether this child has the linked views feature enabled
	bool linkedViews() const override;

	// TODO: use world coordinates here
	void updateROI(int const roi[6]) override;
	void setROIVisible(bool visible) override;

	void setWindowTitleAndFile(const QString &f);
	QString currentFile() const override;
	QFileInfo const & fileInfo() const override;
	QString filePath() const override;

	//! @{ Multi-Channel rendering
	//! TODO NEWIO: check if we still need it with dataSets!
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
	//void reInitMagicLens(uint id, QString const & name, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf) override;
	int  magicLensSize() const;
	int  magicLensFrameWidth() const;
	vtkRenderer* magicLens3DRenderer() const override;
	//! @}

	void applyVolumeSettings();
	QString layoutName() const override;
	void loadLayout(QString const & layout) override;
	void updateLayout() override;
	void multiview() override;
	//! reset the layout to the way it was directly after setting up this child
	void resetLayout();

	//! If given modality has more than one component, ask user to choose one of them.
	//! (currently used for determining which modality to save)
	//int chooseComponentNr(int modalityNr);

	std::shared_ptr<iADataSet> chooseDataSet(QString const& title = "Choose dataset") override;

	iADataSetListWidget* dataSetListWidget() override;

	//! Store current situation in the given project file:
	//!    - loaded files and their transfer functions, when old project file (.mod) is chosen
	//!    - configuration of opened tools (which support it), when new project file (.iaproj) is chosen
	//!      (to be extended to modalities and TFs soon)
	bool doSaveProject(QString const& projectFileName) override;
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

	void set3DControlVisibility(bool visible) override;

	size_t addDataSet(std::shared_ptr<iADataSet> dataSet) override;
	void removeDataSet(size_t dataSetIdx) override;
	void clearDataSets() override;
	std::shared_ptr<iADataSet> dataSet(size_t dataSetIdx) const override;
	size_t dataSetIndex(iADataSet const* dataSet) const override;
	std::vector<std::shared_ptr<iADataSet>> dataSets() const override;
	std::map<size_t, std::shared_ptr<iADataSet>> const& dataSetMap() const override;

	size_t firstImageDataSetIdx() const override;
	vtkSmartPointer<vtkImageData> firstImageData() const override;
	iADataSetViewer* dataSetViewer(size_t idx) const override;
	void applyRenderSettings(size_t dataSetIdx, QVariantMap const& renderSettings) override;

	bool hasUnsavedData() const;

	void saveSettings(QSettings& settings) override;
	void loadSettings(QSettings const& settings) override;

	void setDataSetMovable(size_t dataSetIdx) override;

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

	//! set slicer interaction on / off
	void enableSlicerInteraction(bool enable);
	//! set renderer interaction on / off
	void enableRendererInteraction(bool enable);

	//! @{ @deprecated will be removed soon, see addDataset instead
	// TODO NEWIO: move volume stack to new tool
	void setupStackView(bool active = false);
	void removeFinishedAlgorithms();
	//! @}

	void updateVolumePlayerView(int updateIndex, bool isApplyForAll);

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

	//! called when the user wants to display the profile handles inside the views
	//! showing the dataset (3D renderer and slicers)
	void toggleProfileHandles(bool isChecked);

private slots:
	void saveRC();
	void saveMovRC();
	void setSlice(int mode, int s);
	void slicerRotationChanged();
	void updatePositionMarker(double x, double y, double z, int mode);
	void updateDataSetInfo();
	void changeMagicLensDataSet(int chg);
	void changeMagicLensOpacity(int chg);
	void changeMagicLensSize(int chg);
	void toggleFullScreen();
	void styleChanged();

private:
	void closeEvent(QCloseEvent *event) override;
	bool addVolumePlayer();
	bool saveNew(std::shared_ptr<iADataSet> dataSet);
	bool saveNew(std::shared_ptr<iADataSet> dataSet, QString const & fileName);
	void set3DSlicePlanePos(int mode, int slice);

	void maximizeDockWidget(QDockWidget * dw);
	void demaximizeDockWidget(QDockWidget * dw);
	void resizeDockWidget(QDockWidget * dw);

	void updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s);
	void snakeNormal(int index, double point[3], double normal[3]);

	//! @{ @deprecated
	void connectSignalsToSlots();
	void addAlgorithm(iAAlgorithm* thread);
	void cleanWorkingAlgorithms();
	//! @}

	void slicerVisibilityChanged(int mode);
	void updatePositionMarkerSize();

	static const unsigned char RC = 0x01;
	static const unsigned char XY = 0x02;
	static const unsigned char YZ = 0x04;
	static const unsigned char XZ = 0x08;
	static const unsigned char MULTI = RC | XY | YZ | XZ;

	MainWindow * m_mainWnd;
	QFileInfo m_fileInfo;
	QString m_curFile, m_path;

	QByteArray m_beforeMaximizeState;
	QDockWidget* m_whatMaximized;

	iARenderSettings m_renderSettings;
	iAVolumeSettings m_volumeSettings;
	iASlicerSettings m_slicerSettings;
	iAPreferences m_preferences;

	bool m_isSmthMaximized;       //!< whether a single dock widget is currently maximized
	bool m_isUntitled;            //!< whether current content is saved as a file already
	bool m_isSliceProfileEnabled; //!< whether slice profile, shown in slices, is enabled
	bool m_profileHandlesEnabled; //!< whether profile handles (profile points) in renderer/slicer are enabled
	bool m_isMagicLensEnabled;    //!< whether magic lens in slicers is enabled

	//! @{ snake slicer related:
	bool m_snakeSlicer;           //!< whether snake slicer is enabled
	vtkAbstractTransform *m_savedSlicerTransform[3];
	vtkSmartPointer<vtkPoints> m_worldSnakePoints;
	vtkSmartPointer<iAParametricSpline> m_parametricSpline;
	//! @}

	vtkTransform * m_axesTransform;    //!< transform for the axes in the 3D renderer; TODO: check usage and if it should be placed somewhere else, or made a smart pointer
	vtkTransform * m_slicerTransform;  //!< transform for the axes in the slicers; TODO: check usage and if it should be placed somewhere else, or made a smart pointer

	iARendererImpl * m_renderer;       //!< access and decoration of 3D renderers
	std::array<iASlicerImpl*, 3> m_slicer; //!< the 3 axis-aligned slicers

	// TODO NEWIO: move volume stack functionality to separate tool
	QScopedPointer<iAVolumeStack> m_volumeStack;
	QList<int> m_checkedList;
	//int m_previousIndexOfVolume;

	QListWidget* m_dataSetInfo;                                         //!< widget showing information on datasets
	iADataSetListWidget* m_dataSetListWidget;                           //!< widget showing list of currently loaded datasets

	//! @{ dock widgets
	iADockWidgetWrapper * m_dwInfo, * m_dwDataSets;                     //!< dock widgets for dataset info and dataset list widgets
	dlg_volumePlayer * m_dwVolumePlayer;                                //!< TODO NEWIO move to separate tool
	dlg_slicer * m_dwSlicer[3];
	dlg_renderer * m_dwRenderer;
	//! @}

	//! @deprecated use jobs instead (iARunAsync + iAJobListView)
	std::vector<iAAlgorithm*> m_workingAlgorithms;

	QMap<uint, QSharedPointer<iAChannelData> > m_channels;
	uint m_nextChannelID;
	uint m_magicLensChannel;
	
	QByteArray m_initialLayoutState;
	QString m_layout;

	size_t m_magicLensDataSet;                                          //!< index of dataset shown in magic lens
	bool m_initVolumeRenderers;                                         //!< @deprecated TODO NEWIO remove
	QMap<QString, std::shared_ptr<iATool>> m_tools;                     //!< list of currently active tools
	iAInteractionMode m_interactionMode;                                //!< current interaction mode in slicers/renderer (see iAInteractionMode)
	bool m_slicerVisibility[3];                                         //!< visibility status of slicers; for forwarding it to the display of slice planes in 3D renderer

	size_t m_nextDataSetID;                                             //!< holds ID for next dataSet (in order to provide a unique ID to each loaded dataset)
	QMutex m_dataSetMutex;                                              //!< used to guarantee that m_nextDataSetID can only be read and modified together
	std::map<size_t, std::shared_ptr<iADataSet>> m_dataSets;            //!< list of all currently loaded datasets.
	std::map<size_t, std::shared_ptr<iADataSetViewer>> m_dataSetViewers;//!< viewer for a currently loaded dataset; manages all aspects of showing the dataset, e.g. in 3D renderer, slicer, etc.

	vtkSmartPointer<iAvtkInteractStyleActor> m_manualMoveStyle[4];      //!< for syncing the manual registration between views
};
