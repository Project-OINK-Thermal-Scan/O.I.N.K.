/*
 * O.I.N.K. — MLX90640 bring-up test
 * -----------------------------------
 * Scans I2C (expects 0x33), then streams a 32x24 temperature grid.
 *
 * Wiring: via Qwiic/STEMMA QT socket on the back of the camera.
 *   black -> GND   red -> 3V3   blue -> SDA(21)   yellow -> SCL(22)
 *
 * Requires library: "Adafruit MLX90640" (Library Manager).
 *
 * NOTE: pins below are for a classic ESP32 DevKit. On S3/C3 change them
 * and make sure Wire.begin() matches your board.
 */

#include <Wire.h>
#include <Adafruit_MLX90640.h>

#define I2C_SDA 21
#define I2C_SCL 22
#define MLX_ADDR 0x33

Adafruit_MLX90640 mlx;
static float frame[32 * 24];

void scanI2C() {
  Serial.println("Scanning I2C bus...");
  uint8_t found = 0;
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      Serial.printf("  found device at 0x%02X\n", a);
      found++;
    }
  }
  if (!found) Serial.println("  (nothing found)");
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);

  scanI2C();

  if (!mlx.begin(MLX_ADDR, &Wire)) {
    Serial.println("MLX90640 NOT found at 0x33.");
    Serial.println("Check: Qwiic cable seated? SDA/SCL right? power on 3V3?");
    while (true) delay(1000);
  }

  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_2_HZ);
  Serial.println("MLX90640 ready. 32x24 grid follows (rows every 2).\n");
}

void loop() {
  if (mlx.getFrame(frame) != 0) {
    Serial.println("Frame read failed.");
    return;
  }

  float tmin = 1e9, tmax = -1e9;
  int imax = 0;
  for (int i = 0; i < 32 * 24; i++) {
    if (frame[i] < tmin) tmin = frame[i];
    if (frame[i] > tmax) { tmax = frame[i]; imax = i; }
  }

  for (int y = 0; y < 24; y += 2) {
    for (int x = 0; x < 32; x++) {
      Serial.printf("%5.1f", frame[y * 32 + x]);
    }
    Serial.println();
  }

  int hx = imax % 32, hy = imax / 32;
  Serial.printf("min %.1fC  max %.1fC (hotspot x=%d y=%d)  center %.1fC\n\n",
                tmin, tmax, hx, hy, frame[12 * 32 + 16]);

  delay(500);
}
