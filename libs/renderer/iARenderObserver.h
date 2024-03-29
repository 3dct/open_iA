// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iArenderer_export.h"

#include <QObject>

#include <vtkCommand.h>
#include <vtkSmartPointer.h>

class vtkLineSource;
class vtkPlane;
class vtkProbeFilter;
class vtkRenderer;
class vtkRenderWindowInteractor;
class vtkTransform;
class vtkWorldPointPicker;

//! Observes the mouse movements in an iARenderer.
//! This class servers the iARenderer class to observe mouse movement and to extract coordinates
//! and the corresponding data "below" the mouse pointer.
class iArenderer_API iARenderObserver : public QObject, public vtkCommand
{
	Q_OBJECT

public:
	iARenderObserver(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren, vtkTransform* pTrans,
		vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3);
	void ReInitialize(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren, vtkTransform* pTrans,
		vtkPlane* plane1, vtkPlane* plane2, vtkPlane* plane3);

	void AddListener(vtkCommand* listener);
	int GetMode();
	vtkRenderWindowInteractor* GetInteractor();
	vtkWorldPointPicker* GetWorldPicker();
	void PickWithWorldPicker();

protected:
	vtkRenderer* m_pRen;
	vtkRenderWindowInteractor* m_pIren;
	vtkTransform* m_pTrans;
	vtkSmartPointer<vtkLineSource> m_pLine;
	vtkSmartPointer<vtkProbeFilter> m_pProbe;
	vtkSmartPointer<vtkWorldPointPicker> m_pWorldPicker;

private:
	enum Axis { X_AXIS = 1, Y_AXIS, Z_AXIS };
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
