# PN532 NFC-Treiber

STM32-HAL-Treiber für ein PN532-NFC-Modul im I2C-Modus. Der aktuelle
Funktionsumfang erkennt eine passive ISO/IEC-14443-Type-A-Karte und liest deren
UID, SENS_RES/ATQA und SEL_RES/SAK.

Der eigentliche Treibercode in `pn532.c` und `pn532.h` wurde nicht verändert.

## Hardware und Modulkonfiguration

Beim Elechouse PN532 NFC Module V3 müssen die Schalter für I2C so stehen:

| Schalter | Stellung |
|---|---|
| Channel 1 | ON |
| Channel 2 | OFF |

Anschlüsse:

| PN532 | STM32 |
|---|---|
| `VCC` | Versorgung gemäß Modul |
| `GND` | GND |
| `SDA` | I2C SDA |
| `SCL` | I2C SCL |

Das V3-Modul besitzt laut Modulhandbuch einen Pegelwandler für I2C. Bei anderen
PN532-Boards müssen Versorgung und Logikpegel separat geprüft werden.

## CubeMX

- Gewünschte I2C-Schnittstelle aktivieren.
- Standard Mode mit 100 kHz verwenden; der PN532 unterstützt auch bis 400 kHz.
- Falls CubeMX einen anderen Handle als `hi2c1` erzeugt, den Beispielcode
  entsprechend anpassen.
- Für Polling ist kein IRQ-Pin erforderlich.

## Einbindung

1. `pn532.c` nach `Core/Src` kopieren.
2. `pn532.h` nach `Core/Inc` kopieren.
3. Den Ablauf aus [`example/main.c`](example/main.c) übernehmen.

## Typischer Ablauf

```c
PN532_Handle nfc;
PN532_FirmwareInfo firmware;
PN532_Target target;

PN532_Init(&nfc, &hi2c1);
PN532_GetFirmwareVersion(&nfc, &firmware);
PN532_ConfigureReader(&nfc);

if (PN532_ReadPassiveTarget(&nfc, &target) == PN532_OK)
{
  /* target.uid[0 .. target.uid_length-1] enthält die UID. */
}
```

## API

| Funktion | Beschreibung |
|---|---|
| `PN532_Init()` | Handle initialisieren, Altkommando abbrechen und I2C prüfen |
| `PN532_GetFirmwareVersion()` | IC-, Versions-, Revisions- und Supportbyte lesen |
| `PN532_SAMConfigure()` | PN532 in den normalen SAM-Modus versetzen |
| `PN532_ConfigureReader()` | SAM und Type-A-Lesebetrieb konfigurieren |
| `PN532_ReadPassiveTarget()` | Eine Type-A-Karte suchen und UID-Daten lesen |

`PN532_ReadPassiveTarget()` liefert `PN532_NO_TARGET`, wenn innerhalb der
konfigurierten Versuche keine Karte erkannt wurde. Ein erfolgreicher Aufruf
füllt `PN532_Target` mit Zielnummer, `sens_res`, `sel_res`, UID-Länge und UID.

## Statuscodes

| Status | Bedeutung |
|---|---|
| `PN532_OK` | Operation erfolgreich |
| `PN532_NO_TARGET` | Keine Karte erkannt |
| `PN532_ERROR_ARGUMENT` | Ungültiger Parameter |
| `PN532_ERROR_I2C` | I2C-Kommunikationsfehler |
| `PN532_ERROR_TIMEOUT` | PN532 wurde nicht rechtzeitig bereit |
| `PN532_ERROR_ACK` | Ungültiger ACK-Frame |
| `PN532_ERROR_FRAME` | Fehlerhafter Antwortframe oder Checksumme |
| `PN532_ERROR_RESPONSE` | Inhalt der Antwort ist nicht plausibel |

## Grenzen des aktuellen Treibers

- ausschließlich I2C
- eine passive ISO/IEC-14443-Type-A-Karte
- UID-Erkennung, aber noch kein MIFARE-Datenaustausch oder NDEF
- Polling statt IRQ-gesteuerter Kommunikation

## Dokumentation

- [`doc/141520.pdf`](doc/141520.pdf): NXP PN532 User Manual
- [`doc/PN532_ Manual_V3.pdf`](doc/PN532_%20Manual_V3.pdf): Elechouse Modulhandbuch
