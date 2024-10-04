// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <vtkInteractorStyleFlight.h>
#include <vtkInteractorStyleJoystickCamera.h>
#include <vtkInteractorStyleTrackballCamera.h>

#include <QObject>

// TODO: Remove duplication (what to do with Name?)

//! default interactor style for renderer, based on vtkInteractorStyleTrackballCamera, providing signals on mouse wheel
class iAvtkInteractorStyleTrackballCamera : public QObject, public vtkInteractorStyleTrackballCamera
{
	Q_OBJECT
public:
	static constexpr const char Name[] = "Trackball";

	static iAvtkInteractorStyleTrackballCamera* New();
	vtkTypeMacro(iAvtkInteractorStyleTrackballCamera, vtkInteractorStyleTrackballCamera);

	void OnMouseWheelForward() override;
	void OnMouseWheelBackward() override;
	void OnChar() override;  //!< disable all key interactions from vtkInteractorStyle
signals:
	void ctrlShiftMouseWheel(int);
private:
	//! disable default constructor.
	iAvtkInteractorStyleTrackballCamera();
	Q_DISABLE_COPY_MOVE(iAvtkInteractorStyleTrackballCamera);
};

//! interactor style for renderer, based on vtkInteractorStyleJoystickCamera, providing signals on mouse wheel
class iAvtkInteractorStyleJoystickCamera : public QObject, public vtkInteractorStyleJoystickCamera
{
	Q_OBJECT
public:
	static constexpr const char Name[] = "Joystick";

	static iAvtkInteractorStyleJoystickCamera* New();
	vtkTypeMacro(iAvtkInteractorStyleJoystickCamera, vtkInteractorStyleJoystickCamera);

	void OnMouseWheelForward() override;
	void OnMouseWheelBackward() override;
	void OnChar() override;  //!< disable all key interactions from vtkInteractorStyle
signals:
	void ctrlShiftMouseWheel(int);
private:
	//! disable default constructor.
	iAvtkInteractorStyleJoystickCamera();
	Q_DISABLE_COPY_MOVE(iAvtkInteractorStyleJoystickCamera);
};

//! interactor style for renderer, based on vtkInteractorStyleFlight, providing signals on mouse wheel
class iAvtkInteractorStyleFlight : public QObject, public vtkInteractorStyleFlight
{
	Q_OBJECT
public:
	static constexpr const char Name[] = "Flight";

	static iAvtkInteractorStyleFlight* New();
	vtkTypeMacro(iAvtkInteractorStyleFlight, vtkInteractorStyleFlight);

	void OnMouseWheelForward() override;
	void OnMouseWheelBackward() override;
	void OnChar() override;  //!< disable all key interactions from vtkInteractorStyle
signals:
	void ctrlShiftMouseWheel(int);
private:
	//! disable default constructor.
	iAvtkInteractorStyleFlight();
	Q_DISABLE_COPY_MOVE(iAvtkInteractorStyleFlight);
};
