#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "MiniSat";
const char* pass = "12345678";
WebServer server(80);

void handleRoot() {
  server.send(200, "text/plain", "MiniSat AP OK (Fase 1)");
}

void setup() {
  Serial.begin(115200);
  delay(200);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.setSleep(true);
  WiFi.softAP(ssid, pass, 1 /*canal*/, 0 /*visible*/, 2 /*max conn*/);
  WiFi.setTxPower(WIFI_POWER_2dBm);

  Serial.print("AP: "); Serial.println(ssid);
  Serial.print("IP: "); Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.begin();

  Serial.println("🚀 Fase 1 lista (SoftAP mínimo)");
}

void loop() {
  server.handleClient();
}
