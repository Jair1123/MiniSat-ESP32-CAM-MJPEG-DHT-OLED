#include <WiFi.h>
#include <WebServer.h>
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "esp_camera.h"

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

// Camara
volatile bool camera_ok = false;

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* page = "<!doctype html><html><body style='background:#000;color:#0f8;font:16px monospace'>"
                   "<h1>MiniSat (Fase 4)</h1>"
                   "<p>Camara inicializada (SIN /stream). /data OK.</p></body></html>";

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

void startCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;   config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;   config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;   config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;   config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk  = XCLK_GPIO_NUM;
  config.pin_pclk  = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QQVGA;
  config.jpeg_quality = 18;
  config.fb_count     = psramFound() ? 2 : 1;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Error camara: 0x%x\n", err);
    camera_ok = false;
  } else {
    Serial.println("✅ Camara iniciada");
    camera_ok = true;
  }
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

  pinMode(4, OUTPUT); digitalWrite(4, LOW); // LED flash OFF

  Wire.begin(14, 15);
  Wire.setClock(100000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED no encontrada");
    while(true);
  }
  display.clearDisplay();
  display.display();

  dht.setup(DHTPIN, DHTesp::DHT11);

  delay(2000); // estabiliza AP
  startCamera();

  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.begin();

  Serial.println("🚀 Fase 4 lista (camara sin stream)");
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