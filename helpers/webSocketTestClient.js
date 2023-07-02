const { Server } = require("ws");

console.log("Iniciou");

const Socket = new Server({ port: 81 });

let IS_CONTROL_RUNNING = false;
let CURRENT_TARGET_VEL = 0;

function getCurrentStatus() {
  return JSON.stringify({
    type: "getData",
    isControlRunning: IS_CONTROL_RUNNING,
    // isRegistrationMode: CURRENT_TARGET_VEL,
  });
}

function updateToAll() {
  const newData = getCurrentStatus();
  console.log("Sending new data", { newData });
  Socket.clients.forEach((client) => {
    client.send(newData);
  });
}

Socket.on("connection", (ws) => {
  console.log("New client connected");

  ws.on("message", (message) => {
    const messageData = JSON.parse(message);
    console.log("Message received: ", messageData);

    switch (messageData.type) {
      case "getData":
        const data = getCurrentStatus();
        console.log("Solicitando dados.", { data });
        ws.send(data);
        break;
      // case "isRegistrationMode":
      //   CURRENT_TARGET_VEL = Boolean(messageData.value);
      //   console.log("IS_REGISTER_MODE: ", CURRENT_TARGET_VEL, messageData.value);
      //   updateToAll();
      //   break;
      case "isControlRunning":
        IS_CONTROL_RUNNING = Boolean(messageData.value);
        console.log("isControlRunning: ", IS_CONTROL_RUNNING), messageData.value;
      
        updateToAll();
        break;
    }
  });

  ws.on("close", () => console.log("Client has disconnected!"));
});

setInterval(() => {
  Socket.clients.forEach((client) => {
    IS_CONTROL_RUNNING = !IS_CONTROL_RUNNING;
    CURRENT_TARGET_VEL = Math.random() * 100;
    const data = JSON.stringify({
      type: "getData",
      isControlRunning: IS_CONTROL_RUNNING,
      currentVelocity: CURRENT_TARGET_VEL,
    });
    client.send(data);
  });
}, 5000);