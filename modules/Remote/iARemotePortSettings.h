// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iADefaultSettings.h"

inline constexpr char RemotePortSettingsName[] = "Default Settings/Remote: Ports";
//! Port settings for the remote rendering
class iARemotePortSettings : iASettingsObject<RemotePortSettingsName, iARemotePortSettings>
{
public:
	static constexpr char UnityPort[] = "Unity port";
	static constexpr char RemoteWebSocketPort[] = "Remote rendering websocket port";
	static constexpr char RemoteHTTPPort[] = "Remote rendering http port";

	static iAAttributes& defaultAttributes()
	{
		static iAAttributes attr;
		if (attr.isEmpty())
		{
			addAttr(attr, RemoteWebSocketPort, iAValueType::Discrete, 1234, 1, 65535);
			addAttr(attr, RemoteHTTPPort, iAValueType::Discrete, 8080, 1, 65535);
			addAttr(attr, UnityPort, iAValueType::Discrete, 8180, 1, 65535);
			selfRegister();
		}
		return attr;
	}
};
