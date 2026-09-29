/*
 * O.I.N.K. — ADC pin probe (diagnostic v3)
 * ----------------------------------------
 * v1 (pull-up only) and v2 (no-pull + pulls) both failed to settle whether
 * any sensor wire reaches the ESP32. v2's no-pull column was especially
 * misleading: reading a floating pin 200x in a tight loop returns the same
 * value every time from residual pin capacitance, so "0/200" and "200/200"
 * do not prove a connection. Only the pull-up/pull-down column was real.
 *
 * This version measures VOLTS instead of logic levels. That is the one
 * measurement that separates the two remaining possibilities, because an
 * unpowered module and an absent module look identical to a digital read
 * but not to an ADC.
 *
 * The key pin is GPIO34: it is the MQ-135 ADC input, it sits behind a
 * 10k/10k divider, and the MQ-135 runs on 5 V with its LED lit. So GPIO34
 * must sit at a stable mid-level voltage if that wire actually lands on the
 * board. A wandering reading means nothing is connected.
 *
 * ADC1 (32, 33, 34, 35, 36, 39) is usable at any time.
 * ADC2 (4, 2, 13, 14, 15, 25, 26, 27) is usable only while Wi-Fi is off,
 * which it is here.
 *
 * Reading guide:
 *   stable ~0 mV     -> tied to GND
 *   stable ~3300 mV  -> tied to 3V3
 *   stable mid level -> a real signal is wired to this pin  <-- what we want
 *   wandering        -> nothing connected (floating)
 */

#include <Arduino.h>

static const uint8_t ADC_PINS[] = {
  32, 33, 34, 35, 36, 39,        // ADC1
  4, 2, 13, 14, 15, 25, 26, 27   // ADC2 (Wi-Fi is off)
};
static const size_t NPINS = sizeof(ADC_PINS) / sizeof(ADC_PINS[0]);
static const int    SAMPLES = 200;

static void probePin(uint8_t p) {
  analogSetPinAttenuation(p, ADC_11db);
  delay(2);

  int lo = 99999, hi = -1;
  long sum = 0;

  for (int i = 0; i < SAMPLES; i++) {
    int raw = analogRead(p);
    int mv  = (int)((long)raw * 3300L / 4095L);
    if (mv < lo) lo = mv;
    if (mv > hi) hi = mv;
    sum += mv;
    delayMicroseconds(500);
  }

  int mean   = (int)(sum / SAMPLES);
  int spread = hi - lo;

  const char* verdict;
  if (spread < 200) {
    if (mean < 200)         verdict = "STABLE ~0 V     -> tied to GND";
    else if (mean > 3100)   verdict = "STABLE ~3.3 V   -> tied to 3V3";
    else                    verdict = "STABLE MID      -> REAL SIGNAL WIRED HERE";
  } else {
    verdict = "wandering       -> nothing connected (floating)";
  }

  Serial.printf("%-3u | mean %4d mV | min %4d | max %4d | spread %4d | %s\n",
                p, mean, lo, hi, spread, verdict);
}

static void probeAll() {
  Serial.println();
  Serial.println("=== ADC pin probe v3 ===");
  Serial.println("pin |   mean     |   min   |   max   | spread  | verdict");

  for (size_t i = 0; i < NPINS; i++) {
    probePin(ADC_PINS[i]);
  }

  Serial.println("--- end ---");
  Serial.println("Watch GPIO34: stable mid level = MQ-135 wire reaches the board.");
  Serial.println("Wandering = no sensor wire is connected to the ESP32 at all.");
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println("O.I.N.K. - ADC pin probe v3");
  probeAll();
  Serial.println("(re-probing every 15 s)");
}

void loop() {
  delay(15000);
  probeAll();
}
