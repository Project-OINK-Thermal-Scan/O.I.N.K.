/*
 * O.I.N.K. — DHT pin / model finder (diagnostic)
 * ----------------------------------------------
 * dht22_test returned NaN on GPIO4. Two things cause that and both are
 * invisible from the outside:
 *
 *   1. the data wire does not actually reach GPIO4
 *   2. the module is a DHT11, which reads as a timeout when the sketch
 *      asks for a DHT22
 *
 * This sketch tries every candidate GPIO with BOTH models, one read each,
 * then prints a verdict. It is deliberately slow: a DHT needs >= 1 s to
 * settle, and the wrong model always reads as a timeout, so both must be
 * attempted on every pin.
 *
 * Pins skipped, and why:
 *   0, 2, 12, 15  boot strapping (12 also sets flash voltage — unsafe)
 *   1, 3          USB serial
 *   6-11          internal SPI flash
 *   34-39         input-only, cannot drive the DHT one-wire line
 */

#include "DHTesp.h"

static const uint8_t CANDIDATES[] = {
  4, 5, 13, 14, 16, 17, 18, 19, 21, 22, 23, 25, 26, 27, 32, 33
};
static const size_t NCAND = sizeof(CANDIDATES) / sizeof(CANDIDATES[0]);

DHTesp dht;

struct Hit {
  uint8_t     pin;
  const char* model;
  float       t;
  float       h;
};

static bool tryRead(uint8_t pin, DHTesp::DHT_MODEL_t model, float* t, float* h) {
  dht.setup(pin, model);
  delay(1200);                        // settle time, and DHT22 wants >= 1 s
  TempAndHumidity d = dht.getTempAndHumidity();

  if (isnan(d.temperature) || isnan(d.humidity)) return false;
  if (d.temperature < -60.0f || d.temperature > 125.0f) return false;  // nonsense
  if (d.humidity < 0.0f || d.humidity > 100.0f) return false;

  *t = d.temperature;
  *h = d.humidity;
  return true;
}

static void scan() {
  Hit hits[NCAND];
  int nHits = 0;

  Serial.println();
  Serial.println("=== DHT pin scan ===");

  for (size_t i = 0; i < NCAND; i++) {
    uint8_t pin = CANDIDATES[i];
    float t = 0.0f, h = 0.0f;

    Serial.printf("GPIO%-2u  ", pin);
    Serial.flush();

    if (tryRead(pin, DHTesp::DHT22, &t, &h)) {
      Serial.printf("DHT22  T=%6.1f C  RH=%5.1f %%\n", t, h);
      hits[nHits].pin = pin; hits[nHits].model = "DHT22";
      hits[nHits].t = t;     hits[nHits].h = h;
      nHits++;
      continue;
    }

    if (tryRead(pin, DHTesp::DHT11, &t, &h)) {
      Serial.printf("DHT11  T=%6.1f C  RH=%5.1f %%\n", t, h);
      hits[nHits].pin = pin; hits[nHits].model = "DHT11";
      hits[nHits].t = t;     hits[nHits].h = h;
      nHits++;
      continue;
    }

    Serial.println("-");
  }

  Serial.println("--- verdict ---");
  if (nHits == 0) {
    Serial.println("No DHT answered on ANY candidate pin.");
    Serial.println("=> This is not a wrong-pin problem. Check, in order:");
    Serial.println("   1. module '+' really on 3V3 (not 5 V, not empty)");
    Serial.println("   2. module '-' shares a common GND with the ESP32");
    Serial.println("   3. the expansion board is fully seated on the DevKit");
    Serial.println("   4. the module itself (swap it if you have a spare)");
  } else {
    for (int i = 0; i < nHits; i++) {
      Serial.printf(">>> %s responds on GPIO%u\n", hits[i].model, hits[i].pin);
    }
    Serial.println("=> Move the data wire to GPIO4 and reflash dht22_test.");
    Serial.println("   If the model above is DHT11, edit dht22_test/src/main.cpp:");
    Serial.println("   change DHTesp::DHT22 to DHTesp::DHT11.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);
  Serial.println();
  Serial.println("O.I.N.K. — DHT finder");
  scan();
  Serial.println("(rescanning in 20 s)");
}

void loop() {
  delay(20000);
  scan();
}
