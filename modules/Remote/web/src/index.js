import SmartConnect from 'wslink/src/SmartConnect';
import vtkWSLinkClient from '@kitware/vtk.js/IO/Core/WSLinkClient';
import vtkRemoteView from '@kitware/vtk.js/Rendering/Misc/RemoteView';

vtkWSLinkClient.setSmartConnectClass(SmartConnect);

document.body.style.padding = '0';
document.body.style.margin = '0';

const divRenderer = document.createElement('div');
const divRenderer2 = document.createElement('div');
const divRenderer3 = document.createElement('div');
const divRenderer4 = document.createElement('div');

document.getElementById("1").appendChild(divRenderer);
document.getElementById("2").appendChild(divRenderer2);
document.getElementById("3").appendChild(divRenderer3);
document.getElementById("4").appendChild(divRenderer4);

divRenderer.style.position = 'relative';
divRenderer2.style.position = 'relative';
divRenderer3.style.position = 'relative';
divRenderer4.style.position = 'relative';

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
  sessionURL: 'ws://'+location.hostname+':1234/ws',
};

// Connect
clientToConnect
  .connect(config)
  .then((validClient) => {
    const viewStream = clientToConnect
      .getImageStream()
      .createViewStream('3D');

    const view = vtkRemoteView.newInstance({
      rpcWheelEvent: 'viewport.mouse.zoom.wheel',
      viewStream,
    });
    const session = validClient.getConnection().getSession();
    view.setSession(session);
    view.setContainer(divRenderer);
    view.setInteractiveRatio(0.7); // the scaled image compared to the clients view resolution
    view.setInteractiveQuality(50); // jpeg quality

    window.addEventListener('resize', view.resize);
  })
  .catch((error) => {
    console.error(error);
  });


  // Connect
clientToConnect
.connect(config)
.then((validClient) => {
  const viewStream = clientToConnect
    .getImageStream()
    .createViewStream('XY');

  const view = vtkRemoteView.newInstance({
    rpcWheelEvent: 'viewport.mouse.zoom.wheel',
    viewStream,
  });
  const session = validClient.getConnection().getSession();
  view.setSession(session);
  view.setContainer(divRenderer2);
  view.setInteractiveRatio(0.7); // the scaled image compared to the clients view resolution
  view.setInteractiveQuality(50); // jpeg quality

  window.addEventListener('resize', view.resize);
})
.catch((error) => {
  console.error(error);
});

// Connect
clientToConnect
  .connect(config)
  .then((validClient) => {
    const viewStream = clientToConnect
      .getImageStream()
      .createViewStream('XZ');

    const view = vtkRemoteView.newInstance({
      rpcWheelEvent: 'viewport.mouse.zoom.wheel',
	  rpcGestureEvent:'viewport.gesture',
      viewStream,
    });
    const session = validClient.getConnection().getSession();
    view.setSession(session);
    view.setContainer(divRenderer3);
    view.setInteractiveRatio(0.7); // the scaled image compared to the clients view resolution
    view.setInteractiveQuality(50); // jpeg quality

    window.addEventListener('resize', view.resize);
  })
  .catch((error) => {
    console.error(error);
  });


  // Connect
clientToConnect
.connect(config)
.then((validClient) => {
  const viewStream = clientToConnect
    .getImageStream()
    .createViewStream('YZ');

  const view = vtkRemoteView.newInstance({
    rpcWheelEvent: 'viewport.mouse.zoom.wheel',
    viewStream,
  });
  const session = validClient.getConnection().getSession();
  view.setSession(session);
  view.setContainer(divRenderer4);
  view.setInteractiveRatio(0.7); // the scaled image compared to the clients view resolution
  view.setInteractiveQuality(50); // jpeg quality
  

  window.addEventListener('resize', view.resize);
})
.catch((error) => {
  console.error(error);
});