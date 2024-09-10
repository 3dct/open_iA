// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "iADefaultSettings.h"

#if QT_VERSION >= QT_VERSION_CHECK(6,5,0)
#include <QtTypes>    // for quint16
#else
#include <QtGlobal>
#endif

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

//! @{
//! make the given server listen to a port, preferrably the one given as port parameter, but
//! also a higher port number if not available; give up if  we haven't found a free port
//! after maxTries tried ports
//! @return the port to which the server is listening (or 0 if connection was unsuccessful)

class QWebSocketServer;
quint16 connectWebSocket(QWebSocketServer* server, quint16 port, int const maxTries = 10);

#ifdef QT_HTTPSERVER
class QHttpServer;
quint16 connectHttp(QHttpServer* server, quint16 port, int const maxTries = 10);
#endif

//! @}

void removeClosedPort(quint16 port);
