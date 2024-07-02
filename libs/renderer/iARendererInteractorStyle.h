// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkInteractorStyleTrackballCamera.h>

#include <QObject>

//! default interactor style for renderer
class iARendererInteractorStyle : public QObject, public vtkInteractorStyleTrackballCamera
{
	Q_OBJECT
public:
	static iARendererInteractorStyle* New();
	vtkTypeMacro(iARendererInteractorStyle, vtkInteractorStyleTrackballCamera);

	void OnMouseWheelForward() override;
	void OnMouseWheelBackward() override;
signals:
	void ctrlShiftMouseWheel(int);
private:
	//! disable default constructor.
	iARendererInteractorStyle();
	Q_DISABLE_COPY_MOVE(iARendererInteractorStyle);
};
