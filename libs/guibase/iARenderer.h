// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"

#include <QObject>

class iARenderSettings;
class iARenderObserver;

class vtkActor;
class vtkCamera;
class vtkOpenGLRenderer;
class vtkPlane;
class vtkPoints;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkTextActor;
class vtkTransform;

//! Class encapsulating a main and a label renderer for displaying 3D objects,
//! and displaying several helper widgets for a 3D vtk rendering window.
class iAguibase_API iARenderer: public QObject
{
	Q_OBJECT
public:
	iARenderer(QObject* parent) : QObject(parent) {}
	virtual ~iARenderer() {}
	
	//! @{ Get/Set the VTK camera object (shared by main and label renderer)
	virtual vtkCamera* camera() = 0;
	virtual void setCamera(vtkCamera* c) = 0;
	//! @}

	//! Update the view (to be called if something has changed in the underlying data
	//! which requires VTK to redraw the scene).
	virtual void update() = 0;

	//! @{ Access to the slice planes
	virtual vtkPlane* plane1() = 0;
	virtual vtkPlane* plane2() = 0;
	virtual vtkPlane* plane3() = 0;
	//! @}

	//! Access to the render window interactor
	virtual vtkRenderWindowInteractor* interactor() = 0;

	//! Access to the render window.
	virtual vtkRenderWindow* renderWindow() = 0;

	//! Access to "main" VTK renderer, used for volumes etc.
	virtual vtkOpenGLRenderer* renderer() = 0;

	//! Access to "label" VTK renderer, used for text (which should be shown in front of volumes).
	virtual vtkOpenGLRenderer* labelRenderer() = 0;

	//! Adds a custom renderer to the render window.
	virtual void addRenderer(vtkRenderer* renderer) = 0;

	//! Apply the given settings to the renderer.
	//! @param settings data holder for all settings.
	//! @param slicePlaneVisibility initial visibility of the single slice planes (can be modified independently via showSlicePlanes as well).
	virtual void applySettings(iARenderSettings const& settings, bool slicePlaneVisibility[3]) = 0;

	//! Set the default interactor style
	virtual void setDefaultInteractor() = 0;

	//! Access to selected actor (when selection is enabled).
	//! Currently only used in DynamicVolumeLines module)
	virtual vtkActor* selectedActor() = 0;

	//! @{ Access to the transform of the coordinate system axis actor.
	virtual void setAxesTransform(vtkTransform* transform) = 0;
	virtual vtkTransform* coordinateSystemTransform() = 0;
	//! @}

	//! Access to the renderer observer.
	virtual iARenderObserver * getRenderObserver() = 0;

signals:
	void cellsSelected(vtkPoints* selCellPoints);
	void noCellsSelected();
	void onSetCamera();
};
