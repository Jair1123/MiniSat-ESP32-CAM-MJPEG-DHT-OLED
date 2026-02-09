# MiniSat ESP32-CAM (MJPEG) + DHT11 + OLED

Proyecto MiniSat para **AI Thinker ESP32-CAM**:

- 📡 El módulo crea una red WiFi (SoftAP) llamada **MiniSat**.
- 🌐 Dashboard web sin recargar: `http://192.168.4.1/`
- 🎥 Video MJPEG en vivo: `http://192.168.4.1/stream`
- 📦 Telemetría JSON: `http://192.168.4.1/data`

## Sensores activos
- **DHT11** (temperatura y humedad) en **GPIO13**.
- **OLED SSD1306 I2C** (0x3C) en **SDA=GPIO14** y **SCL=GPIO15**.

## Presión (valor de referencia)
El sistema muestra una presión **constante** como referencia: **773.3 hPa**.
Este valor corresponde a convertir un ejemplo típico reportado en CDMX (~580 mmHg) a hPa.
Puedes cambiarlo editando `PRESION_HPA` en el código.

## Conexiones
### OLED SSD1306 (I2C)
- VCC → 3.3V
- GND → GND
- SDA → GPIO14
- SCL → GPIO15

### DHT11
- VCC → 3.3V
- GND → GND
- DATA → GPIO13
- (Recomendado) Pull-up 10k entre DATA y 3.3V si el sensor no es módulo.

## Alimentación (IMPORTANTE)
- Usa **UNA sola fuente** a la vez. No combines USB y 3.3V externo sin aislamiento.
- Para estabilidad, capacitores cerca del módulo:
  - 1000µF + 100nF en 5V-GND (si alimentas por 5V/USB)
  - 1000µF + 100nF en 3V3-GND

## Arduino IDE
- Placa: **AI Thinker ESP32-CAM**
- Librerías:
  - Adafruit SSD1306
  - Adafruit GFX
  - DHTesp

## Uso
1. Conéctate a la WiFi **MiniSat** (password `12345678`).
2. Abre `http://192.168.4.1/`.

## Licencia
MIT
