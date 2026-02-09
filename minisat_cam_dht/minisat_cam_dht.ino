#include "esp_camera.h"
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WebServer.h>

/*************************************************
 * MiniSat (AI Thinker ESP32-CAM)
 * - SoftAP: SSID MiniSat / PASS 12345678
 * - Dashboard: http://192.168.4.1/
 * - Stream MJPEG: http://192.168.4.1/stream
 * - JSON: http://192.168.4.1/data
 *
 * Sensores activos:
 * - DHT11: Temperatura + Humedad (GPIO13)
 *
 * Presion:
 * - Constante de referencia (hPa) (puedes ajustarla)
 *
 * OLED:
 * - 128x64 SSD1306 I2C (0x3C)
 * - I2C en ESP32-CAM: SDA=GPIO14, SCL=GPIO15
 *************************************************/

// =====================
// WiFi AP
// =====================
const char* ssid = "MiniSat";
const char* pass = "12345678";
WebServer server(80);

// =====================
// DHT11
// =====================
#define DHTPIN 13
DHTesp dht;

// =====================
// OLED
// =====================
Adafruit_SSD1306 display(128, 64, &Wire);

// =====================
// Variables
// =====================
float tempC = NAN;
float hum = NAN;

// Presion de referencia (hPa)
const float PRESION_HPA = 773.3;

// =====================
// Pines cámara AI Thinker (OV2640)
// =====================
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

// =====================
// HTML
// =====================
String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>MiniSat Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { background:#000; color:#00ff88; font-family: monospace; text-align:center; margin:0; padding:12px; }
    h1 { color:#00ffaa; margin:10px 0 12px; }
    .wrap { display:flex; flex-wrap:wrap; justify-content:center; gap:10px; }
    .card { border:1px solid #00ff88; padding:10px 12px; border-radius:12px; min-width:170px; }
    .val { font-size:22px; color:#aaffdd; }
    .lbl { opacity:.85; }
    img { border:2px solid #00ff88; border-radius:12px; margin-top:12px; width: 92vw; max-width: 480px; height:auto; }
    .note { opacity:.7; font-size:12px; margin-top:8px; }
  </style>
</head>
<body>
  <h1>MiniSat Telemetria</h1>

  <div class="wrap">
    <div class="card"><div class="lbl">Temperatura</div><div class="val" id="temp">--</div></div>
    <div class="card"><div class="lbl">Humedad</div><div class="val" id="hum">--</div></div>
    <div class="card"><div class="lbl">Presion</div><div class="val" id="pres">--</div></div>
  </div>

  <img src="/stream" id="cam" />

  <div class="note">Si el video se traba: sube el delay del stream (frameDelayMs) o sube jpeg_quality en el codigo.</div>

  <script>
    async function updateData(){
      try{
        const r = await fetch('/data', {cache:'no-store'});
        const d = await r.json();
        document.getElementById('temp').textContent = d.temp.toFixed(1) + ' °C';
        document.getElementById('hum').textContent  = d.hum.toFixed(0) + ' %';
        document.getElementById('pres').textContent = d.pres.toFixed(1) + ' hPa';
      }catch(e){}
    }
    setInterval(updateData, 1000);
    updateData();
  </script>
</body>
</html>
)rawliteral";

// =====================
// Web handlers
// =====================
void handleRoot() {
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "text/html", html);
}

void handleData() {
  float t = tempC;
  float h = hum;
  if (isnan(t)) t = 0.0;
  if (isnan(h)) h = 0.0;

  String json = "{";
  json += "\"temp\":" + String(t, 2) + ",";
  json += "\"hum\":"  + String(h, 2) + ",";
  json += "\"pres\":" + String(PRESION_HPA, 1);
  json += "}";

  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.send(200, "application/json", json);
}

// =====================
// MJPEG Stream
// =====================
void handleStream() {
  WiFiClient client = server.client();

  client.print(
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
    "Cache-Control: no-store, no-cache, must-revalidate, max-age=0\r\n"
    "Pragma: no-cache\r\n"
    "Connection: close\r\n\r\n"
  );

  // Ajusta FPS/consumo (mayor = mas estable)
  const int frameDelayMs = 60; // 45-120 segun estabilidad

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      delay(10);
      continue;
    }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write((const uint8_t*)fb->buf, fb->len);
    client.print("\r\n");

    esp_camera_fb_return(fb);

    delay(frameDelayMs);
    yield();
  }
}

// =====================
// Camera init
// =====================
void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

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

  // Config estable
  config.frame_size   = FRAMESIZE_QQVGA; // 160x120
  config.jpeg_quality = 16;              // 14-20
  config.fb_count     = psramFound() ? 2 : 1;
  config.grab_mode    = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Error al iniciar camara: 0x%x\n", err);
  } else {
    Serial.println("✅ Camara iniciada");
  }
}

// =====================
// OLED render
// =====================
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0,0);

  display.println("MiniSAT");
  display.println("Instituto Cumbres");
  display.println("Bosques");
  display.println("");

  display.print("T: ");
  if (isnan(tempC)) display.print("--");
  else display.print(tempC, 1);
  display.println(" C");

  display.print("H: ");
  if (isnan(hum)) display.print("--");
  else display.print(hum, 0);
  display.println(" %");

  display.print("P: ");
  display.print(PRESION_HPA, 1);
  display.println(" hPa");

  display.display();
}

// =====================
// Setup / Loop
// =====================
unsigned long lastRead = 0;

void setup() {
  Serial.begin(115200);
  delay(200);

  // I2C OLED
  Wire.begin(14, 15);
  Wire.setClock(100000);

  // DHT
  dht.setup(DHTPIN, DHTesp::DHT11);

  // OLED (0x3C)
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED no encontrada");
    while(true);
  }
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  display.display();
  updateOLED();

  // Camara
  startCamera();

  // WiFi AP
  WiFi.softAP(ssid, pass);
  WiFi.setTxPower(WIFI_POWER_2dBm); // 2-4m suficiente, menos picos

  Serial.print("AP: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  // Rutas
  server.on("/", handleRoot);
  server.on("/data", handleData);
  server.on("/stream", HTTP_GET, handleStream);
  server.begin();

  Serial.println("🚀 MiniSat listo!");
}

void loop() {
  server.handleClient();

  // DHT11 mejor cada 1500 ms
  if (millis() - lastRead > 1500) {
    lastRead = millis();

    TempAndHumidity d = dht.getTempAndHumidity();
    // Actualiza solo si es valido
    if (!isnan(d.temperature) && d.temperature > -20 && d.temperature < 60) {
      tempC = d.temperature;
    }
    if (!isnan(d.humidity) && d.humidity >= 0 && d.humidity <= 100) {
      hum = d.humidity;
    }

    updateOLED();
  }
}
