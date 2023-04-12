// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iArenderer_export.h"

#include <iARenderer.h>

#include "iAVec3.h"
#include "iAAABB.h"

#include <vtkSmartPointer.h>

#include <QObject>
#include <QColor>    // for signal support of QColor (parameters to bgColorChanged)

#include <vector>

struct iALineSegment;
class iARenderSettings;
class iARenderObserver;

class vtkActor;
class vtkAnnotatedCubeActor;
class vtkAxesActor;
class vtkCamera;
class vtkCornerAnnotation;
class vtkCubeSource;
class vtkDataSetMapper;
class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkInteractorStyleSwitch;
class vtkLineSource;
class vtkOpenGLRenderer;
class vtkOrientationMarkerWidget;
class vtkPlane;
class vtkPoints;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkSphereSource;
class vtkTextActor;
class vtkTransform;
class vtkUnstructuredGrid;

//! Concrete implementation of iARenderer, displays several helper widgets for a 3D rendering window.
class iArenderer_API iARendererImpl: public iARenderer
{
	Q_OBJECT
public:
	iARendererImpl(QObject *parent, vtkGenericOpenGLRenderWindow* renderWindow);
	virtual ~iARendererImpl( );

	void setDefaultInteractor() override;
	void enableInteractor(bool enable);
	bool isInteractorEnabled() const;
	void setAxesTransform(vtkTransform* transform) override;

	void setPlaneNormals( vtkTransform *tr ) ;
	//! Set the position of the position marker to the given world coordinates
	void setPositionMarkerCenter(double x, double y, double z);
	//! Set size of a single standard "unit" across all shown datasets; used
	//! for the size of the position marker and the width of the slice planes
	//! unit: world coordinates
	void setUnitSize(std::array<double, 3> size);

	//! see iARenderer
	void setSceneBounds(iAAABB const & boundingBox) override;

	//! Sets one of the pre-defined camera positions
	//! @param pos descriptor of the position, @see iACameraPosition
	void setCamPosition(int pos);
	//! Sets viewup, position and focal point of a renderer from the information in a double array.
	//! @param camOptions All informations of the camera stored in a double array
	//! @param rsParallelProjection boolean variable to determine if parallel projection option is on.
	void setCamPosition( double * camOptions, bool rsParallelProjection  );
	//! Returns viewup, position and focal point information of a renderer in a double array.
	//! @param camOptions double array where all informations about the camera will be stored
	void camPosition ( double * camOptions );
	void setCamera(vtkCamera* c) override;
	vtkCamera* camera() override;

	//! sets opacity of the slicing planes
	void setSlicePlaneOpacity(float opc);

	void update() override;
	void showHelpers(bool show);
	void showRPosition(bool show);
	//! show or hide the slice plane for the given axis
	//! @param axis index of the axis (x..0, y..1, z..2)
	//! @param show whether to show (true) or hide (false) the given axis slice plane
	void showSlicePlane(int axis, bool show);

	vtkPlane* plane1() override;
	vtkPlane* plane2() override;
	vtkPlane* plane3() override;
	void setSlicePlanePos(int planeID, double originX, double originY, double originZ);
	vtkRenderWindowInteractor* interactor() override;
	vtkRenderWindow* renderWindow() override;
	vtkRenderer* renderer() override;
	vtkTransform* coordinateSystemTransform() override;
	vtkRenderer* labelRenderer() override;
	vtkTextActor* txtActor();

	//! @{ check for better way to get access to these in PickCallbackFunction
	vtkActor* selectedActor() override;
	vtkUnstructuredGrid* finalSelection();
	vtkDataSetMapper* selectedMapper();
	//! @}

	//sets bounds of the slicing volume, using the spacing of image
	void setSlicingBounds(const int roi[6], const double *spacing);

	//! Visibility of the ROI indicator
	void setROIVisible(bool visible);

	void saveMovie(const QString& fileName, int mode, int qual = 2);	//!< move out of here
	iARenderObserver * getRenderObserver() override;
	void addRenderer(vtkRenderer* renderer) override;
	//! apply the given settings to the renderer
	//! @param settings data holder for all settings.
	//! @param slicePlaneVisibility initial visibility of the single slice planes (can be modified independently via showSlicePlanes as well).
	void applySettings(iARenderSettings const & settings, bool slicePlaneVisibility[3]) override;
	void setBackgroundColors(iARenderSettings const& settings);

	void touchStart();
	void touchScaleSlot(float relScale);

signals:
	void cellsSelected(vtkPoints* selCellPoints);
	void noCellsSelected();
	void reInitialized();
	void onSetupRenderer();
	void onSetCamera();
	void bgColorChanged(QColor bgTop, QColor bgBottom);
	//! called when user presses 'a'/'A' or 'c'/'C' key to change between modifying actors and camera
	void interactionModeChanged(bool camera);

public slots:
	void mouseRightButtonReleasedSlot();
	void mouseLeftButtonReleasedSlot();
	void setProfilePoint(int pointIndex, double const * coords);
	void setProfileHandlesOn(bool isOn);

private:

	bool m_initialized;    //!< flag indicating whether initialization of widget has finished
	iARenderObserver *m_renderObserver;
	vtkSmartPointer<vtkDataSetMapper> m_selectedMapper;
	vtkSmartPointer<vtkActor> m_selectedActor;
	vtkSmartPointer<vtkUnstructuredGrid> m_finalSelection;

	vtkGenericOpenGLRenderWindow* m_renWin;
	vtkRenderWindowInteractor* m_interactor;  //!< convenience store for m_renWin->GetInteractor()
	vtkSmartPointer<vtkOpenGLRenderer> m_ren, m_labelRen;
	vtkSmartPointer<vtkCamera> m_cam;

	//! @{ Text actor, e.g., to show the selection mode
	vtkSmartPointer<vtkTextActor> m_txtActor;
	//! @}

	//! @{ position marker cube
	vtkSmartPointer<vtkCubeSource> m_cSource;
	vtkSmartPointer<vtkPolyDataMapper> m_cMapper;
	vtkSmartPointer<vtkActor> m_cActor;
	//! @}

	//! @{ Axes direction information:
	vtkSmartPointer<vtkAnnotatedCubeActor> m_annotatedCubeActor;
	vtkSmartPointer<vtkAxesActor> m_axesActor;
	vtkSmartPointer<vtkOrientationMarkerWidget> m_orientationMarkerWidget;
	//! @}

	//! cutting planes:
	vtkSmartPointer<vtkPlane> m_plane1, m_plane2, m_plane3;


	//! @{ movable axes
	// TODO: check what the movable axes are useful for!
	vtkTransform* m_moveableAxesTransform;
	vtkSmartPointer<vtkAxesActor> m_moveableAxesActor;
	//! @}

	//! @{ Line profile
	std::vector<iALineSegment>         m_profileLine;
	vtkSmartPointer<vtkSphereSource>   m_profileLineStartPointSource;
	vtkSmartPointer<vtkPolyDataMapper> m_profileLineStartPointMapper;
	vtkSmartPointer<vtkActor>          m_profileLineStartPointActor;
	vtkSmartPointer<vtkSphereSource>   m_profileLineEndPointSource;
	vtkSmartPointer<vtkPolyDataMapper> m_profileLineEndPointMapper;
	vtkSmartPointer<vtkActor>          m_profileLineEndPointActor;
	//! @}

	//! @{ Slice planes
	vtkSmartPointer<vtkCubeSource>     m_slicePlaneSource[3];
	vtkSmartPointer<vtkPolyDataMapper> m_slicePlaneMapper[3];
	vtkSmartPointer<vtkActor>          m_slicePlaneActor[3];
	float m_slicePlaneOpacity; //!< Slice Plane Opacity
	//! @}

	vtkSmartPointer<vtkCubeSource> m_roiCube;
	vtkSmartPointer<vtkPolyDataMapper> m_roiMapper;
	vtkSmartPointer<vtkActor> m_roiActor;

	//! bounding box for "stick-out" information (currently used for lines leading to profile points)
	iAVec3d m_stickOutBox[2];

	double m_touchStartScale;   //! for touch interaction: scale when touch started (if parallel projection used)
	iAVec3d m_touchStartCamPos; //! for touch interaction: camera position (for non-parallel projection)

	std::array<double, 3> m_unitSize; //!< size of a single "item", e.g. the position marker; also thickness of slicing planes
};
