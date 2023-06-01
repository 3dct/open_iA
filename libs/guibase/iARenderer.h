// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iaguibase_export.h"

#include <QObject>

class iAAABB;
//class iARenderSettings;
class iARenderObserver;

class vtkActor;
class vtkCamera;
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

	//! Access to the slice planes
	virtual std::array<vtkPlane*,3> slicePlanes() const = 0;

	//! Access to the render window interactor
	virtual vtkRenderWindowInteractor* interactor() = 0;

	//! Access to the render window.
	virtual vtkRenderWindow* renderWindow() = 0;

	//! Access to "main" VTK renderer, used for volumes etc.
	virtual vtkRenderer* renderer() = 0;

	//! Access to "label" VTK renderer, used for text (which should be shown in front of volumes).
	virtual vtkRenderer* labelRenderer() = 0;

	//! Adds a custom renderer to the render window.
	virtual void addRenderer(vtkRenderer* renderer) = 0;

	//! Apply the given settings to the renderer.
	//! @param paramValues the values for the settings.
	virtual void applySettings(QVariantMap const & paramValues) = 0;

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

	//! set the bounds of the scene to the given bounding box;
	//! adapts axes markers, plane indicators etc. to be properly visible for a scene 
	//! where all datasets fit within the given bounding box
	virtual void setSceneBounds(iAAABB const & boundingBox) = 0;

	//! Set whether currently a dataset cutting is active.
	//! If enabled, the renderer is updated on changes to the slicing planes (even if the planes themselves are not shown).
	virtual void setCuttingActive(bool enabled) = 0;

signals:
	void cellsSelected(vtkPoints* selCellPoints);
	void noCellsSelected();
	void onSetCamera();
};
