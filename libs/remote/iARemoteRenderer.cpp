#include "iARemoteRenderer.h"


iARemoteRenderer::iARemoteRenderer(vtkRenderWindow* child)
{

	//child->renderer();
	//child->slicer(0);

	m_websocket = new WebsocketAPI(1234);

}