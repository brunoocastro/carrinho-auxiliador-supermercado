#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <MFRC522.h>
#include <FS.h>

#define SDAPin 5
#define RSTPin 2
#define RelayPin 12

#define GreenLedPin 22
#define RedLedPin 13

File cardsFile;
int maxCards = 100;
String cards[100] = {};

MFRC522 CardReader(SDAPin, RSTPin);

const char *accessPointSSID = "CarrinhoSupermercado";
const char *accessPointPassword = "michels2023";

IPAddress local_IP(192, 168, 13, 07);
IPAddress gateway(192, 168, 1, 5);
IPAddress subnet(255, 255, 255, 0);

AsyncWebServer webServer(80);
WebSocketsServer webSocket = WebSocketsServer(81);

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

  cardsFile = SPIFFS.open("/savedCards.txt", "r");
  delay(300);

  if (!cardsFile)
  {
    Serial.println("[FILE] Error opening file.");
    return ESP.restart();
  }

  Serial.println("[FILE] - Successfully opened cards file.");
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
  // webSocket.onEvent(onWebSocketEvent);

  webServer.begin();
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
  };
};

void setup()
{
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);
  // Set pin mode

  delay(500);

  initSPIFFS();
  initAccessPoint();
  initWebServer();

  delay(500);

  initWebSocket();

  delay(500);
}

void loop()
{
  webSocket.loop();
  delay(500);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(500);
  digitalWrite(BUILTIN_LED, LOW);
}