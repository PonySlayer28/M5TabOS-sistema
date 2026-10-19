# Flashing M5TabOS (English) to Tab5

**Device:** M5Stack Tab5, **ESP32-P4 Revision v1.3** (verified on 18.10.2026).
The ready-made firmware in `firmware-p4-chip_v1/` is built specifically for this device.

## Directory Structure

| Folder | Contents |
|---|---|
| `firmware-p4-chip_v1/` | Main firmware (P4): bootloader, partition table, `M5tabOS.bin`, `flash_args` |
| `firmware-c6/` | Firmware for ESP32-C6 radio chip (optional, see Section 5) |
| `M5TabOS-sistema/` | Source code, `english` branch |

---

## 1. Setup (once per terminal)

```bash
export IDF_PATH=~/esp/esp-idf-v5.5.6
source $IDF_PATH/export.sh
```

Connect Tab5 via USB-C and power it on. Verify the device is detected:

```bash
ls /dev/ttyACM*        # expected: /dev/ttyACM0
```

**No `/dev/ttyACM0`?** Try a different USB cable (some can only charge).  
**No access?** Your user is already in the `dialout` group (pre-configured).

**Important:** No other program may have the serial port open (serial monitor, `screen`, Arduino IDE, etc.), or flashing will fail.

## 2. (Recommended) Backup Current Firmware

Save the complete 16 MB flash so you can restore it later:

```bash
cd ~/Desktop/M5TabOS-english
python -m esptool --chip esp32p4 -p /dev/ttyACM0 -b 460800 read_flash 0 0x1000000 backup-tab5-before.bin
```

To restore later:

```bash
python -m esptool --chip esp32p4 -p /dev/ttyACM0 -b 460800 write_flash 0 backup-tab5-before.bin
```

## 3. Flash Main Firmware

### Option A: Pre-built binaries (fast, no compilation)

```bash
cd ~/Desktop/M5TabOS-english/firmware-p4-chip_v1
python -m esptool --chip esp32p4 -p /dev/ttyACM0 -b 460800 \
  --before default_reset --after hard_reset \
  write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m @flash_args
```

At the end, you should see `Hash of data verified.` and `Hard resetting via RTS pin...`
The Tab5 will then reboot with the English system.

### Option B: Using the build script (builds and opens monitor afterward)

```bash
cd ~/Desktop/M5TabOS-english/M5TabOS-sistema
export IDF_PATH=~/esp/esp-idf-v5.5.6
CHIP=v1 ./gravar.sh
```

- The first build downloads components from the internet and takes a few minutes.
- Specify `CHIP=v1`: The automatic detection in the script uses `chip-id` (esptool v5), but your IDF has esptool v4 (`chip_id`). Without this setting, the script also defaults to v1, which is correct for your device.
- The serial monitor opens afterward. Exit with **Ctrl + ]**.

### Connection Issues? (Download Mode)

If you see `Failed to connect` / `No serial data received`:  
Put Tab5 in download mode manually (per M5Stack documentation): Hold the reset button for ~2 seconds until the green LED blinks rapidly, then run the command again.  
After flashing, press reset briefly once.

## 4. Verification

Open the serial monitor:

```bash
cd ~/Desktop/M5TabOS-english/M5TabOS-sistema
idf.py -p /dev/ttyACM0 monitor        # exit with Ctrl + ]
```

Type `help`: The help text should be in English. On the screen, app names are now *Settings, Files, Power, Tasks, Example, Mesh, ESP-NOW, Camera, IMU*.

**Note:** The **console command keywords** are intentionally kept in Portuguese (`lista`, `abrir`, `receber`, `apagar`, ...) — only their descriptions are in English.

## 5. (Optional) Update ESP32-C6 Radio Chip

Only necessary if the C6 doesn't have M5TabOS firmware yet (missing ESP-NOW/Bluetooth) or if you want C6 logs in English. The main firmware works fine without this.

The C6 has no dedicated USB port. The easiest way is **through Tab5 itself** (enable WiFi in *Settings*):

1. Copy `firmware-c6/tab5_c6.bin` to the SD card at `apps/Sistema/tab5_c6.bin` (using a card reader on your PC), then reinsert the card into Tab5 — **or** send it via cable (close monitor first):

   ```bash
   cd ~/Desktop/M5TabOS-english/firmware-c6
   SIZE=$(stat -c %s tab5_c6.bin)
   stty -F /dev/ttyACM0 raw -echo
   { printf "receber Sistema tab5_c6.bin $SIZE\r"; sleep 2; cat tab5_c6.bin; } > /dev/ttyACM0
   ```

2. In the serial monitor, type:

   ```
   c6 gravar sim
   ```

   The C6 and then Tab5 will reboot automatically. **Do not power off during this process.** If the new C6 firmware is not confirmed, the C6 will automatically fall back to the previous version.

3. Verify in the monitor: `farejo info`

**Alternative with external programmer** (Tab5 **powered off**, programmer connected to C6's programming port): `M5TabOS-sistema/c6/gravar.sh` — see `M5TabOS-sistema/RADIO_C6.md` and `README.md` for details.

## 6. Known Limitations & Notes

### Partial Translation
Not all strings in the firmware could be translated to English. This is ongoing work. Items that remain in Portuguese or partially translated:

- **External documentation links** — Often point to Portuguese resources or hardware datasheets
- **Hardware-specific register names** — Kept as labeled by manufacturers
- **CLI command keywords** (`lista`, `abrir`, `receber`, `apagar`, `enviar`, etc.) — Retained for backward compatibility with existing scripts and documentation
- **Some UI strings** — A few button labels and menu items may still show in Portuguese due to display constraints

### Display & Layout
- SD card folders for renamed apps are now in English (`apps/Settings`, `apps/Tasks`, `apps/Example`, ...). Data from old Portuguese folders will no longer be found automatically.
- Some English text strings are longer than before and may be cut off at the edges (help pages in *Settings*, "ADD FAVORITE" in *Mesh*).

### Building From Source
To rebuild after code changes: use Option B, or run `idf.py build` in the `M5TabOS-sistema` folder (after `source ambiente.sh`).

---

## Translation Status

**Last Updated:** October 18, 2026  
**Status:** 🟡 **In Progress** — Main UI translated, CLI and some hardware labels remain in Portuguese by design.

For contributions or additional translations, see `M5TabOS-sistema/` source on GitHub.
