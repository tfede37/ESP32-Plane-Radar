# Firmware precompilato — 0.1alfa

Branch di soli binari, separato dal codice: serve a rendere scaricabili le
immagini della pre-release [`0.1alfa`](https://github.com/tfede37/ESP32-Plane-Radar/releases/tag/0.1alfa),
i cui asset non sono stati caricati sulla release.

Codice sorgente corrispondente: branch
[`TurboTime29/waveshare-lcd-2.8c`](https://github.com/tfede37/ESP32-Plane-Radar/tree/TurboTime29/waveshare-lcd-2.8c),
commit `bc80c71`.

## Download

| File | Board | Offset |
|------|-------|--------|
| [`plane-radar-2.8c-0.1alfa.bin`](https://github.com/tfede37/ESP32-Plane-Radar/raw/firmware-bin/plane-radar-2.8c-0.1alfa.bin) | Waveshare ESP32-S3-Touch-LCD-**2.8C** (480×480) | `0x0` |
| [`plane-radar-1.28-0.1alfa.bin`](https://github.com/tfede37/ESP32-Plane-Radar/raw/firmware-bin/plane-radar-1.28-0.1alfa.bin) | Waveshare ESP32-S3-Touch-LCD-**1.28** (240×240) | `0x0` |

Immagini già unite (bootloader + tabella partizioni + boot_app0 + applicazione):
un solo file a `0x0` è sufficiente. I file `.sha256` servono a verificare il download.

```bash
sha256sum -c plane-radar-2.8c-0.1alfa.bin.sha256
```

## Come caricarlo

Collega il cavo alla porta Type-C marcata **USB TO UART** (chip CH343P: ha il
circuito di download automatico ed è la porta dei log seriali a 115200 baud).

**Da browser** — [ESP Web Flasher](https://espressif.github.io/esptool-js/):
chip **ESP32-S3**, file a offset **`0x0`**, *Program*.

**Da riga di comando**:

```bash
pip install esptool
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 \
           write_flash 0x0 plane-radar-2.8c-0.1alfa.bin
```

Se la porta non viene rilevata: tieni premuto **BOOT**, premi e rilascia
**RESET**, rilascia **BOOT**, poi riprova.

## Primo avvio

1. Connettiti alla rete Wi-Fi **`PlaneRadar-Setup`**
2. Apri `http://plane-radar.local` (o `http://192.168.4.1`)
3. Inserisci Wi-Fi, latitudine e longitudine

Tap sullo schermo: alterna radar e meteo. **BOOT** breve: scala 5/10/15/25 km.
**BOOT** premuto 3 s: cancella Wi-Fi e impostazioni.

> La build per la 2.8C non è ancora stata validata su hardware fisico.
> Se rosso e blu appaiono invertiti o compare tearing, i parametri sono isolati
> in `include/boards/waveshare_lcd_2_8c.h` (`kRgbSwapRedBlue`, timing RGB).
