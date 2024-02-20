// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iagui_export.h"

// TODO: get rid of ui_ includes (use ui internally only, see MainWindow::m_ui)
#include "ui_Mdichild.h"
#include "ui_renderer.h"

// guibase
#include "qthelper/iAQTtoUIConnector.h"
#include "iAMdiChild.h"
#include "iAPreferences.h"
#include "iASavableProject.h"
#include "iASlicerSettings.h"

#include <vtkSmartPointer.h>

#include <QFileInfo>
#include <QMap>
#include <QMutex>
#include <QString>

#include <array>
#include <functional>
#include <memory>
#include <vector>

class QListWidget;
class QSpinBox;

class vtkAbstractTransform;
class vtkImageData;
class vtkPiecewiseFunction;
class vtkPoints;
class vtkScalarsToColors;
class vtkTransform;

// gui
class iADataSetListWidget;
class iADataSetViewer;
class iAParametricSpline;
class iAvtkInteractStyleActor;
class MainWindow;

// guibase
class iAChannelData;
class iATool;

// slicer / renderer
class dlg_slicer;
class iARendererImpl;
class iASlicerImpl;

// base
class iADockWidgetWrapper;

typedef iAQTtoUIConnector<QDockWidget, Ui_renderer>  dlg_renderer;

//! Child window of MainWindow's mdi area for showing datasets and visualizations.
//! Most tools in the modules can be added to MdiChild to extend its functionality.
class iAgui_API MdiChild : public iAMdiChild, public Ui_Mdichild, public iASavableProject
{
	Q_OBJECT
public:

	MdiChild(MainWindow* mainWnd, iAPreferences const& preferences, bool unsavedChanges);

	//! performs initialization that needs to be done after the widget is being displayed
	void initializeViews();

	bool save();
	void saveVolumeStack();

	void applyPreferences(iAPreferences const & p);
	void applySlicerSettings(iASlicerSettings const & ss);
	iASlicerSettings const & slicerSettings() const override;
	iAPreferences    const & preferences()    const override;

	void toggleSnakeSlicer(bool isEnabled);
	bool isSnakeSlicerToggled() const;
	void toggleSliceProfile(bool isEnabled);
	bool isSliceProfileEnabled() const;
	void initProfilePoints(double const* start, double const* end) override;

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

	void setWindowTitleAndFile(const QString &f) override;
	QString currentFile() const override;
	QFileInfo const & fileInfo() const override;
	QString filePath() const override;

	//! @{ Multi-Channel rendering
	//! TODO NEWIO: check if we still need multi-channel feature or if it can be replaced with dataSets!
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

	//! @{ 3D Magic Lens
	void toggleMagicLens3D(bool isEnabled);
	bool isMagicLens3DEnabled() const;
	vtkRenderer* magicLens3DRenderer() const override;
	//! @}
	//! @{ 2D Magic Lens
	void toggleMagicLens2D(bool isEnabled);
	bool isMagicLens2DEnabled() const override;
	void setMagicLensInput(uint id) override;
	void setMagicLensEnabled(bool isOn) override;
	int  magicLensSize() const;
	int  magicLensFrameWidth() const;
	//! @}

	QString layoutName() const override;
	void loadLayout(QString const & layout) override;
	void updateLayout() override;
	void multiview() override;
	//! reset the layout to the way it was directly after setting up this child
	void resetLayout();

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

	//! Add a new tool to this child window
	//! @param key a unique key for identifying this tool; will also be used for storing the tool state in a project file
	//! @param tool an instance of an iATool-derived class providing some graphical or computational tool
	void addTool(QString const & key, std::shared_ptr<iATool> tool) override;
	//! Remove tool with the given key from this window
	//! @param key the tool's unique identifying key (the one that was used in addTool for adding the same tool)
	void removeTool(QString const& key) override;
	//! Retrieve all currently attached tools and their keys
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
	std::map<size_t, std::shared_ptr<iADataSet>> const& dataSetMap() const override;
	size_t firstImageDataSetIdx() const override;
	vtkSmartPointer<vtkImageData> firstImageData() const override;
	iADataSetViewer* dataSetViewer(size_t idx) const override;

	bool hasUnsavedData() const;

	void saveSettings(QSettings& settings) override;
	void loadSettings(QSettings const& settings) override;

	void setDataSetMovable(size_t dataSetIdx) override;

	void updatePositionMarkerSize() override;

public slots:
	//! maximize the renderer (so that it takes all of the child's space, all other dock widgets are hidden)
	void maximizeRenderer();

	//! update a specific slicer (specified through slicer mode, @see iASlicerMode)
	void updateSlicer(int index);
	//! Update all 3 axis-aligned slicer
	void updateSlicers() override;
	//! Update 3D renderer
	void updateRenderer() override;
	//! Update all dataset views (3D renderer + all 3 axis-aligned slicers)
	void updateViews() override;

	//! set slicer interaction on / off
	void enableSlicerInteraction(bool enable);
	//! set renderer interaction on / off
	void enableRendererInteraction(bool enable);

	//! Calls the setCamPosition function of iARenderer (described there in more detail).
	//! @param pos set one of the predefined camera positions
	void setPredefCamPos(int pos);

	//! called when the user wants to display the profile handles inside the views
	//! showing the dataset (3D renderer and slicers)
	void toggleProfileHandles(bool isChecked);

private slots:
	void saveRC();
	void saveMovRC();
	void setSlice(int mode, int s);
	void slicerRotationChanged(int mode, double angle);
	void updatePositionMarker(double x, double y, double z, int mode);
	void changeMagicLensDataSet(int chg);
	void toggleFullScreen();

private:
	void closeEvent(QCloseEvent *event) override;
	void dragEnterEvent(QDragEnterEvent* e) override;
	void dropEvent(QDropEvent* e) override;

	bool saveDataSet(std::shared_ptr<iADataSet> dataSet);
	bool saveDataSet(std::shared_ptr<iADataSet> dataSet, QString const & fileName);
	void set3DSlicePlanePos(int mode, int slice);
	void maximizeDockWidget(QDockWidget * dw);
	void demaximizeDockWidget(QDockWidget * dw);
	void resizeDockWidget(QDockWidget * dw);
	void updateSnakeSlicer(QSpinBox* spinBox, iASlicer* slicer, int ptIndex, int s);
	void snakeNormal(int index, double point[3], double normal[3]);
	void connectSignalsToSlots();
	void updateDataSetInfo();

	MainWindow * m_mainWnd;
	QFileInfo m_fileInfo;
	QString m_curFile, m_path;

	QByteArray m_beforeMaximizeState;
	QDockWidget* m_whatMaximized;

	iASlicerSettings m_slicerSettings;
	iAPreferences m_preferences;

	bool m_isSmthMaximized;       //!< whether a single dock widget is currently maximized
	bool m_isUntitled;            //!< whether current content is saved as a file already
	bool m_isSliceProfileEnabled; //!< whether slice profile, shown in slices, is enabled
	bool m_profileHandlesEnabled; //!< whether profile handles (profile points) in renderer/slicer are enabled
	bool m_isMagicLensEnabled;    //!< whether magic lens in slicers is enabled

	//! @{ snake slicer related; move to separate tool maybe?
	bool m_snakeSlicer;           //!< whether snake slicer is enabled
	vtkAbstractTransform *m_savedSlicerTransform[3];
	vtkSmartPointer<vtkPoints> m_worldSnakePoints;
	vtkSmartPointer<iAParametricSpline> m_parametricSpline;
	//! @}

	iARendererImpl * m_renderer;       //!< access and decoration of 3D renderers
	std::array<iASlicerImpl*, 3> m_slicer; //!< the 3 axis-aligned slicers
	vtkSmartPointer<vtkTransform> m_slicerTransform;  //!< the slicer transform (to share rotation between slicers)

	QListWidget* m_dataSetInfo;                                         //!< widget showing information on datasets
	iADataSetListWidget* m_dataSetListWidget;                           //!< widget showing list of currently loaded datasets

	//! @{ dock widgets
	iADockWidgetWrapper * m_dwInfo, * m_dwDataSets;                     //!< dock widgets for dataset info and dataset list widgets
	dlg_slicer * m_dwSlicer[3];
	dlg_renderer * m_dwRenderer;
	//! @}

	QMap<uint, std::shared_ptr<iAChannelData> > m_channels;
	uint m_nextChannelID;
	uint m_magicLensChannel;

	QByteArray m_initialLayoutState;
	QString m_layout;

	size_t m_magicLensDataSet;                                          //!< index of dataset shown in magic lens
	QMap<QString, std::shared_ptr<iATool>> m_tools;                     //!< list of currently active tools
	iAInteractionMode m_interactionMode;                                //!< current interaction mode in slicers/renderer (see iAInteractionMode)

	size_t m_nextDataSetID;                                             //!< holds ID for next dataSet (in order to provide a unique ID to each loaded dataset)
	QMutex m_dataSetMutex;                                              //!< used to guarantee that m_nextDataSetID can only be read and modified together
	std::map<size_t, std::shared_ptr<iADataSet>> m_dataSets;            //!< list of all currently loaded datasets.
	std::map<size_t, std::shared_ptr<iADataSetViewer>> m_dataSetViewers;//!< viewer for a currently loaded dataset; manages all aspects of showing the dataset, e.g. in 3D renderer, slicer, etc.

	vtkSmartPointer<iAvtkInteractStyleActor> m_manualMoveStyle[4];      //!< for syncing the manual registration between views
};
