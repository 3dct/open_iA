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

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkSmartPointer.h>

#include <QObject>

#include <set>
#include <vector>
#include "iAConsole.h"

class iAChannelVisualizationData;
class iAChannelRenderData;
class iARenderSettings;
class iARenderObserver;

class vtkActor;
class vtkAnnotatedCubeActor;
class vtkAxesActor;
class vtkCamera;
class vtkCellLocator;
class vtkCornerAnnotation;
class vtkCubeSource;
class vtkDataSetMapper;
class vtkImageData;
class vtkInteractorStyleSwitch;
class vtkLineSource;
class vtkLogoRepresentation;
class vtkLogoWidget;
class vtkOpenGLRenderer;
class vtkOrientationMarkerWidget;
class vtkPicker;
class vtkPlane;
class vtkPlaneSource;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkQImageToImageSource;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkSphereSource;
class vtkTextActor;
class vtkTransform;
class vtkUnstructuredGrid;

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

	/*sets transparency of the slicing planes*/
	void setSlicePlaneOpacity(float opc) {
		if ((opc > 1.0) || (opc < 0.0f))
		{
			DEBUG_LOG(QString("Invalid slice plane opacity %1").arg(opc));
			return; 
		}

		m_SlicePlaneOpacity = opc;
	}

	void setAreaPicker();
	void setPointPicker();

	void setupCutter();
	void setupCube();
	void setupAxes(double spacing[3]);
	void setupOrientationMarker();
	void setupRenderer();
	void update();
	void showHelpers(bool show);
	void showRPosition(bool show);
	void showSlicePlanes(bool show);

	vtkPlane* getPlane1();
	vtkPlane* getPlane2();
	vtkPlane* getPlane3();
	void setSlicePlane(int planeID, double originX, double originY, double originZ);
	vtkRenderWindowInteractor* GetInteractor() { return interactor; }
	vtkRenderWindow* GetRenderWindow() { return renWin;  }
	vtkOpenGLRenderer * GetRenderer();
	vtkActor* GetPolyActor();
	vtkTransform* getCoordinateSystemTransform();
	vtkOpenGLRenderer * GetLabelRenderer ();
	vtkPolyDataMapper* GetPolyMapper() const;
	vtkTextActor* GetTxtActor();

	void saveMovie(const QString& fileName, int mode, int qual = 2);	//!< move out of here
	iARenderObserver * getRenderObserver(){ return renderObserver; }
	void AddRenderer(vtkRenderer* renderer);
	void ApplySettings(iARenderSettings & settings);
	
	void emitSelectedCells(vtkUnstructuredGrid* selectedCells);
	void emitNoSelectedCells();
	vtkSmartPointer<vtkDataSetMapper> selectedMapper;
	vtkSmartPointer<vtkActor> selectedActor;
	vtkSmartPointer<vtkUnstructuredGrid> finalSelection;
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

	//! @{ Text actor, e.g., to show the selection mode
	vtkSmartPointer<vtkTextActor> txtActor;
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
	//! @{ Line profile
	vtkSmartPointer<vtkLineSource>     m_profileLineSource;
	vtkSmartPointer<vtkPolyDataMapper> m_profileLineMapper;
	vtkSmartPointer<vtkActor>          m_profileLineActor;
	vtkSmartPointer<vtkSphereSource>   m_profileLineStartPointSource;
	vtkSmartPointer<vtkPolyDataMapper> m_profileLineStartPointMapper;
	vtkSmartPointer<vtkActor>          m_profileLineStartPointActor;
	vtkSmartPointer<vtkSphereSource>   m_profileLineEndPointSource;
	vtkSmartPointer<vtkPolyDataMapper> m_profileLineEndPointMapper;
	vtkSmartPointer<vtkActor>          m_profileLineEndPointActor;
	//! @}

	//! @{ Slice plane
	vtkSmartPointer<vtkPlaneSource>    m_slicePlaneSource[3];
	vtkSmartPointer<vtkPolyDataMapper> m_slicePlaneMapper[3];
	vtkSmartPointer<vtkActor>          m_slicePlaneActor[3];
	//! @}

	float m_SlicePlaneOpacity; //Slice Plane Opacity

public slots:
	void mouseRightButtonReleasedSlot();
	void mouseLeftButtonReleasedSlot();
	void setArbitraryProfile(int pointIndex, double * coords);
	void setArbitraryProfileOn(bool isOn);
Q_SIGNALS:
	void msg(QString s);
	void progress(int);
	void Clicked(int, int, int);
	void cellsSelected(vtkPoints* selCellPoints);
	void noCellsSelected();
	void reInitialized();
	void onSetupRenderer();
	void onSetCamera();
};
