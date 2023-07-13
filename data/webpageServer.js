const HOST = !!window ? window?.location?.hostname : "localhost";

const currentVelocityBuffer = [];
const targetVelocityBuffer = [];
const velocityBufferSize = 80
const PlotDivRef = document.getElementById('plotly');

function getXAxis() {
  return Array.from(Array(velocityBufferSize).keys());
}

const getTraceFromArray = (array, name, color) => ({
  x: getXAxis(),
  y: array,
  // fill: "tozerox", 
  fillcolor: "rgba(0,100,80,0.2)",
  line: { color },
  name,
  showlegend: true,
  type: "scatter"
});

var data = [getTraceFromArray(targetVelocityBuffer, 'Target', 'red'), getTraceFromArray(currentVelocityBuffer, 'Current', 'blue')];
var layout = {
  paper_bgcolor: "rgb(255,255,255)",
  plot_bgcolor: "rgb(229,229,229)",
  xaxis: {
    gridcolor: "rgb(255,255,255)",
    range: [1, velocityBufferSize],
    showgrid: true,
    showline: false,
    showticklabels: true,
    tickcolor: "rgb(127,127,127)",
    ticks: "outside",
    zeroline: false
  },
  yaxis: {
    gridcolor: "rgb(255,255,255)",
    showgrid: true,
    showline: false,
    showticklabels: true,
    tickcolor: "rgb(127,127,127)",
    ticks: "outside",
    zeroline: false
  }
}

console.log("[HOST] - Tentando conectar ao host :", HOST);

const bufferDelayInMS = 500


let updateVelocityTimer;

function CreatePlotlyGraph() {
  Plotly.newPlot(PlotDivRef, [getTraceFromArray(targetVelocityBuffer, 'Target', 'red'), getTraceFromArray(currentVelocityBuffer, 'Current', 'blue')], layout);
}

function setCurrentVelocity(velocity) {
  document.getElementById('CartCurrentVel').textContent = Number(velocity).toFixed(1);
}

function setTargetVelocity(velocity) {
  document.getElementById('CartTargetVel').textContent = Number(velocity).toFixed(1);
}

const availableEvents = {
  isControlRunning: updateCartControlState,
  currentVelocity: updateCartCurrentVelocityFromSocket,
}

let Socket;

function sendWebsocketMessage(socketMsg) {
  Socket.send(JSON.stringify(socketMsg));
}

function onWebsocketMessage(event) {
  processCommand(event);
}
function onWebsocketOpen() {
  console.log('[WS] -> Connected successfully!')
  requestCurrentStatusFromSocket();
}
function onWebsocketClose() {
  console.log('[WS] -> Connection lost. Trying to reconnect in 2 seconds!');
  setTimeout(initWebSocket, 2000);
}

function handleReceivedStateFromSocket(data) {
  const { currentVelocity, isControlRunning, targetVelocity } = data;
  console.log("[WS] - Received system state from Socket: ", { currentVelocity, isControlRunning, targetVelocity });

  if (isControlRunning !== undefined && isControlRunning !== null)
    updateCartControlState(isControlRunning);

  if (currentVelocity !== undefined && currentVelocity !== null)
    updateCartCurrentVelocityFromSocket(currentVelocity);

  if (targetVelocity !== undefined && targetVelocity !== null)
    updateCartTargetVelocityFromSocket(targetVelocity);

  Plotly.extendTraces(PlotDivRef, { y: [getTraceFromArray(targetVelocityBuffer, 'Target', 'red'), getTraceFromArray(currentVelocityBuffer, 'Current', 'blue')] }, [0])


  document.getElementById("loadingContainer").style.display = 'none';
}

function setCartControlOFF() {
  document.getElementById('CartControlStatus').textContent = 'INATIVO';
  document.getElementById('CartControlStatus').style.color = 'rgb(184, 75, 61)';
}
function setCartControlON() {
  document.getElementById('CartControlStatus').textContent = 'ATIVO';
  document.getElementById('CartControlStatus').style.color = 'rgb(46, 138, 52)';
}

function requestCurrentStatusFromSocket() {
  const socketMsg = { type: 'getData', value: true };
  sendWebsocketMessage(socketMsg);
}

function updateCartTargetVelocityFromSocket(newVelocity) {
  console.log("[WS] - Updating cart target velocity to " + newVelocity);

  const listLastPosition = current[targetVelocityBuffer.length - 1];

  if (listLastPosition.velocity !== newVelocity || !areTimeRemaining(listLastPosition.date)) {
    if (targetVelocityBuffer.length > velocityBufferSize) targetVelocityBuffer.shift();
    targetVelocityBuffer.push({ velocity: newVelocity, date: new Date().getTime() });
    setTargetVelocity(newVelocity)
  }
}

function updateCartCurrentVelocityFromSocket(newVelocity) {
  console.log("[WS] - Updating cart current velocity to " + newVelocity);

  const listLastPosition = currentVelocityBuffer[currentVelocityBuffer.length - 1];

  if (listLastPosition.velocity !== newVelocity || !areTimeRemaining(listLastPosition.date)) {
    if (currentVelocityBuffer.length > velocityBufferSize) currentVelocityBuffer.shift();
    currentVelocityBuffer.push({ velocity: newVelocity, date: new Date().getTime() });
    setCurrentVelocity(newVelocity);
  }
}

function areTimeRemaining(time) {
  //write a function to validate if time is 
  const targetTime = new Date(time)
  const targetWithDelay = new Date(targetTime.getTime() + bufferDelayInMS)

  const nowTime = new Date()

  return targetWithDelay > nowTime

}

function updateCartControlState(isCartControlled) {
  console.log("[WS] - Updating cart control state to: " + isCartControlled);

  if (isCartControlled) {
    setCartControlON();
  } else {
    setCartControlOFF();
  }
}

function processCommand(event) {
  let { type, value, ...content } = JSON.parse(event.data);
  // console.log("[WS] - Received Event: ", { type, value, content });

  if (type === "getData") {
    // console.log("[WS] - State update event received: ", { content, type });
    return handleReceivedStateFromSocket(content);
  }

  if (availableEvents[type]) availableEvents[type](value);
}

function initWebSocket() {
  CreatePlotlyGraph()
  console.log('[WS] Trying to initialize websocket connection.');
  Socket = new WebSocket('ws://' + HOST + ':81/');
  Socket.onmessage = onWebsocketMessage;
  Socket.onclose = onWebsocketClose;
  Socket.onopen = onWebsocketOpen;
}

window.onload = initWebSocket;