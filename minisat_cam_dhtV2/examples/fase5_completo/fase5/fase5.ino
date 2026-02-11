// ===== PASO 5: Proyecto completo con /stream seguro =====
#include <WiFi.h>
#include <WebServer.h>
#include <DHTesp.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include "esp_camera.h"

const char* ssid = "MiniSat";
const char* pass = "12345678";
WebServer server(80);

// ------ DHT11 ------
#define DHTPIN 13
DHTesp dht;
float tempC = NAN, hum = NAN;
unsigned long lastRead = 0;

// ------ OLED ------
Adafruit_SSD1306 display(128, 64, &Wire);
const float PRESION_HPA = 773.3;

// ------ Camara ------
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

  config.frame_size   = FRAMESIZE_QQVGA; // 160x120
  config.jpeg_quality = 18;              // 16-20 (18 es conservador)
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

String html() {
  return String(F(
    "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
    "<title>MiniSat Dashboard</title>"
    "<style>body{background:#000;color:#0f8;font-family:monospace;text-align:center;margin:0;padding:12px}"
    "h1{color:#0fa;margin:10px 0 12px}.wrap{display:flex;flex-wrap:wrap;justify-content:center;gap:10px}"
    ".card{border:1px solid #0f8;padding:10px 12px;border-radius:12px;min-width:170px}"
    ".val{font-size:22px;color:#adf}.lbl{opacity:.85}"
    "img{border:2px solid #0f8;border-radius:12px;margin-top:12px;width:92vw;max-width:480px;height:auto}"
    ".note{opacity:.7;font-size:12px;margin-top:8px}</style></head><body>"
    "<h1>MiniSat Telemetria</h1>"
    "<div class='wrap'>"
    "<div class='card'><div class='lbl'>Temperatura</div><div class='val' id='temp'>--</div></div>"
    "<div class='card'><div class='lbl'>Humedad</div><div class='val' id='hum'>--</div></div>"
    "<div class='card'><div class='lbl'>Presion</div><div class='val' id='pres'>--</div></div>"
    "</div>"
    "<img src='/stream' id='cam'/>"
    "<div class='note'>Si el video se traba: sube frameDelayMs o sube jpeg_quality en el codigo.</div>"
    "<script>"
    "async function updateData(){try{const r=await fetch('/data',{cache:'no-store'});"
    "const d=await r.json();"
    "document.getElementById('temp').textContent=d.temp.toFixed(1)+' °C';"
    "document.getElementById('hum').textContent=d.hum.toFixed(0)+' %';"
    "document.getElementById('pres').textContent=d.pres.toFixed(1)+' hPa';}catch(e){}}"
    "setInterval(updateData,1000);updateData();</script>"
    "</body></html>"
  ));
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", html());
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

void handleStream() {
  if (!camera_ok) {
    server.send(503, "text/plain", "Camera not ready");
    return;
  }

  WiFiClient client = server.client();
  client.print(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
    "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
    "Pragma: no-cache\r\n"
    "Connection: close\r\n\r\n"
  );

  const int frameDelayMs = 100; // súbelo si notas inestabilidad (120-150)
  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) { delay(20); yield(); continue; }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write((const uint8_t*)fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);
    delay(frameDelayMs);
    yield();
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(true);
  WiFi.softAP(ssid, pass, 1, 0, 2);
  WiFi.setTxPower(WIFI_POWER_2dBm);

  Serial.print("AP: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  // Asegura LED flash apagado (evita picos)
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);

  // OLED
  Wire.begin(14, 15);
  Wire.setClock(100000);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED no encontrada");
    while(true);
  }
  display.clearDisplay();
  display.display();

  // DHT
  dht.setup(DHTPIN, DHTesp::DHT11);

  // Estabiliza AP antes de cámara
  delay(2000);

  // Cámara
  startCamera();

  // Rutas
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/stream", HTTP_GET, handleStream);
  server.begin();

  Serial.println("🚀 Proyecto completo listo (con /stream)");
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
