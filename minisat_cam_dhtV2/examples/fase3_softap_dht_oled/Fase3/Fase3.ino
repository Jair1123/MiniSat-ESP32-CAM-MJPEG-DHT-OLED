#include <WiFi.h>
#include <WebServer.h>
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>

const char* ssid = "MiniSat";
const char* pass = "12345678";
WebServer server(80);

// DHT11
#define DHTPIN 13
DHTesp dht;
float tempC = NAN, hum = NAN;
unsigned long lastRead = 0;
const float PRESION_HPA = 773.3;

// OLED
Adafruit_SSD1306 display(128, 64, &Wire);

const char* page = "<!doctype html><html><body style='background:#000;color:#0f8;font:16px monospace'>"
                   "<h1>MiniSat (Fase 3)</h1>"
                   "<p>DHT + OLED. Visita /data</p></body></html>";

void updateOLED(float t, float h, float p) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("MiniSAT");
  display.println("Instituto Cumbres");
  display.println("Bosques");
  display.println("");
  display.print("T: "); display.print(t,1); display.println(" C");
  display.print("H: "); display.print(h,0); display.println(" %");
  display.print("P: "); display.print(p,1); display.println(" hPa");
  display.display();
}

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

  Wire.begin(14, 15);
  Wire.setClock(100000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED no encontrada");
    while(true);
  }
  display.clearDisplay();
  display.display();

  dht.setup(DHTPIN, DHTesp::DHT11);

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("🚀 Fase 3 lista (DHT + OLED)");
}

void loop() {
  server.handleClient();

  if (millis() - lastRead > 1500) {
    lastRead = millis();
    TempAndHumidity d = dht.getTempAndHumidity();
    if (!isnan(d.temperature) && d.temperature > -20 && d.temperature < 60) tempC = d.temperature;
    if (!isnan(d.humidity) && d.humidity >= 0 && d.humidity <= 100)        hum   = d.humidity;
    updateOLED(isnan(tempC)?0.0:tempC, isnan(hum)?0.0:hum, PRESION_HPA);
  }
}
