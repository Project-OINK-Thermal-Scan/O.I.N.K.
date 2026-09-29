/*
 * O.I.N.K. — DHT22 (AM2302) temperature + humidity test
 * ------------------------------------------------------
 * Module pins (follow the labels on the board):
 *    +   -> 3V3     (NOT 5V: the onboard pull-up ties DATA to VCC)
 *    out -> GPIO4   (a normal GPIO; not 34-39)
 *    -   -> GND
 *
 * Library: DHTesp  (lib_deps = beegee-tokyo/DHTesp)
 * Board:   classic ESP32 DevKit (board = esp32dev)
 */

#include "DHTesp.h"

#define DHT_PIN 4

DHTesp dht;

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.setup(DHT_PIN, DHTesp::DHT22);
  Serial.println("DHT22 ready on GPIO4");
  Serial.println("(first reading after power-up is often bad - discard it)");
}

void loop() {
  TempAndHumidity d = dht.getTempAndHumidity();

  if (isnan(d.temperature) || isnan(d.humidity)) {
    Serial.println("DHT read failed (check wiring; DHT22 needs >= 2 s between reads)");
  } else {
    Serial.printf("Temp %.1f C   Humidity %.1f %%\n", d.temperature, d.humidity);
  }

  delay(2000);   // DHT22 requires >= 2 s between reads
}
