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

	void setDefaultInteractor();
	void disableInteractor();
	void enableInteractor();
	void setAxesTransform(vtkTransform *transform);
	vtkTransform * axesTransform(void);

	void setPlaneNormals( vtkTransform *tr ) ;
	void setCubeCenter( int x, int y, int z );
	void setCamPosition ( int uvx, int uvy, int uvz, int px, int py, int pz );

	//! Set viewup, position and focal point of a renderer from the information in a double array.
	//! @param camOptions All informations of the camera stored in a double array
	//! @param rsParallelProjection boolean variable to determine if parallel projection option is on.
	void setCamPosition( double * camOptions, bool rsParallelProjection  );
	//! Returns viewup, position and focal point information of a renderer in a double array.
	//! @param camOptions double array where all informations about the camera will be stored
	void camPosition ( double * camOptions );
	void setCamera(vtkCamera* c);
	vtkCamera* camera();

	//! set size of statistical extent
	void setStatExt(int s);

	//! sets opacity of the slicing planes
	void setSlicePlaneOpacity(float opc);

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

	vtkPlane* plane1();
	vtkPlane* plane2();
	vtkPlane* plane3();
	void setSlicePlane(int planeID, double originX, double originY, double originZ);
	vtkRenderWindowInteractor* interactor();
	vtkRenderWindow* renderWindow();
	vtkOpenGLRenderer * renderer();
	vtkTransform* coordinateSystemTransform();
	vtkOpenGLRenderer * labelRenderer ();
	vtkTextActor* txtActor();
	//! @{ access to polydata rendering
	//! TODO: remove from here! -> separate class similar to iAVolumeRenderer?
	vtkPolyData* polyData();
	vtkActor* polyActor();
	vtkPolyDataMapper* polyMapper() const;
	//! @}
	//! @{ check for better way to get access to these in PickCallbackFunction
	vtkActor* selectedActor();
	vtkUnstructuredGrid* finalSelection();
	vtkDataSetMapper* selectedMapper();

	//sets bounds of the slicing volume, using the spacing of image
	void setSlicingBounds(const int roi[6], const double *spacing);
	
	void setCubeVisible(bool visible); //Visibility of the slice cube
	
	void saveMovie(const QString& fileName, int mode, int qual = 2);	//!< move out of here
	iARenderObserver * getRenderObserver();
	void addRenderer(vtkRenderer* renderer);
	void applySettings(iARenderSettings const & settings);
	
	void emitSelectedCells(vtkUnstructuredGrid* selectedCells);
	void emitNoSelectedCells();

private:
	void initObserver();

	iARenderObserver *m_renderObserver;
	//! @{ things that are set from the outside
	vtkRenderWindowInteractor* m_interactor;
	vtkPolyData* m_polyData;
	// TODO: VOLUME: check if this can be removed:
	vtkImageData* m_imageData;
	//! @}
	vtkSmartPointer<vtkDataSetMapper> m_selectedMapper;
	vtkSmartPointer<vtkActor> m_selectedActor;
	vtkSmartPointer<vtkUnstructuredGrid> m_finalSelection;

	vtkSmartPointer<vtkGenericOpenGLRenderWindow> m_renWin;
	vtkSmartPointer<vtkOpenGLRenderer> m_ren, m_labelRen;
	vtkSmartPointer<vtkCamera> m_cam;
	vtkSmartPointer<vtkCellLocator> m_cellLocator;
	vtkSmartPointer<vtkPolyDataMapper> m_polyMapper;
	vtkSmartPointer<vtkActor> m_polyActor;

	//! @{ Logo
	vtkSmartPointer<vtkLogoRepresentation> m_logoRep;
	vtkSmartPointer<vtkLogoWidget> m_logoWidget;
	vtkSmartPointer<vtkQImageToImageSource> m_logoImage;
	//! @}

	//! @{ Text actor, e.g., to show the selection mode
	vtkSmartPointer<vtkTextActor> m_txtActor;
	//! @}

	//! @{ position marker cube
	vtkSmartPointer<vtkCubeSource> m_cSource;
	vtkSmartPointer<vtkPolyDataMapper> m_cMapper;
	vtkSmartPointer<vtkActor> m_cActor;
	//! @}

	vtkSmartPointer<vtkAnnotatedCubeActor> m_annotatedCubeActor;
	vtkSmartPointer<vtkAxesActor> m_axesActor;
	vtkSmartPointer<vtkOrientationMarkerWidget> m_orientationMarkerWidget;
	vtkSmartPointer<vtkPlane> m_plane1, m_plane2, m_plane3;
	vtkSmartPointer<vtkPicker> m_pointPicker;

	//! @{ movable axes
	// TODO: check what the movable axes are useful for!
	vtkTransform* m_moveableAxesTransform;
	vtkSmartPointer<vtkAxesActor> m_moveableAxesActor;
	//! @}
	
	int m_ext; //!< statistical extent size
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

	vtkSmartPointer<vtkCubeSource> m_slicingCube;
	vtkSmartPointer<vtkPolyDataMapper> m_sliceCubeMapper;
	vtkSmartPointer<vtkActor> m_sliceCubeActor;

	float m_slicePlaneOpacity; //Slice Plane Opacity

public slots:
	void mouseRightButtonReleasedSlot();
	void mouseLeftButtonReleasedSlot();
	void setArbitraryProfile(int pointIndex, double * coords);
	void setArbitraryProfileOn(bool isOn);

Q_SIGNALS:
	void msg(QString s);
	void progress(int);
	void cellsSelected(vtkPoints* selCellPoints);
	void noCellsSelected();
	void reInitialized();
	void onSetupRenderer();
	void onSetCamera();
};
