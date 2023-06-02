// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iarenderer_export.h"

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
	iARenderObserver(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren, vtkTransform* pTrans, std::array<vtkPlane*, 3> planes);
	void ReInitialize(vtkRenderer* pRen, vtkRenderWindowInteractor* pIren, vtkTransform* pTrans, std::array<vtkPlane*, 3> planes);

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
	std::array<vtkPlane*, 3> m_planes;
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

//! retrieve the normal vector for the given slicer mode
std::vector<double> slicerNormal(int mode);
