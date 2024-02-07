// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAvtkVR.h"

class iAImNDTInteractionsImpl;
class iAImNDTMain;

class vtkInteractorStyle3D;

//! Enumeration for Touchpad positions
enum class iAVRTouchpadPosition {
	Unknown = -1,
	Middle,
	Up,
	Right,
	Down,
	Left
};

//! Enumeration for head view directions
enum class iAVRViewDirection {
	Unknown = -1,
	Backwards,
	Right,
	Forward,
	Left,
	Down,
	Up
};

class iAImNDTInteractions
{
public:
	iAImNDTInteractions(iAvtkVR::Backend backend, iAImNDTMain* vrMain);
	~iAImNDTInteractions();

	//! encapsulate a 2D vector to avoid returning a pointer to const double
	struct iAVec2d {	double c[2];	};

	//! retrieve the position of the last interaction with the trackpad (since it's not available on a click in the event directly)
	iAVec2d getTrackPadPos(vtkEventDataDevice device);

	vtkInteractorStyle3D* style();

	static iAVRTouchpadPosition getTouchedPadSide(float position[3]);
	static iAVRViewDirection getViewDirection(double viewDir[3]);

private:
	std::unique_ptr<iAImNDTInteractionsImpl> m_impl;
};
