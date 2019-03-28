/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2019  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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
#include "iASlicerSettings.h"    // for iASingleSlicerSettings
#include "iASlicerMode.h"

#include <vtkSmartPointer.h>

#include <QCursor>
#include <QMap>
#include <QSharedPointer>

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
class vtkPlaneSource;
class vtkPointPicker;
class vtkQImageToImageSource;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkScalarBarWidget;
class vtkTextProperty;
class vtkTextActor3D;
class vtkTransform;

class iAChannelSlicerData;
class iAChannelData;
class iAInteractorStyleImage;
class iAMagicLens;
class iARulerWidget;
class iASingleSlicerSettings;
class iASlicer;
class iAWrapperText;

/**
 * \brief	implements a slicer widget
 *
 * This class implements a slicer widget to evaluate 3D datasets.
 */
class open_iA_Core_API iASlicerData :  public QObject
{
	Q_OBJECT
public:
	iASlicerData( iASlicer * slicerMaster, QObject * parent = 0, bool decorations=true);
	virtual ~iASlicerData();

	void initialize(vtkAbstractTransform * tr);
	void setTransform(vtkAbstractTransform * tr);

	void setDefaultInteractor();

	void setup(iASingleSlicerSettings const & settings);

	void addChannel(uint id, iAChannelData* chData);
	void removeChannel(uint id);
	void updateChannel(uint id, iAChannelData * chData);
	void setChannelOpacity(uint id, double opacity );
	void enableChannel(uint id, bool enabled, double x, double y, double z);
	void enableChannel(uint id, bool enabled );
	void setResliceChannelAxesOrigin(uint id, double x, double y, double z);

	void addImageActor(vtkSmartPointer<vtkImageActor> imgActor);
	void removeImageActor(vtkSmartPointer<vtkImageActor> imgActor);

	//void blend(vtkAlgorithmOutput *data, vtkAlgorithmOutput *data2, double opacity, double * range);

	void setResliceAxesOrigin(double x, double y, double z);
	void setSliceNumber(int sliceNumber);
	int getSliceNumber() const;
	//! set the position of the position marker (in slicer coordinates)
	void setPositionMarkerCenter(double x, double y);
	void setContours(int numberOfContours, double contourMin, double contourMax);
	void setContours(int numberOfContours, double * contourValues );
	//void setMeasurementStartPoint(int x, int y);
	void setShowText(bool isVisible);
	void setMouseCursor( QString s );

	void disableInteractor();
	void enableInteractor();
	void showIsolines(bool s);
	void showPosition(bool s);
	void saveMovie(QString& fileName, int qual = 2);
	void update();

	void updateROI(int const r[6]);
	void setROIVisible(bool visible);

	vtkRenderWindowInteractor * getInteractor();
	vtkGenericOpenGLRenderWindow * getRenderWindow();
	vtkRenderer * getRenderer();
	vtkCamera * getCamera();
	void setCamera(vtkCamera * camera, bool camOwner=true);
	void resetCamera();

	iASlicerMode getMode() const { return m_mode; }
	void setMode(const iASlicerMode mode);

	//! Provides the possibility to save an png image of the image viewer native resolution or the current view.
	void saveAsImage() const;
	//! Provides the possibility to save an image stack of the current view.
	void saveImageStack();
	void setStatisticalExtent(int statExt);

	void execute(vtkObject * caller, unsigned long eventId, void * callData);

	void setMagicLensInput(uint id);
	iAChannelSlicerData * getChannel(uint id);
	size_t GetEnabledChannels();

	void updateChannelMappers();
	void rotateSlice( double angle );
	//void switchContourSourceToChannel( uint id );

	void setManualBackground(double r, double g, double b);

	vtkScalarBarWidget * getScalarBarWidget();
	
	QCursor getMouseCursor();
	
	void setRightButtonDragZoomEnabled(bool enabled);
	void setSlabThickness(int thickness);
	void setSlabCompositeMode(int compositeMode);
protected:
	void updateResliceAxesDirectionCosines();
	void updateBackground();
	void printVoxelInformation(double xCoord, double yCoord, double zCoord);
	void executeKeyPressEvent();
	void defaultOutput();
	void updateReslicer();

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
Q_SIGNALS:
	void msg(QString s);
	void progress(int);
	void updateSignal();
	void clicked();
	void clicked(int x, int y, int z);
	void rightClicked(int x, int y, int z);
	void released(int x, int y, int z);
	void UserInteraction();
	//mouse move
	void oslicerPos(int x, int y, int z, int mode);
	void oslicerCol(double cl, double cw, int mode);
	//key press
	void Pick();
private:
	iAMagicLens * m_magicLensExternal;
	vtkRenderWindowInteractor * m_interactor;
	iAInteractorStyleImage * m_interactorStyle;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renWin;
	vtkSmartPointer<vtkRenderer> m_ren;
	vtkCamera * m_camera; // TODO: smart pointer?
	bool m_cameraOwner;
	vtkAbstractTransform * m_transform; // TODO: smart pointer?
	vtkPointPicker * m_pointPicker;
	QMap<uint, QSharedPointer<iAChannelSlicerData> > m_channels;
	vtkSmartPointer<vtkScalarBarWidget> m_scalarBarWidget;
	vtkSmartPointer<vtkTextProperty> m_textProperty;

	// TODO: extract/ unify with iARenderer
	vtkSmartPointer<vtkLogoWidget> m_logoWidget;
	vtkSmartPointer<vtkLogoRepresentation> m_logoRep;
	vtkSmartPointer<vtkQImageToImageSource> m_logoImage;

	vtkSmartPointer<iAWrapperText> m_textInfo;
	vtkSmartPointer<iARulerWidget> m_rulerWidget;

	// position marker / statistical extent
	vtkSmartPointer<vtkPlaneSource> m_positionMarkerSrc;
	vtkSmartPointer<vtkPolyDataMapper> m_positionMarkerMapper;
	vtkSmartPointer<vtkActor> m_positionMarkerActor;
	bool m_showPositionMarker;

	iASingleSlicerSettings m_settings;
	int m_slabThickness;       //! current slab thickness (default = 1, i.e. only a single voxel slice); TODO: move to iASingleslicerSettings?
	int m_slabCompositeMode;   //! current slab mode (how to combine the voxels of the current slab into a single pixel); TODO: move to iASingleslicerSettings?

	vtkSmartPointer<vtkLineSource> pLineSource;
	vtkSmartPointer<vtkPolyDataMapper> pLineMapper;
	vtkSmartPointer<vtkActor> pLineActor;
	vtkSmartPointer<vtkDiskSource> pDiskSource;
	vtkSmartPointer<vtkPolyDataMapper> pDiskMapper;
	vtkSmartPointer<vtkActor> pDiskActor;

	vtkSmartPointer<vtkPlaneSource> m_roiSource;
	vtkSmartPointer<vtkPolyDataMapper> m_roiMapper;
	vtkSmartPointer<vtkActor> m_roiActor;
	bool m_roiActive;
	int m_roiSlice[2];

	vtkSmartPointer<vtkTransform> m_axisTransform[2];
	vtkSmartPointer<vtkTextActor3D> m_axisTextActor[2];

	bool m_isolines;
	bool m_poly;

	bool m_decorations;
	iASlicerMode m_mode;
	int m_ext;
	bool m_disabled;

	double m_angleX, m_angleY, m_angleZ;

	bool m_userSetBackground;
	double m_backgroundRGB[3];

	int m_sliceNumber; // for fisheye transformation

	//mouse move
	double m_ptMapped[3];
	double m_startMeasurePoint[2];

	QCursor m_mouseCursor;

	QSharedPointer<iAChannelSlicerData> createChannel(uint id);
	void getMouseCoord(double & xCoord, double & yCoord, double & zCoord, double* result);
	void updatePositionMarkerExtent();

	void setupColorMapper();
};
