// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iarenderer_export.h"

#include <iARenderer.h>

#include <iAvtkSourcePoly.h>

#include <iAAABB.h>
#include <iAAttributes.h>
#include <iAVec3.h>

#include <vtkCubeSource.h>
#include <vtkLineSource.h>
#include <vtkSmartPointer.h>
#include <vtkSphereSource.h>

#include <QColor>    // for signal support of QColor (parameters to bgColorChanged)
#include <QObject>
#include <QVariantMap>

class iARenderObserver;

class vtkActor;
class vtkAnnotatedCubeActor;
class vtkAxesActor;
class vtkCamera;
class vtkCornerAnnotation;
class vtkDataSetMapper;
class vtkGenericOpenGLRenderWindow;
class vtkImageData;
class vtkInteractorStyleSwitch;
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
	static constexpr const char ShowSlicePlanes[] = "Show Slice Planes";
	static constexpr const char SlicePlaneOpacity[] = "Slice Plane Opacity";
	static constexpr const char ShowAxesCube[] = "Show Axes Cube";
	static constexpr const char ShowOriginIndicator[] = "Show Origin Indicator";
	static constexpr const char ShowPosition[] = "Show Position";
	static constexpr const char ParallelProjection[] = "Parallel Projection";
	static constexpr const char UseStyleBGColor[] = "Use Style Background Color";
	static constexpr const char BackgroundTop[] = "Background Top";
	static constexpr const char BackgroundBottom[] = "Background Bottom";
	static constexpr const char UseFXAA[] = "Use Fast Approximate Anti-aliasing";
	static constexpr const char MultiSamples[] = "MultiSamples";
	static constexpr const char UseSSAO[] = "Use Screen Space Ambient Occlusion";
	static constexpr const char SSAORadius[] = "Screen Space Ambient Occlusion Radius";
	static constexpr const char SSAOBias[] = "Screen Space Ambient Occlusion Bias";
	static constexpr const char SSAOKernelSize[] = "Screen Space Ambient Occlusion Kernel Size";
	static constexpr const char SSAOBlur[] = "Screen Space Ambient Occlusion Blur";
	static constexpr const char StereoRenderMode[] = "Stereo Render Mode";
	static constexpr const char InteractionStyle[] = "Interaction Style";
	static constexpr const char UseDepthPeeling[] = "Use Depth Peeling";
	static constexpr const char DepthPeelOcclusionRatio[] = "Depth Peels Occlusion Ratio";
	static constexpr const char DepthPeelsMax[] = "Depth Peels Maximum Number";
	static constexpr const char MagicLensSize[] = "Magic Lens size";
	static constexpr const char MagicLensFrameWidth[] = "Magic Lens Frame Width";
	static constexpr const char ShowFPS[] = "Show Frames Per Second";

	iARendererImpl(QObject *parent, vtkGenericOpenGLRenderWindow* renderWindow);
	virtual ~iARendererImpl( );

	void setDefaultInteractor() override;
	void enableInteractor(bool enable);
	bool isInteractorEnabled() const;

	void setPlaneNormals(vtkTransform* tr);
	//! Set the position of the position marker to the given world coordinates
	void setPositionMarkerCenter(double x, double y, double z);
	//! Set size of a single standard "unit" across all shown datasets; used
	//! for the size of the position marker and the width of the slice planes
	//! unit: world coordinates
	void setUnitSize(std::array<double, 3> size);

	//! see iARenderer
	void setSceneBounds(iAAABB const & boundingBox) override;

	//! see iARenderer
	iAAABB sceneBounds() const override;

	//! Sets one of the pre-defined camera positions
	//! @param pos descriptor of the position, @see iACameraPosition
	void setCamPosition(int pos);
	void setCamera(vtkCamera* c) override;
	vtkCamera* camera() override;

	void update() override;
	void showOriginIndicator(bool show);
	void showAxesCube(bool show);
	void showRPosition(bool show);

	//! sets opacity of the slicing planes
	void setSlicePlaneOpacity(float opc);
	//! Show or hide the slice plane for the given axis.
	//! Method for setting visibility of a single slice plane (used to make plane visibility depending on whether the respective slicer view is currently shown);
	//! for the slice plane to be visible, also the "Show slice plane" setting has to be enabled.
	//! @param axis index of the axis (x..0, y..1, z..2)
	//! @param show whether to show (true) or hide (false) the given axis slice plane
	void showSlicePlane(int axis, bool show);
	//! Whether showing slice planes is generally enabled or disabled.
	bool isShowSlicePlanes() const;
	//! see iARenderer
	std::array<vtkPlane*, 3> slicePlanes() const override;
	//! update the position of a specific slice plane
	void setSlicePlanePos(int planeID, double originX, double originY, double originZ);
	//! see iARenderer
	void setCuttingActive(bool enabled) override;

	vtkRenderWindowInteractor* interactor() override;
	vtkRenderWindow* renderWindow() override;
	vtkRenderer* renderer() override;
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

	void saveMovie(const QString& fileName, int mode, int qual, int fps, int numSteps);	//!< move out of here
	iARenderObserver * getRenderObserver() override;
	void addRenderer(vtkRenderer* renderer) override;
	//! Apply the given settings to the renderer.
	//! @param paramValues values for all settings.
	void applySettings(QVariantMap const & paramValues) override;
	//! Access to current settings.
	QVariantMap const& settings() const;
	void editSettings();

	void touchStart();
	void touchScaleSlot(float relScale);

	//! initialize profile points (set positions, without triggering an update
	void initProfilePoints(double const* start, double const* end);

	//! proxy access to default settings object
	static iAAttributes& defaultSettings();

signals:
	void cellsSelected(vtkPoints* selCellPoints);
	void noCellsSelected();
	void reInitialized();
	void onSetupRenderer();
	void onSetCamera();
	void bgColorChanged(QColor bgTop, QColor bgBottom);
	//! called when user presses 'a'/'A' or 'c'/'C' key to change between modifying actors and camera
	void interactionModeChanged(bool camera);
	void settingsChanged();

public slots:
	void mouseRightButtonReleasedSlot();
	void mouseLeftButtonReleasedSlot();
	void setProfilePoint(int pointIndex, double const * coords);
	void setProfileHandlesOn(bool isOn);

private:
	void updateBackgroundColors();
	//! Internal method for switching between showing / not showing the slice plane; this method
	//! does not care for the "Show slice plane" setting, and therefore should only be called internally.
	//! @see showSlicePlane for public facing function that changes slice plane visibility respecting the "Show slice plane" setting.
	void showSlicePlaneActor(int axis, bool show);

	//! set profile point data (without triggering updates)
	void setProfilePointInternal(int pointIndex, double const* coords);

	bool m_initialized;    //!< flag indicating whether initialization of widget has finished
	iARenderObserver *m_renderObserver;
	vtkSmartPointer<vtkDataSetMapper> m_selectedMapper;
	vtkSmartPointer<vtkActor> m_selectedActor;
	vtkSmartPointer<vtkUnstructuredGrid> m_finalSelection;

	vtkGenericOpenGLRenderWindow* m_renWin;
	vtkRenderWindowInteractor* m_interactor;    //!< convenience store for m_renWin->GetInteractor()
	vtkSmartPointer<vtkOpenGLRenderer> m_ren, m_labelRen;
	vtkSmartPointer<vtkCamera> m_cam;

	vtkSmartPointer<vtkTextActor> m_txtActor;   //!< Text actor, e.g., to show the selection mode

	iAvtkSourcePoly<vtkCubeSource> m_posMarker; //!< position marker cube

	//! @{ Axes direction information:
	vtkSmartPointer<vtkAnnotatedCubeActor> m_annotatedCubeActor;
	vtkSmartPointer<vtkAxesActor> m_axesActor;
	vtkSmartPointer<vtkOrientationMarkerWidget> m_orientationMarkerWidget;
	//! @}

	//! @{ Line profile
	static const int NumOfProfileLines = 7;
	std::array<iAvtkSourcePoly<vtkLineSource>, NumOfProfileLines> m_profileLines;
	std::array<iAvtkSourcePoly<vtkSphereSource>, 2> m_profileLinePoints;
	//! @}

	//! Slice planes: actual plane data
	vtkSmartPointer<vtkPlane> m_slicePlanes[3];
	//! Slice planes: keep track of which slicing/cutting planes should be visible:
	std::array<bool, 3> m_slicePlaneVisible;
	//! Slice planes: data for geometric objects shown
	std::array<iAvtkSourcePoly<vtkCubeSource>, 3> m_slicePlaneViews;
	//! Slice planes_ Opacity
	float m_slicePlaneOpacity;
	//! Whether currently a dataset cutting is active:
	bool m_cuttingActive;
	std::array<std::array<double, 3>, 3>  m_slicePlaneOrigin;

	vtkSmartPointer<vtkCubeSource> m_roiCube;
	vtkSmartPointer<vtkPolyDataMapper> m_roiMapper;
	vtkSmartPointer<vtkActor> m_roiActor;

	iAVec3d m_stickOutBox[2];   //!< bounding box for "stick-out" information (currently used for lines leading to profile points)
	iAAABB m_sceneBB;           //!< bounding box of all shown visualizations

	double m_touchStartScale;   //! for touch interaction: scale when touch started (if parallel projection used)
	iAVec3d m_touchStartCamPos; //! for touch interaction: camera position (for non-parallel projection)

	std::array<double, 3> m_unitSize; //!< size of a single "item", e.g. the position marker; also thickness of slicing planes

	QVariantMap m_settings;     //!< current settings of this renderer

	unsigned long m_fpsCallback;//!< handle for the callback that implements FPS output
};
