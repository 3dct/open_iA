/*************************************  open_iA  ************************************ *
* **********  A tool for scientific visualisation and 3D image processing  ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2017  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan,            *
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

#include "open_iA_Core_export.h"

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>

#include <QObject>

#include <set>
#include <vector>

class iAChannelVisualizationData;
class iAChannelRenderData;
class iAObserverProgress;
class iARenderSettings;
class iARenderObserver;

class vtkActor;
class vtkAnnotatedCubeActor;
class vtkAxesActor;
class vtkCamera;
class vtkCellLocator;
class vtkColorTransferFunction;
class vtkCornerAnnotation;
class vtkCubeSource;
class vtkImageData;
class vtkInteractorStyleSwitch;
class vtkLogoRepresentation;
class vtkLogoWidget;
class vtkOpenGLRenderer;
class vtkOrientationMarkerWidget;
class vtkPicker;
class vtkPiecewiseFunction;
class vtkPlane;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkQImageToImageSource;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkTransform;


class open_iA_Core_API iARenderer: public QObject
{
	Q_OBJECT
public:
	iARenderer( QObject *parent = 0 );
	virtual ~iARenderer( );

	void initialize( vtkImageData* ds, vtkPolyData* pd, int e = 10 );
	void reInitialize( vtkImageData* ds, vtkPolyData* pd, int e = 10 );
	void setPolyData( vtkPolyData* pd );
	vtkPolyData* getPolyData();

	void disableInteractor();
	void enableInteractor();
	void setAxesTransform(vtkTransform *transform) { axesTransform = transform; }
	vtkTransform * getAxesTransform(void) { return axesTransform; }

	void setPlaneNormals( vtkTransform *tr ) ;
	void setCubeCenter( int x, int y, int z );
	void setCamPosition ( int uvx, int uvy, int uvz, int px, int py, int pz );

	/**
	* \brief	Set viewup, position and focal point of a renderer from the information in a double array.
	*
	* This function is used to assign camera settings from one mdichild to others.
	*
	* \param	camOptions	All informations of the camera stored in a double array
	* \param	rsParallelProjection	boolean variable to determine if parallel projection option is on.
	*/
	void setCamPosition( double * camOptions, bool rsParallelProjection  );
	void setCamera(vtkCamera* c);
	vtkCamera* getCamera();

	/**
	* \brief	Returns viewup, position and focal point information of a renderer in a double array.
	*
	* \param	camOptions	double array where all informations about the camera will be stored
	*/
	void getCamPosition ( double * camOptions );
	void setStatExt( int s ) { ext = s; };

	void setupCutter();
	void setupCube();
	void setupAxes(double spacing[3]);
	void setupOrientationMarker();
	void setupRenderer();
	void update();
	void showHelpers(bool show);
	void showRPosition(bool show);

	vtkPlane* getPlane1();
	vtkPlane* getPlane2();
	vtkPlane* getPlane3();
	vtkRenderWindowInteractor* GetInteractor() { return interactor; }
	vtkRenderWindow* GetRenderWindow() { return renWin;  }
	vtkOpenGLRenderer * GetRenderer();
	vtkActor* GetPolyActor();
	vtkTransform* getCoordinateSystemTransform();
	void GetImageDataBounds(double bounds[6]); //!< remove
	vtkOpenGLRenderer * GetLabelRenderer ();
	vtkPolyDataMapper* GetPolyMapper() const;

	void saveMovie(const QString& fileName, int mode, int qual = 2);	//!< move out of here
	iARenderObserver * getRenderObserver(){ return renderObserver; }
	void AddRenderer(vtkRenderer* renderer);
	void ApplySettings(iARenderSettings & settings);
protected:
	void InitObserver();
	iARenderObserver *renderObserver;
private:
	//! @{ things that are set from the outside
	vtkRenderWindowInteractor* interactor;
	vtkPolyData* polyData;
	// TODO: VOLUME: check if this can be removed:
	vtkImageData* imageData;
	//! @}

	vtkSmartPointer<vtkInteractorStyleSwitch> interactorStyle;
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> renWin;
	vtkSmartPointer<vtkOpenGLRenderer> ren, labelRen;
	vtkSmartPointer<vtkCamera> cam;
	vtkSmartPointer<vtkCellLocator> cellLocator;
	vtkSmartPointer<vtkPolyDataMapper> polyMapper;
	vtkSmartPointer<vtkActor> polyActor;

	//! @{ Logo
	vtkSmartPointer<vtkLogoRepresentation> logoRep;
	vtkSmartPointer<vtkLogoWidget> logoWidget;
	vtkSmartPointer<vtkQImageToImageSource> logoImage;
	//! @}

	//! @{ position marker cube
	vtkSmartPointer<vtkCubeSource> cSource;
	vtkSmartPointer<vtkPolyDataMapper> cMapper;
	vtkSmartPointer<vtkActor> cActor;
	//! @}
	
	vtkSmartPointer<vtkAnnotatedCubeActor> annotatedCubeActor;
	vtkSmartPointer<vtkAxesActor> axesActor;
	vtkSmartPointer<vtkOrientationMarkerWidget> orientationMarkerWidget;
	vtkSmartPointer<vtkPlane> plane1, plane2, plane3;
	vtkSmartPointer<vtkPicker> pointPicker;

	//! @{ movable axes
	// TODO: check what the movable axes are useful for!
	vtkTransform* axesTransform;
	vtkSmartPointer<vtkAxesActor> moveableAxesActor;
	//! @}

	int ext; //!< statistical extent size

public slots:
	void mouseRightButtonReleasedSlot();
	void mouseLeftButtonReleasedSlot();
Q_SIGNALS:
	void msg(QString s);
	void progress(int);
	void Clicked(int, int, int);

	void reInitialized();
	void onSetupRenderer();
	void onSetCamera();
};
