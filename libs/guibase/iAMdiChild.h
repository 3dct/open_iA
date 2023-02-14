// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <iALog.h>

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QMap>
#include <QSharedPointer>

#include <functional>
#include <memory>

class iAChannelData;
class iAChartWithFunctionsWidget;
class iADataSet;
class iADataSetViewer;
class iADataSetListWidget;
class iAMainWindow;
class iAPreferences;
class iARenderer;
class iARenderSettings;
class iASlicer;
class iASlicerSettings;
class iATool;
class iATransferFunction;
class iAVolumeSettings;
class iAVolumeStack;

class vtkImageData;
class vtkPiecewiseFunction;
class vtkScalarsToColors;
class vtkRenderer;
class vtkTransform;

class QFileInfo;
class QHBoxLayout;
class QSettings;
class QSlider;

class iAguibase_API iAMdiChild : public QMainWindow
{
	Q_OBJECT
public:
	enum iAInteractionMode
	{
		imCamera,
		imRegistration
	};
	//! Retrieve the current interaction mode (whether camera is changed,
	//! or manual registration is active, see iAInteractionMode)
	virtual iAInteractionMode interactionMode() const = 0;

	virtual void setROIVisible(bool isVisible) = 0;
	virtual void updateROI(int const roi[6]) = 0;

	//! Access to slicer dock widget for the given mode
	//! @param mode slicer to access - use constants from iASlicerMode enum
	virtual QDockWidget* slicerDockWidget(int mode) = 0;
	//! Access to 3D renderer dock widget
	virtual QDockWidget* renderDockWidget() = 0;
	//! Access to dataset information dock widget
	virtual QDockWidget* dataInfoDockWidget() = 0;

	//! Access slicer for given mode (use iASlicerMode enum for mode values)
	virtual iASlicer* slicer(int mode) = 0;
	// Access to some slicer GUI internals (used only in NModalTF at the moment):
	//! Access to the scroll bar next to a slicer
	virtual QSlider* slicerScrollBar(int mode) = 0;
	//! Access to the layout in the slicer dockwidget containing the actual iASlicer
	virtual QHBoxLayout* slicerContainerLayout(int mode) = 0;

	//! Access to widget containing the 3D renderer
	virtual QWidget* rendererWidget() = 0;
	//! Access to the 3D renderer widget
	virtual iARenderer* renderer() = 0;

	// Layout:
	//! Loads the layout with the given name from the settings store, and tries to restore the according dockwidgets configuration
	virtual void loadLayout(QString const& layout) = 0;
	//! Returns the name of the layout currently applied to this child window.
	virtual QString layoutName() const = 0;
	//! Apply the layout currently selected in the layout combobox in the main window.
	virtual void updateLayout() = 0;
	//! Apply the "multiview" layout (i.e. where not only one dock widget but multiple are shown)
	virtual void multiview() = 0;
	
	// Settings:
	virtual iARenderSettings const& renderSettings() const = 0;
	virtual iAVolumeSettings const& volumeSettings() const = 0;
	virtual iASlicerSettings const& slicerSettings() const = 0;
	virtual iAPreferences const& preferences() const = 0;
	//! Whether this child has the linked views feature enabled
	virtual bool linkedViews() const = 0;

	//! Name of the currently open file (project file / first modality file / ...)
	//! If possible, use something more specific (e.g. file name from specific modality)
	virtual QString currentFile() const = 0;
	// path of current file (just path, without filename)
	virtual QString filePath() const = 0;

	// Multi-Channel Rendering:

	//! Create a new channel, return its ID.
	virtual uint createChannel() = 0;
	//! Initialize the renderers for a channel.
	virtual void initChannelRenderer(uint id, bool use3D, bool enableChannel = true) = 0;
	//! Enable/Disable channel rendering for a given channel ID
	virtual void setChannelRenderingEnabled(uint, bool enabled) = 0;
	//! Update the data of the given channel ID.
	virtual void updateChannel(uint id, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf,
		vtkPiecewiseFunction* otf, bool enable) = 0;
	//! Update opacity of the given channel ID.
	virtual void updateChannelOpacity(uint id, double opacity) = 0;
	//! Remove channel in all slicers.
	virtual void removeChannel(uint id) = 0;
	//! @{
	//! Retrieve data for a given channel ID
	virtual iAChannelData* channelData(uint id) = 0;
	virtual iAChannelData const* channelData(uint id) const = 0;
	//! @}

	// Tools:
	//! add a tool to this child (a collection of UI elements with their own behavior and state)
	virtual void addTool(QString const& key, std::shared_ptr<iATool> tool) = 0;
	//! removes the given tool from this child
	virtual void removeTool(QString const& key) = 0;
	//! retrieve all tools attached to this child
	virtual QMap<QString, std::shared_ptr<iATool>> const& tools() = 0;

	// Magic Lens:
	//! Set the ID of the channel which should be the input to the 2D magic lens in slicer
	virtual void setMagicLensInput(uint id) = 0;
	//! Enable/Disable the 2D magic lens in slicer
	virtual void setMagicLensEnabled(bool isOn) = 0;
	//! whether the 2D magic lens in slicer is currently enabled
	virtual bool isMagicLens2DEnabled() const = 0;
	//! Reinitialize magic lens channel?
	//! @deprecated, use channel mechanisms / setMagicLensInput instead!
	//virtual void reInitMagicLens(uint id, QString const& name, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf) = 0;
	virtual vtkRenderer* magicLens3DRenderer() const = 0;

	//! Access the "volume stack" if a stack of volumes is loaded
	virtual iAVolumeStack* volumeStack() = 0;

	//! whether the child currently has volume data loaded
	virtual bool isVolumeDataLoaded() const = 0;

	//! apply the given (3D) renderer settings
	virtual void applyRendererSettings(iARenderSettings const& rs, iAVolumeSettings const& vs) = 0;
	//! Apply settings to the 3D renderer of the dataset with given index (the given map can also contain a subset of the list of available render parameters, the rest will be left at default)
	virtual void applyRenderSettings(size_t dataSetIdx, QVariantMap const& renderSettings) = 0;

	// Datasets:
	// TODO NEWIO: There's potential for better encapsulation / better API here! Maybe extract to separate dataset container?
	//! add a dataset
	virtual size_t addDataSet(std::shared_ptr<iADataSet> dataSet) = 0;
	//! remove dataset with given ID
	virtual void removeDataSet(size_t dataSetIdx) = 0;
	//! clear (remove) all datasets
	virtual void clearDataSets() = 0;
	//! Retrieve a dataset by its index
	virtual std::shared_ptr<iADataSet> dataSet(size_t dataSetIdx) const = 0;
	//! Retrieve the index of a dataset
	virtual size_t dataSetIndex(iADataSet const* dataSet) const = 0;
	//! Retrieve a list of the indices of all datasets loaded in this window
	virtual std::map<size_t, std::shared_ptr<iADataSet>> const & dataSetMap() const = 0;
	//! If more than one dataset loaded, ask user to choose one of them (used for saving)
	virtual std::shared_ptr<iADataSet> chooseDataSet(QString const& title = "Choose dataset") = 0;
	//! Constant indicating an invalid dataset index
	static const size_t NoDataSet = std::numeric_limits<size_t>::max();
	//! Retrieve dataset list
	virtual iADataSetListWidget* dataSetListWidget() = 0;

	// Methods currently required by some modules for specific dataset access
	// {
	// TODO NEWIO: There's potential for better encapsulation / better API here! Maybe extract to separate dataset container?
	//! Retrieve the first image dataset (if any loaded).
	//! Will produce an error log entry if no image data is found so use with care
	virtual vtkSmartPointer<vtkImageData> firstImageData() const = 0;
	//! Retrieve the index of the first image data set (if any loaded), or NoDataSet if none loaded.
	virtual size_t firstImageDataSetIdx() const = 0;
	//! Retrieve the viewer for the dataset with given index
	//! @return viewer for the given dataset index, nullptr if there is no dataset with given index or if there is no viewer available
	virtual iADataSetViewer* dataSetViewer(size_t idx) const =0;

	//! set window title, and if a file name is given, set it as window file and add it to recent files
	virtual void setWindowTitleAndFile(QString const& f) = 0;

	//! Access to file info of "current" file
	//! @deprecated. Use access via datasets instead
	virtual QFileInfo const & fileInfo() const = 0;
	
	//! @deprecated access transform used in slicer. should be removed from here; no replacement in place yet
	virtual vtkTransform* slicerTransform() = 0;

	virtual void set3DControlVisibility(bool visible) = 0;

	//! save state, for example camera position
	virtual void saveSettings(QSettings& settings) = 0;
	//! load state (saved via saveState)
	virtual void loadSettings(QSettings const& settings) = 0;

	//! a crutch for letting interactor know which dataset is to be moved and synced across slicers
	virtual void setDataSetMovable(size_t dataSetIdx) = 0;

	//! called to set profile point positions in renderer and slicer
	virtual void setProfilePoints(double const* start, double const* end) = 0;

signals:
	void closed();
	//! @deprecated. no direct replacement
	void rendererDeactivated(int c);

	//! emitted when the file data is loaded; the initialization operations are not fully
	//! done yet then - use dataSetRendered instead if you require the file to be fully loaded!
	void fileLoaded();

	//! emitted when the preferences have changed
	void preferencesChanged();

	//! emitted when the renderer settings have changed
	void renderSettingsChanged();

	//! emitted when the slicer settings have changed
	void slicerSettingsChanged();

	//! emitted when the slicer/renderer views have been updated, and when their camera has been reset
	void viewsUpdated();

	//! emitted whenever the magic lens has been toggled on or off
	void magicLensToggled(bool isToggled);

	//! emitted whenever one of the profile points changes
	void profilePointChanged(int pointIdx, double* globalPos);

	// {
	// TODO NEWIO: move to some separate dataset list manager class?
	//! emitted when all data for displaying a dataset has been prepared
	void dataSetPrepared(size_t dataSetIdx);
	//! emitted when the dataset has been added to all relevant views
	void dataSetRendered(size_t dataSetIdx);
	//! emitted when a dataset has been selected in the data list
	void dataSetSelected(size_t dataSetIdx);
	//! emitted when properties of a dataset have been changed
	void dataSetChanged(size_t dataSetIdx);
	//! emitted when a dataset has been removed
	void dataSetRemoved(size_t dataSetIdx);
	// }

public slots:
	//! Updates all views (slicers, renderers)
	virtual void updateViews() = 0;
	//! Update all slice views
	virtual void updateSlicers() = 0;
	//! Update 3D renderer
	virtual void updateRenderer() = 0;
};

//! return the first tool that matches the given type for the current child
template <typename T>
T* getTool(iAMdiChild* child)
{
	static_assert(std::is_base_of<iATool, T>::value, "getTool: given type must inherit from iATool!");
	for (auto t : child->tools())
	{
		auto dt = dynamic_cast<T*>(t.get());
		if (dt)
		{
			return dt;
		}
	}
	return nullptr;
}
