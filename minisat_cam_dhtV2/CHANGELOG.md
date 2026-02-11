# MiniSat – CHANGELOG

## v2.0.0 (2026-02-10)
### Added
- Nueva arquitectura modular del proyecto.
- Carpeta `src/` con código estable final.
- Carpeta `examples/` con 5 fases de diagnóstico:
  - Fase 1: SoftAP mínimo.
  - Fase 2: SoftAP + DHT11.
  - Fase 3: DHT11 + OLED.
  - Fase 4: DHT11 + OLED + Cámara (sin streaming).
  - Fase 5: Sistema completo (streaming + dashboard).
- Carpeta `docs/` con guía completa del proyecto.
- Implementación segura del streaming MJPEG con guardas (`camera_ok`).
- Optimización de alimentación y reducción de picos (WIFI_POWER_2dBm, delays, sleep).
- Mejora en la estabilidad del sistema OLED + DHT + Cámara.

### Changed
- Reordenado el flujo de inicialización:
  1) WiFi
  2) Delay
  3) Cámara
  4) Rutas del servidor
- Streaming más estable con `frameDelayMs = 100`.
- Calidad JPEG ajustada según carga (`jpeg_quality = 18`).
- Dashboard reescrito y optimizado.

### Fixed
- Brownout causado por picos en WiFi + cámara.
- Guru Meditation (StoreProhibited) por punteros nulos en streaming.
- OLED bloqueándose al compartir bus I2C.

### Removed
- Código redundante del proyecto original no compatible con arquitectura actual.

---

## v1.0.0
Versión inicial del proyecto MiniSat.