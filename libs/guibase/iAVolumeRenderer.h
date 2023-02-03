// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAguibase_export.h"
#include "iAVolumeSettings.h"

#include <vtkActor.h>
#include <vtkOutlineFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

class iATransferFunction;

class vtkImageData;
class vtkPlane;
class vtkOpenGLRenderer;
class vtkRenderer;

//! Collects all vtk classes required for rendering a volume.
//! Provides convenience functionality for adding it to a render window,
//! as well as for showing its bounding box
//! TODO: merge with iAVolRenderer
class iAguibase_API iAVolumeRenderer
{
public:
	iAVolumeRenderer(
		iATransferFunction * transfer,
		vtkSmartPointer<vtkImageData> imgData);
	void applySettings(iAVolumeSettings const & rs);
	double const * orientation() const;
	double const * position() const;
	void setPosition(double *);
	void setOrientation(double *);
	void addTo(vtkRenderer* w);
	void remove();
	vtkSmartPointer<vtkVolume> volume();
	vtkRenderer* currentRenderer();
	void update();
	void showVolume(bool visible);

	void setCuttingPlanes(vtkPlane* p1, vtkPlane* p2, vtkPlane* p3);
	void removeCuttingPlanes();

	void addBoundingBoxTo(vtkRenderer* w);
	void removeBoundingBox();
	void updateBoundingBox();
	void showBoundingBox(bool visible);

	void setImage(iATransferFunction * transfer, vtkSmartPointer<vtkImageData> imgData);

	vtkRenderer * getCurrentRenderer()
	{
		return m_currentRenderer;
	}

	void setMovable(bool movable);

	iAVolumeSettings const & volumeSettings() const;
	bool isRendered() const;
	bool isVisible() const;

private:
	iAVolumeSettings m_volSettings;
	vtkSmartPointer<vtkVolume> m_volume;
	vtkSmartPointer<vtkVolumeProperty> m_volProp;
	vtkSmartPointer<vtkSmartVolumeMapper> m_volMapper;
	vtkRenderer* m_currentRenderer;

	//! @{ Bounding Box
	vtkSmartPointer<vtkOutlineFilter> m_outlineFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_outlineMapper;
	vtkSmartPointer<vtkActor> m_outlineActor;
	vtkRenderer* m_currentBoundingBoxRenderer;
	//! @}

	bool m_isFlat;
};
