# BMP280/BME280 Temperatur- und Luftdrucktreiber

STM32-HAL-Treiber für die Temperatur- und Luftdruckmessung mit einem Bosch
BMP280 oder BME280 über I2C. Beim BME280 wird die Luftfeuchtigkeit nicht
ausgewertet.

## Hardware und CubeMX

- Sensorversorgung entsprechend dem verwendeten Breakout-Board anschließen.
- `SDA` und `SCL` mit einer I2C-Schnittstelle des STM32 verbinden.
- Gemeinsame Masse zwischen STM32 und Sensor herstellen.
- I2C in STM32CubeMX aktivieren; Standard Mode mit 100 kHz ist ausreichend.
- Die Sensoradresse wird automatisch als `0x76` oder `0x77` erkannt.

> Die meisten nackten Sensoren arbeiten mit 3,3 V. Ob ein Breakout-Board 5 V
> verträgt, hängt von dessen Spannungsregler und Pegelwandler ab.

## Einbindung

1. `bmp280.c` nach `Core/Src` kopieren.
2. `bmp280.h` nach `Core/Inc` kopieren.
3. Den Inhalt aus [`example/main.c`](example/main.c) in das CubeMX-Projekt
   übernehmen und den verwendeten I2C-Handle anpassen.

## Typischer Ablauf

```c
bmp280_t sensor;

if (bmp280_init(&hi2c1, &sensor) == BMP280_OK)
{
  if (bmp280_read_all(&sensor) == BMP280_OK)
  {
    float temperature_c = sensor.temperature;
    float pressure_hpa = sensor.pressure_hpa;
  }
}
```

`bmp280_read_all()` liest Druck und Temperatur in einem gemeinsamen Burst. So
stammen beide Werte sicher aus demselben Messzyklus. Auch
`bmp280_read_pressure()` aktualisiert deshalb beide Ergebnisfelder.

## API

| Funktion | Beschreibung |
|---|---|
| `bmp280_init()` | Sensor suchen, Chip-ID prüfen, zurücksetzen und konfigurieren |
| `bmp280_read_temperature()` | Nur Temperatur lesen und kompensieren |
| `bmp280_read_pressure()` | Kohärentes Temperatur-/Drucksample lesen |
| `bmp280_read_all()` | Temperatur und Luftdruck gemeinsam aktualisieren |

Ergebnisse stehen anschließend in:

- `sensor.temperature` in Grad Celsius
- `sensor.pressure_hpa` in hPa
- `sensor.chip_id`: `0x58` für BMP280 oder `0x60` für BME280

## Statuscodes

| Status | Bedeutung |
|---|---|
| `BMP280_OK` | Operation erfolgreich |
| `BMP280_ERROR_ARGUMENT` | Ungültiger Zeiger oder nicht initialisiertes Handle |
| `BMP280_ERROR_NOT_FOUND` | Kein Gerät auf `0x76` oder `0x77` gefunden |
| `BMP280_ERROR_CHIP_ID` | Gerät antwortet, ist aber kein unterstützter Sensor |
| `BMP280_ERROR_I2C` | Fehler bei der I2C-Übertragung |
| `BMP280_ERROR_TIMEOUT` | NVM-Kopiervorgang nach Reset nicht beendet |
| `BMP280_ERROR_CALIBRATION` | Ungültige Kalibrierdaten |
| `BMP280_ERROR_DATA` | Messkanal deaktiviert oder Rohwert ungültig |

## Konfiguration

Der Treiber verwendet Temperatur- und Druck-Oversampling x1, Normal Mode,
1000 ms Standby-Zeit und einen deaktivierten IIR-Filter. Die Kompensation
entspricht den Integerformeln des Bosch-Datenblatts.

## Dokumentation

Das BME280-Datenblatt liegt unter [`doc/bst-bme280-ds002.pdf`](doc/bst-bme280-ds002.pdf).
Es gilt auch für die gemeinsam verwendeten Temperatur-/Druckregister und
Kompensationsformeln. Die Doxygen-Kommentare befinden sich direkt in
`bmp280.h` und `bmp280.c`.
