/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2020  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
*                 Amirkhanov, J. Weissenböck, B. Fröhler, M. Schiwarth, P. Weinberger *
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

#include "iAcore_export.h"

#include <vtkSmartPointer.h>

#include <QObject>

class iARenderSettings;
class iARenderObserver;

class vtkActor;
class vtkCamera;
class vtkDataSetMapper;
class vtkImageData;
class vtkOpenGLRenderer;
class vtkPlane;
class vtkPoints;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkRenderer;
class vtkRenderWindow;
class vtkRenderWindowInteractor;
class vtkTextActor;
class vtkTransform;
class vtkUnstructuredGrid;

//! Displays several helper widgets for a 3D vtk rendering window.
class iAcore_API iARenderer: public QObject
{
	Q_OBJECT
public:
	iARenderer(QObject* parent) : QObject(parent) {}
	virtual ~iARenderer() {}
	virtual void setPolyData( vtkPolyData* pd ) = 0;

	virtual void setDefaultInteractor() = 0;
	virtual void setAxesTransform(vtkTransform* transform) = 0;
	virtual void setCamera(vtkCamera* c) = 0;
	virtual vtkCamera* camera() = 0;

	virtual void setAreaPicker() = 0;
	virtual void update() = 0;
	virtual void showHelpers(bool show) = 0;

	virtual vtkPlane* plane1() = 0;
	virtual vtkPlane* plane2() = 0;
	virtual vtkPlane* plane3() = 0;
	virtual vtkRenderWindowInteractor* interactor() = 0;
	virtual vtkRenderWindow* renderWindow() = 0;
	virtual vtkOpenGLRenderer* renderer() = 0;
	virtual vtkTransform* coordinateSystemTransform() = 0;
	virtual vtkOpenGLRenderer * labelRenderer() = 0;
	//! @{ access to polydata rendering
	//! TODO: remove from here! -> separate class similar to iAVolumeRenderer?
	virtual vtkPolyData* polyData() = 0;
	virtual vtkActor* polyActor() = 0;
	virtual vtkPolyDataMapper* polyMapper() const = 0;
	//! @}
	//! @{ check for better way to get access to these in PickCallbackFunction
	virtual vtkActor* selectedActor() = 0;
	virtual vtkUnstructuredGrid* finalSelection() = 0;
	virtual vtkDataSetMapper* selectedMapper() = 0;

	virtual iARenderObserver * getRenderObserver() = 0;
	virtual void addRenderer(vtkRenderer* renderer) = 0;
	//! apply the given settings to the renderer
	//! @param settings data holder for all settings.
	//! @param slicePlaneVisibility initial visibility of the single slice planes (can be modified independently via showSlicePlanes as well).
	virtual void applySettings(iARenderSettings const & settings, bool slicePlaneVisibility[3]) = 0;

signals:
	void cellsSelected(vtkPoints* selCellPoints);
	void noCellsSelected();
	void reInitialized();
	void onSetupRenderer();
	void onSetCamera();
};
