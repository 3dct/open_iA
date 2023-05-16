// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAvtkVR.h"

class iAImNDTInteractionsImpl;
class iAImNDTMain;

class vtkInteractorStyle3D;

using inputScheme = std::vector<std::vector<std::vector<std::vector<int>>>>;

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

	//! Returns a vector for the input scheme (operation definition)
	//! For every [device] an [inputID] and its [action] on an selection [option] a specific interaction is specified
	inputScheme* getInputScheme();

	//! if >0 then has an action applied
	std::vector<int>* getActiveInput();

	//! encapsulate a 2D vector to avoid returning a pointer to const double
	struct iAVec2d {	double c[2];	};

#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)
	//! retrieve the position of the last interaction with the trackpad (since it's not available on a click in the event directly)
	iAVec2d getTrackPadPos(vtkEventDataDevice device);
#endif

	vtkInteractorStyle3D* style();

	static iAVRTouchpadPosition getTouchedPadSide(float position[3]);
	static iAVRViewDirection getViewDirection(double viewDir[3]);

private:
	std::unique_ptr<iAImNDTInteractionsImpl> m_impl;
};
