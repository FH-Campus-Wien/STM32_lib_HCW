# STM32 Library HCW

Treiber und Lehrbeispiele für STM32-Mikrocontroller mit STM32Cube HAL.

**Ersteller:** FH Prof. Herbert Paulis

## Enthaltene Treiber

| Verzeichnis | Baustein | Schnittstelle | Funktion |
|---|---|---|---|
| @ref bmp280_driver "bmp280_temp" | BMP280/BME280 | I2C | Temperatur und Luftdruck |
| @ref pn532_driver "pn532_nfc" | PN532 | I2C | NFC/RFID Type A erkennen und UID lesen |

Jeder Treiber enthält seine Quelldateien, eine eigene README, ein minimales
STM32Cube-HAL-Beispiel und die zugehörigen Herstellerunterlagen.

## Doxygen-Dokumentation

Die Kommentare in den Header- und C-Dateien verwenden das Doxygen-Format. Mit
einer lokalen Doxygen-Installation wird die HTML-Dokumentation so erzeugt:

```sh
doxygen Doxyfile
```

Die Startseite liegt danach unter `doc/generated/html/index.html` und kann auf
macOS direkt geöffnet werden:

```sh
open doc/generated/html/index.html
```

Es wird bewusst kein TeX/LaTeX benötigt. Falls zusätzlich ein PDF der
Doxygen-Ausgabe gewünscht ist, kann die HTML-Dokumentation im Browser über
`Ablage > Drucken > Als PDF sichern` exportiert werden. Eine bereits erzeugte,
kompakte PDF-Fassung befindet sich unter `doc/STM32_Library_HCW_API.pdf`.

## Hinweise

- Die Beispiele setzen ein mit STM32CubeMX erzeugtes Projekt voraus.
- I2C-Handle und Instanz können an das eigene Projekt angepasst werden.
- Die Treiber und Beispiele werden ohne Gewähr bereitgestellt. Vor dem Einsatz
  müssen Pinbelegung, Versorgungsspannung und Buskonfiguration geprüft werden.
