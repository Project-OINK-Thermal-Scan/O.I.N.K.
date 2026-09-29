/*
 * O.I.N.K. — INMP441 I2S microphone capture
 * -----------------------------------------
 * INMP441: VDD -> 3V3, GND -> GND, L/R -> GND, SCK -> 26, WS -> 25, SD -> 33
 *
 * 16 kHz, 32-bit slots, LEFT channel. Prints an RMS level bar so you can
 * confirm the mic works before moving to the host-side classifier.
 *
 * API NOTE: this uses the legacy driver/i2s.h interface. It works on
 * Arduino-ESP32 2.x and still compiles on 3.x (deprecated). If you prefer the
 * newer ESP_I2S.h (I2SClass) API on core 3.x, the capture parameters are the
 * same: master RX, 16000 Hz, 32-bit slots, mono LEFT.
 *
 * GOTCHA: INMP441 data is 24-bit, left-justified in a 32-bit slot. We shift
 * >>14 to get a usable 16-bit-range value. Adjust the shift if it clips.
 */

#include <driver/i2s.h>
#include <math.h>

#define I2S_PORT      I2S_NUM_0
#define PIN_SCK       26
#define PIN_WS        25
#define PIN_SD        33
#define SAMPLE_RATE   16000

static void i2s_init() {
  i2s_config_t cfg = {
    .mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate          = SAMPLE_RATE,
    .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
    .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,   // L/R pin tied to GND
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count        = 8,
    .dma_buf_len          = 256,
    .use_apll             = false,
    .tx_desc_auto_clear   = false,
    .fixed_mclk           = 0
  };

  i2s_pin_config_t pins = {
    .bck_io_num   = PIN_SCK,
    .ws_io_num    = PIN_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num  = PIN_SD
  };

  i2s_driver_install(I2S_PORT, &cfg, 0, NULL);
  i2s_set_pin(I2S_PORT, &pins);
  i2s_zero_dma_buffer(I2S_PORT);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  i2s_init();
  Serial.println("INMP441 capture @ 16 kHz. L/R must be tied to GND.");
  Serial.println("Speak near the mic — the bar should move.");
}

void loop() {
  static int32_t raw[256];
  size_t bytesRead = 0;

  esp_err_t err = i2s_read(I2S_PORT, raw, sizeof(raw), &bytesRead, portMAX_DELAY);
  if (err != ESP_OK || bytesRead == 0) {
    Serial.println("i2s_read failed");
    return;
  }

  int n = bytesRead / sizeof(int32_t);

  int32_t peak  = 0;
  double  sumsq = 0.0;

  for (int i = 0; i < n; i++) {
    int32_t s = raw[i] >> 14;          // 24-bit data -> usable range
    if (labs(s) > peak) peak = labs(s);
    sumsq += (double)s * (double)s;
  }

  double rms = sqrt(sumsq / n);

  int bars = (int)(rms / 200.0);
  if (bars > 60) bars = 60;

  Serial.printf("n=%3d peak=%6ld rms=%7.0f | ", n, (long)peak, rms);
  for (int i = 0; i < bars; i++) Serial.print('#');
  Serial.println();
}
