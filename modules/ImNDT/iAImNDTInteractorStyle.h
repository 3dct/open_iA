// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iAImNDTMain.h"

#include <vtkEventData.h>
#include <vtkOpenVRInteractorStyle.h>
#include <vtkSmartPointer.h>
#include <vtkVersion.h>

#define NUMBER_OF_DEVICES static_cast<int>(vtkEventDataDevice::NumberOfDevices)
#define NUMBER_OF_INPUTS static_cast<int>(vtkEventDataDeviceInput::NumberOfInputs)
#define NUMBER_OF_ACTIONS static_cast<int>(vtkEventDataAction::NumberOfActions)
#define NUMBER_OF_OPTIONS static_cast<int>(iAVRInteractionOptions::NumberOfInteractionOptions)

using inputScheme = std::vector < std::vector < std::vector<std::vector<int>>>>;

// Enumeration for Touchpad positions
enum class iAVRTouchpadPosition {
	Unknown = -1,
	Up,
	Right,
	Down,
	Left
};

// Enumeration for head view directions
enum class iAVRViewDirection {
	Unknown = -1,
	Backwards,
	Right,
	Forward,
	Left,
	Down,
	Up
};

//! Base Class for specific interaction callbacks
class iAImNDTInteractorStyle : public vtkOpenVRInteractorStyle
{
public:
	static iAImNDTInteractorStyle* New();
	vtkTypeMacro(iAImNDTInteractorStyle, vtkOpenVRInteractorStyle);

	void setVRMain(iAImNDTMain* vrMain);

	//! Calls, depending on Device - its input and action, the corresponding method
	//! Events can occur through left/right Controller and its input (trigger, grip, Trackpad,...) and an Action (Press, Release, Touch,...)
	void OnButton3D(vtkEventData* edata) override;
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)

	// The VTK VR interactor comes with its own predefined "actions" and input manifest json files
	// We could define our fully own manifest with all actions; then we would however need to reimplement the vtkOpenVRInteractorStyle / vtkOpenVRRenderWindowInteractor
	// for now, as a workaround, we map the actions from VTK to our own ImNDT actions; for example:
	//     "OnNextPose3D", by default linked to the left trigger (see vtkOpenVRInteractorStyle::SetInteractor: ...AddAction("/actions/vtk/in/NextCameraPose", vtkCommand::NextPose3DEvent...)
	// We just forward that and all other press/touch/release/untouch events to the startInteraction method in ImNDTMain
	// in contrast to the input manifest shipped with VTK, we have added a few more actions to be able to map all input:
	//      - /actions/vtk/in/ShowMenuLeft      for retrieving left menu button
	//      - /actions/vtk/in/SecondButtonRight (currently unused on VIVE as this would be the system button which cannot be caught, always goes to the Steam menu)
	//      - /actions/vtk/in/SecondButtonLeft  (currently unused on VIVE as this would be the system button which cannot be caught, always goes to the Steam menu)
	//      - /actions/vtk/in/TrackPadLeftClick for retrieving left trackpad click
	//      - /actions/vtk/in/TrackPadLeftMove  for retrieving left trackpad move/touch
	//    (see action_manifests/vtk_openvr_actions.json etc.)

	//! for setting up some actions required for VTK >= 9.1.0
	void SetInteractor(vtkRenderWindowInteractor* iren) override;
	//! called on right trigger press (set up in vtkOpenVRInteractorStyle); forwards to OnButton3D>
	void OnSelect3D(vtkEventData* edata) override;
	//! called on left trigger press (set up in vtkOpenVRInteractorStyle); forwards to OnButton3D>
	void OnNextPose3D(vtkEventData* edata) override;
	//! called on right menu button press (set up in vtkOpenVRInteractorStyle); forwards to OnButton3D
	void OnMenu3D(vtkEventData* edata) override;
	void Dolly3D(vtkEventData* edata) override;
	void OnViewerMovement3D(vtkEventData* edata) override;

	//! encapsulate a 2D vector to avoid returning a pointer to const double
	struct iAVec2d {	double c[2];	};
	//! retrieve the position of the last interaction with the trackpad (since it's not available on a click in the event directly)
	iAVec2d getTrackPadPos(vtkEventDataDevice device);
#endif
	void OnMove3D(vtkEventData* edata) override;
	void OnPinch() override;

	inputScheme* getInputScheme();	// returns the vector for the Operation definition
	std::vector<int>* getActiveInput(); //if >0 then has an action applied
	static iAVRTouchpadPosition getTouchedPadSide(float position[3]);
	iAVRViewDirection getViewDirection(double viewDir[3]);

protected:
	iAImNDTInteractorStyle();

private:
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 1, 0)
	iAVec2d m_leftTrackPadPos, m_rightTrackPadPos;
	void updateTrackPadPos(vtkEventData* edata);
#endif

	iAImNDTMain* m_vrMain;
	double m_eventPosition[3];
	double m_eventOrientation[4];
	double m_movePosition[3];
	inputScheme* m_inputScheme;
	std::vector<int>* m_activeInput;
};
