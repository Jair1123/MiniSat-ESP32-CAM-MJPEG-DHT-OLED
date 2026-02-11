#include <WiFi.h>
#include <WebServer.h>
#include <DHTesp.h>

const char* ssid = "MiniSat";
const char* pass = "12345678";
WebServer server(80);

// DHT11
#define DHTPIN 13
DHTesp dht;
float tempC = NAN, hum = NAN;
unsigned long lastRead = 0;
const float PRESION_HPA = 773.3;

const char* page = "<!doctype html><html><body style='background:#000;color:#0f8;font:16px monospace'>"
                   "<h1>MiniSat (Fase 2)</h1>"
                   "<p>DHT11 activo. Visita /data</p></body></html>";

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", page);
}

void handleData() {
  float t = isnan(tempC) ? 0.0 : tempC;
  float h = isnan(hum)   ? 0.0 : hum;
  String json = "{";
  json += "\"temp\":" + String(t, 2) + ",";
  json += "\"hum\":"  + String(h, 2) + ",";
  json += "\"pres\":" + String(PRESION_HPA, 1);
  json += "}";
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(true);
  WiFi.softAP(ssid, pass, 1, 0, 2);
  WiFi.setTxPower(WIFI_POWER_2dBm);

  Serial.print("AP: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  dht.setup(DHTPIN, DHTesp::DHT11);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("🚀 Fase 2 lista (DHT + /data)");
}

void loop() {
  server.handleClient();

  if (millis() - lastRead > 1500) {
    lastRead = millis();
    TempAndHumidity d = dht.getTempAndHumidity();
    if (!isnan(d.temperature) && d.temperature > -20 && d.temperature < 60) tempC = d.temperature;
    if (!isnan(d.humidity) && d.humidity >= 0 && d.humidity <= 100)        hum   = d.humidity;
  }
}
