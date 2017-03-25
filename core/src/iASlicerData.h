/*********************************  open_iA 2016 06  ******************************** *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, J. Weissenböck, *
*                     Artem & Alexander Amirkhanov, B. Fröhler                        *
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
#include "iAChannelVisualizationData.h"
#include "open_iA_Core_export.h"
#include "iASlicer.h"
#include "iAWrapperText.h"

#include <vtkRenderWindowInteractor.h>
#include <vtkDiskSource.h>
#include <vtkSmartPointer.h>

#include <QThread>
#include <QLocale>
#include <QDateTime>
#include <QSharedPointer>

#include <string>
#include <QMdiSubWindow>
#include "mainwindow.h"
#include "ui_sliceXY.h"
#include "ui_sliceXZ.h"
#include "ui_sliceYZ.h"

class vtkScalarBarWidget;
class vtkTextProperty;
class vtkColorTransferFunction;
class vtkImageMapToColors;
class vtkLogoWidget;
class vtkLogoRepresentation;
class vtkQImageToImageSource;
class vtkMarchingContourFilter;
class vtkCamera;
class vtkTransform;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkPolyDataMapper;
class vtkPlaneSource;
class vtkPointPicker;
class vtkObject;
class vtkLineSource;
class vtkInteractorStyle;
class vtkImageReslice;
class vtkImageData;
class vtkImageActor;
class vtkGenericOpenGLRenderWindow;
class vtkActor;

class iARulerWidget;
class iASlicerObserver;
class iAMagicLens;

class iAInteractorStyleImage;

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

	void initialize( vtkImageData *ds, vtkTransform *tr, vtkColorTransferFunction* ctf, bool showIsoLines = false, bool showPolygon = false );
	void reInitialize( vtkImageData *ds, vtkTransform *tr, vtkColorTransferFunction* ctf, bool showIsoLines = false, bool showPolygon = false );
	void changeImageData(vtkImageData *idata);
	void setup(iASingleSlicerSettings const & settings);
	
	void initializeChannel(iAChannelID id, iAChannelVisualizationData * chData);
	void reInitializeChannel(iAChannelID id, iAChannelVisualizationData * chData);
	void setChannelOpacity(iAChannelID, double opacity );
	void enableChannel(iAChannelID id, bool enabled, double x, double y, double z);
	void enableChannel( iAChannelID id, bool enabled );
	void setResliceChannelAxesOrigin(iAChannelID id, double x, double y, double z);

	void blend(vtkAlgorithmOutput *data, vtkAlgorithmOutput *data2, double opacity, double * range);

	void setResliceAxesOrigin(double x, double y, double z);
	void setSliceNumber(int sliceNumber);
	void setPlaneCenter(double x, double y, double z );
	void setContours(int n, double mi, double ma);
	void setContours( int n, double * contourValues );
	void setMeasurementStartPoint(int x, int y) { measureStart[0] = x; measureStart[1] = y; };
	void setROI(int r[6]) { roi = r; };
	void setShowText(bool isVisible);

	void disableInteractor() { interactor->Disable(); disabled = true; }
	void enableInteractor() { interactor->ReInitialize(); disabled = false; }
	void showIsolines(bool s);
	void showPosition(bool s);	
	void saveMovie(QString& fileName, int qual = 2);
	void update();
	void updateROI();

	vtkImageReslice *GetReslicer() { return reslicer; }
	vtkRenderWindowInteractor* GetInteractor() { return interactor; };
	vtkGenericOpenGLRenderWindow* GetRenderWindow();
	vtkRenderer* GetRenderer();
	vtkCamera* GetCamera();
	void SetCamera(vtkCamera* camera, bool camOwner=true);
	vtkImageData* GetImageData() const { return imageData; };

	vtkActor *getROIActor() { return roiActor; };
	iASlicerMode getMode() const { return m_mode; }
	void setMode(const iASlicerMode mode);

	/**	Provides the possibility to save an png image of the image viewer native resolution or the current view. */
	void saveAsImage() const;

	/** Provides the possibility to save an image stack of the current view. */
	void saveImageStack();
	void setStatisticalExtent(int statExt);

	virtual void Execute(vtkObject * caller, unsigned long eventId, void * callData);

	void setMagicLensInput(iAChannelID id);
	void setMagicLensCaption(std::string const & caption);
	
	iAChannelSlicerData * GetChannel(iAChannelID id);
	size_t GetEnabledChannels();

	void updateChannelMappers();
	void rotateSlice( double angle );
	void switchContourSourceToChannel( iAChannelID id );

	void SetManualBackground(double r, double g, double b);
	//void UnsetManualBackground(); // not used yet

	vtkScalarBarWidget * GetScalarWidget();
	vtkImageActor* GetImageActor();

protected:

	void UpdateResliceAxesDirectionCosines();
	void UpdateBackground();
	//mouse move
	void printVoxelInformation(int xCoord, int yCoord, int zCoord, double* result);
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
	void snap(double &x, double &y);
	void InitReslicerWithImageData();
	void UpdateReslicer();	
	
Q_SIGNALS:
	void msg(QString s);
	void progress(int);
	void updateSignal();
	void clicked();
	void clicked(int x, int y, int z);
	void UserInteraction();
	//mouse move
	void oslicerPos(int x, int y, int z, int mode);
	void oslicerCol(double cl, double cw, int mode);
	//key press
	void Pick();

private:
	iAMagicLens * m_magicLensExternal;
	
	iASlicerObserver * observerMouseMove;
	vtkRenderWindowInteractor* interactor;
	iAInteractorStyleImage* interactorStyle;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> renWin;
	vtkSmartPointer<vtkRenderer> ren;
	vtkCamera* m_camera; // smart pointer?
	bool m_cameraOwner;

	vtkImageReslice* reslicer;
	vtkImageData* imageData;
	vtkColorTransferFunction* colorTransferFunction;
	vtkImageMapToColors* colormapper;
	vtkImageActor* imageActor;
	vtkPointPicker* pointPicker;

	QMap<iAChannelID, QSharedPointer<iAChannelSlicerData> > m_channels;

	vtkScalarBarWidget *scalarWidget;
	vtkTextProperty *textProperty;

	// TODO: extract/ unify with iARenderer
	vtkLogoWidget *logoWidget;
	vtkLogoRepresentation *logoRep;
	vtkQImageToImageSource *logoImage;

	iAWrapperText* textInfo;

	//statistical extent plane (green rectangle)
	vtkPlaneSource *m_planeSrc;
	vtkPolyDataMapper *m_planeMapper;
	vtkActor *m_planeActor;

	vtkMarchingContourFilter *cFilter;
	vtkPolyDataMapper *cMapper;
	vtkActor *cActor;

	vtkLineSource *pLineSource;
	vtkPolyDataMapper *pLineMapper;
	vtkActor *pLineActor;

	vtkDiskSource *pDiskSource;
	vtkPolyDataMapper *pDiskMapper;
	vtkActor *pDiskActor;

	vtkPlaneSource *roiSource;
	vtkPolyDataMapper *roiMapper;
	vtkActor *roiActor;

	iARulerWidget *rulerWidget;
	//iARulerActor * scaleActor;

	vtkTransform *transform;
	bool isolines;
	bool poly;

	bool m_decorations;
	iASlicerMode m_mode;
	int m_ext;
	bool disabled;
	int no;
	double min, max;
	
	int *roi;
	int measureStart[2];
	double angleX, angleY, angleZ;

	bool m_userSetBackground;
	double rgb[3];

	//mouse move
	double m_ptMapped[3];
	double m_startMeasurePoint[2];

	iAChannelSlicerData & GetOrCreateChannel(iAChannelID id);
	void GetMouseCoord(int & xCoord, int & yCoord, int & zCoord, double* result);
};
