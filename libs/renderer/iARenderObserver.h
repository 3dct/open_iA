/*************************************  open_iA  ************************************ *
* **********   A tool for visual analysis and processing of 3D CT images   ********** *
* *********************************************************************************** *
* Copyright (C) 2016-2022  C. Heinzl, M. Reiter, A. Reh, W. Li, M. Arikan, Ar. &  Al. *
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

#include "iArenderer_export.h"

#include <QObject>

#include <vtkCommand.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>

class vtkLineSource;
class vtkPicker;
class vtkPlane;
class vtkProbeFilter;
class vtkCellLocator;
class vtkWorldPointPicker;

//! Observes the mouse movements in an iARenderer.
//! This class servers the iARenderer class to observe mouse movement and to extract coordinates
//! and the corresponding data "below" the mouse pointer.
class iArenderer_API iARenderObserver : public QObject, public vtkCommand
{
	Q_OBJECT

	enum Axis { X_AXIS = 1, Y_AXIS, Z_AXIS };

public:
	iARenderObserver(vtkRenderer* pRen, vtkRenderer* pLabelRen, vtkRenderWindowInteractor* pIren, vtkPicker* pPicker,
		vtkTransform* pTrans, vtkImageData* pImageData, vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3,
		vtkCellLocator* cellLocator);
	~iARenderObserver();
	static iARenderObserver* New(vtkRenderer* pRen, vtkRenderer* pLabelRen, vtkRenderWindowInteractor* pIren,
		vtkPicker* pPicker, vtkTransform* pTrans, vtkImageData* pImageData, vtkPlane* plane1, vtkPlane* plane2,
		vtkPlane* plane3, vtkCellLocator* cellLocator);
	void ReInitialize(vtkRenderer* pRen, vtkRenderer* pLabelRen, vtkRenderWindowInteractor* pIren, vtkPicker* pPicker,
		vtkTransform* pTrans, vtkImageData* pImageData, vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3,
		vtkCellLocator* cellLocator);

	void AddListener(vtkCommand* listener);
	int GetMode();
	vtkCellLocator * GetCellLocator();
	vtkRenderWindowInteractor* GetInteractor();
	vtkImageData* GetImageData();
	vtkRenderer* GetLabelRenderer();
	vtkPicker* GetPicker();
	vtkWorldPointPicker* GetWorldPicker();
	void PickWithWorldPicker();

protected:
	vtkRenderer* m_pRen, *m_pLabelRen;
	vtkRenderWindowInteractor* m_pIren;
	vtkPicker* m_pPicker;
	vtkWorldPointPicker *m_pWorldPicker;
	vtkTransform* m_pTrans;
	vtkImageData* m_pImageData;

	vtkLineSource* m_pLine;
	vtkProbeFilter* m_pProbe;
	vtkCellLocator * m_pcellLocator;

private:
	vtkPlane* m_pPlane1;
	vtkPlane* m_pPlane2;
	vtkPlane* m_pPlane3;
	int mode;
	double speed;
	double scale;
	std::vector<vtkCommand*> m_listener;

	void Execute(vtkObject *caller, unsigned long, void*) override;
	void PickVolume(double point[3]);
	void SetAxis(Axis axis, double point[3]);

signals:
	// currently not emitted since we do not need it anywhere, and it would need to be rebuilt to emit world/scene coordinates instead of pixel ones
	//void clicked(int x, int y, int z);
	void keyPressed(int keyCode);
};
