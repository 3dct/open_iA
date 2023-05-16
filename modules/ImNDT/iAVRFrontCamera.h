// Copyright 2016-2023, the open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

//! Interface to be implemented by VR backend specific classes for switching between VR (default) and video see-through mode
class iAVRFrontCamera
{
public:
	virtual ~iAVRFrontCamera();
	virtual bool refreshImage();
	virtual bool setEnabled(bool enabled);
};

#include "iAvtkVR.h"

#include <memory>

class iAVREnvironment;

std::unique_ptr<iAVRFrontCamera> createARViewer(iAVREnvironment* env);
