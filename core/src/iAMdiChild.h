/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "open_iA_Core_export.h"

#include "iAVtkWidgetFwd.h"

#include <vtkSmartPointer.h>

#include <QMainWindow>
#include <QSharedPointer>

class dlg_modalities;
class iAAlgorithm;
class iAChannelData;
class iAChartWithFunctionsWidget;
class iAIO;
class iAMainWindow;
class iAModality;
class iAModalityList;
class iAPreferences;
class iAProjectBase;
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

class open_iA_Core_API iAMdiChild : public QMainWindow
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
	//! Access to image property dock widget
	virtual QDockWidget* imagePropertyDockWidget() = 0;
	//! Access to histogram dock widget
	virtual QDockWidget* histogramDockWidget() = 0;



	//! Access slicer for given mode (use iASlicerMode enum for mode values)
	virtual iASlicer* slicer(int mode) = 0;

	//! Access to the 3D renderer widget
	virtual iARenderer* renderer() = 0;
	//! Access to the 3D renderer vtk widget
	virtual iAVtkWidget* renderVtkWidget() = 0;

	virtual iAChartWithFunctionsWidget* histogram() = 0;
	
	//! Clear current histogram (i.e. don't show it anymore)
	virtual void clearHistogram() = 0;


	// Layout:
	//! Loads the layout with the given name from the settings store, and tries to restore the according dockwidgets configuration
	virtual void loadLayout(QString const& layout) = 0;


	// Settings:
	virtual iARenderSettings const& renderSettings() const = 0;
	virtual iAVolumeSettings const& volumeSettings() const = 0;
	virtual iASlicerSettings const& slicerSettings() const = 0;
	virtual iAPreferences const& preferences() const = 0;
	//! Whether this child has the linked views feature enabled
	virtual bool linkedViews() const = 0;

	//! Current coordinate position (defined by the respective slice number in the slice views).
	virtual int const* position() const = 0;


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


	// Projects:
	//! add project
	virtual void addProject(QString const& key, QSharedPointer<iAProjectBase> project) = 0;
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

	//! "set" rednder settings
	virtual bool editRendererSettings(iARenderSettings const& rs, iAVolumeSettings const& vs) = 0;

	//! display an image or a mesh
	//! might be deprecated soon.
	//! Use modality methods instead - though probably still required for first dataset at the moment.
	virtual bool displayResult(QString const& title, vtkImageData* image = nullptr, vtkPolyData* poly = nullptr) = 0;

	// Deprecated:
	//! @deprecated. Use iARunAsync / iAJobListView directly
	//!    also, don't use iAAlgorithm anymore!
	virtual void connectThreadSignalsToChildSlots(iAAlgorithm* thread) = 0;

	//! @deprecated. Use iARunASync / new IO mechanism (to be devised)...
	virtual void connectIOThreadSignals(iAIO* thread) = 0;

	//! Access to main window.
	//! @deprecated should not be available here
	virtual iAMainWindow* mainWnd() = 0;

	//! Adds a message to the status bar.
	//! @deprecated. Status bar will be removed soon in favor of the log window. Use iALog instead.
	virtual void addStatusMsg(QString const& txt) = 0;

	//! Access to the "main image"
	//! @deprecated retrieve images via the modalities (modality(int) etc.) instead!
	virtual vtkImageData* imageData() = 0;

	//! Access to the "main image"
	//! @deprecated retrieve images via the modalities (modality(int) etc.) instead!
	virtual vtkSmartPointer<vtkImageData> imagePointer() = 0;

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
signals:
	void closed();
	//! @deprecated. no direct replacement
	void rendererDeactivated(int c);

	//! emitted when the file data is loaded; the initialization operations are not fully
	//! done yet then - use histogramAvailable instead if you require the file to be fully loaded!
	void fileLoaded();

	//! emitted when a file is fully loaded and its statistics and histogram are available.
	void histogramAvailable();

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

	//! obscure signal, actual use has to be determined
	void active();

	//! emitted whenever the magic lens has been toggled on or off
	void magicLensToggled(bool isToggled);

public slots:
	//! Updates all views (slicers, renderers)
	virtual void updateViews() = 0;
	//! Update all slice views
	virtual void updateSlicers() = 0;
	//! method "enabling render windows", basically called when dataset is ready to be shown
	//! but quite convoluted and confusing, so:
	//! @deprecated. will be removed soon, no direct replacement
	virtual void enableRenderWindows() = 0;
	//! @deprecated. will be removed soon, no direct replacement
	virtual void disableRenderWindows(int ch) = 0;
};