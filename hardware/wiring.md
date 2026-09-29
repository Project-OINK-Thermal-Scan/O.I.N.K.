# O.I.N.K. — Edge Node Wiring

Solderless build guide for the ESP32 capture node that feeds the TIRPigEar
detector. Assumes a classic **ESP32 DevKit (WROOM-32)** and 3.3 V logic.

> Board varies: on **ESP32-S3 / C3** the default I2C pins differ. Any GPIO works
> if you pass them to `Wire.begin(sda, scl)` — the pin numbers here are for the
> classic DevKit V1.

---

## 1. Pin map (whole node)

| Signal | ESP32 GPIO | Rail |
|---|---|---|
| MLX90640 SDA | GPIO21 | 3.3 V logic |
| MLX90640 SCL | GPIO22 | 3.3 V logic |
| MQ-135 AO (after divider) | GPIO34 (ADC1) | analog |
| Sound module OUT (3-pin) | GPIO27 | digital |
| INMP441 SCK (BCLK) | GPIO26 | 3.3 V logic |
| INMP441 WS (LRCLK) | GPIO25 | 3.3 V logic |
| INMP441 SD (DATA) | GPIO33 | 3.3 V logic |
| DHT22 `out` (DATA) | GPIO4 | 3.3 V logic |

**Reserved / unavailable:**
- GPIO 6–11 — SPI flash, never touch
- GPIO 34–39 — input-only (fine as ADC); ADC2 pins unusable while Wi-Fi is on
- GPIO 0/2/12/15 — strapping pins, avoid

Only **ADC1** pins are used for analog (GPIO34), because O.I.N.K. needs Wi-Fi.

**GPIO4** (DHT22) is a normal GPIO — not input-only and not a strapping pin, so
it is safe here. The DHT22 is a **digital** sensor, so it does not need an ADC.

---

## 2. Power rails

| Rail | Source | Loads |
|---|---|---|
| **5 V** (`VIN`/`5V`) | USB via data cable | MQ-135 heater only (~150 mA) |
| **3.3 V** (`3V3`) | ESP32 regulator | Camera, DHT22, sound module, INMP441, ESP32 core |

Budget note: USB gives ~500 mA. ESP32 Wi-Fi bursts (up to ~500 mA peaks) +
MQ-135 heater (150 mA) + camera (~25 mA) can be tight. Use a solid supply and
add a **100 µF** cap across 3.3 V/GND if you see brownouts.

### 3.3 V distribution — the rule that bites mid-build

**Every sensor's `VCC` / `+` / `VDD` goes to 3V3, except the MQ-135.** If a
sensor's data line is pulled up to its own supply — as the DHT22 and the 3-pin
sound module both are — then powering that sensor from 5 V puts **5 V on an
ESP32 GPIO**, which is out of spec (pins are rated to ~3.6 V). It often *looks*
like it works, because the 10 kΩ pull-up limits the current, but the pin is
being degraded.

The DevKit breaks out only **one** 3V3 pin, which is why it jams up. Current is
**not** the limit — the regulator handles ~800 mA and these sensors draw a few
tens of mA together. It is purely a **connector count** problem:

| Option | Notes |
|---|---|
| **Breadboard** ✅ best | Put the ESP32 on it; the power rails give unlimited 3V3/GND taps. Fixes this for the whole build. |
| Multiple wires in one pin | A dupont female takes 2–3 wires at once — works, but easy to knock loose. |
| Dupont Y-splitter (1→2 or 1→3) | Cheap and tidy off the 3V3 pin. |
| Header strip soldered as a bus | Permanent, needs a little soldering. |

```
   3V3  ──┬── MLX90640 red
          ├── DHT22  +
          ├── Sound  VCC
          ├── INMP441 VDD
          └── (future sensors)

   5V   ──┴── MQ-135 VCC   (this one ONLY)

   GND  ── every sensor's GND / -
```

Test after moving a sensor from 5 V to 3V3: it should read sensibly. Nonsense
readings or an ESP32 reboot means a 5 V data line is still in play.

---

## 3. MLX90640 — thermal camera (solderless, via Qwiic)

Use the **STEMMA QT / Qwiic socket on the back** of the camera. Do **not** solder
the 5-pin 0.1" header; leave those holes and the `3Vo` pin unused.

**Cable:** JST-SH, 4-pin, **1.0 mm** pitch (Qwiic/STEMMA QT) → female sockets.
⚠️ Not JST-PH (2.0 mm) — it will not fit.

| Qwiic wire | Signal | ESP32 |
|---|---|---|
| Black | GND | GND |
| Red | VCC (3.3 V) | **3V3** (not 5 V) |
| Blue | SDA | GPIO21 |
| Yellow | SCL | GPIO22 |

```
   MLX90640 [JST socket] --> Qwiic cable --> ESP32 pins
        black -> GND     red -> 3V3     blue -> GPIO21     yellow -> GPIO22
```

- I2C address: **0x33**
- The socket is keyed — it only inserts one way. Do not force it.
- Power off while plugging in. Feed power from **one** source only.
- On-board 4.7 kΩ pull-ups and level shifters are already fitted.

---

## 4. MQ-135 — air quality (NH3 / general)

| Pin | Goes to |
|---|---|
| VCC | **5 V** (`VIN`) — the heater needs it |
| GND | GND |
| AO | 10k/10k divider → **GPIO34** |
| DO | leave unconnected (5 V logic + binary only) |

```
   MQ AO ──[10k]──┬── ESP32 GPIO34 (ADC1)
                  [10k]
                   └── GND
```

- AO swings up to ~5 V → divider is required (halves it to 2.5 V max).
- **Burn-in 24–48 h** continuously before readings are valid.
- Warm-up ~2–3 min after each power-on; report "warming up" until then.
- Target gas curve (NH3): `ppm = 102.2 * (Rs/R0)^-2.473`, `R0 = Rs_clean / 3.6`.

---

## 5. Sound module — 3-pin digital (VCC / GND / OUT)

| Pin | Goes to |
|---|---|
| VCC | **3V3** (keeps OUT at a safe 3.3 V — do not use 5 V) |
| GND | GND |
| OUT | **GPIO27** (digital input) |

- **No divider needed** (digital output at 3.3 V).
- Set the trimpot: quiet room → `DOUT LED` off; clap → LED on.
- Provides a **noise event only** — no loudness, no sound content.
- Add 10 µF + 100 nF across VCC/GND if it false-triggers.

---

## 6. INMP441 — I2S microphone (audio / classification)

| INMP441 | ESP32 |
|---|---|
| VDD | 3V3 |
| GND | GND |
| **L/R** | **GND** (selects LEFT channel — must not float) |
| SCK (BCLK) | GPIO26 |
| WS (LRCLK) | GPIO25 |
| SD (DATA) | GPIO33 |

- 3.3 V only. Add a 100 nF decoupling cap near the mic.
- Keep the mic a few cm from the ESP32 antenna (Wi-Fi RF couples into it).
- Capture format: **16 kHz, 32-bit slots, LEFT channel** (24-bit data left-justified).

---

## 7. DHT22 (AM2302) — temperature + humidity

The module is the black PCB with the white grille and a 3-pin header labelled
`+ / out / -`.

| Pin | Goes to | Why |
|---|---|---|
| `+` | **3V3** — never 5 V | the module's pull-up ties `out` to `VCC` (see below) |
| `out` | **GPIO4** | normal GPIO — not 34–39, which are input-only |
| `-` | GND | common ground with the ESP32 |

```
   DHT22:   +  ──> 3V3        out  ──> GPIO4        -  ──> GND
```

⚠️ **Never power this module from 5 V.** Its on-board 10 kΩ pull-up ties the
data line to `VCC`, so at 5 V `out` idles at **5 V** and drives that straight
into GPIO4. The same applies to the **3-pin sound module**, whose `OUT` also
follows `VCC` — both must be on 3V3. If 3V3 is "full", distribute it (see
**§2 → 3.3 V distribution**) rather than moving the sensor to 5 V.

- Power off while wiring, and feed the module from **one** source only.
- **≥ 2 s between reads.** Faster polling returns stale or failed readings.
- The **first reading after power-up is usually bad** — discard it.
- Keep the run under ~1 m; add 100 nF across VCC/GND if readings are jittery.
- The ESP32 just reads it; humidity is stored/forwarded alongside temperature.

**Firmware:** `firmware/dht22_test/` (PlatformIO, `board = esp32dev`,
`lib_deps = beegee-tokyo/DHT sensor library for ESPx`, include `DHTesp.h`).

```bash
cd "/home/joal/Projects/O.I.N.K. - dataset/firmware/dht22_test"
pio run              # build
pio run -t upload    # flash
pio device monitor   # 115200 baud
```

Expected output every 2 s:

```
Temp 31.4 C   Humidity 58.2 %
```

`DHT read failed (check wiring...)` means bad wiring, or reads issued faster
than the 2 s minimum.

---

## 8. Node overview

```
                    ESP32 (WROOM-32)
   MLX90640 ──I2C──> 21/22 ──┐
   MQ-135   ──ADC1─> 34    ──┤
   DHT22    ──DIO──> 4     ──┤
   Sound    ──DIO──> 27    ──┤── Wi-Fi ──> host (YOLO detector + audio classifier)
   INMP441  ──I2S──> 26/25/33
   MQ heater ──5V───────────── (USB)
```

---

## 9. Bring-up checklist

1. [ ] Wire with power off. Verify MQ-135 VCC is on **5 V**, everything else on **3.3 V**.
2. [ ] Power via a **data-rated** USB cable (`dmesg | tail` shows ch341/cp210x).
3. [ ] `ir_camera_test.ino` → I2C scan finds **0x33**, prints a 32×24 grid.
4. [ ] `dht22_test` → `Temp … C   Humidity … %` every 2 s (discard the first read).
5. [ ] `sound_event.ino` → clap produces a logged event (adjust trimpot).
6. [ ] `i2s_mic_capture.ino` → values swing when you speak; not all zero, not constant.
7. [ ] MQ-135 → burn-in running; readings not yet trusted until 24–48 h.

---

## 10. ESP32-CAM — optional, separate video node

**What it is:** ESP32-S + OV2640 **visible-light** camera + microSD slot.
**What it is NOT:** thermal. It cannot feed the TIRPigEar detector (that model
expects thermal IR), and it cannot host this sensor node — the camera claims
most of the GPIOs.

### 10.1 Free pins (AI-Thinker ESP32-CAM)

| Group | GPIOs |
|---|---|
| Camera (do not touch) | 0, 5, 18, 19, 21, 22, 23, 25, 26, 27, 34, 35, 36, 39 |
| microSD (if used) | 2, 4, 12, 13, 14, 15 |
| PSRAM | 16 |
| **Free** | **1 (TX), 3 (RX), 32, 33** (+ 12–15 if you skip SD) |

⚠️ **GPIO21/22 (this node's I2C) and GPIO34 (this node's MQ-135 ADC) are
consumed by the camera.** That is exactly why the sensor node stays on the
DevKit.

### 10.2 Power

- **5 V, ≥ 1 A** supply. Add **100–470 µF** across 5V/GND.
- Wi-Fi + camera bursts cause brownouts on weak supplies or thin USB cables
  (classic "random reboot" symptom).

### 10.3 Confirmed hardware & programming path

**This build (confirmed):** ESP32-CAM (ESP-32S module, AI-Thinker-compatible
pinout, OV2640, **no microphone**) mounted on an **ESP32-CAM-MB** shield — the
shield carries the micro-USB port and the `IO0` / `RST` buttons.

➡️ **Use Option A (MB baseboard). No FTDI adapter is needed.** The FTDI wiring is
kept below as a fallback reference only.

- **Option A — ESP32-CAM-MB baseboard** (has USB): plug in, done. Easiest.
- **Option B — FTDI / USB-serial adapter:** wire as below (**not needed here**).

| FTDI | ESP32-CAM |
|---|---|
| 5V | 5V |
| GND | GND |
| TX | U0R (GPIO3) |
| RX | U0T (GPIO1) |
| — | GPIO0 → GND (**only while flashing**) |

Use an adapter with **3.3 V I/O logic**; powering the board at 5 V is fine.

### 10.4 Step-by-step — Option A: MB baseboard (your path)

Confirmed: **ESP32-CAM (ESP-32S, AI-Thinker-compatible) + ESP32-CAM-MB shield.**
Mount the camera module with the **camera facing outward/away** from the shield.

1. [ ] Seat the ESP32-CAM onto the MB shield's two female headers; camera outward. Press down firmly.
2. [ ] Connect a **micro-USB data** cable (not charge-only) to the PC. Power LED should light.
3. [ ] Arduino IDE → Boards Manager → install **"esp32 by Espressif Systems"**.
4. [ ] Tools → Board: **AI Thinker ESP32-CAM**.
5. [ ] Tools → PSRAM: **Enabled**; Partition Scheme: **Huge APP (3MB No OTA/1MB SPIFFS)**.
6. [ ] Tools → Port: the **CH340** port (install the CH340 driver on Windows if missing).
7. [ ] Open **File → Examples → ESP32 → Camera → CameraWebServer**.
8. [ ] Set `#define CAMERA_MODEL_AI_THINKER` (comment out the others); fill in Wi-Fi SSID/password.
9. [ ] Upload. If it stalls on `Connecting........_____`: hold **`IO0`**, press+release **`RST`**, release **`IO0`**, then retry.
10. [ ] Press **`RST`** once; open Serial Monitor @ **115200** → note the printed IP.
11. [ ] Browse to `http://<ip>` → live MJPEG stream + capture controls. ✅

*Troubleshooting:* `Camera init failed` / black image → **reseat the camera ribbon**
(lift the ZIF latch, insert straight, gold contacts toward the board, close latch).
Random reboots while streaming → better USB cable/port, or add **100 µF** across 5V/GND.
No serial output → baud must be **115200**.

### 10.4b Step-by-step — Option B: FTDI (fallback reference only)

1. [ ] Power off. Wire `5V`, `GND`, and **cross** `TX↔Rx`, `RX↔Tx`.
2. [ ] Jumper **GPIO0 → GND** (download mode). Leave it in place for now.
3. [ ] Plug the FTDI into the PC; note the COM port in Device Manager / `dmesg`.
4. [ ] Same IDE settings as Option A, steps 3–8.
5. [ ] Upload. If it can't connect, **hold GPIO0 low and press RESET**, then retry.
6. [ ] **Remove the GPIO0→GND jumper** and press RESET (normal boot).
7. [ ] Open Serial Monitor @ **115200** → note the IP → browse to `http://<ip>`.

### 10.5 Role in O.I.N.K.

- Use it as a **separate node** for **RGB + thermal fusion** research, or for
  demo/B-roll footage while the DevKit does the measurement work.
- **Alignment caveat:** the OV2640 (RGB) and MLX90640 (thermal) differ in FOV,
  resolution and frame rate — they must be spatially aligned before fusion.
- **Modality caveat:** RGB-trained models ≠ thermal-trained models. The TIRPigEar
  weights will not run on RGB frames and vice versa.
- On-board **flash LED is GPIO4** — bright at close range; disable it or pulse briefly.
