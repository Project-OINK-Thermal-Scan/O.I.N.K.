/*
 * O.I.N.K. — sound event counter (3-pin digital sound module)
 * ----------------------------------------------------------
 * Module pins: VCC -> 3V3, GND -> GND, OUT -> GPIO27.
 *
 * This module is a comparator threshold output, NOT a measurement.
 * It chatters during sustained sound, so we debounce it and count events.
 *
 * Set the onboard trimpot first: quiet room -> DOUT LED off; clap -> LED on.
 */

#define SOUND_PIN     27
#define ACTIVE_LOW    1     // most LM393 sound modules pull LOW when sound trips
#define DEBOUNCE_MS   80

int           lastStable  = -1;
unsigned long lastChange  = 0;
unsigned long eventCount  = 0;
unsigned long windowStart = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(SOUND_PIN, INPUT);   // module drives the line (has its own pull-up)
  Serial.println("Sound event monitor.");
  Serial.println("Adjust trimpot: quiet = LED off, clap = LED on.");
  windowStart = millis();
}

void loop() {
  int raw   = digitalRead(SOUND_PIN);
  int level = ACTIVE_LOW ? !raw : raw;   // 1 = sound present
  unsigned long now = millis();

  if (level != lastStable && (now - lastChange) > DEBOUNCE_MS) {
    lastChange = now;
    if (level == 1) {
      eventCount++;
      Serial.printf("[%lu ms] SOUND event #%lu\n", now, eventCount);
    }
    lastStable = level;
  }

  if (now - windowStart >= 60000UL) {
    Serial.printf("--- %lu events in the last minute ---\n", eventCount);
    eventCount  = 0;
    windowStart = now;
  }
}
