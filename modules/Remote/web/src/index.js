
import SmartConnect from 'wslink/src/SmartConnect';
import vtkWSLinkClient from '@kitware/vtk.js/IO/Core/WSLinkClient';
import vtkRemoteView from '@kitware/vtk.js/Rendering/Misc/RemoteView';


vtkWSLinkClient.setSmartConnectClass(SmartConnect);

document.body.style.padding = '0';
document.body.style.margin = '0';

const divRenderer = document.createElement('div');
document.body.appendChild(divRenderer);

divRenderer.style.position = 'relative';
divRenderer.style.width = '100vw';
divRenderer.style.height = '100vh';
divRenderer.style.overflow = 'hidden';

const clientToConnect = vtkWSLinkClient.newInstance();

// Error
clientToConnect.onConnectionError((httpReq) => {
  const message =
    (httpReq && httpReq.response && httpReq.response.error) ||
    `Connection error`;
  console.error(message);
  console.log(httpReq);
});

// Close
clientToConnect.onConnectionClose((httpReq) => {
  const message =
    (httpReq && httpReq.response && httpReq.response.error) ||
    `Connection close`;
  console.error(message);
  console.log(httpReq);
});

// hint: if you use the launcher.py and ws-proxy just leave out sessionURL
// (it will be provided by the launcher)
const config = {
  application: 'cone',
  sessionURL: 'ws://10.52.1.201:1234/ws',
};

// Connect
clientToConnect
  .connect(config)
  .then((validClient) => {
    const viewStream = clientToConnect
      .getImageStream()
      .createViewStream('-1');

    const view = vtkRemoteView.newInstance({
      rpcWheelEvent: 'viewport.mouse.zoom.wheel',
      viewStream,
    });
    const session = validClient.getConnection().getSession();
    view.setSession(session);
    view.setContainer(divRenderer);
    view.setInteractiveRatio(0.25); // the scaled image compared to the clients view resolution
    view.setInteractiveQuality(40); // jpeg quality

    window.addEventListener('resize', view.resize);
  })
  .catch((error) => {
    console.error(error);
  });