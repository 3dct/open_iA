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

#include "iAguibase_export.h"
#include "iASlicerMode.h"
#include "iAVtkWidget.h"

#include <vtkSmartPointer.h>

class iAChannelData;
class iAChannelSlicerData;
class iAMagicLens;
class iAMdiChild;
class iASingleSlicerSettings;

class vtkAlgorithmOutput;
class vtkCamera;
class vtkGenericOpenGLRenderWindow;
class vtkImageActor;
class vtkObject;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkScalarsToColors;

//! vtk-based slicer widget. "Channels" (i.e. image layers) are inserted via the addChannel method
class iAguibase_API iASlicer : public iAVtkWidget
{
	Q_OBJECT
public:
	iASlicer(QWidget* parent):
		iAVtkWidget(parent)
	{}
	//! Sets up the slicer with the given settings.
	virtual void setup(iASingleSlicerSettings const & settings) = 0;
	virtual ~iASlicer(){};

	//! @{ Magic Lens methods
	virtual void setMagicLensEnabled( bool isEnabled ) = 0;
	virtual void setMagicLensCount(int count) = 0;
	virtual void setMagicLensInput(uint id) = 0;
	virtual uint magicLensInput() const = 0;
	virtual void setMagicLensOpacity(double opacity) = 0;
	virtual double magicLensOpacity() const = 0;
	virtual void updateMagicLensColors() = 0;
	virtual void updateMagicLens() = 0;
	virtual iAMagicLens* magicLens() = 0;
	//! @}

	virtual void disableInteractor() = 0;
	virtual void enableInteractor() = 0;  //also updates widget

	//! @{ management of channels - each channel represents one "layer"
	virtual void addChannel(uint id, iAChannelData const& chData, bool enable) = 0;
	virtual void removeChannel(uint id) = 0;
	virtual void updateChannel(uint id, iAChannelData const& chData) = 0;
	virtual iAChannelSlicerData* channel(uint id) = 0;
	virtual void setChannelOpacity(uint id, double opacity) = 0;
	virtual void enableChannel(uint id, bool enabled) = 0;
	//size_t channelCount() const;
	virtual bool hasChannel(uint id) const = 0;
	//! @}

	// { TODO: check whether these can be removed somehow!
	virtual void addImageActor(vtkSmartPointer<vtkImageActor> imgActor) = 0;
	virtual void removeImageActor(vtkSmartPointer<vtkImageActor> imgActor) = 0;
	// }

	//! @{ ROI rectangle
	virtual void setROIVisible(bool isVisible) = 0;
	virtual void updateROI(int const roi[6]) = 0;
	//! @}

	virtual void setResliceAxesOrigin(double x, double y, double z) = 0;
	//! Access to the slicer's main renderer.
	virtual vtkRenderer* renderer() = 0;
	//! Access to the slicer's render window.
	virtual vtkGenericOpenGLRenderWindow* renderWindow() = 0;
	//! Access to the interactor of this slicer's render window.
	virtual vtkRenderWindowInteractor* interactor() = 0;
	//! Access to the slicer's main renderer's camera.
	virtual vtkCamera* camera() = 0;

	//! Get the slice mode (which axis-aligned slice-plane is used for slicing).
	virtual iASlicerMode mode() const = 0;
	//! Sets the slice mode (which axis-aligned slice-plane to use for slicing).
	virtual void setMode(const iASlicerMode mode) = 0;

	//! Set the camera for the slicer's main renderer.
	//! Use this if you want share the camera between multiple views (i.e. synchronize their viewing parameters)
	//! @param camera the new camera to assing
	//! @param camOwner whether the slicer should assume ownership of the camera. If true, Delete() will be called on it in the destructor
	virtual void setCamera(vtkCamera* camera, bool camOwner = true) = 0;
	//! Resets the slicer's main renderer's camera such that all channels in it are visible.
	virtual void resetCamera() = 0;
	//! Sets the background color of the slicer.
	//! By default, background color is auto-determined via the slicer mode. If set manually
	//! via this method, it will keep the given color indefinitely
	//! @param r red color part (0..1)
	//! @param g green color part (0..1)
	//! @param b blue color part (0..1)
	virtual void setBackground(double r, double g, double b) = 0;

	virtual void setDefaultInteractor() = 0;

	//! Blend two images. Should probably be implemented in terms of two channels?
	virtual void blend(vtkAlgorithmOutput* data1, vtkAlgorithmOutput* data2, double opacity, double* range) = 0;

	//! Get current slice number
	virtual int sliceNumber() const = 0;

	//! @{ set contour line parameters.
	virtual void setContours(int numberOfContours, double contourMin, double contourMax) = 0;
	virtual void setContours(int numberOfContours, double const* contourValues) = 0;
	//! @}

	//! Enable/disable the tooltip text
	virtual void setShowText(bool isVisible) = 0;

	virtual void execute(vtkObject* caller, unsigned long eventId, void* callData) = 0;

	virtual void updateChannelMappers() = 0;
	//void switchContourSourceToChannel( uint id );

	virtual void setScalarBarTF(vtkScalarsToColors* ctf) = 0;

	virtual void setIndex(int x, int y, int z) = 0;
	//! in case the "linked mdi" feature is used, use this to set the mdi child this slicer is linked to.
	virtual void setLinkedMdiChild(iAMdiChild* mdiChild) = 0;
public slots:
	//! Save an image of the image viewer native resolution or the current view.
	virtual void saveAsImage() = 0;
	//! Save a movie of a full slice-through of the specimen from top to bottom
	virtual void saveMovie() = 0;
	virtual void setSliceNumber(int sliceNumber) = 0;
	virtual void rotateSlice( double angle ) = 0;
	virtual void update() = 0;

signals:
	void shiftMouseWheel(int angle);
	void altMouseWheel(int angle);
	void ctrlMouseWheel(int angle);
	void clicked();
	void dblClicked();
	void updateSignal();
	void leftClicked(int x, int y, int z);
	void rightClicked(int x, int y, int z);
	void released(int x, int y, int z);
	void userInteraction();
	void oslicerPos(int x, int y, int z, int mode); //!< triggered on mouse move
	void pick();
	//! triggered when slice number changed.
	//! @param mode slicer mode (=plane)
	//! @param sliceNumber number of the slice that was switched to
	void sliceNumberChanged(int mode, int sliceNumber);
	void magicLensToggled(bool enabled);
};
