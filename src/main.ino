#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <FS.h>
#include <esp_task_wdt.h>

#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

TaskHandle_t task1Handle;
volatile bool buttonState = false;

int bottomPWMLimit = 150;
int bottomVelocityLimit = 150;
int topVelocityLimit = 220;

// inicialização das variaveis
float t1 = 0, t2 = 0, delta = 0, controlZ = 0, controlZLastState = 0, errorZ = 0, errorZLastState = 0, targetVelocity = 0; // tempos
float currentCartVelocity = 0;

// Motor A
int IN2 = 18;
int IN1 = 19;
int ENA = 2;

// Motor B
int IN4 = 16;
int IN3 = 17;
int ENB = 15;

// Entradas
int sensorPin = 14;
int buttonPin = 36;
int potentiometerPin = 39; // Pino analógico GPIO34 no ESP32

// pré sets pwm
int frequency = 30000;
int pwmChannel = 0;
int resolution = 8;

int dutyCycle = 200;

const char *accessPointSSID = "CarrinhoSupermercado";
const char *accessPointPassword = "michels2023";

IPAddress local_IP(192, 168, 13, 07);
IPAddress gateway(192, 168, 1, 5);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer webServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

float getCurrentVelocity()
{
  delta = t2 - t1;
  if (delta > 150)
    delta = 150;
  if (delta < 5)
    delta = 5;

  currentCartVelocity = 60000 / (delta * 15); // tempo de uma volta em milissegundos
  return currentCartVelocity;
}

void readEncoderTask(void *parameter)
{
  volatile bool buttonPressed = false;
  volatile bool encoderIn = false;
  volatile bool lastEncoderIn = false;

  while (true)
  {
    encoderIn = digitalRead(sensorPin);
    if (encoderIn == HIGH && lastEncoderIn == LOW)
    {
      t1 = t2;
      t2 = (esp_timer_get_time() / 1000);
      buttonPressed = true;
    }

    lastEncoderIn = encoderIn;
    vTaskDelay(pdMS_TO_TICKS(1)); // Aguarda 10 milissegundos
    esp_task_wdt_reset();
  }
}

void setControlOff()
{
  digitalWrite(IN2, LOW);
  digitalWrite(IN1, LOW);
  digitalWrite(IN4, LOW);
  digitalWrite(IN3, LOW);

  dutyCycle = 0;
  controlZ = 0;
  controlZLastState = 0;
  errorZ = 0;
  errorZLastState = 0;
}

int getTargetVelocity()
{
  return map(analogRead(potentiometerPin), 0, 4095, bottomVelocityLimit, topVelocityLimit);
}

void handleControl()
{
  targetVelocity = getTargetVelocity();
  currentCartVelocity = getCurrentVelocity();

  errorZ = targetVelocity - currentCartVelocity;
  controlZ = controlZLastState + 0.002092172678595 * errorZ * 1.5 - 0.001944587 * errorZLastState * 1.5;

  digitalWrite(IN2, LOW);
  digitalWrite(IN1, HIGH);
  digitalWrite(IN4, LOW);
  digitalWrite(IN3, HIGH);

  controlZLastState = controlZ;
  errorZLastState = errorZ;

  dutyCycle = controlZ;

  if (dutyCycle >= 255)
  {
    dutyCycle = 255;
    controlZ = 0.99;
  }
  if (dutyCycle <= bottomPWMLimit)
  {
    dutyCycle = bottomPWMLimit;
    controlZ = 0.588;
  }

  ledcWrite(pwmChannel, dutyCycle);

  // Serial.print("Duty Cycle: ");
  // Serial.println(dutyCycle);

  // Serial.print("Target Velocity: ");
  // Serial.println(targetVelocity);

  // Serial.print("Velocidade: ");
  // Serial.println(currentCartVelocity);

  // Serial.print("error Z: ");
  // Serial.println(errorZ);

  // Serial.print("controlZ: ");
  // Serial.println(controlZ);

  // Serial.println("--------------");
}

bool isButtonPressed()
{
  buttonState = digitalRead(buttonPin);

  if (buttonState == LOW)
  {
    return true;
  }

  return false;
}

void initAccessPoint()
{
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  delay(500);
  WiFi.mode(WIFI_AP);

  Serial.print("[WIFI] Starting Access Point ... ");
  Serial.println(WiFi.softAP(accessPointSSID, accessPointPassword) ? "Ready" : "Failed!");
  delay(3000);

  Serial.print("[WIFI] Setting up Access Point ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("[WIFI] IP address = ");
  Serial.println(WiFi.softAPIP());
};

void initSPIFFS()
{
  if (!SPIFFS.begin())
  {
    Serial.println("[SPIFFS] Cannot mount SPIFFS volume... Restarting in 5 seconds!");
    delay(5000);
    return ESP.restart();
  }
};

void initWebServer()
{
  webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
               { request->send(SPIFFS, "/webpage.html", "text/html"); });

  webServer.onNotFound([](AsyncWebServerRequest *request)
                       { request->send(404, "text/plain", "Not found. Just '/' endpoint exists!"); });

  webServer.serveStatic("/", SPIFFS, "/");
};

void initWebSocket()
{
  webSocket.begin();
  webSocket.onEvent(onWebSocketEvent);

  webServer.begin();
};

void sendCurrentStatus()
{
  String currentStatusParsed = String("{\"type\":\"getData\",\"isControlRunning\":") + isButtonPressed() + ",\"currentVelocity\":" + getCurrentVelocity() + ",\"targetVelocity\":" + getTargetVelocity() + "}";

  // Serial.println("[WS] Send Current Status to WEB -> " + currentStatusParsed);

  webSocket.broadcastTXT(currentStatusParsed);
};

void handleValidEvents(String Event, bool newStatus)
{
  Serial.println("[WS] Handling with event of type: " + Event + " with value: " + newStatus);

  if (Event == "getData")
  {
    sendCurrentStatus();
  }
};

void onWebSocketEvent(byte clientId, WStype_t eventType, uint8_t *payload, size_t length)
{
  switch (eventType)
  {
  case WStype_DISCONNECTED:
    Serial.println("[WS] Client with ID" + String(clientId) + " disconnected now!");
    break;
  case WStype_CONNECTED:
    Serial.println("[WS] Client with ID " + String(clientId) + " connected now!");
    break;
  case WStype_TEXT:
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error)
    {
      Serial.print(F("[WS] Error on deserialize Json content: "));
      Serial.println(error.f_str());
      return;
    }

    const char *event_type = doc["type"];
    bool event_value = doc["value"];

    Serial.println("[WS] Event type " + String(event_type) + " : " + event_value);
    handleValidEvents(String(event_type), event_value);
  };
};

void setup()
{
  Serial.begin(115200);

  delay(500);

  initSPIFFS();
  initAccessPoint();
  initWebServer();

  delay(500);

  initWebSocket();

  delay(500);

  pinMode(BUILTIN_LED, OUTPUT);
  // sets the pins as outputs:
  pinMode(IN2, OUTPUT);
  pinMode(IN1, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(sensorPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);

  xTaskCreatePinnedToCore(readEncoderTask, "Task 1", 4096, NULL, 1, &task1Handle, 0);

  // Configure LED PWM functionalities
  ledcSetup(pwmChannel, frequency, resolution);

  // Attach the channel to the GPIO to be controlled
  ledcAttachPin(ENA, pwmChannel);
  ledcAttachPin(ENB, pwmChannel);
}

void loop()
{
  webSocket.loop();

  if (isButtonPressed())
    handleControl();
  else
    setControlOff();

  sendCurrentStatus();
}