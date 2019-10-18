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
#include "iAVolumeSettings.h"

#include <vtkSmartPointer.h>

#include <QSharedPointer>

class iATransferFunction;

class vtkActor;
class vtkImageData;
class vtkPlane;
class vtkOpenGLRenderer;
class vtkOutlineFilter;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkSmartVolumeMapper;
class vtkVolume;
class vtkVolumeProperty;

//! Collects all vtk classes required for rendering a volume.
//! Provides convenience functionality for adding it to a render window,
//! as well as for showing its bounding box
class open_iA_Core_API iAVolumeRenderer
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

	void setImage(vtkImageData * data); //todo is this necessary??? 
	//just for testing
	vtkRenderer * getCurrentRenderer() {
		return m_currentRenderer; 
	}


	void setMovable(bool movable);

	iAVolumeSettings const & volumeSettings() const;
	bool isRendered() const;
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
