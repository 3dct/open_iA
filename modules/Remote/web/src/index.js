import SmartConnect from 'wslink/src/SmartConnect';
import vtkWSLinkClient from '@kitware/vtk.js/IO/Core/WSLinkClient';
import vtkRemoteView from '@kitware/vtk.js/Rendering/Misc/RemoteView';

vtkWSLinkClient.setSmartConnectClass(SmartConnect);

document.body.style.padding = '0';
document.body.style.margin = '0';

const clientToConnect = vtkWSLinkClient.newInstance();
const InteractiveRatio = 0.5; // the scaled image compared to the clients view resolution
const JPEGQuality = 40;       // jpeg quality

function connectionChangeHandler(httpReq, actionName)
{
	const message = (httpReq && httpReq.response && httpReq.response.error) || "Connection "+actionName;
	console.error(message);
	//console.log(httpReq);
}

clientToConnect.onConnectionError((httpReq) => { connectionChangeHandler(httpReq, "error"); });
clientToConnect.onConnectionClose((httpReq) => { connectionChangeHandler(httpReq, "closed"); });

// hint: if you use the launcher.py and ws-proxy just leave out sessionURL
// (it will be provided by the launcher)
const config = {
	application: 'cone',
	sessionURL: 'ws://'+location.hostname+':1234/ws',
};

const views = [ "3D", "XY", "XZ", "YZ" ];
views.forEach(function(viewName, index) {
	const divRenderer = document.createElement('div');
    document.getElementById( (index+1).toString() ).appendChild(divRenderer);
	divRenderer.style.position = 'relative';
	clientToConnect
		.connect(config)
		.then((validClient) => {
			const viewStream = clientToConnect
				.getImageStream()
				.createViewStream(viewName);
			const view = vtkRemoteView.newInstance({
				rpcWheelEvent: 'viewport.mouse.zoom.wheel',
				rpcGestureEvent:'viewport.gesture',
				viewStream,
			});
			const session = validClient.getConnection().getSession();
			view.setSession(session);
			view.setContainer(divRenderer);
			view.setInteractiveRatio(InteractiveRatio);
			view.setInteractiveQuality(JPEGQuality);
			window.addEventListener('resize', view.resize);
		})
		.catch((error) => {
			console.error(error);
		});
});
