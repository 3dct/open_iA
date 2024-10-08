// Copyright (c) open_iA contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "iARemotePortSettings.h"

//#include <iALog.h>

#include <set>

#include <QWebSocketServer>
#ifdef QT_HTTPSERVER
#include <QHttpServer>
#endif

namespace
{
	std::set<quint16>& portsInUse()
	{
		static std::set<quint16> s_portsInUse;
		return s_portsInUse;
	}
}

template <class ServerType>
quint16 connectPortInternal(ServerType* server, quint16 port, int const maxTries)
{
	bool connected = false;
	int numTried = 0;
	do
	{
		while (portsInUse().contains(port))
		{
			//LOG(lvlDebug, QString("Skipping port %1 because it is (probably) still in use").arg(port));
			++port;
		}
		connected = server->listen(QHostAddress::Any, port);
		// If listening above fails, the port is in use by something else than this tool; even though ports only get
		// cleared from portsInUse on closing this tool, we are anyway adding the current port number to
		// portsInUse independent of whether listening was successful or not.
		// This helps us from repeatedly trying ports that are _probably_ still in use; and
		// we don't expect open_iA to be running so long that we run out of ports to try.
		//if (!connected)
		//{
		//	LOG(lvlDebug, QString("Listening to port %1 failed").arg(port));
		//}
		portsInUse().insert(port);
		++numTried;
	}
	while (!connected && numTried < maxTries);
	return connected ? port : 0;
}

quint16 connectWebSocket(QWebSocketServer* server, quint16 port, int const maxTries)
{
	return connectPortInternal(server, port, maxTries);
}

#ifdef QT_HTTPSERVER
quint16 connectHttp(QHttpServer* server, quint16 port, int const maxTries)
{
	return connectPortInternal(server, port, maxTries);
}
#endif

void removeClosedPort(quint16 port)
{
	portsInUse().erase(port);
}
