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

#include "iAguibase_export.h"

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QSharedPointer>

#include <functional>
#include <memory>



class dlg_modalities;
class iAAlgorithm;
class iAChannelData;
class iAChartWithFunctionsWidget;
class iADataSet;
class iADataSetRenderer;
class iAIO;
class iAMainWindow;
class iAModality;
class iAModalityList;
class iAModalityTransfer;
class iAPreferences;
class iATool;
class iARenderSettings;
class iASlicer;
class iASlicerSettings;
class iARenderer;
class iAVolumeSettings;
class iAVolumeStack;

class vtkImageData;
class vtkPiecewiseFunction;
class vtkPolyData;
class vtkScalarsToColors;
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

	//! Retrieve the list of all currently loaded modalities.
	virtual QSharedPointer<iAModalityList> modalities() = 0;

	//! Retrieve data for modality with given index.
	virtual QSharedPointer<iAModality> modality(int idx) = 0;

	//! Set list of modalities.
	virtual void setModalities(QSharedPointer<iAModalityList> modList) =0;

	virtual void setROIVisible(bool isVisible) = 0;
	virtual void updateROI(int const roi[6]) = 0;

	//! Access to modalities dock widget
	//! (TODO: separate dock widget from rest of functionality)
	virtual dlg_modalities* dataDockWidget() = 0;

	//! Access to slicer dock widget for the given mode
	//! @param mode slicer to access - use constants from iASlicerMode enum
	//! (TODO: separate dock widget from rest of functionality)
	virtual QDockWidget* slicerDockWidget(int mode) = 0;
	
	//! Access to 3D renderer dock widget
	virtual QDockWidget* renderDockWidget() = 0;
	//! Access to dataset information dock widget
	virtual QDockWidget* dataInfoDockWidget() = 0;
	//! Access to histogram dock widget
	virtual QDockWidget* histogramDockWidget() = 0;

	//! Access slicer for given mode (use iASlicerMode enum for mode values)
	virtual iASlicer* slicer(int mode) = 0;

	//! Access to 3D renderer widget
	virtual QWidget* rendererWidget() = 0;

	// Access to some slicer GUI internals (used only in NModalTF at the moment):
	//! Access to the scroll bar next to a slicer
	virtual QSlider* slicerScrollBar(int mode) = 0;
	//! Access to the layout in the slicer dockwidget containing the actual iASlicer
	virtual QHBoxLayout* slicerContainerLayout(int mode) = 0;

	//! Access to the 3D renderer widget
	virtual iARenderer* renderer() = 0;

	virtual iAChartWithFunctionsWidget* histogram() = 0;
	
	//! Clear current histogram (i.e. don't show it anymore)
	virtual void clearHistogram() = 0;


	// Layout:
	//! Loads the layout with the given name from the settings store, and tries to restore the according dockwidgets configuration
	virtual void loadLayout(QString const& layout) = 0;
	
	//! whether the current qss theme is bright mode (true) or dark mode (false)
	//virtual bool brightMode() const = 0;


	// Settings:
	virtual iARenderSettings const& renderSettings() const = 0;
	virtual iAVolumeSettings const& volumeSettings() const = 0;
	virtual iASlicerSettings const& slicerSettings() const = 0;
	virtual iAPreferences const& preferences() const = 0;
	//! Whether this child has the linked views feature enabled
	virtual bool linkedViews() const = 0;

	//! Checks whether the main image data is fully loaded.
	virtual bool isFullyLoaded() const = 0;

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

	//! If more than one modality loaded, ask user to choose one of them.
	//! (currently used for determining which modality to save)
	virtual int chooseModalityNr(QString const& caption) = 0;

	//! If more than one dataset loaded, ask user to choose one of them (used for saving)
	virtual std::shared_ptr<iADataSet> chooseDataSet(QString const& title = "Choose dataset") = 0;

	//! @{
	//! Retrieve data for a given channel ID
	virtual iAChannelData* channelData(uint id) = 0;
	virtual iAChannelData const* channelData(uint id) const = 0;
	//! @}


	// Layouts:
	//! Returns the name of the layout currently applied to this child window.
	virtual QString layoutName() const = 0;
	//! Apply the layout currently selected in the layout combobox in the main window.
	virtual void updateLayout() = 0;
	//! Apply the "multiview" layout (i.e. where not only one dock widget but multiple are shown)
	//! Should probably not be used anymore, might be deprecated soon
	virtual void multiview() = 0;


	// Tools:
	//! add a tool to this child (a collection of UI elements with their own behavior and state)
	virtual void addTool(QString const& key, std::shared_ptr<iATool> tool) = 0;
	//! removes the given tool from this child
	virtual void removeTool(QString const& key) = 0;
	//! retrieve all tools attached to this child
	virtual QMap<QString, std::shared_ptr<iATool>> const& tools() = 0;
	//! save currently loaded files / tools in given project file
	virtual void saveProject(QString const& fileName) = 0;

	// Magic Lens:
	//! Set the ID of the channel which should be the input to the 2D magic lens in slicer
	virtual void setMagicLensInput(uint id) = 0;
	//! Enable/Disable the 2D magic lens in slicer
	virtual void setMagicLensEnabled(bool isOn) = 0;
	//! whether the 2D magic lens in slicer is currently enabled
	virtual bool isMagicLens2DEnabled() const = 0;
	//! Reinitialize magic lens channel?
	//! @deprecated, use channel mechanisms / setMagicLensInput instead!
	virtual void reInitMagicLens(uint id, QString const& name, vtkSmartPointer<vtkImageData> imgData, vtkScalarsToColors* ctf) = 0;

	
	//! Access the "volume stack" if a stack of volumes is loaded
	virtual iAVolumeStack* volumeStack() = 0;

	//! whether the child currently has volume data loaded
	virtual bool isVolumeDataLoaded() const = 0;

	//! apply the given (3D) renderer settings
	virtual bool applyRendererSettings(iARenderSettings const& rs, iAVolumeSettings const& vs) = 0;
	//! Apply settings to the 3D renderer of the dataset with given index (the given map can also contain a subset of the list of available render parameters, the rest will be left at default)
	virtual void applyRenderSettings(size_t dataSetIdx, QVariantMap const& renderSettings) = 0;

	//! add a dataset
	virtual size_t addDataSet(std::shared_ptr<iADataSet> dataSet) = 0;
	//! remove dataset with given ID
	virtual void removeDataSet(size_t dataSetIdx) = 0;
	//! clear (remove) all datasets
	virtual void clearDataSets() = 0;
	//! Retrieve a list of all datasets loaded in this window. NOTE: The index in this data structure does not reflect the index! TODO: maybe remove this to avoid confusion because of the indexing differences!
	virtual std::vector<std::shared_ptr<iADataSet>> dataSets() const = 0;
	//! Retrieve a dataset by its index
	virtual std::shared_ptr<iADataSet> dataSet(size_t dataSetIdx) const = 0;
	//! Retrieve a list of the indices of all datasets loaded in this window
	virtual std::vector<size_t> dataSetIndices() const = 0;
	//! Constant indicating an invalid dataset index
	static const size_t NoDataSet = std::numeric_limits<size_t>::max();

	// Methods currently required by some modules for specific dataset access
	// {
	// ToDo: There's potential for better encapsulation / better API here!
	//! Retrieve the first image dataset (if any loaded).
	//! Will produce an error log entry if no image data is found so use with care
	virtual vtkSmartPointer<vtkImageData> firstImageData() const = 0;
	//! Retrieve the index of the first image data set (if any loaded), or NoDataSet if none loaded.
	virtual size_t firstImageDataSetIdx() const = 0;
	//! Retrieve the transfer function for an (image) dataset with given index
	//! @return transfer function of dataset, nullptr if dataset with given index is not an image dataset
	virtual	iAModalityTransfer* dataSetTransfer(size_t idx) const = 0;
	//! Retrieve the 3D renderer for dataSet with given index
	//! @return the renderer or nullptr if dataset with given index does not exist or has no renderer
	virtual iADataSetRenderer* dataSetRenderer(size_t idx) const = 0;
	// }

	//! set window title, and if a file name is given, set it as window file and add it to recent files
	virtual void setWindowTitleAndFile(QString const& f) = 0;

	//! display an image or a mesh
	//! @deprecated use addDataset instead
	//! Use modality methods instead - though probably still required for first dataset at the moment.
	virtual bool displayResult(QString const& title, vtkImageData* image = nullptr, vtkPolyData* poly = nullptr) = 0;

	// Deprecated:
	//! @deprecated. Use iARunAsync / iAJobListView directly
	//!    also, don't use iAAlgorithm anymore!
	virtual void connectThreadSignalsToChildSlots(iAAlgorithm* thread) = 0;

	//! @deprecated. Use iARunASync / new IO mechanism (io library - iAFileIO)
	virtual void connectIOThreadSignals(iAIO* thread) = 0;

	//! Access to main window.
	//! @deprecated should not be available here
	virtual iAMainWindow* mainWnd() = 0;

	//! Adds a message to the status bar.
	//! @deprecated. Status bar will be removed soon in favor of the log window. Use iALog instead.
	virtual void addStatusMsg(QString const& txt) = 0;

	//! Access to "main" polydata object (if any)
	//! @deprecated move out of mdi child, into something like an iAModality
	virtual vtkPolyData* polyData() = 0;

	//! Access to file info of "current" file
	//! @deprecated. Use access via modalities instead
	virtual QFileInfo const & fileInfo() const = 0;

	//! Load a file
	//! @deprecated. Use modality methods / new IO structure (to be defined)
	virtual bool loadFile(const QString& f, bool isStack) = 0;

	//! Set "main image" - does not update views (see displayResult for a method that does)!
	//! @deprecated all access to images should proceed via modalities (modality(int) / setModalities /...) or channels (createChannel/updateChannel)
	virtual void setImageData(vtkImageData* iData) = 0;
	
	//! @deprecated. Access slicers directly?
	virtual vtkTransform* slicerTransform() = 0;

	//! @deprecated. can be removed together with enableRenderWindow/disableRenderWindow
	virtual void setReInitializeRenderWindows(bool reInit) = 0;

	//! @{ for recomputing histogram. should probably be made private somehow; or members of modality, or triggered automatically on modality creation...
	//! @deprecated
	virtual bool histogramComputed(size_t newBinCount, QSharedPointer<iAModality>) = 0;
	virtual void computeHistogramAsync(std::function<void()> callbackSlot, size_t newBinCount, QSharedPointer<iAModality>) = 0;
	virtual void setHistogramModality(int modalityIdx) = 0;
	//! @}
	virtual void set3DControlVisibility(bool visible) = 0;

	//! save state, for example camera position
	virtual void saveSettings(QSettings& settings) = 0;
	//! load state (saved via saveState)
	virtual void loadSettings(QSettings const& settings) = 0;

signals:
	void closed();
	//! @deprecated. no direct replacement
	void rendererDeactivated(int c);

	//! emitted when the file data is loaded; the initialization operations are not fully
	//! done yet then - use histogramAvailable instead if you require the file to be fully loaded!
	void fileLoaded();

	//! emitted when a file is fully loaded and its statistics and histogram are available.
	//! @deprecated use dataset functionality instead -> dataForDisplayCreated, dataSetRendered
	void histogramAvailable(int modalityIdx);

	//! emitted when the renderer settings have changed
	void renderSettingsChanged();

	//! emitted when the slicer settings have changed
	void slicerSettingsChanged();

	//! emitted when the slicer/renderer views have been updated, and when their camera has been reset
	void viewsUpdated();

	//! emitted when a transfer function changed
	void transferFunctionChanged();

	//! @{
	//! emitted when transfer function is edited
	void pointSelected();
	void noPointSelected();
	void endPointSelected();
	//! @}

	//! emitted whenever the magic lens has been toggled on or off
	void magicLensToggled(bool isToggled);

	//! emitted when the data for displaying a dataset has been computed
	void dataForDisplayCreated(size_t dataSetIdx);
	//! emitted when the renderer for a dataset has been added to the display
	void dataSetRendered(size_t dataSetIdx);

public slots:
	//! Updates all views (slicers, renderers)
	virtual void updateViews() = 0;
	//! Update all slice views
	virtual void updateSlicers() = 0;
	//! Update 3D renderer
	virtual void updateRenderer() = 0;

	//! transfer function was changed.
	virtual void changeTransferFunction() = 0;

	//! method "enabling render windows", basically called when dataset is ready to be shown
	//! but quite convoluted and confusing, so:
	//! @deprecated. will be removed soon, no direct replacement
	virtual void enableRenderWindows() = 0;
	//! @deprecated. will be removed soon, no direct replacement
	virtual void disableRenderWindows(int ch) = 0;

};

//! helper function to add a tool to the current mdi child (if it exists)
template <typename T>
T* addToolToActiveMdiChild(QString const & name, iAMainWindow* mainWnd)
{
	auto child = mainWnd->activeMdiChild();
	if (!child)
	{
		LOG(lvlWarn, "addTool: child not set!");
		return nullptr;
	}
	auto t = std::make_shared<T>(mainWnd, child);
	child->addTool(name, t);
	return t.get();
}

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
