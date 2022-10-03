#pragma once

#include "iAremote_export.h"

#include "WebsocketAPI.h"
#include <vtkRenderWindow.h>



class iAremote_API iARemoteRenderer
{
public:
	iARemoteRenderer(vtkRenderWindow* child);

	WebsocketAPI* m_websocket;
};

