/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                          Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth       *
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
#include "iASlicerMode.h"
#include "iASlicerSettings.h"    // for iASingleSlicerSettings
#include "iAVtkWidget.h"

#include <vtkSmartPointer.h>

#include <QCursor>
#include <QMap>
#include <QSharedPointer>


class iASlicerProfile;
class iASlicerProfileHandles;
class iAChannelData;
class iAChannelSlicerData;
class iAInteractorStyleImage;
class iAMagicLens;
class iARulerWidget;
class iASingleSlicerSettings;
class iASlicer;
class iASnakeSpline;
class iAWrapperText;
class MdiChild;

class vtkAbstractTransform;
class vtkActor;
class vtkAlgorithmOutput;
class vtkCamera;
class vtkScalarsToColors;
class vtkDiskSource;
class vtkGenericOpenGLRenderWindow;
class vtkImageActor;
class vtkImageData;
class vtkImageMapToColors;
class vtkImageReslice;
class vtkInteractorStyle;
class vtkLineSource;
class vtkLogoWidget;
class vtkLogoRepresentation;
class vtkMarchingContourFilter;
class vtkObject;
class vtkCubeSource;
class vtkPoints;
class vtkQImageToImageSource;
class vtkPolyDataMapper;
class vtkRegularPolygonSource;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkScalarBarWidget;
class vtkTextProperty;
class vtkTextActor3D;
class vtkThinPlateSplineTransform;
class vtkTransform;
class vtkWorldPointPicker;

class QAction;
class QMenu;
class QWidget;

//! vtk-based slicer widget. "Channels" (i.e. image layers) are inserted via the addChannel method
class open_iA_Core_API iASlicer : public iAVtkWidget
{
	Q_OBJECT
public:
	enum InteractionMode {
		Normal,
		SnakeEdit,
		SnakeShow
	};
	//! Creates a new slicer widget.
	//! @param parent the parent widget; can be nullptr for no current parent.
	//! @param mode determines which axis-aligned slice-plane is used for slicing.
	//! @param decorations whether to show the scalar bar widget, the measure bar, the logo and the tooltip.
	//! @param magicLensAvailable whether a magic lens should be available.
	//! @param transform the basic transform the reslicers inside the channels of this slicer (should probably be removed here).
	//! @param snakeSlicerPoints the array of points in the snake slicer (leave at default nullptr if you don't require snake slicer).
	iASlicer(QWidget * parent, const iASlicerMode mode, bool decorations = true, bool magicLensAvailable = true,
		vtkAbstractTransform *transform = nullptr, vtkPoints* snakeSlicerPoints = nullptr);
	//! Sets up the slicer with the given settings.
	void setup(iASingleSlicerSettings const & settings);
	virtual ~iASlicer();

	//! @{ Magic Lens methods
	void setMagicLensEnabled( bool isEnabled );
	void setMagicLensSize(int newSize);
	int  magicLensSize() const;
	void setMagicLensFrameWidth(int newWidth);
	void setMagicLensCount(int count);
	void setMagicLensInput(uint id);
	uint magicLensInput() const;
	void setMagicLensOpacity(double opacity);
	double magicLensOpacity() const;
	void updateMagicLensColors();
	void updateMagicLens();
	iAMagicLens * magicLens();
	//! @}

	void disableInteractor();
	void enableInteractor(); //also updates widget

	//! @{ management of channels - each channel represents one "layer"
	void addChannel(uint id, iAChannelData const & chData, bool enable);
	void removeChannel(uint id);
	void updateChannel(uint id, iAChannelData const & chData );
	iAChannelSlicerData * channel(uint id);
	void setChannelOpacity(uint id, double opacity);
	void enableChannel(uint id, bool enabled);
	//size_t channelCount() const;
	bool hasChannel(uint id) const;
	//! @}

	// { TODO: check whether these can be removed somehow!
	void addImageActor(vtkSmartPointer<vtkImageActor> imgActor);
	void removeImageActor(vtkSmartPointer<vtkImageActor> imgActor);
	// }

	//! @{ ROI rectangle
	void setROIVisible(bool isVisible);
	void updateROI(int const roi[6]);
	//! @}

	void setResliceAxesOrigin(double x, double y, double z);
	//! Access to the slicer's main renderer.
	vtkRenderer * renderer();
	//! Access to the slicer's render window.
	vtkGenericOpenGLRenderWindow * renderWindow();
	//! Access to the interactor of this slicer's render window.
	vtkRenderWindowInteractor * interactor();
	//! Access to the slicer's main renderer's camera.
	vtkCamera * camera();

	//! Get the slice mode (which axis-aligned slice-plane is used for slicing).
	iASlicerMode mode() const;
	//! Sets the slice mode (which axis-aligned slice-plane to use for slicing).
	void setMode(const iASlicerMode mode);

	void setStatisticalExtent(int statExt);

	//! Set the camera for the slicer's main renderer.
	//! Use this if you want share the camera between multiple views (i.e. synchronize their viewing parameters)
	//! @param camera the new camera to assing
	//! @param camOwner whether the slicer should assume ownership of the camera. If true, Delete() will be called on it in the destructor
	void setCamera(vtkCamera * camera, bool camOwner = true);
	//! Resets the slicer's main renderer's camera such that all channels in it are visible.
	void resetCamera();
	//! Sets the background color of the slicer.
	//! By default, background color is auto-determined via the slicer mode. If set manually
	//! via this method, it will keep the given color indefinitely
	//! @param r red color part (0..1)
	//! @param g green color part (0..1)
	//! @param b blue color part (0..1)
	void setBackground(double r, double g, double b);

	void setTransform(vtkAbstractTransform * tr);

	void setDefaultInteractor();

	//! Blend two images. Should probably be implemented in terms of two channels?
	void blend(vtkAlgorithmOutput *data1, vtkAlgorithmOutput *data2, double opacity, double * range);

	//! Get current slice number
	int sliceNumber() const;
	//! Set the position of the position marker (in slicer coordinates).
	void setPositionMarkerCenter(double x, double y, double z);

	//! Enable/disable contour lines.
	void showIsolines(bool s);
	//! @{ set contour line parameters.
	void setContours(int numberOfContours, double contourMin, double contourMax);
	void setContours(int numberOfContours, double const * contourValues);
	//! @}

	//! Enable/disable the tooltip text
	void setShowText(bool isVisible);
	void setMouseCursor(QString const & s);

	void showPosition(bool s);
	void saveSliceMovie(QString const & fileName, int qual = 2);

	void execute(vtkObject * caller, unsigned long eventId, void * callData);

	void updateChannelMappers();
	//void switchContourSourceToChannel( uint id );

	void setScalarBarTF(vtkScalarsToColors* ctf);

	QCursor mouseCursor();

	void setRightButtonDragZoomEnabled(bool enabled);

	void setIndex(int x, int y, int z);
	//! in case the "linked mdi" feature is used, use this to set the mdi child this slicer is linked to.
	void setLinkedMdiChild(MdiChild* mdiChild);
	//! call if the dimension of the input in direction of the slice axis has changed.
	void triggerSliceRangeChange();
public slots:
	//! Save an image of the image viewer native resolution or the current view.
	void saveAsImage();
	//! Save an image stack of the current view.
	void saveImageStack();
	//! Save a movie of a full slice-through of the specimen from top to bottom
	void saveMovie();
	//! Toggle interactor state of this slicer between enabled/disabled
	void toggleInteractorState();
	void setSliceNumber(int sliceNumber);
	void rotateSlice( double angle );
	void setSlabThickness(int thickness);
	void setSlabCompositeMode(int compositeMode);
	void update();

	//! Moves a point of the snake slicer to a new position.
	void movePoint(size_t selectedPointIndex, double xPos, double yPos, double zPos);

	//! Function to deselect points in snake slicer (necessary to avoid endless loops with signals and slots).
	void deselectPoint();

	//! Switches between interaction modi (normal, snake slicer view or editing)
	//! @param mode mode which should be switched to  (see InteractionMode enum)
	void switchInteractionMode(int mode);
	//! Toggle the "raw" profile mode, i.e. whether the profile is shown on top of the slicer image
	void setSliceProfileOn(bool isOn);
	//! Toggle the possibility to move start and end point of the profile
	void setProfileHandlesOn(bool isOn);
	//! Sets coordinates for line profile
	bool setProfilePoint(int pointInd, double* Pos);

	//! Adds a new spline point to the end of the spline curve.
	void addPoint(double x, double y, double z);
	//! Deletes the current spline curve.
	void deleteSnakeLine();
	//! Called when the delete snake line menu is clicked.
	void menuDeleteSnakeLine();

private slots:
	void menuCenteredMagicLens();
	void menuOffsetMagicLens();
	void toggleLinearInterpolation();
	void toggleWindowLevelAdjust();
	void toggleShowTooltip();

signals:
	void addedPoint(double x, double y, double z);
	void movedPoint(size_t selectedPointIndex, double xPos, double yPos, double zPos);
	void profilePointChanged(int pointInd, double * Pos);
	void deselectedPoint();
	void switchedMode(int mode);
	void deletedSnakeLine();
	void shiftMouseWheel(int angle);
	void altMouseWheel(int angle);
	void ctrlMouseWheel(int angle);
	void clicked();
	void dblClicked();
	void msg(QString s);
	void progress(int);
	void updateSignal();
	void leftClicked(int x, int y, int z);
	void rightClicked(int x, int y, int z);
	void released(int x, int y, int z);
	void userInteraction();
	void oslicerPos(int x, int y, int z, int mode); //!< triggered on mouse move
	void pick();
	void sliceRotated(); //!< triggered when slice was rotated
	//! triggered when slice number changed.
	//! @param mode slicer mode (=plane)
	//! @param sliceNumber number of the slice that was switched to
	void sliceNumberChanged(int mode, int sliceNumber);
	void sliceRangeChanged(int minIdx, int maxIdx);

protected:
	QAction* m_actionLinearInterpolation, * m_actionToggleWindowLevelAdjust,
		* m_actionMagicLens, * m_actionMagicLensCentered, * m_actionMagicLensOffset,
		* m_actionDeleteSnakeLine, * m_actionShowTooltip;
	QMenu *         m_contextMenu;               //!< context menu
	InteractionMode m_interactionMode;           //!< current edit mode
	int             m_xInd, m_yInd, m_zInd;      //!< current position
	iASnakeSpline * m_snakeSpline;				 //!< holds the visualization data for the points of the snake splicer
	vtkPoints *     m_worldSnakePoints;          //!< points of the snake slicer (owned by mdichild, not by this slicer)
	bool            m_isSliceProfEnabled;        //!< if slice profile mode is enabled
	iASlicerProfile	* m_sliceProfile;            //!< a slice profile drawn directly on the slicer
	bool            m_profileHandlesEnabled;     //!< if profile handles are enabled (shown)
	iASlicerProfileHandles * m_profileHandles;   //!< handles for the start and end point of the profile plot

	void keyPressEvent(QKeyEvent * event) override;
	void mousePressEvent(QMouseEvent * event) override;
	void mouseMoveEvent(QMouseEvent * event) override;
	void mouseReleaseEvent(QMouseEvent * event) override;
	void mouseDoubleClickEvent(QMouseEvent* event) override;
	void contextMenuEvent(QContextMenuEvent * event) override;
	void resizeEvent(QResizeEvent * event) override;
	void wheelEvent(QWheelEvent*) override;

	void updateBackground();
	void printVoxelInformation();
	void executeKeyPressEvent();
	void defaultOutput();

	//!	This function is used to check whether any agreeable maximum gradient is near the given point.
	//!	The ROI is 2 voxels on all four direction. if yes move to the closest maximum gradient.
	//!
	//! Input is the cursor point. Calculate the gradient magnitude for varying "x" value with "y" constant.
	//! The maximum gradient value is taken as H_maxCoord. If there are two same gradient magnitude values, the point
	//! closer to the cursor point is taken as H_maxCoord. Apply the above procedure for constant "x" and varying "y"
	//! to calculate V_maxCoord. Check whether H_maxCoord gradient magnitude and V_maxCoord gradient magnitude are
	//! higher than an gradient threshold value (5% of max intensity in the image). If H_maxCoord gradient magnitude
	//! is higher and V_maxCoord gradient magnitude is lesser than the threshold, take H_maxCoord as the closet
	//! gradient to cursor point. And if it is vice versa take V_maxCoord take as closest gradient to the cursor point.
	//! If point are higher than threshold, the point closest to the cursor point is take as the next point.
	//! @param [in,out]	x	The x coordinate.
	//! @param [in,out]	y	The y coordinate.
	//void snapToHighGradient(double &x, double &y);

private:
	iASlicerMode m_mode;            //!< the (axis-aligned) slice plane this slicer views

	bool m_decorations;             //!< whether "decorations" will be shown, i.e. scalar bar widget, text on hover, ...
	bool m_userSetBackground;       //!< whether the user has set a background
	bool m_showPositionMarker;      //!< whether the position marker is shown in the slicer

	uint m_magicLensInput;
	QSharedPointer<iAMagicLens> m_magicLens;

	//! @{ fish-eye lens
	void initializeFisheyeLens(vtkImageReslice* reslicer);
	void updateFisheyeTransform(double focalPt[3], vtkImageReslice* reslicer, double lensRadius, double innerLensRadius);

	bool m_fisheyeLensActivated;
	double m_fisheyeRadius;
	double m_innerFisheyeRadius;

	// variables for transformation
	vtkSmartPointer<vtkThinPlateSplineTransform> m_fisheyeTransform;
	vtkSmartPointer<vtkPoints> m_pointsSource;
	vtkSmartPointer<vtkPoints> m_pointsTarget;
	// variables for lens appearance
	vtkSmartPointer<vtkRegularPolygonSource> m_fisheye;
	vtkSmartPointer<vtkPolyDataMapper> m_fisheyeMapper;
	vtkSmartPointer<vtkActor> m_fisheyeActor;

	QList<vtkSmartPointer<vtkRegularPolygonSource>> m_circle1List;
	QList<vtkSmartPointer<vtkActor>> m_circle1ActList;
	QList<vtkSmartPointer<vtkRegularPolygonSource>> m_circle2List;
	QList<vtkSmartPointer<vtkActor>> m_circle2ActList;
	//! @}

	vtkRenderWindowInteractor * m_interactor;  //!< FIXME: only convenience to access interactor of underlying QVTKOpenGLNativeWidget!
	iAInteractorStyleImage * m_interactorStyle;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renWin;
	vtkSmartPointer<vtkRenderer> m_ren;
	vtkCamera * m_camera; // TODO: smart pointer?
	bool m_cameraOwner;
	vtkAbstractTransform * m_transform; // TODO: smart pointer?
	vtkSmartPointer<vtkWorldPointPicker> m_pointPicker;
	QMap<uint, QSharedPointer<iAChannelSlicerData> > m_channels;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarBarWidget;
	vtkSmartPointer<vtkTextProperty> m_textProperty;

	// TODO: extract/ unify with iARenderer
	vtkSmartPointer<vtkLogoWidget> m_logoWidget;
	vtkSmartPointer<vtkLogoRepresentation> m_logoRep;
	vtkSmartPointer<vtkQImageToImageSource> m_logoImage;

	vtkSmartPointer<iAWrapperText> m_textInfo;
	vtkSmartPointer<iARulerWidget> m_rulerWidget;

	//! @{ position marker / statistical extent
	vtkSmartPointer<vtkCubeSource> m_positionMarkerSrc;
	vtkSmartPointer<vtkPolyDataMapper> m_positionMarkerMapper;
	vtkSmartPointer<vtkActor> m_positionMarkerActor;
	//! @}

	iASingleSlicerSettings m_settings;
	int m_slabThickness;       //! current slab thickness (default = 0, i.e. only a single voxel slice); TODO: move to iASingleslicerSettings?
	int m_slabCompositeMode;   //! current slab mode (how to combine the voxels of the current slab into a single pixel); TODO: move to iASingleslicerSettings?

	//! @{ for indicating current measurement ('m' key)
	vtkSmartPointer<vtkLineSource> m_lineSource;
	vtkSmartPointer<vtkPolyDataMapper> m_lineMapper;
	vtkSmartPointer<vtkActor> m_lineActor;
	vtkSmartPointer<vtkDiskSource> m_diskSource;
	vtkSmartPointer<vtkPolyDataMapper> m_diskMapper;
	vtkSmartPointer<vtkActor> m_diskActor;
	//! @}

	vtkSmartPointer<vtkCubeSource> m_roiSource;
	vtkSmartPointer<vtkPolyDataMapper> m_roiMapper;
	vtkSmartPointer<vtkActor> m_roiActor;
	bool m_roiActive;
	int m_roiSlice[2];

	vtkSmartPointer<vtkTransform> m_axisTransform[2];
	vtkSmartPointer<vtkTextActor3D> m_axisTextActor[2];

	int m_ext;
	double m_angle[3];          //!< current rotation angle
	double m_backgroundRGB[3];  //!< manual background RGB
	int m_sliceNumber;          //!< current slice

	double m_slicerPt[3];       //!< point of last interaction in slicer coordinates
	double m_globalPt[4];       //!< point of last interaction in global coordinates
	double m_startMeasurePoint[2];

	QCursor m_mouseCursor;
	bool m_cursorSet;

	MdiChild* m_linkedMdiChild;  //! main window access for linked mdi childs feature - get rid of this somehow!

	QSharedPointer<iAChannelSlicerData> createChannel(uint id, iAChannelData const & chData);
	//! compute the voxel coordinates in the given channel for the current slicer coordinate point.
	//! @param xCoord x coordinate (pixel index) in channel
	//! @param yCoord y coordinate (pixel index) in channel
	//! @param zCoord z coordinate (pixel index) in channel
	void computeCoords(double * coords, uint channelID);
	void updatePositionMarkerExtent();
	void setResliceChannelAxesOrigin(uint id, double x, double y, double z);
	void updatePosition();

	//! Update the position of the raw profile line.
	void updateRawProfile(double posY);

	//! Sets coordinates for line profile
	bool setProfilePointWithClamp(int pointInd, double* Pos, bool doClamp);
	void setLinearInterpolation(bool enabled);
};
