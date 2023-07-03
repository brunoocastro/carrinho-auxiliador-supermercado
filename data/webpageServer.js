const HOST = !!window?.location?.hostname ? window?.location?.hostname : "localhost";
  console.log("[HOST] - Tentando conectar ao host :", HOST);

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

    setTargetVelocity(newVelocity)
  }

  function updateCartCurrentVelocityFromSocket(newVelocity) {
    console.log("[WS] - Updating cart current velocity to " + newVelocity);

    setCurrentVelocity(newVelocity)
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
    console.log('[WS] Trying to initialize websocket connection.');
    Socket = new WebSocket('ws://' + HOST + ':81/');
    Socket.onmessage = onWebsocketMessage;
    Socket.onclose = onWebsocketClose;
    Socket.onopen = onWebsocketOpen;
  }

window.onload = initWebSocket;