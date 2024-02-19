// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iAImNDTInteractorStyle.h"

#include "iAImNDTMain.h"

#include <iALog.h>

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkInteractorStyle3D.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkObjectFactory.h>
#include <vtkProperty.h>

#include <vtkEventData.h>
#include <vtkSmartPointer.h>

vtkSmartPointer<vtkInteractorStyle3D> createInteractorStyle(iAvtkVR::Backend backend, iAImNDTInteractionsImpl* impl);

//! common interaction code for OpenVR and OpenXR:
class iAImNDTInteractionsImpl
{
public:
	iAImNDTInteractionsImpl(iAvtkVR::Backend backend, iAImNDTMain* vrMain):
		m_style(createInteractorStyle(backend, this)),
		m_backend(backend),
		m_vrMain(vrMain)
	{}
	vtkSmartPointer<vtkInteractorStyle3D> m_style;
	iAImNDTInteractions::iAVec2d m_leftTrackPadPos, m_rightTrackPadPos;
	iAvtkVR::Backend m_backend;
	iAImNDTMain* m_vrMain;

	double m_eventPosition[3];
	double m_eventOrientation[4];
	double m_movePosition[3];

	iAImNDTInteractions::iAVec2d getTrackPadPos(vtkEventDataDevice device) const
	{
		return device == vtkEventDataDevice::LeftController ? m_leftTrackPadPos : m_rightTrackPadPos;
	}

	void OnPinch()
	{
		double dyf = m_style->GetInteractor()->GetScale() / m_style->GetInteractor()->GetLastScale();
		vtkCamera* camera = m_style->GetCurrentRenderer()->GetActiveCamera();
		vtkRenderWindowInteractor3D* rwi = static_cast<vtkRenderWindowInteractor3D*>(m_style->GetInteractor());
		double physicalScale = rwi->GetPhysicalScale();
		m_style->SetScale(camera, physicalScale / dyf);
		m_vrMain->onZoom();
	}

	//! Is called when a Controller moves. Forwards the event to the main class
	void OnMove3D(vtkEventData* edata)
	{
		vtkEventDataDevice3D* device = edata->GetAsEventDataDevice3D();
		assert(device);

		int x = m_style->GetInteractor()->GetEventPosition()[0];
		int y = m_style->GetInteractor()->GetEventPosition()[1];
		m_style->FindPokedRenderer(x, y);

		device->GetWorldPosition(m_movePosition);
		device->GetWorldOrientation(m_eventOrientation);

		/* //Orientation?
		vtkRenderer* ren = this->CurrentRenderer;
		vtkOpenXRRenderWindow* renWin =
			vtkOpenXRRenderWindow::SafeDownCast(this->Interactor->GetRenderWindow());
		vtkOpenXRRenderWindowInteractor* iren =
			static_cast<vtkOpenXRRenderWindowInteractor*>(this->Interactor);

		vtkOpenXRModel* cmodel = renWin->GetTrackedDeviceModel(device->GetDevice());

		renWin->GetTrackedDevicePose(cmodel->TrackedDevice);
		*/

		m_eventOrientation[1] = vtkMath::DegreesFromRadians(m_eventOrientation[1]);
		m_eventOrientation[2] = vtkMath::DegreesFromRadians(m_eventOrientation[2]);
		m_eventOrientation[3] = vtkMath::DegreesFromRadians(m_eventOrientation[3]);

		//this->FindPickedActor(m_movePosition, nullptr);

		m_vrMain->onMove(device, m_movePosition, m_eventOrientation);
	}

	void updateTrackPadPos(vtkEventData* edata)
	{
		vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
		assert(edd);
		const double* pos = edd->GetTrackPadPosition();
		if (pos[0] == 0 && pos[1] == 0)
		{  // only if not both are 0 does it seem to be actually coming from the touch/trackpad...
			return;
		}
		if (edd->GetDevice() == vtkEventDataDevice::LeftController)
		{
			m_leftTrackPadPos.c[0] = pos[0];
			m_leftTrackPadPos.c[1] = pos[1];
		}
		else
		{
			m_rightTrackPadPos.c[0] = pos[0];
			m_rightTrackPadPos.c[1] = pos[1];
		}
	}

	void updateEventData(vtkEventData* edata)
	{
		vtkEventDataDevice3D* device = edata->GetAsEventDataDevice3D(); // Used Device

		// TODO Performance?
		int x = m_style->GetInteractor()->GetEventPosition()[0];
		int y = m_style->GetInteractor()->GetEventPosition()[1];
		m_style->FindPokedRenderer(x, y);

		device->GetWorldPosition(m_eventPosition);
		device->GetWorldOrientation(m_eventOrientation);

		m_eventOrientation[1] = vtkMath::DegreesFromRadians(m_eventOrientation[1]);
		m_eventOrientation[2] = vtkMath::DegreesFromRadians(m_eventOrientation[2]);
		m_eventOrientation[3] = vtkMath::DegreesFromRadians(m_eventOrientation[3]);
	}
	bool viewerMovement3D(vtkEventData* edata)
	{
		vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
		assert(edd);
		// right trackpad position changes as well as clicks are both mapped to ViewerMovement3D in VTK, we have to distinguish:
		if (edd->GetAction() == vtkEventDataAction::Unknown)
		{
			// position change
			updateTrackPadPos(edata);
			return true;
		}
		return false;
	}
	void mainInteraction(vtkEventData* edata, vtkProp3D* InteractionProp)
	{
		vtkEventDataDevice3D* device = edata->GetAsEventDataDevice3D(); // Used Device
		vtkEventDataAction action = device->GetAction();                // Action of Input Method
		if (action == vtkEventDataAction::Press || action == vtkEventDataAction::Touch)
		{
			m_vrMain->startInteraction(device, InteractionProp, m_eventPosition, m_eventOrientation);
		}
		else if (action == vtkEventDataAction::Release || action == vtkEventDataAction::Untouch)
		{
			m_vrMain->endInteraction(device, InteractionProp, m_eventPosition, m_eventOrientation);
		}
	}
};

//! Base Class for specific interaction callbacks
#ifdef OPENXR_AVAILABLE

#include <vtkOpenXRInteractorStyle.h>

#include <vtkOpenXRRenderWindowInteractor.h>

class iAImNDTOpenXRInteractorStyle : public vtkOpenXRInteractorStyle
{
public:
	static iAImNDTOpenXRInteractorStyle* New();
	vtkTypeMacro(iAImNDTOpenXRInteractorStyle, vtkOpenXRInteractorStyle);

	void setImpl(iAImNDTInteractionsImpl* impl);

	//! Calls, depending on Device - its input and action, the corresponding method
	//! Events can occur through left/right Controller and its input (trigger, grip, Trackpad,...) and an Action (Press, Release, Touch,...)
	void OnButton3D(vtkEventData* edata) override;

	// The VTK VR interactor comes with its own predefined "actions" and input manifest json files
	// We could define our fully own manifest with all actions; then we would however need to reimplement the InteractorStyle / RenderWindowInteractor
	// for now, as a workaround, we map the actions from VTK to our own ImNDT actions; for example:
	//     "OnNextPose3D", by default linked to the left trigger (see InteractorStyle::SetInteractor: ...AddAction("/actions/vtk/in/NextCameraPose", vtkCommand::NextPose3DEvent...)
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
	//! called on right trigger press (set up in InteractorStyle); forwards to OnButton3D
	void OnSelect3D(vtkEventData* edata) override;
	//! called on left trigger press (set up in InteractorStyle); forwards to OnButton3D
	void OnNextPose3D(vtkEventData* edata) override;
	//! called on right menu button press (set up in InteractorStyle); forwards to OnButton3D
	void OnMenu3D(vtkEventData* edata) override;
	void Dolly3D(vtkEventData* edata) override;
	void OnViewerMovement3D(vtkEventData* edata) override;
	
	// !Is called when a Controller moves. Forwards the event to the main class
	void OnMove3D(vtkEventData* edata) override
	{
		m_impl->OnMove3D(edata);
	}
	void OnPinch() override
	{
		m_impl->OnPinch();
	}

protected:
	iAImNDTOpenXRInteractorStyle();

private:
	iAImNDTInteractionsImpl* m_impl;
};

vtkStandardNewMacro(iAImNDTOpenXRInteractorStyle);

iAImNDTOpenXRInteractorStyle::iAImNDTOpenXRInteractorStyle()
{}

void iAImNDTOpenXRInteractorStyle::setImpl(iAImNDTInteractionsImpl* impl)
{
	m_impl = impl;
}

void iAImNDTOpenXRInteractorStyle::SetInteractor(vtkRenderWindowInteractor* iren)
{
	this->Superclass::SetInteractor(iren);
	auto oiren = vtkOpenXRRenderWindowInteractor::SafeDownCast(iren);
	assert(oiren);
	// works in conjunction with the actions defined in the action manifest specified via interactor->SetActionManifestFileName in iAVRMainThread!
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9,3,0)
	oiren->AddAction("complexgestureaction",
#else
	oiren->AddAction("leftgripaction",
		[this](vtkEventData* edata)
		{
			// for some reason, the input ID is not set; set it to the proper value
			vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
			assert(edd);
			edd->SetInput(vtkEventDataDeviceInput::Grip);
			OnButton3D(edata);
		});
	oiren->AddAction("rightgripaction",
#endif
		[this](vtkEventData* edata)
		{
			// for some reason, the input ID is not set; set it to the proper value
			vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
			assert(edd);
			edd->SetInput(vtkEventDataDeviceInput::Grip);
			OnButton3D(edata);
		});
	oiren->AddAction("trackpadleftclick",
		[this](vtkEventData* edata)
		{
			OnButton3D(edata);
		});
	oiren->AddAction("trackpadleftmove",
		[this](vtkEventData* edata)
		{
			m_impl->updateTrackPadPos(edata);
		});
	oiren->AddAction("showmenuleft",
		[this](vtkEventData* edata)
		{
			// for some reason, the input ID is not set; set it to the proper value
			vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
			assert(edd);
			edd->SetInput(vtkEventDataDeviceInput::ApplicationMenu);
			OnButton3D(edata);
		});
	oiren->AddAction("secondbuttonright",
		[](vtkEventData* edata)
		{	// currently unused (not working on VIVE as system button there apparently cannot be remapped)
			Q_UNUSED(edata);
			LOG(lvlInfo, QString("Right Second Button."));
		});
	oiren->AddAction("secondbuttonleft",
		[](vtkEventData* edata)
		{	// currently unused (not working on VIVE as system button there apparently cannot be remapped)
			Q_UNUSED(edata);
			LOG(lvlInfo, QString("Left Second Button."));
		});
}

void iAImNDTOpenXRInteractorStyle::OnNextPose3D(vtkEventData* edata)
{	// left trigger event - forward:
	OnButton3D(edata);
}

void iAImNDTOpenXRInteractorStyle::OnMenu3D(vtkEventData* edata)
{	// right controller menu button press
	// for some reason, the input ID is not set; set it to the proper Application Menu
	vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
	assert(edd);
	edd->SetInput(vtkEventDataDeviceInput::ApplicationMenu);
	OnButton3D(edata);
}

void iAImNDTOpenXRInteractorStyle::Dolly3D(vtkEventData* edata)
{
	Q_UNUSED(edata);
	// disable dollying
}

void iAImNDTOpenXRInteractorStyle::OnViewerMovement3D(vtkEventData* edata)
{
	if (m_impl->viewerMovement3D(edata))
	{
		return;
	}
	// trackpad click:
	OnButton3D(edata);
}

void iAImNDTOpenXRInteractorStyle::OnSelect3D(vtkEventData* edata)
{	// right trigger event - forward:
	OnButton3D(edata);
}

// FindPickedActor is protected, therefore we cannot move this function to Impl!
void iAImNDTOpenXRInteractorStyle::OnButton3D(vtkEventData* edata)
{
	m_impl->updateEventData(edata);
	this->PickingManagedOff();
	this->FindPickedActor(m_impl->m_eventPosition, nullptr);
	m_impl->mainInteraction(edata, InteractionProp);
}

#endif




#ifdef OPENVR_AVAILABLE

#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkOpenVRInteractorStyle.h>

class iAImNDTOpenVRInteractorStyle : public vtkOpenVRInteractorStyle
{
public:
	static iAImNDTOpenVRInteractorStyle* New();
	vtkTypeMacro(iAImNDTOpenVRInteractorStyle, vtkOpenVRInteractorStyle);
	void setImpl(iAImNDTInteractionsImpl* impl)
	{
		m_impl = impl;
	}

	//! Calls, depending on Device - its input and action, the corresponding method
	//! Events can occur through left/right Controller and its input (trigger, grip, Trackpad,...) and an Action (Press, Release, Touch,...)
	void OnButton3D(vtkEventData* edata) override;

	// The VTK VR interactor comes with its own predefined "actions" and input manifest json files
	// We could define our fully own manifest with all actions; then we would however need to reimplement the InteractorStyle / RenderWindowInteractor
	// for now, as a workaround, we map the actions from VTK to our own ImNDT actions; for example:
	//     "OnNextPose3D", by default linked to the left trigger (see InteractorStyle::SetInteractor: ...AddAction("/actions/vtk/in/NextCameraPose", vtkCommand::NextPose3DEvent...)
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
	//! called on right trigger press (set up in InteractorStyle); forwards to OnButton3D
	void OnSelect3D(vtkEventData* edata) override;
	//! called on left trigger press (set up in InteractorStyle); forwards to OnButton3D
	void OnNextPose3D(vtkEventData* edata) override;
	//! called on right menu button press (set up in InteractorStyle); forwards to OnButton3D
	void OnMenu3D(vtkEventData* edata) override;
	void Dolly3D(vtkEventData* edata) override;
	void OnViewerMovement3D(vtkEventData* edata) override;

	//! retrieve the position of the last interaction with the trackpad (since it's not available on a click in the event directly)
	iAImNDTInteractions::iAVec2d getTrackPadPos(vtkEventDataDevice device);

	void OnMove3D(vtkEventData* edata) override
	{
		m_impl->OnMove3D(edata);
	}
	void OnPinch() override
	{
		m_impl->OnPinch();
	}

protected:
	iAImNDTOpenVRInteractorStyle();

private:
	iAImNDTInteractionsImpl* m_impl;
};

vtkStandardNewMacro(iAImNDTOpenVRInteractorStyle);

iAImNDTOpenVRInteractorStyle::iAImNDTOpenVRInteractorStyle()
{}

void iAImNDTOpenVRInteractorStyle::SetInteractor(vtkRenderWindowInteractor* iren)
{
	this->Superclass::SetInteractor(iren);
	if (!iren)    // needed for shutdown: on setting another interactor style, the interactor calls SetInteractor(nullptr)
	{
		return;
	}
	auto oiren = vtkOpenVRRenderWindowInteractor::SafeDownCast(iren);
	assert(oiren);
	// works in conjunction with the actions defined in the action manifest specified via interactor->SetActionManifestFileName in iAVRMainThread!
#if VTK_VERSION_NUMBER >= VTK_VERSION_CHECK(9, 3, 0)
	oiren->AddAction("/actions/vtk/in/complexgestureaction", false,
#else
	oiren->AddAction("/actions/vtk/in/leftgripaction", false,
		[this](vtkEventData* edata)
		{
			// for some reason, the input ID is not set; set it to the proper value
			vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
			assert(edd);
			edd->SetInput(vtkEventDataDeviceInput::Grip);
			OnButton3D(edata);
		});
	oiren->AddAction("/actions/vtk/in/rightgripaction", false,
#endif
		[this](vtkEventData* edata)
		{
			// for some reason, the input ID is not set; set it to the proper value
			vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
			assert(edd);
			edd->SetInput(vtkEventDataDeviceInput::Grip);
			OnButton3D(edata);
		});
	oiren->AddAction("/actions/vtk/in/TrackPadLeftClick", false,
		[this](vtkEventData* edata)
		{
			OnButton3D(edata);
		});
	oiren->AddAction("/actions/vtk/in/TrackPadLeftMove", true,
		[this](vtkEventData* edata)
		{
			m_impl->updateTrackPadPos(edata);
		});
	oiren->AddAction("/actions/vtk/in/ShowMenuLeft", false,
		[this](vtkEventData* edata)
		{
			// for some reason, the input ID is not set; set it to the proper value
			vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
			assert(edd);
			edd->SetInput(vtkEventDataDeviceInput::ApplicationMenu);
			OnButton3D(edata);
		});
	oiren->AddAction("/actions/vtk/in/SecondButtonRight", false,
		[](vtkEventData* edata)
		{	// currently unused (not working on VIVE as system button there apparently cannot be remapped)
			Q_UNUSED(edata);
			LOG(lvlInfo, QString("Right Second Button."));
		});
	oiren->AddAction("/actions/vtk/in/SecondButtonLeft", false,
		[](vtkEventData* edata)
		{	// currently unused (not working on VIVE as system button there apparently cannot be remapped)
			Q_UNUSED(edata);
			LOG(lvlInfo, QString("Left Second Button."));
		});
}

void iAImNDTOpenVRInteractorStyle::OnNextPose3D(vtkEventData* edata)
{	// left trigger event - forward:
	OnButton3D(edata);
}

void iAImNDTOpenVRInteractorStyle::OnMenu3D(vtkEventData* edata)
{	// right controller menu button press
	// for some reason, the input ID is not set; set it to the proper Application Menu
	vtkEventDataDevice3D* edd = edata->GetAsEventDataDevice3D();
	assert(edd);
	edd->SetInput(vtkEventDataDeviceInput::ApplicationMenu);
	OnButton3D(edata);
}

void iAImNDTOpenVRInteractorStyle::Dolly3D(vtkEventData* edata)
{
	Q_UNUSED(edata);
	// disable dollying
}

void iAImNDTOpenVRInteractorStyle::OnViewerMovement3D(vtkEventData* edata)
{
	if (m_impl->viewerMovement3D(edata))
	{
		return;
	}
	// trackpad click:
	OnButton3D(edata);
}

void iAImNDTOpenVRInteractorStyle::OnSelect3D(vtkEventData* edata)
{	// right trigger event - forward:
	OnButton3D(edata);
}

// FindPickedActor is protected, therefore we cannot move this function to Impl!
void iAImNDTOpenVRInteractorStyle::OnButton3D(vtkEventData* edata)
{
	m_impl->updateEventData(edata);
	this->PickingManagedOff();
	this->FindPickedActor(m_impl->m_eventPosition, nullptr);
	m_impl->mainInteraction(edata, InteractionProp);
}

#endif






vtkSmartPointer<vtkInteractorStyle3D> createInteractorStyle(iAvtkVR::Backend backend, iAImNDTInteractionsImpl* impl)
{
#ifdef OPENVR_AVAILABLE
	if (backend == iAvtkVR::OpenVR)
	{
		auto result = vtkSmartPointer<iAImNDTOpenVRInteractorStyle>::New();
		result->setImpl(impl);
		return result;
	}
#endif
#ifdef OPENXR_AVAILABLE
	if (backend == iAvtkVR::OpenXR)
	{
		auto result = vtkSmartPointer<iAImNDTOpenXRInteractorStyle>::New();
		result->setImpl(impl);
		return result;
	}
#endif
	LOG(lvlError, "Could not create ImNDT interactor for given environment, using dummy!");
	return vtkSmartPointer<vtkInteractorStyle3D>::New();
}



iAImNDTInteractions::iAImNDTInteractions(iAvtkVR::Backend backend, iAImNDTMain* vrMain) :
	m_impl(std::make_unique<iAImNDTInteractionsImpl>(backend, vrMain))
{
}

iAImNDTInteractions::~iAImNDTInteractions() = default;    // required for enabling unique_ptr member

//! retrieve the position of the last interaction with the trackpad (since it's not available on a click in the event directly)
iAImNDTInteractions::iAVec2d iAImNDTInteractions::getTrackPadPos(vtkEventDataDevice device) const
{
	return m_impl->getTrackPadPos(device);
}

vtkInteractorStyle3D* iAImNDTInteractions::style()
{
	return m_impl->m_style;
}

//! Calculates the vector of the diagonals in the touchpad "square" and returns if the position is Up, Right, Down or left on the pad.
//! The touchpad forms a square from (-1,-1) to (1,1)
iAVRTouchpadPosition iAImNDTInteractions::getTouchedPadSide(float position[3])
{
	vtkVector2f p0 = vtkVector2f(-1, -1);
	vtkVector2f p1 = vtkVector2f(1, 1);

	vtkVector2f q0 = vtkVector2f(1, -1);
	vtkVector2f q1 = vtkVector2f(-1, 1);

	double distFromCenter = std::sqrt(position[0] * position[0] + position[1] * position[1]);

	if (distFromCenter < 0.5)
	{
		return iAVRTouchpadPosition::Middle;
	}

	float sideOfDiag1 = ((p1.GetX() - p0.GetX()) * (position[1] - p0.GetY()) - (p1.GetY() - p0.GetY()) * (position[0] - p0.GetX()));
	float sideOfDiag2 = ((q1.GetX() - q0.GetX()) * (position[1] - q0.GetY()) - (q1.GetY() - q0.GetY()) * (position[0] - q0.GetX()));

	if (sideOfDiag1 > 0 && sideOfDiag2 > 0) return iAVRTouchpadPosition::Left;
	if (sideOfDiag1 > 0 && sideOfDiag2 < 0) return iAVRTouchpadPosition::Up;
	if (sideOfDiag1 < 0 && sideOfDiag2 > 0) return iAVRTouchpadPosition::Down;
	if (sideOfDiag1 < 0 && sideOfDiag2 < 0) return iAVRTouchpadPosition::Right;

	return iAVRTouchpadPosition::Unknown;
}

iAVRViewDirection iAImNDTInteractions::getViewDirection(double viewDir[3])
{
	double maxVal = 0;
	int maxDir = -1;

	for (int i = 0; i < 3; i++)
	{
		if (maxVal < abs(viewDir[i]))
		{
			maxVal = abs(viewDir[i]);
			maxDir = i;
		}
	}

	if (maxDir == 0 && viewDir[maxDir] > 0) return iAVRViewDirection::Right;
	if (maxDir == 0 && viewDir[maxDir] < 0) return iAVRViewDirection::Left;
	if (maxDir == 1 && viewDir[maxDir] > 0) return iAVRViewDirection::Up;
	if (maxDir == 1 && viewDir[maxDir] < 0) return iAVRViewDirection::Down;
	if (maxDir == 2 && viewDir[maxDir] > 0) return iAVRViewDirection::Backwards;
	if (maxDir == 2 && viewDir[maxDir] < 0) return iAVRViewDirection::Forward;

	return iAVRViewDirection::Unknown;
}

vtkSmartPointer<vtkInteractorStyle3D> defaultVRinteractorStyle(iAvtkVR::Backend backend)
{
#ifdef OPENXR_AVAILABLE
	if (backend == iAvtkVR::OpenXR)
	{
		return vtkSmartPointer<vtkOpenXRInteractorStyle>::New();
	}
#endif
#ifdef OPENVR_AVAILABLE
	if (backend == iAvtkVR::OpenVR)
	{
		return vtkSmartPointer<vtkOpenVRInteractorStyle>::New();
	}
#endif
	LOG(lvlError, "defaultVRInteractor: Invalid/unavailable backend!");
	return vtkSmartPointer<vtkInteractorStyle3D>();
}