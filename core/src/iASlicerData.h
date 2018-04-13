/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2018  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
*                          J. Weissenböck, Artem & Alexander Amirkhanov, B. Fröhler   *
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

#include "iAChannelID.h"
#include "open_iA_Core_export.h"
#include "iASlicerMode.h"

#include <vtkSmartPointer.h>

#include <QCursor>
#include <QMap>
#include <QSharedPointer>

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
class iAChannelVisualizationData;
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
	iASlicerData( iASlicer const * slicerMaster, QObject * parent = 0, bool decorations=true);
	virtual ~iASlicerData();

	void initialize( vtkImageData *ds, vtkTransform *tr, vtkScalarsToColors* ctf);
	void reInitialize( vtkImageData *ds, vtkTransform *tr, vtkScalarsToColors* ctf, bool showisolines = false, bool showpolygon = false);
	void changeImageData(vtkImageData *idata);
	void setup(iASingleSlicerSettings const & settings);
	
	void initializeChannel(iAChannelID id, iAChannelVisualizationData * chData);
	void removeChannel(iAChannelID id);
	void reInitializeChannel(iAChannelID id, iAChannelVisualizationData * chData);
	void setChannelOpacity(iAChannelID, double opacity );
	void enableChannel(iAChannelID id, bool enabled, double x, double y, double z);
	void enableChannel( iAChannelID id, bool enabled );
	void setResliceChannelAxesOrigin(iAChannelID id, double x, double y, double z);

	void AddImageActor(vtkSmartPointer<vtkImageActor> imgActor);
	void RemoveImageActor(vtkSmartPointer<vtkImageActor> imgActor);

	void blend(vtkAlgorithmOutput *data, vtkAlgorithmOutput *data2, double opacity, double * range);

	void setResliceAxesOrigin(double x, double y, double z);
	void setSliceNumber(int sliceNumber);
	int getSliceNumber() const;
	//! set the position of the position marker (in slicer coordinates)
	void setPositionMarkerCenter(double x, double y);
	void setContours(int n, double mi, double ma);
	void setContours( int n, double * contourValues );
	void setMeasurementStartPoint(int x, int y) { measureStart[0] = x; measureStart[1] = y; };
	void setShowText(bool isVisible);
	void setMouseCursor( QString s );

	void disableInteractor();
	void enableInteractor();
	void showIsolines(bool s);
	void showPosition(bool s);	
	void saveMovie(QString& fileName, int qual = 2);
	void update();

	void UpdateROI(int const r[6]);
	void SetROIVisible(bool visible);

	vtkImageReslice *GetReslicer() { return reslicer; }
	vtkRenderWindowInteractor* GetInteractor() { return interactor; };
	vtkGenericOpenGLRenderWindow* GetRenderWindow();
	vtkRenderer* GetRenderer();
	vtkCamera* GetCamera();
	void SetCamera(vtkCamera* camera, bool camOwner=true);
	void ResetCamera();
	vtkImageData* GetImageData() const { return imageData; };

	iASlicerMode getMode() const { return m_mode; }
	void setMode(const iASlicerMode mode);

	/**	Provides the possibility to save an png image of the image viewer native resolution or the current view. */
	void saveAsImage() const;
	/** Provides the possibility to save an image stack of the current view. */
	void saveImageStack();
	void setStatisticalExtent(int statExt);

	virtual void Execute(vtkObject * caller, unsigned long eventId, void * callData);

	void setMagicLensInput(iAChannelID id);
	iAChannelSlicerData * GetChannel(iAChannelID id);
	size_t GetEnabledChannels();

	void updateChannelMappers();
	void rotateSlice( double angle );
	void switchContourSourceToChannel( iAChannelID id );

	void SetManualBackground(double r, double g, double b);

	vtkScalarBarWidget * GetScalarWidget();
	vtkImageActor* GetImageActor();
	QCursor getMouseCursor();

	vtkScalarsToColors * GetColorTransferFunction();

	int getSliceNumber(); // for fisheye transformation

protected:
	void UpdateResliceAxesDirectionCosines();
	void UpdateBackground();
	//mouse move
	void printVoxelInformation(double xCoord, double yCoord, double zCoord);
	void executeKeyPressEvent();
	void defaultOutput();

	/**
	* \brief	This function is used to check whether any agreeable maximum gradient is near the given point.
	*	The ROI is 2 voxels on all four direction. if yes move to the closest maximum gradient.
	*
	* \detail	Input is the cursor point. Calculate the gradient magnitude for varying "x" value with "y" constant.
	*	The maximum gradient value is taken as H_maxCoord. If there are two same gradient magnitude values, the point
	*	closer to the cursor point is taken as H_maxCoord. Apply the above procedure for constant "x" and varying "y"
	*	to calculate V_maxCoord. Check whether H_maxCoord gradient magnitude and V_maxCoord gradient magnitude are
	*	higher than an gradient threshold value (5% of max intensity in the image). If H_maxCoord gradient magnitude
	*	is higher and V_maxCoord gradient magnitude is lesser than the threshold, take H_maxCoord as the closet
	*	gradient to cursor point. And if it is vice versa take V_maxCoord take as closest gradient to the cursor point.
	*	If point are higher than threshold, the point closest to the cursor point is take as the next point.
	*
	* \param [in,out]	x	The x coordinate.
	* \param [in,out]	y	The y coordinate.
	*/
	void snapToHighGradient(double &x, double &y);
	void InitReslicerWithImageData();
	void UpdateReslicer();
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
	vtkRenderWindowInteractor* interactor;
	iAInteractorStyleImage* interactorStyle;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> renWin;
	vtkSmartPointer<vtkRenderer> ren;
	vtkCamera* m_camera; // smart pointer?
	bool m_cameraOwner;

	vtkImageReslice* reslicer;
	vtkImageData* imageData;
	vtkScalarsToColors* colorTransferFunction;
	vtkImageMapToColors* colormapper;
	vtkImageActor* imageActor;
	vtkPointPicker* pointPicker;

	QMap<iAChannelID, QSharedPointer<iAChannelSlicerData> > m_channels;

	vtkScalarBarWidget *scalarWidget;
	vtkTextProperty *textProperty;

	// TODO: extract/ unify with iARenderer
	vtkSmartPointer<vtkLogoWidget> logoWidget;
	vtkSmartPointer<vtkLogoRepresentation> logoRep;
	vtkSmartPointer<vtkQImageToImageSource> logoImage;

	iAWrapperText* textInfo;

	// position marker / statistical extent
	vtkSmartPointer<vtkPlaneSource> m_positionMarkerSrc;
	vtkSmartPointer<vtkPolyDataMapper> m_positionMarkerMapper;
	vtkSmartPointer<vtkActor> m_positionMarkerActor;
	bool m_showPositionMarker;

	vtkMarchingContourFilter *cFilter;
	vtkPolyDataMapper *cMapper;
	vtkActor *cActor;

	vtkSmartPointer<vtkLineSource> pLineSource;
	vtkSmartPointer<vtkPolyDataMapper> pLineMapper;
	vtkSmartPointer<vtkActor> pLineActor;
	vtkSmartPointer<vtkDiskSource> pDiskSource;
	vtkSmartPointer<vtkPolyDataMapper> pDiskMapper;
	vtkSmartPointer<vtkActor> pDiskActor;

	vtkSmartPointer<vtkPlaneSource> roiSource;
	vtkSmartPointer<vtkPolyDataMapper> roiMapper;
	vtkSmartPointer<vtkActor> roiActor;
	bool roiActive;
	int roiSlice[2];

	vtkSmartPointer<vtkTransform> axisTransform[2];
	vtkSmartPointer<vtkTextActor3D> axisTextActor[2];

	iARulerWidget *rulerWidget;

	vtkTransform *transform;
	bool isolines;
	bool poly;

	bool m_decorations;
	iASlicerMode m_mode;
	int m_ext;
	bool disabled;
	int no;
	double contourMin, contourMax;
	
	int measureStart[2];
	double angleX, angleY, angleZ;

	bool m_userSetBackground;
	double rgb[3];


	int m_sliceNumber; // for fisheye transformation

	//mouse move
	double m_ptMapped[3];
	double m_startMeasurePoint[2];

	iAChannelSlicerData & GetOrCreateChannel(iAChannelID id);
	void GetMouseCoord(double & xCoord, double & yCoord, double & zCoord, double* result);
	void UpdatePositionMarkerExtent();

	QCursor m_mouseCursor;
};
